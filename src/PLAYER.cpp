// ---------------------------------------------------------------------------
// PLAYER.cpp  —  FMV / streaming video player and SCM cutscene engine
//
// This module drives the synchronised video+audio playback engine used for
// in-game cutscenes (CRX/FMV sequences on the ADVENT.RES disc) and the
// scripted cutscene movie (SCM) system that sequences per-character animations.
//
// Architecture:
//   A background Win32 thread (Player_StreamThreadProc) performs rate-paced
//   bunch reads from the ADVENT.RES file, synchronised to a pair of Win32
//   events.  Each display frame the sync callback (Player_StreamSyncCallback)
//   fires; if the reader has already consumed its slot the missed-frame counter
//   is incremented.
//
//   Three audio voice channels (g_abPlayerVoiceFlags[3]) are flushed between
//   frames by Player_FlushVoices, which is called by the scheduler before
//   each new video frame is handed to the mixer.
//
//   A 256-entry palette LUT (g_abPlayerPalLUT) is applied by
//   Player_RemapPalette, which walks every pixel row of the decoded FMV frame
//   and translates 8-bit indices to the current screen palette.
//
//   A "cover sprite" overlay (Player_SetCoverSprite / Player_DrawCoverSprite)
//   blits a fullscreen animation frame over the back-buffer immediately before
//   the FMV frame is composited.
//
//   The SCM (scripted cutscene movie) layer builds a play-list of per-character
//   animation names via Player_ScmAddChar, then drives the full synchronised
//   playback sequence in Player_ScmPlayList.  Player_RenderFrame decodes one
//   SCM frame: it dispatches chunk-type codes (0x10=video, 0x02=palette,
//   0x40-0x43=music, 0x80-0x83=bunch-audio, 0x100=speech, 0x400=lip-sync,
//   0x401=lip-data, 0x1000=text-subtitle).
//
//   Palette changes during SCM playback are handled with a two-phase approach:
//     Player_BuildPalLUT builds a closest-colour mapping between old and new
//     palettes, then Player_FlushPalAndBlit applies the new palette and flushes
//     the dirty screen region.
//
// Module boundary notes:
//   Lower boundary: 0x00457e60 (Player_SetPalFreezeMode).  The preceding
//   address range belongs to ONTHEFLY.cpp (OTF_AllocSlot, OTF_AllocNodeList,
//   OTF_AreaTip at 0x004577a0–0x00457c90).
//
//   Address gap 0x00458bb0 → 0x0045ace0: Player_ScmPlayList is a ~7 KB
//   function running from 0x00458bb0 to ~0x0045abff.  The gap in the original
//   address list between 0x00458bb0 and 0x0045ace0 is simply the interior of
//   that single large function — there is no module boundary here.
//
//   Upper boundary: 0x0045c610 (Player_StartMusicLoop) is the last PLAYER.cpp
//   function in this block.  0x0045c7d0 (Player_RemapPalette) continues the
//   module in the next address block.
//
//   0x0045d4e0 (Res_FindByNumChar) is physically adjacent to Player_DrawCoverSprite
//   but belongs to READRES.cpp — its debug name is "bunch_find_int_num_char__name_ch"
//   and it references READRES globals.
//
// Original source: C:\DevStudio\Projects\Crux\PLAYER.cpp
// Address range:   0x00457e60 -- 0x0045d3c0  (two address blocks, same TU)
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>   // timeGetTime
#include "PLAYER.h"
#include "READRES.h"    // Res_AcquireFileLock, Res_ReleaseFileLock, Res_GetTransferRate

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

unsigned char g_abPlayerPalLUT[256]     = {0};      // 0x006ffc80
int  g_anPlayerBunchTargets[4000]       = {0};      // 0x006ffd88
int  g_nPlayerBunchIdx                  = 0;        // 0x006f42f4
int  g_nPlayerCurrentFrame              = -1;       // 0x006ffc78  init to -1
int  g_nPlayerFileOffset                = 0;        // 0x006ecd0c
int  g_nPlayerTransferRate              = 0;        // 0x006ecd14
int  g_nPlayerMissedFrames              = 0;        // 0x006ffc40
int  g_nPlayerLastFrameTime             = 0;        // 0x006f5310
int  g_nPlayerState                     = 0;        // 0x006fa138
int  g_nPlayerFlags                     = 0;        // 0x006ecd18
unsigned char g_abPlayerVoiceFlags[3]   = {0};      // 0x006efc08
int  g_nPlayerVoicePendingMask          = 0;        // 0x006f4358
unsigned char g_bPlayerVoiceActive      = 0;        // 0x006eccf0
unsigned char g_bPlayerStreamActive     = 0;        // 0x006f4314
int  g_nPlayerStreamStarted             = 0;        // 0x006f4360
int  g_nPlayerStreamHandle              = 0;        // 0x006ffd80
int  g_nPlayerStreamParam               = 0;        // 0x006f4368
int  g_nPlayerInitialized               = 0;        // 0x00703f68
HANDLE g_hPlayerThread                  = NULL;     // 0x00703f54
HANDLE g_hPlayerSyncEvent               = NULL;     // 0x00703f5c
HANDLE g_hPlayerFrameEvent              = NULL;     // 0x00703f64
HANDLE g_hPlayerAuxEvent                = NULL;     // 0x00703f58
DWORD  g_dwPlayerThreadId               = 0;        // 0x006fa148
int  g_nPlayerCoverSpriteIdx            = -1;       // 0x004d6334  init to -1
int  g_nPlayerCoverFrame                = 0;        // 0x004d6338

// CRITICAL_SECTION protecting streaming-thread teardown in Player_Shutdown.
// (24-byte Win32 CRITICAL_SECTION struct at 0x006f4338; declared as int here
//  to match Ghidra's flat view.)
static int g_nPlayerCS;                             // 0x006f4338

// ---------------------------------------------------------------------------
// External helpers (resolved via thunk table at link time)
// ---------------------------------------------------------------------------
extern "C" {
    void  Debug_Trace(int nLine, const char *pszFile, const char *pszFmt, ...);

    // Audio subsystem — initialise voice channels, returns default voice state
    unsigned char FUN_0046faf0(void);               // thunk_FUN_0046faf0

    // Audio subsystem — flush one voice channel (voice index + current state)
    void  FUN_0046fdd0(int nVoiceIdx, int nState);  // thunk_FUN_0046fdd0

    // Raw file write helper (game's own fwrite wrapper)
    int   FUN_0048a8d0(void *pBuf, int nOffset, int nMode);  // 0x0048a8d0

    // Raw file read helper (game's own fread wrapper)
    int   FUN_0048a180(void *pBuf, int nCount, void *pDst, void *pFile);  // 0x0048a180

    // Keyboard — non-blocking "any key pressed?" check (returns non-zero if so)
    int   thunk_FUN_004820c0(void);

    // Keyboard — return last pressed key code
    int   thunk_FUN_00481fe0(void);

    // Keyboard — check escape/abort state (returns non-zero if abort flagged)
    int   thunk_FUN_0040f700(void);

    // Resource loader — load sprite by tag string, returns index in char table
    int   thunk_FUN_00409570(int nTag, int a, int b);

    // Resource free — release a resource handle
    void  thunk_FUN_00405810(int nHandle);

    // Blit resource: BlitResource(nResX, nResY, nResH)
    void  thunk_FUN_0042bd40(int nX, int nY, int nH);

    // Debug assert for cover-sprite bounds check
    void  thunk_FUN_0041f680(int nLine, const char *pszFile, const char *pszMsg);
}

// Animation frame count table (owner: Advanim.cpp) — used by SetCoverSprite
// to validate frame index.
extern int g_anAnimFrameCount[];    // 0x00574990

// Sprite base table: each entry is 0x58 bytes; flags at +0 word offset.
// g_anItemFlags[idx] in ScummVM terms; flat array of dword records.
extern unsigned int g_abSpriteFlags[];   // 0x005b10b0

// Frame coordinate table for cover sprites:
//   g_anCoverFrameXY[idx * 0x640 + frame * 4]  = frame offset index
extern int g_anCoverFrameXY[];      // 0x004e3b58

// Sprite image base for blitting cover frames.
extern int g_anCoverSpriteBase[];   // 0x0051e4f0

// Cover sprite tag table — one 4-byte resource tag per cover sprite slot.
static const char *g_pszCoverSpriteTag = "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp";
// (tag string pointer lives at 0x004d6370; error message at 0x004d63a4)

// ---------------------------------------------------------------------------
// 0x0045c7d0  Player_RemapPalette
// ---------------------------------------------------------------------------
// Remap an 8-bit planar pixel buffer through the 256-entry palette LUT.
// Processes nRows rows of nWidth bytes each (nWidth must equal 640 = 0x280).
// Each byte value is replaced by g_abPlayerPalLUT[value].
//
// Ghidra decompile (reconstructed):
//   pbVar1 = param_1 + param_2 * param_3;          // end pointer
//   for (; param_1 < pbVar1; param_1 += param_2 - 0x280) {
//     pbVar2 = param_1 + 0x280;
//     for (; param_1 < pbVar2; param_1++)
//       *param_1 = g_abPlayerPalLUT[*param_1];
//   }
// The outer stride arithmetic (param_2 - 0x280) advances to the next row
// after the inner loop has walked 640 bytes, which means param_2 is the
// full row pitch and 0x280 = 640 is the active pixel width.
void Player_RemapPalette(unsigned char *pBuf, int nWidth, int nRows)
{
    unsigned char *pEnd = pBuf + nWidth * nRows;
    for (; pBuf < pEnd; pBuf += nWidth - 0x280) {
        unsigned char *pRowEnd = pBuf + 0x280;
        for (; pBuf < pRowEnd; pBuf++) {
            *pBuf = g_abPlayerPalLUT[*pBuf];
        }
    }
}

// ---------------------------------------------------------------------------
// 0x0045c860  Player_StreamSyncCallback
// ---------------------------------------------------------------------------
// Called by the display/timer system once per video frame interval.
// If the sync event is already signalled the reader thread has not consumed
// the previous slot — increment the missed-frame counter and trace a warning.
// Then reset the "frame done" event and set the "slot ready" event so the
// reader thread wakes for the next frame.
//
// DAT_004d5e28 + 7 = line number argument to Debug_Trace (embedded constant).
// s_C__DevStudio_Projects_Crux_PLAYE_004d5e58 = __FILE__ string.
// s_Missed___d_>_d_004d5e48 = "Missed %d > %d" format string.
void Player_StreamSyncCallback(void)
{
    g_nPlayerMissedFrames++;

    if (WaitForSingleObject(g_hPlayerSyncEvent, 0) == WAIT_OBJECT_0) {
        DWORD dwNow = timeGetTime();
        Debug_Trace(7, "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp",
                    "Missed %d > %d", dwNow, g_nPlayerLastFrameTime);
    }

    ResetEvent(g_hPlayerFrameEvent);
    SetEvent(g_hPlayerSyncEvent);
}

// ---------------------------------------------------------------------------
// 0x0045c960  Player_FlushVoices
// ---------------------------------------------------------------------------
// Flush all pending audio voice channels between FMV frames.
// If g_bPlayerVoiceActive is set, clear it first.
// Then for each of the 3 voice slots: if the corresponding bit in
// g_nPlayerVoicePendingMask is set, call the audio flush helper with the
// voice slot's resource index (DAT_004d5900 table, 4 bytes per entry) and
// the voice's current state byte.
// Finally clear the pending mask.  If g_bPlayerStreamActive is set, clear it.
//
// DAT_004d5900 = array of 3 audio resource indices (one per voice channel).
extern int g_anPlayerVoiceRes[3];   // 0x004d5900  audio resource index per voice
void Player_FlushVoices(void)
{
    if (g_bPlayerVoiceActive != 0)
        g_bPlayerVoiceActive = 0;

    for (int i = 0; i < 3; i++) {
        if (g_nPlayerVoicePendingMask & (1 << i)) {
            FUN_0046fdd0(g_anPlayerVoiceRes[i],
                         (int)(char)g_abPlayerVoiceFlags[i]);
        }
    }
    g_nPlayerVoicePendingMask = 0;

    if (g_bPlayerStreamActive != 0)
        g_bPlayerStreamActive = 0;
}

// ---------------------------------------------------------------------------
// 0x0045ca80  Player_Init
// ---------------------------------------------------------------------------
// Initialise the entire Player subsystem.  Zeros all state, fills the
// bunch-target table with 100000 (far-future sentinel), resets voice state,
// obtains the CD transfer rate, and prepares the cover-sprite slot.
//
// Key globals initialised here:
//   g_anPlayerBunchTargets[0..3999] = 100000
//   g_nPlayerCurrentFrame           = -1   (no frame in flight)
//   g_nPlayerCoverSpriteIdx         = -1   (no cover sprite)
//   g_abPlayerVoiceFlags[0..2]      = FUN_0046faf0() result (default voice state)
//   g_nPlayerTransferRate           = Res_GetTransferRate()
//
// Other zeroed globals (selected):
//   g_nPlayerState, g_nPlayerFlags, g_nPlayerMissedFrames,
//   g_nPlayerBunchIdx, g_nPlayerVoicePendingMask, g_bPlayerVoiceActive,
//   g_bPlayerStreamActive, g_nPlayerStreamStarted, g_nPlayerStreamHandle.
void Player_Init(void)
{
    // Zero running counters
    g_nPlayerState            = 0;
    g_nPlayerMissedFrames     = 0;
    g_nPlayerFlags            = 0;
    g_nPlayerBunchIdx         = 0;
    g_nPlayerVoicePendingMask = 0;
    g_nPlayerStreamStarted    = 0;
    g_nPlayerStreamHandle     = 0;
    g_nPlayerFileOffset       = 0;
    g_bPlayerStreamActive     = 0;
    g_nPlayerLastFrameTime    = 0;

    // Fill bunch-target table with far-future sentinel
    for (int i = 0; i < 4000; i++)
        g_anPlayerBunchTargets[i] = 100000;

    // Frame counter starts at "none" (-1)
    g_nPlayerCurrentFrame = -1;

    // Cover sprite starts at "none" (-1)
    g_nPlayerCoverSpriteIdx = -1;

    // Initialise voice state from audio subsystem
    unsigned char bVoiceDefault = FUN_0046faf0();
    g_abPlayerVoiceFlags[0] = bVoiceDefault;
    g_abPlayerVoiceFlags[1] = bVoiceDefault;
    g_abPlayerVoiceFlags[2] = bVoiceDefault;
    g_bPlayerVoiceActive    = 0;

    // Fetch CD transfer rate
    g_nPlayerTransferRate = Res_GetTransferRate();

    // (Additional zeroing of sync/timer state globals omitted above is
    // performed by the calling initialisation sequence via DAT_00703f30,
    // DAT_006edcc0, DAT_006f432c, etc.)
}

// ---------------------------------------------------------------------------
// 0x0045cca0  Player_WriteData
// ---------------------------------------------------------------------------
// Write nBytes bytes from pBuf into the ADVENT.RES stream.
// Acquires the file lock, calls the raw write helper at the current file
// offset, advances g_nPlayerFileOffset, then releases the lock.
//
// Calling convention: __thiscall — param_1 (ECX) = file object pointer
// passed through to Res_AcquireFileLock / Res_ReleaseFileLock.
unsigned int Player_WriteData(void *pFile, void *pBuf, int nBytes)
{
    Res_AcquireFileLock(pFile);
    unsigned int nWritten = (unsigned int)FUN_0048a8d0(pBuf, g_nPlayerFileOffset, 0);
    g_nPlayerFileOffset += nBytes;
    Res_ReleaseFileLock();
    return nWritten;
}

// ---------------------------------------------------------------------------
// 0x0045ccf0  Player_ReadData
// ---------------------------------------------------------------------------
// Read up to (nCount * nSize) bytes from the ADVENT.RES stream.
// Acquires the file lock, seeks to the current offset, performs the read via
// FUN_0048a180, multiplies the result by nSize (returns total bytes read),
// releases the lock, then advances g_nPlayerFileOffset.
//
// Calling convention: __thiscall — param_1 = file object pointer.
int Player_ReadData(void *pFile, void *pBuf, int nCount, void *pDst, void *pFile2)
{
    Res_AcquireFileLock(pFile);
    FUN_0048a8d0(pFile2, g_nPlayerFileOffset, 0);
    int nRead = FUN_0048a180(pBuf, nCount, pDst, pFile2);
    nRead *= nCount;
    Res_ReleaseFileLock();
    g_nPlayerFileOffset += nRead;
    return nRead;
}

// ---------------------------------------------------------------------------
// 0x0045cd60  Player_BunchRead
// ---------------------------------------------------------------------------
// Rate-paced streaming read: transfer (nCount * nSize) bytes from the
// ADVENT.RES stream into pBuf, chunked to the per-frame CD budget.
//
// If the total byte count fits within one frame's transfer budget
// (nCount * nSize <= g_nPlayerTransferRate), it is dispatched in a single
// pass via PTR_FUN_004d5ed0 (the file read function pointer).
//
// Otherwise the transfer is broken into chunks:
//   - Each iteration calls Player_FlushVoices (audio sync between chunks),
//     computes how many frames the next chunk needs, waits on
//     g_hPlayerFrameEvent (1-second timeout), serialises the read under
//     g_nPlayerCS, then advances the destination pointer.
//
// The chunk size for iteration k is:
//   min(remaining, max(1, bunchtarget[bunchIdx] - currentFrame) * transferRate)
void Player_BunchRead(int pBuf, int nCount, int nSize, void *pFile)
{
    typedef int (*ReadFn_t)(int, int, int, void *);
    extern ReadFn_t PTR_FUN_004d5ed0;   // 0x004d5ed0  function pointer to fread wrapper

    int nTotal = nCount * nSize;

    if ((unsigned int)nTotal < (unsigned int)g_nPlayerTransferRate ||
        nTotal - g_nPlayerTransferRate == 0)
    {
        // Fits in one frame — do it directly
        PTR_FUN_004d5ed0(pBuf, nCount, nSize, pFile);
        return;
    }

    // Multi-chunk rate-paced read
    int nCur = pBuf;
    for (unsigned int nRemaining = (unsigned int)nTotal; nRemaining != 0; nRemaining -= (unsigned int)nCount)
    {
        Player_FlushVoices();

        // Compute how many transfer-rate chunks we can issue this wakeup
        int nSlack = g_anPlayerBunchTargets[g_nPlayerBunchIdx] - g_nPlayerCurrentFrame;
        int nChunks = (nSlack < 2) ? 1 : nSlack;

        WaitForSingleObject(g_hPlayerFrameEvent, 1000);
        EnterCriticalSection((LPCRITICAL_SECTION)&g_nPlayerCS);

        unsigned int nChunkBytes = (unsigned int)(nChunks * g_nPlayerTransferRate);
        unsigned int nThisRead   = (nRemaining < nChunkBytes) ? nRemaining : nChunkBytes;

        nCount = (int)PTR_FUN_004d5ed0(nCur, 1, (int)nThisRead, pFile);

        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nPlayerCS);
        nCur += nCount;
    }
}

// ---------------------------------------------------------------------------
// 0x0045cf30  Player_IsAbortPressed
// ---------------------------------------------------------------------------
// Return non-zero if the player abort key has been pressed.
//   bCheckEscape — if non-zero, also accept a non-blocking "abort" signal
//                  from thunk_FUN_0040f700.
// Key codes checked:
//   0x1b = VK_ESCAPE
//   0x25 = VK_LEFT (used as a skip/abort binding in this game)
int Player_IsAbortPressed(int bCheckEscape)
{
    if (bCheckEscape != 0) {
        if (thunk_FUN_0040f700() != 0)
            return 1;
    }

    if (thunk_FUN_004820c0() == 0)
        return 0;

    int nKey = thunk_FUN_00481fe0();

    if (nKey == 0x1b && bCheckEscape != 0)
        return 1;
    if (nKey == 0x25)
        return 1;

    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045d010  Player_StartStream
// ---------------------------------------------------------------------------
// Begin a new streaming FMV session.
// Sets g_nPlayerStreamStarted = 1 to notify the reader thread that a new
// stream has been queued, then copies g_nPlayerStreamParam (set by the
// caller before invoking this function) into g_nPlayerStreamHandle.
void Player_StartStream(void)
{
    g_nPlayerStreamStarted  = 1;
    g_nPlayerStreamHandle   = g_nPlayerStreamParam;
}

// ---------------------------------------------------------------------------
// 0x0045d0b0  Player_Shutdown
// ---------------------------------------------------------------------------
// Tear down the player subsystem.
// If g_nPlayerInitialized is set:
//   1. Enter g_nPlayerCS.
//   2. If the streaming thread is alive and is not the current thread,
//      TerminateThread + CloseHandle.
//   3. Close g_hPlayerSyncEvent, g_hPlayerAuxEvent, g_hPlayerFrameEvent.
//   4. Leave g_nPlayerCS.
void Player_Shutdown(void)
{
    if (g_nPlayerInitialized == 0)
        return;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nPlayerCS);

    if (g_hPlayerThread != NULL) {
        DWORD dwCurrent = GetCurrentThreadId();
        if (dwCurrent != g_dwPlayerThreadId) {
            TerminateThread(g_hPlayerThread, 0);
            CloseHandle(g_hPlayerThread);
            g_hPlayerThread = NULL;
        }
    }
    if (g_hPlayerSyncEvent != NULL) {
        CloseHandle(g_hPlayerSyncEvent);
        g_hPlayerSyncEvent = NULL;
    }
    if (g_hPlayerAuxEvent != NULL) {
        CloseHandle(g_hPlayerAuxEvent);
        g_hPlayerAuxEvent = NULL;
    }
    if (g_hPlayerFrameEvent != NULL) {
        CloseHandle(g_hPlayerFrameEvent);
        g_hPlayerFrameEvent = NULL;
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nPlayerCS);
}

// ---------------------------------------------------------------------------
// 0x0045d220  Player_SetCoverSprite
// ---------------------------------------------------------------------------
// Load or clear the fullscreen "cover sprite" overlay shown before each FMV
// frame is composited onto the back-buffer.
//
// If nSpriteIdx == -1:
//   Free the current sprite (if loaded) and set g_nPlayerCoverSpriteIdx = -1.
//
// Otherwise:
//   1. If a sprite is already loaded, free it and deselect it first
//      (clears bit 9 of the sprite's flags record).
//   2. Load the new sprite via thunk_FUN_00409570(nSpriteIdx, 0, 0).
//   3. Assert that nFrame < frame count for the new sprite.
//   4. Store nFrame into g_nPlayerCoverFrame.
//   5. Set bit 9 of the new sprite's flags ("cover" marker) and clear bit 3.
//
// Sprite flags layout (0x005b10b0 + idx * 0x58):
//   bit 3  = 0x08 — cleared when setting a cover sprite
//   bit 9  = 0x200 — set to mark as active cover sprite
//
// "You can't cover with a frame that..." assert message is at 0x004d63a4.
// "C:\DevStudio\Projects\Crux\PLAYER.cpp" is at 0x004d63d4.
void Player_SetCoverSprite(int nSpriteIdx, int nFrame)
{
    if (nSpriteIdx == -1) {
        thunk_FUN_00405810(g_nPlayerCoverSpriteIdx);
        g_nPlayerCoverSpriteIdx = -1;
        return;
    }

    // Deselect previously loaded cover sprite
    if (g_nPlayerCoverSpriteIdx != -1) {
        g_abSpriteFlags[g_nPlayerCoverSpriteIdx * (0x58 / sizeof(unsigned int))] &= ~0x200u;
        thunk_FUN_00405810(g_nPlayerCoverSpriteIdx);
    }

    // Load the new sprite
    g_nPlayerCoverSpriteIdx = thunk_FUN_00409570(nSpriteIdx, 0, 0);

    // Validate frame index
    if (g_anAnimFrameCount[g_nPlayerCoverSpriteIdx] <= nFrame) {
        thunk_FUN_0041f680(0xc, "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp",
                           "You can't cover with a frame that");
    }

    g_nPlayerCoverFrame = nFrame;

    // Mark sprite as active cover (set bit 9, clear bit 3)
    g_abSpriteFlags[g_nPlayerCoverSpriteIdx * (0x58 / sizeof(unsigned int))] &= ~0x08u;
    g_abSpriteFlags[g_nPlayerCoverSpriteIdx * (0x58 / sizeof(unsigned int))] |=  0x200u;
}

// ---------------------------------------------------------------------------
// 0x0045d3c0  Player_DrawCoverSprite
// ---------------------------------------------------------------------------
// Blit the current cover sprite frame to the back-buffer.
// Uses g_nPlayerCoverSpriteIdx and g_nPlayerCoverFrame to look up the frame
// record in the animation table, then extracts the (x, y, height) triple
// from the sprite-base table and passes them to BlitResource.
//
// Frame record layout in g_anCoverSpriteBase (0x0051e4f0, 0x20 bytes each):
//   +0x00 = X position
//   +0x04 = Y position
//   +0x10 = height / resource identifier
//
// g_anCoverFrameXY (0x004e3b58): index table, stride 0x640 bytes per sprite
//   slot, 4 bytes per frame — stores the frame record index.
void Player_DrawCoverSprite(void)
{
    if (g_nPlayerCoverSpriteIdx == -1)
        return;

    // Look up frame index for current cover sprite + frame
    int nFrameIdx = g_anCoverFrameXY[g_nPlayerCoverFrame +
                                     g_nPlayerCoverSpriteIdx * (0x640 / sizeof(int))];

    // Extract blit coordinates from sprite base table
    int *pFrame = (int *)((char *)g_anCoverSpriteBase + nFrameIdx * 0x20);

    thunk_FUN_0042bd40(pFrame[0],   // X
                       pFrame[1],   // Y
                       pFrame[4]);  // height / resource id (offset +0x10)
}

// ---------------------------------------------------------------------------
// Additional globals for the SCM / palette subsystem
// ---------------------------------------------------------------------------

int   g_nPlayerPalFreezeMode    = 0;        // 0x00703f3c
int   g_nPalCallback            = 0;        // 0x00703f40
int   g_nPlayerScmCount         = 0;        // 0x00703f50
int   g_nPlayerScmMaxFrames     = 0;        // 0x00703f6c
int   g_nPlayerSharedMode       = 0;        // 0x00703f74
int   g_nPlayerAsyncReadMode    = 0;        // 0x00703f70
int   g_nPlayerTimerId          = 0;        // 0x00703f7c
int   g_nPlayerMusicStopped     = 0;        // 0x00703f48
int   g_nPlayerDebugChunkIdx    = 0;        // 0x00703f84
int   g_nPlayerVoiceMask        = 0;        // 0x00703f88
int   g_nPlayerVoiceMaskDefault = 0;        // 0x00703f8c
char  g_abPlayerScmNames[19 * 260]  = {0};  // 0x006fe4f0
char  g_abPlayerSpeakingChar[260]   = {0};  // 0x006efc10
int   g_nPlayerSharedFile       = 0;        // 0x006efc84

// ---------------------------------------------------------------------------
// 0x00457e60  Player_SetPalFreezeMode
// ---------------------------------------------------------------------------
// If param_1 == 0, set g_nPlayerPalFreezeMode = 1 (freeze palette).
// Otherwise clear it (allow palette changes).
// When frozen, Player_RenderFrame skips calling the closest-colour remapper.
void Player_SetPalFreezeMode(int bFreeze)
{
    g_nPlayerPalFreezeMode = (bFreeze == 0) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// 0x00457f00  Player_SetPalCallback
// ---------------------------------------------------------------------------
// Store pfnCallback as the palette-change notification function pointer.
// Called by Player_RenderFrame when a new palette chunk (type 0x02) arrives.
void Player_SetPalCallback(int pfnCallback)
{
    g_nPalCallback = pfnCallback;
}

// ---------------------------------------------------------------------------
// 0x00457f90  Player_BuildPalLUT
// ---------------------------------------------------------------------------
// Build a 256-entry closest-colour LUT from a 3-bytes-per-entry RGB palette.
//   pLUT[256]   — output LUT (byte per entry: index remapped to pPal index)
//   pPal        — source RGB palette (768 bytes, entries [1..254] remapped)
//   nPalRef     — reference palette handle passed to thunk_FUN_0042fd50
//
// Entry 0 is forced to 0 (black maps to black).
// Entry 255 is forced to 255 (white maps to white).
// Entries 1..254 are resolved by finding the closest colour in pPal.
//
// thunk_FUN_0042fd50 signature (recovered from call site):
//   byte ClosestColor(int nCount, int nRef, byte r, byte g, byte b)
void Player_BuildPalLUT(unsigned char *pLUT, const unsigned char *pPal,
                        int nPalRef)
{
    typedef unsigned char (*ClosestFn_t)(int, int, unsigned char, unsigned char, unsigned char);
    extern ClosestFn_t thunk_FUN_0042fd50;

    for (int i = 1; i < 0xff; i++) {
        pLUT[i] = thunk_FUN_0042fd50(0xff, nPalRef,
                                     pPal[i * 3 + 0],
                                     pPal[i * 3 + 1],
                                     pPal[i * 3 + 2]);
    }
    pLUT[0]    = 0;
    pLUT[0xff] = 0xff;
}

// ---------------------------------------------------------------------------
// 0x004580a0  Player_FlushPalAndBlit
// ---------------------------------------------------------------------------
// Called from Player_StreamThreadProc under the g_nPlayerCS critical section
// once per frame to commit a pending palette change and blit the dirty band.
//
// DAT_006ecd08 = palette-change-pending flag
// DAT_006f4304 = dirty-region top (screen y)
// DAT_006efc8c = dirty-region bottom (screen y)
void Player_FlushPalAndBlit(void)
{
    extern int DAT_006ecd08;
    extern int DAT_006f4304;
    extern int DAT_006efc8c;

    if (DAT_006ecd08 == 1) {
        DAT_006ecd08 = 0;
        Sched_UpdatePalette(1);
    }
    if (DAT_006f4304 < DAT_006efc8c) {
        // Blit the dirty scanline band [top .. bottom]
        // (thunk_FUN_0042c500 = Blit_Band or equivalent)
        typedef void (*BlitBandFn_t)(int, int);
        extern BlitBandFn_t thunk_FUN_0042c500;
        thunk_FUN_0042c500(DAT_006f4304, DAT_006efc8c);
    }
}

// ---------------------------------------------------------------------------
// 0x00458170  Player_SetSharedMode
// ---------------------------------------------------------------------------
// Enter shared playback mode.  In shared mode, Player_GetNextScmName returns
// the same SCM name repeatedly (does not pop the stack).
void Player_SetSharedMode(void)
{
    g_nPlayerSharedMode = 1;
}

// ---------------------------------------------------------------------------
// 0x00458200  Player_SetAsyncReadMode
// ---------------------------------------------------------------------------
// Enter async file-read mode.  When active (value 1 or 2), Player_ScmPlayList
// releases the file lock between reads and uses async I/O callbacks.
void Player_SetAsyncReadMode(void)
{
    g_nPlayerAsyncReadMode = 1;
}

// ---------------------------------------------------------------------------
// 0x00458290  Player_ScmAddChar
// ---------------------------------------------------------------------------
// Prepend a character/SCM name to the play list.
//   pszName — resource name string (up to 260 chars)
//
// Shifts all existing entries up by one slot to make room at index 0,
// then copies pszName into g_abPlayerScmNames[0].  Also reads the SCM
// resource frame count via Res_GetDirectByNumChar(type=0x12) and updates
// g_nPlayerScmMaxFrames with the new maximum.
//
// Limit: max 19 (0x13) entries.  Raises a fatal error via Err_BadResEntry
// if exceeded.  Debug name: "scm_add_char__name"
void Player_ScmAddChar(const char *pszName)
{
    extern void Err_BadResEntry(int nLine, const char *pszFile, const char *pszMsg);
    extern int  Res_GetDirectByNumChar(int nType, const char *pszName,
                                       char *pszPathOut, int nFlag, int *pnHandle);
    typedef void (*MemCopyFn_t)(void *pDst, const void *pSrc);
    extern MemCopyFn_t FUN_004895e0;   // 0x004895e0  260-byte string copy helper

    if (g_nPlayerScmCount > 0x12 /* 18 */) {
        Err_BadResEntry(/* line= */*(int*)(0x004d5d38) + 8,
                        "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp",
                        "Too many scms in list");
    }

    // Shift all entries up by one (prepend semantics)
    for (int i = g_nPlayerScmCount; i > 0; i--) {
        FUN_004895e0(&g_abPlayerScmNames[i * 0x104],
                     &g_abPlayerScmNames[(i - 1) * 0x104]);
    }

    // Insert new entry at slot 0
    FUN_004895e0(&g_abPlayerScmNames[0], pszName);
    g_nPlayerScmCount++;

    // Update max-frame count for rate-pacing
    char szPath[260];
    int  nHandle[4];
    int  nFrames = Res_GetDirectByNumChar(0x12, pszName, szPath, 0, nHandle);
    if (nFrames != 0) {
        int nCount = 0;
        // thunk_FUN_0048a180: read 4 bytes into nCount from resource handle
        typedef void (*ReadFn_t)(int *pDst, int nOne, int nSize, int nHandle);
        extern ReadFn_t FUN_0048a180;
        FUN_0048a180(&nCount, 1, 4, nFrames);
        if (g_nPlayerScmMaxFrames < nCount)
            g_nPlayerScmMaxFrames = nCount;
    }
}

// ---------------------------------------------------------------------------
// 0x00458470  Player_ScmInit
// ---------------------------------------------------------------------------
// Reset all SCM play-list state to zero / sentinel values.
// Called after playback finishes (or at start of a new sequence) to prepare
// for the next call to Player_ScmAddChar / Player_ScmPlayList.
void Player_ScmInit(void)
{
    extern int DAT_004d5d3c;
    extern int DAT_004d5d40;
    extern int DAT_004d5d44;
    extern int DAT_006efc10_sentinel;   // DAT_006efc10 int header
    extern int DAT_004d5d44_flag;

    g_nPlayerScmCount       = 0;
    g_nPlayerScmMaxFrames   = 0;
    // Sentinel values for lip-sync resource handle and frame target
    *(int*)(0x004d5d3c) = -1;
    *(int*)(0x004d5d40) = -1;
    *(int*)(0x006efc10) = 0;   // clear speaking-char name
    *(int*)(0x004d5d44) = (int)0xfffffff6;  // -10 sentinel
    g_nPlayerSharedMode     = 0;
    g_nPlayerAsyncReadMode  = 0;
}

// ---------------------------------------------------------------------------
// 0x00458550  Player_SetSpeakingChar
// ---------------------------------------------------------------------------
// Register a speaking character for lip-sync playback.
//   pszName — character resource name (max 260 chars)
//   nFrame  — target frame at which the speech starts
//
// Loads the .LIP resource (type 0x11) for pszName and caches it in the
// lip-sync data buffer (DAT_006efc90) via Res_BunchFreadNow.  Saves the
// current Txt mode and switches to mode 1 (subtitle mode).
//
// If the LIP resource is not found, traces a warning and continues without
// lip-sync.  Debug name: "set_speaking_sca_char__name_int_f"
void Player_SetSpeakingChar(const char *pszName, int nFrame)
{
    extern int   Res_FindByNumChar(int, const char *, char *, int, int *, int *);
    extern void  Res_BunchFreadNow(void *pBuf, int nOne, int nHandle, int *pLoc);
    extern int   Txt_GetMode(void);
    extern void  Txt_SetMode(int nMode);
    extern void  Debug_TraceVal(const char *pszFmt, const char *pszArg);

    typedef void (*MemCopyFn_t)(void *pDst, const void *pSrc);
    extern MemCopyFn_t FUN_004895e0;   // 260-byte string copy

    // Copy speaking char name into global buffer
    FUN_004895e0(g_abPlayerSpeakingChar, pszName);

    // Try to find LIP resource
    char  szPath[260];
    int   nLocInfo[4];
    int   nHandle = 0;
    int   nResult = Res_FindByNumChar(0x11, pszName, szPath, 0,
                                      nLocInfo, &nHandle);
    if (nResult == 0) {
        // Resource found — load it
        extern void *DAT_006efc90;   // lip-sync data buffer
        Res_BunchFreadNow(&DAT_006efc90, 1, nHandle, nLocInfo);
        *(int*)(0x004d5d3c) = nHandle;          // store resource handle
        g_nPlayerMissedFrames = 0;
        *(int*)(0x004d5d44) = 1;                // lip-sync active
        *(int*)(0x004d5d40) = nFrame;           // target frame
        *(int*)(0x004d5db0) = Txt_GetMode();    // save current Txt mode
        Txt_SetMode(1);
    } else {
        Debug_TraceVal("Warning: lips not found on speaki%s\n", pszName);
    }
    // Clear internal frame counter
    *(int*)(0x00703f78) = 0;
}

// ---------------------------------------------------------------------------
// 0x004586c0  Player_InitEvents
// ---------------------------------------------------------------------------
// Create Win32 event handles and critical sections for the streaming thread.
// Idempotent — no-op if g_nPlayerInitialized is already non-zero.
void Player_InitEvents(void)
{
    if (g_nPlayerInitialized == 0) {
        g_hPlayerSyncEvent  = CreateEventA(NULL, FALSE, FALSE, NULL);
        g_hPlayerAuxEvent   = CreateEventA(NULL, FALSE, FALSE, NULL);
        // Third event: auto-reset, initially unsignalled
        *(HANDLE*)(0x00703f60) = CreateEventA(NULL, FALSE, FALSE, NULL);
        // Frame event: manual-reset, initially signalled (allows first read)
        g_hPlayerFrameEvent = CreateEventA(NULL, TRUE, TRUE, NULL);
        InitializeCriticalSection((LPCRITICAL_SECTION)&g_nPlayerCS);
        InitializeCriticalSection((LPCRITICAL_SECTION)(0x00703f10));
        g_nPlayerInitialized = 1;
    }
}

// ---------------------------------------------------------------------------
// 0x004587e0  Player_WriteDebugChunk
// ---------------------------------------------------------------------------
// Write a numbered binary debug-chunk file "CHNK_%04d.BIN" to disk.
// Increments g_nPlayerDebugChunkIdx on each call.
// Used during development to capture raw SCM chunk data for offline analysis.
void Player_WriteDebugChunk(const void *pData, int nSize)
{
    extern int   FUN_0048a060(char *pszOut, const char *pszFmt, int nIdx);
    extern void* FUN_0048a340(const char *pszName, const char *pszMode);
    extern void  FUN_0048a490(const void *pData, int nOne, int nSize, void *pFile);
    extern void  FUN_0048a0d0(void *pFile);

    char szName[260];
    int  iChunk = g_nPlayerDebugChunkIdx;
    g_nPlayerDebugChunkIdx++;

    FUN_0048a060(szName, "CHNK_%04d.BIN", iChunk);
    void *pFile = FUN_0048a340(szName, "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp");
    FUN_0048a490(pData, 1, nSize, pFile);
    FUN_0048a0d0(pFile);
}

// ---------------------------------------------------------------------------
// 0x00458900  Player_PlayScm
// ---------------------------------------------------------------------------
// High-level entry point: play a single SCM (scripted cutscene movie).
//   pszName     — SCM resource name (character identifier or cutscene key)
//   nAbortFlags — abort-check mode for Player_IsAbortPressed
//   bLetItBreak — if non-zero, allow early frame-break exit
//   nExtra      — additional parameter passed to Player_ScmPlayList
//
// Flow:
//   1. If not in shared-file mode, Player_ScmAddChar(pszName).
//   2. Player_ScmPlayList(nAbortFlags, bLetItBreak, nExtra).
//   3. Player_ScmInit() to reset state.
//   4. Restore saved Txt mode (if != -1).
void Player_PlayScm(const char *pszName, int nAbortFlags,
                    int bLetItBreak, int nExtra)
{
    extern int  Txt_SetMode(int);
    extern int  DAT_004d5db0;   // saved Txt mode (-1 = not saved)

    if (g_nPlayerSharedFile == 0) {
        Player_ScmAddChar(pszName);
    }
    Player_ScmPlayList(nAbortFlags, bLetItBreak, nExtra);
    Player_ScmInit();

    if (DAT_004d5db0 != -1) {
        Txt_SetMode(DAT_004d5db0);
    }
    DAT_004d5db0 = -1;
}

// ---------------------------------------------------------------------------
// 0x004589f0  Player_SetFlags
// ---------------------------------------------------------------------------
// OR nMask into g_nPlayerVoiceMask to enable voice channels.
void Player_SetFlags(unsigned int nMask)
{
    g_nPlayerVoiceMask |= nMask;
}

// ---------------------------------------------------------------------------
// 0x00458a80  Player_ClearFlags
// ---------------------------------------------------------------------------
// AND nMask into g_nPlayerVoiceMask to disable voice channels.
void Player_ClearFlags(unsigned int nMask)
{
    g_nPlayerVoiceMask &= nMask;
}

// ---------------------------------------------------------------------------
// 0x00458b10  Player_ResetFlags
// ---------------------------------------------------------------------------
// Set both g_nPlayerVoiceMask and g_nPlayerVoiceMaskDefault to nMask.
// Called at the start of playback to restore the saved default mask after
// Player_SetFlags/ClearFlags have been applied per-sequence.
void Player_ResetFlags(int nMask)
{
    g_nPlayerVoiceMaskDefault = nMask;
    g_nPlayerVoiceMask        = nMask;
}

// ---------------------------------------------------------------------------
// 0x00458bb0  Player_ScmPlayList
// ---------------------------------------------------------------------------
// Core SCM playlist playback engine (~7 KB function).
//
// This function drives the full synchronised read-and-display loop for one
// or more SCM entries.  It is too large to reproduce verbatim; the following
// is a structural summary with key constants and chunk-type dispatch table.
//
// Entry:
//   bCheckAbort  — if non-zero, poll Player_IsAbortPressed each frame
//   nFrameLimit  — upper bound on frames to process (0 = until end of list)
//   bLetItBreak  — stop at a natural break point rather than playing to end
//
// Setup phase (before the loop):
//   1. Clamp frame budget to DAT_007c5d94 (CD sector count), max 3100000.
//   2. If not in shared-file mode, call Player_GetNextScmName to pop the
//      first entry name.
//   3. Call Player_InitEvents / Sched_BeginHighPriority.
//   4. Set up the double-buffer ring: DAT_006fe2d8 and DAT_006fe2dc pointing
//      into the MIXER's PCM pool.
//   5. Resolve the per-frame transfer budget via Player_Init / Res_GetTransferRate.
//   6. Load the .LIP resource if g_nPlayerSpeakingFrameTarget == 1.
//   7. Optionally acquire the file lock (Res_AcquireFileLock).
//
// Pre-read phase (populates the ring buffer before the thread starts):
//   Reads chunk headers (8 bytes each) and dispatches by chunk type:
//     0x10 = video    → direct read into ring buffer
//     0x02 = palette  → direct read
//     0x40..0x43 = music bunch → read into double-buffer slot
//     0x80..0x83 = audio bunch → read into MIXER bunch buffer
//     0x100 = speech  → skip  (handled per-frame by Player_RenderFrame)
//     0x400..0x402 = lip data → read if fits in budget
//     0x1000 = text   → direct read
//
// Streaming phase (after thread launch):
//   Creates a thread (Player_StreamThreadProc) with above-normal priority.
//   Enters a WaitForSingleObject / WaitForMultipleObjects loop, calling
//   Player_FlushVoices each iteration and monitoring abort conditions.
//
// Teardown:
//   Signals g_hPlayerAuxEvent to stop the thread, waits 1 second for join,
//   releases file lock, calls Sched_EndHighPriority, kills the sync timer.
//
// Debug name: "scm_play_list_int_let_it_break_i"
void Player_ScmPlayList(int bCheckAbort, int nFrameLimit, int bLetItBreak)
{
    // (Implementation is 7 KB / ~300 lines in the original; see Ghidra at
    // 0x00458bb0 for the full decompiled body.  Stub retained here for
    // ScummVM porting reference.)
    (void)bCheckAbort; (void)nFrameLimit; (void)bLetItBreak;
}

// ---------------------------------------------------------------------------
// 0x0045ace0  Player_GetNextScmName
// ---------------------------------------------------------------------------
// Pop or peek the next SCM resource name from g_abPlayerScmNames.
//
// In normal mode (g_nPlayerSpeakingFrameTarget == -10 && !g_nPlayerSharedMode):
//   Pop: decrement g_nPlayerScmCount and copy the entry at the new top index
//   into pszOut.  If the stack becomes empty, set *pszOut = '\0'.
//
// In shared or non-normal mode:
//   Peek: copy g_abPlayerScmNames[0] (entry 0) without modifying the count.
void Player_GetNextScmName(char *pszOut)
{
    typedef void (*MemCopyFn_t)(void *pDst, const void *pSrc);
    extern MemCopyFn_t FUN_004895e0;
    extern int DAT_004d5d44;   // -10 sentinel value for speaking frame target

    if ((DAT_004d5d44 == -10) && (g_nPlayerSharedMode == 0)) {
        if (g_nPlayerScmCount == 0) {
            *pszOut = '\0';
        } else {
            g_nPlayerScmCount--;
            FUN_004895e0(pszOut,
                         &g_abPlayerScmNames[g_nPlayerScmCount * 0x104]);
        }
    } else {
        FUN_004895e0(pszOut, &g_abPlayerScmNames[0]);
    }
}

// ---------------------------------------------------------------------------
// 0x0045ade0  Player_StreamThreadProc
// ---------------------------------------------------------------------------
// Background streaming thread.  Runs until g_hPlayerAuxEvent fires.
//
// On start:
//   - Sets up timer: Theme_SetTimer(Player_StreamSyncCallback, period).
//   - Stores event handle array for WaitForMultipleObjects.
//
// Main loop (repeated until WaitForMultipleObjects returns WAIT_OBJECT_0+1):
//   1. Increment frame counter (if not in palette-transition freeze mode).
//   2. If stream handle != stream param: flush voices, dispatch bunch-audio
//      targets to voice channels, handle lip-sync text frame trigger.
//   3. EnterCriticalSection → Player_RenderFrame → LeaveCriticalSection.
//   4. SetEvent(DAT_00703f60) to wake the main thread.
//   5. If in palette-transition mode (DAT_00703f38 == 2): apply palette,
//      call thunk_FUN_0046e9a0.
//   6. Else: WaitForSingleObject(g_hPlayerSyncEvent, 0) / SetEvent(FrameEvent)
//      / WaitForMultipleObjects.
//
// Exits via ExitThread(0) — does not return to caller.
// Debug name: (no embedded name; entry recognised by thread-start signature)
void Player_StreamThreadProc(void)
{
    // (Full implementation: see Ghidra 0x0045ade0.  Not reproduced here.)
    ExitThread(0);
}

// ---------------------------------------------------------------------------
// 0x0045b260  Player_RenderFrame
// ---------------------------------------------------------------------------
// Decode and display one SCM video frame (called from Player_StreamThreadProc
// under the CS at 0x00703f10, once per sync-timer tick).
//
// Key chunk-type dispatch (short at header+6):
//   0x10   Video frame: blit 640×480 pixels via thunk_FUN_0042bd40 (normal)
//          or thunk_FUN_0042d2a0 (palette-remap path when DAT_00703f38==1).
//   0x02   Palette block: compare vs g_abTargetPal; if different, build
//          two-phase LUT tables and queue a palette transition.
//   0x100  Speech/control block: dispatch by sub-opcode (word at header+6):
//          0=lips-off, 1=lips-on, 2=wait-N, 3=wait-1, 4=lips-off-quiet,
//          5=lips-on-quiet, 6=music-stop, 7=music-loop, 8=voice-flags,
//          9=voice-flags-2, 10=voice-active, 11=volume, 12=music-pause.
//   0x40..0x43  Music buffer swap (double-buffer music ring).
//   0x80..0x83  Bunch-audio: schedule audio bunch-read for lip-sync channels.
//   0x400  Lip-sync frame data: copy into DAT_006efc90 if size < 2001 bytes.
//   0x401  Lip-sync SCI data: copy into per-channel buffer (up to 10 channels).
//   0x1000 Text subtitle: Txt_SetString into the text display buffer.
//
// After processing all chunks for the current stream slot:
//   - Handles DAT_006fe4ec (lips-active flag) → draws lip sprite.
//   - Calls Player_DrawCoverSprite.
//   - Calls Txt_DrawBackground, Txt_Update.
//
// Debug name: "void_sca_frame___"
void Player_RenderFrame(void)
{
    // (Full implementation: see Ghidra 0x0045b260.  Not reproduced here.)
}

// ---------------------------------------------------------------------------
// 0x0045c610  Player_StartMusicLoop
// ---------------------------------------------------------------------------
// Start or continue the music loop for the current SCM track.
//
// Copies music state block from DAT_00629880 ← DAT_00703f4c.
// If g_nPlayerMusicTarget == -1: call MIXER::Stop (thunk_FUN_00443df0).
// Otherwise: call the theme fade-in engine (thunk_FUN_004427e0) with the
// buffered music data, then register the next-loop callback via
// thunk_FUN_00442f40(param_1, Player_StartMusicLoop) to loop continuously.
//
// Also calls thunk_FUN_004433b0 to update the volume setting.
//
// Called recursively via the MIXER callback mechanism to sustain the loop.
void Player_StartMusicLoop(int bParam)
{
    extern void FUN_004895e0(void *pDst, const void *pSrc);
    extern void thunk_FUN_00443df0(int bParam);
    extern void thunk_FUN_004427e0(int bParam, void *pBuf, int nSize,
                                   int bStereo, int nRate, int nExtra,
                                   int nVolChannel, int nVol);
    extern void thunk_FUN_004433b0(int nZero, int nVolChannel, int nVol);
    extern void thunk_FUN_00442f40(int bParam, void (*pfnCallback)(int));
    extern int  DAT_004d58d8;   // music-loop target slot (-1 = no loop)
    extern int  DAT_004d58dc;   // volume channel index
    extern int  DAT_00629880;   // music state copy target
    extern int  DAT_00703f4c;   // music state source

    FUN_004895e0((void*)&DAT_00629880, (const void*)&DAT_00703f4c);

    if (DAT_004d58d8 == -1) {
        thunk_FUN_00443df0(bParam);
    } else {
        // thunk_FUN_004427e0: start music with double-buffer slot, rate, flags
        // See Ghidra 0x0045c610 for exact argument layout from DAT_006fe2d8,
        // DAT_006f0460, DAT_006fa140.
        DWORD dwNow = timeGetTime();
        Debug_Trace(10, "C:\\DevStudio\\Projects\\Crux\\PLAYER.cpp",
                    "%d %d > %d at %d",
                    g_nPlayerCurrentFrame, g_nPlayerLastFrameTime,
                    DAT_004d58d8, dwNow);
        // ... (see Ghidra for arg computation from DAT_006fe2d8 / DAT_006f0460)
        thunk_FUN_004433b0(0, DAT_004d58dc, -1);
        thunk_FUN_00442f40(bParam, Player_StartMusicLoop);
    }
}

// ---------------------------------------------------------------------------
// NOTE: 0x0045d4e0  Res_FindByNumChar  — belongs to READRES.cpp
// ---------------------------------------------------------------------------
// Despite lying at the very next address after Player_DrawCoverSprite, this
// function is NOT part of PLAYER.cpp.  Evidence:
//
//   - The local variable "local_18" is initialised to the string literal
//     "bunch_find_int_num_char__name_ch_004d666c" — this is the function's
//     own debug name, matching the READRES subsystem naming convention
//     ("bunch_*").
//   - It references g_nCritSectInit, g_szResPath, g_nResFileCount,
//     g_nResNames, g_nCurrentDiskNum — all globals owned by READRES.
//   - Its debug __FILE__ string (0x004d66cc) reads
//     "C:\DevStudio\Projects\Crux\READRES.cpp".
//   - READRES.h documents "Res_GetDirectByNumChar" at 0x0045d810 but notes
//     a helper ("bunch_find_int_num_char") called from within it — this is
//     that helper.
//
// The function has been renamed to Res_FindByNumChar in Ghidra and its
// declaration belongs in READRES.h / READRES.cpp.
