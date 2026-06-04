// ---------------------------------------------------------------------------
// PLAYER.h  —  FMV / streaming video player public interface
//
// This module drives the synchronised video+audio playback engine used for
// in-game cutscenes.  It owns:
//
//   - A background Win32 thread that performs rate-paced bunch reads from the
//     ADVENT.RES file, synchronised to a pair of Win32 events.
//   - A 4000-entry target-frame table (g_anPlayerBunchTargets) that the
//     scheduler fills with frame deadlines; the reader thread picks tasks in
//     frame order.
//   - Three audio voice channels (g_abPlayerVoiceFlags[3]) that are flushed
//     between FMV frames via Player_FlushVoices.
//   - A 256-entry palette LUT (g_abPlayerPalLUT) applied by Player_RemapPalette
//     to convert 8-bit FMV pixels to the current screen palette.
//   - A "cover sprite" overlay (Player_SetCoverSprite / Player_DrawCoverSprite)
//     that blits a fullscreen animation frame over the back-buffer before each
//     FMV frame is displayed.
//
// Original source: C:\DevStudio\Projects\Crux\PLAYER.cpp
// Address range:   0x00457e60 -- 0x0045d3c0  (two contiguous address blocks)
//
// Note: 0x0045d4e0 (Res_FindByNumChar) sits in the same address range but
// belongs to READRES.cpp — its debug string is "bunch_find_int_num_char__name_ch".
// Note: 0x004577a0–0x00457c90 (OTF_AllocSlot, OTF_AllocNodeList, OTF_AreaTip)
// belong to ONTHEFLY.cpp, not PLAYER.cpp.
// ---------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Palette remapping LUT [256] — filled by SETPAL subsystem, applied per-row by
// Player_RemapPalette to convert raw FMV pixel indices to the current palette.
extern unsigned char g_abPlayerPalLUT[256];         // 0x006ffc80

// Bunch-read target frame table [4000] — each entry holds the frame number
// that the corresponding bunch-read task must complete by.  Initialised to
// 100000 (unreachably far future) by Player_Init.
extern int  g_anPlayerBunchTargets[4000];           // 0x006ffd88

// Current bunch-read queue index — points to the active slot in
// g_anPlayerBunchTargets being serviced by the reader thread.
extern int  g_nPlayerBunchIdx;                      // 0x006f42f4

// Running frame counter for streaming playback.  Initialised to -1 by
// Player_Init; incremented by the display path.  Used by Player_BunchRead to
// decide how many CD sectors can be pre-fetched each wakeup.
extern int  g_nPlayerCurrentFrame;                  // 0x006ffc78

// Running file read/write cursor — offset within the ADVENT.RES stream.
// Incremented by Player_WriteData and Player_ReadData.
extern int  g_nPlayerFileOffset;                    // 0x006ecd0c

// CD-ROM transfer rate in bytes/frame from Res_GetTransferRate().
// Stored at init time; used by Player_BunchRead to size each read chunk.
extern int  g_nPlayerTransferRate;                  // 0x006ecd14

// Missed-frame counter — incremented by Player_StreamSyncCallback each time
// the sync event fires too late (timeGetTime overran the previous frame time).
extern int  g_nPlayerMissedFrames;                  // 0x006ffc40

// Timestamp of the previous frame sync (timeGetTime).
extern int  g_nPlayerLastFrameTime;                 // 0x006f5310

// Player subsystem state: 0 = idle.
extern int  g_nPlayerState;                         // 0x006fa138

// Miscellaneous player flags.
extern int  g_nPlayerFlags;                         // 0x006ecd18

// Per-voice state bytes [3].  Non-zero value = voice pending flush.
// Initialised from thunk_FUN_0046faf0 (audio subsystem default voice state).
extern unsigned char g_abPlayerVoiceFlags[3];       // 0x006efc08

// Bitmask of voices with a pending flush (bits 0..2 correspond to voices 0..2).
extern int  g_nPlayerVoicePendingMask;              // 0x006f4358

// Non-zero if any voice channel is currently active.
extern unsigned char g_bPlayerVoiceActive;          // 0x006eccf0

// Non-zero while a streaming FMV sequence is in progress.
extern unsigned char g_bPlayerStreamActive;         // 0x006f4314

// Set to 1 by Player_StartStream to signal that streaming has begun.
extern int  g_nPlayerStreamStarted;                 // 0x006f4360

// Stream resource handle — copied from g_nPlayerStreamParam by Player_StartStream.
extern int  g_nPlayerStreamHandle;                  // 0x006ffd80

// Source slot for the stream handle, set by the caller before Player_StartStream.
extern int  g_nPlayerStreamParam;                   // 0x006f4368

// Non-zero if the player subsystem is initialised (thread + events are live).
extern int  g_nPlayerInitialized;                   // 0x00703f68

// Handle to the player streaming thread.
extern HANDLE g_hPlayerThread;                      // 0x00703f54

// Event signalled when a streaming buffer slot becomes ready for the next read.
extern HANDLE g_hPlayerSyncEvent;                   // 0x00703f5c

// Event waited on by Player_BunchRead; fired once per display frame.
extern HANDLE g_hPlayerFrameEvent;                  // 0x00703f64

// Auxiliary event handle used by the player thread.
extern HANDLE g_hPlayerAuxEvent;                    // 0x00703f58

// Thread ID of the player streaming thread.
extern DWORD  g_dwPlayerThreadId;                   // 0x006fa148

// Index of the currently loaded cover sprite (-1 = none).
extern int  g_nPlayerCoverSpriteIdx;                // 0x004d6334

// Frame index within the cover sprite animation.
extern int  g_nPlayerCoverFrame;                    // 0x004d6338

// SCM palette freeze: when non-zero, skip palette remapping in RenderFrame.
extern int  g_nPlayerPalFreezeMode;                 // 0x00703f3c

// Palette-change callback function pointer (or 0 for none).
// Signature: void callback(unsigned char *pNewPal256x3).
extern int  g_nPalCallback;                         // 0x00703f40

// Number of SCM entries currently in the play list (max 0x13 = 19).
extern int  g_nPlayerScmCount;                      // 0x00703f50

// Maximum frame count across all SCM entries in the current play list.
extern int  g_nPlayerScmMaxFrames;                  // 0x00703f6c

// Shared mode flag: 1 = broadcast/shared playback, 0 = normal.
extern int  g_nPlayerSharedMode;                    // 0x00703f74

// Async read mode: 0 = sync, 1 = async-start, 2 = async-active.
extern int  g_nPlayerAsyncReadMode;                 // 0x00703f70

// Timer ID from Theme_SetTimer for Player_StreamSyncCallback; 0 = no timer.
extern int  g_nPlayerTimerId;                       // 0x00703f7c

// Non-zero if background music was stopped by SCM command 7 (restart on exit).
extern int  g_nPlayerMusicStopped;                  // 0x00703f48

// Auto-incrementing debug chunk file index (used by Player_WriteDebugChunk).
extern int  g_nPlayerDebugChunkIdx;                 // 0x00703f84

// Voice enable bitmask; bits 0/1 correspond to voice channels 0 and 1.
extern int  g_nPlayerVoiceMask;                     // 0x00703f88

// Saved default voice mask; restored at end of playback by Player_ResetFlags.
extern int  g_nPlayerVoiceMaskDefault;              // 0x00703f8c

// SCM name list: up to 0x13 entries, each 0x104 (260) bytes.
// Managed as a LIFO stack; Player_ScmAddChar prepends; Player_GetNextScmName pops.
extern char g_abPlayerScmNames[19 * 260];           // 0x006fe4f0

// Currently speaking character name (set by Player_SetSpeakingChar).
extern char g_abPlayerSpeakingChar[260];            // 0x006efc10

// Non-zero when playback uses a shared/pre-opened file handle.
extern int  g_nPlayerSharedFile;                    // 0x006efc84

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

// 0x0045c7d0  Remap an 8-bit pixel buffer through g_abPlayerPalLUT.
//   pBuf   — pointer to the first pixel row
//   nWidth — row pitch (bytes per row; must equal 0x280 = 640)
//   nRows  — number of rows to remap
// The function iterates rows of width nWidth, remapping each byte in-place.
void Player_RemapPalette(unsigned char *pBuf, int nWidth, int nRows);

// 0x0045c860  Stream-sync callback — fired by the timer/display system once per
//   frame to signal that a new frame interval has begun.  Increments
//   g_nPlayerMissedFrames if the sync event (g_hPlayerSyncEvent) is already
//   signalled (i.e. the reader fell behind), then resets/sets the events.
void Player_StreamSyncCallback(void);

// 0x0045c960  Flush all pending audio voice channels.
//   For each of the 3 voice slots, if its bit is set in g_nPlayerVoicePendingMask,
//   calls the audio subsystem flush helper, then clears the mask.
void Player_FlushVoices(void);

// 0x0045ca80  Initialise the Player subsystem.
//   Resets all counters, clears the bunch-target table to 100000, initialises
//   voice state from the audio subsystem, and fetches the CD transfer rate.
void Player_Init(void);

// 0x0045cca0  Write nBytes bytes from pBuf into the ADVENT.RES stream at the
//   current file offset.  Acquires/releases the file lock around the write and
//   advances g_nPlayerFileOffset by nBytes.
//   (thiscall — param_1 is the file-object pointer from the caller's context.)
unsigned int Player_WriteData(void *pFile, void *pBuf, int nBytes);

// 0x0045ccf0  Read up to (nCount * nSize) bytes from the ADVENT.RES stream.
//   Acquires the file lock, seeks to the current offset, performs the read, then
//   releases the lock.  Returns the number of bytes actually read.
//   (thiscall — param_1 is the file-object pointer.)
int Player_ReadData(void *pFile, void *pBuf, int nCount, void *pDst, void *pFile2);

// 0x0045cd60  Rate-paced bunch read — transfer (nCount * nSize) bytes from the
//   stream to pBuf, broken into chunks that fit within one frame's CD budget.
//   Waits on g_hPlayerFrameEvent between chunks; uses EnterCriticalSection to
//   serialise the inner read call.
void Player_BunchRead(int pBuf, int nCount, int nSize, void *pFile);

// 0x0045cf30  Return non-zero if the player abort key has been pressed.
//   Checks for ESC (0x1b) or a special skip key (0x25 = left-arrow / PgDn).
//   If bCheckEscape is non-zero, also tests whether a "non-blocking" key check
//   returns non-zero before inspecting the key code.
int  Player_IsAbortPressed(int bCheckEscape);

// 0x0045d010  Start a new stream session.
//   Sets g_nPlayerStreamStarted = 1 and copies g_nPlayerStreamParam into
//   g_nPlayerStreamHandle.  Called just before the reader thread begins.
void Player_StartStream(void);

// 0x0045d0b0  Shut down the player subsystem.
//   If g_nPlayerInitialized is set, enters g_nPlayerCS, terminates and closes
//   the streaming thread and all event handles, then leaves the critical section.
void Player_Shutdown(void);

// 0x0045d220  Load a cover sprite for index nSpriteIdx at frame nFrame.
//   If nSpriteIdx == -1, frees the current sprite and sets g_nPlayerCoverSpriteIdx
//   to -1.  Otherwise frees the old sprite (if any), loads the new one via
//   thunk_FUN_00409570, validates that nFrame is in range, then sets bit 9 in the
//   sprite flags (marks it as "cover") and clears bit 3.
void Player_SetCoverSprite(int nSpriteIdx, int nFrame);

// 0x0045d3c0  Blit the current cover sprite frame to the screen.
//   Uses g_nPlayerCoverSpriteIdx and g_nPlayerCoverFrame to look up the frame
//   pointer from the animation table, then calls BlitResource (thunk_FUN_0042bd40)
//   with the frame's (x, y, resource) triple.
void Player_DrawCoverSprite(void);

// ---------------------------------------------------------------------------
// SCM palette helpers  (0x00457e60 -- 0x004580a0)
// ---------------------------------------------------------------------------

// 0x00457e60  Set the palette freeze mode flag (g_nPlayerPalFreezeMode).
//   If param_1 == 0, set the flag (freeze); otherwise clear it.
//   When frozen, Player_RenderFrame skips palette remapping.
void Player_SetPalFreezeMode(int bFreeze);

// 0x00457f00  Store param_1 as the palette-change callback pointer
//   (g_nPalCallback).  Called with a new RGB palette when a palette chunk
//   is encountered during SCM frame rendering.
void Player_SetPalCallback(int pfnCallback);

// 0x00457f90  Build a 256-entry closest-colour LUT (param_1[256]) from
//   an RGB palette (param_2, 3 bytes/entry) using thunk_FUN_0042fd50 as the
//   closest-colour finder.  Entry 0 is forced to 0; entry 255 to 255.
//   Used by Player_RenderFrame when transitioning between palettes.
void Player_BuildPalLUT(unsigned char *pLUT, const unsigned char *pPal,
                        int nPalRef);

// 0x004580a0  Apply a pending palette update and flush the dirty screen band.
//   If g_nPlayerPalChangeFlag is set, calls Sched_UpdatePalette(1) and clears
//   it.  Then if the current dirty-top < dirty-bottom, calls the blit helper
//   to flush that band to screen.
void Player_FlushPalAndBlit(void);

// ---------------------------------------------------------------------------
// SCM mode flags  (0x00458170 -- 0x00458b10)
// ---------------------------------------------------------------------------

// 0x00458170  Set g_nPlayerSharedMode = 1 (enter shared/broadcast playback mode).
void Player_SetSharedMode(void);

// 0x00458200  Set g_nPlayerAsyncReadMode = 1 (begin async file-read mode).
//   When non-zero, Player_ScmPlayList releases the file lock between reads
//   and uses async I/O helpers.
void Player_SetAsyncReadMode(void);

// ---------------------------------------------------------------------------
// SCM playlist management  (0x00458290 -- 0x00458bb0)
// ---------------------------------------------------------------------------

// 0x00458290  Append a character name (param_1) to the SCM play list.
//   Shifts all existing entries up by one slot (prepend semantics) and
//   inserts param_1 at slot 0 of g_abPlayerScmNames.  Also reads the SCM
//   resource length via Res_GetDirectByNumChar(0x12) and updates
//   g_nPlayerScmMaxFrames.  Max 0x13 (19) entries; fatal error if exceeded.
//   Debug name: "scm_add_char__name"
void Player_ScmAddChar(const char *pszName);

// 0x00458470  Initialise the SCM play-list subsystem.
//   Zeros g_nPlayerScmCount, g_nPlayerScmMaxFrames, resets sentinel values,
//   clears g_nPlayerSharedMode, g_nPlayerAsyncReadMode, and other SCM state.
//   Debug name: (no embedded name; inferred from context)
void Player_ScmInit(void);

// 0x00458550  Set the currently speaking character for lip-sync.
//   Copies param_1 (character name) into g_abPlayerSpeakingChar, then loads
//   the lip-sync (.LIP) data via Res_FindByNumChar(0x11), reads it into the
//   lip-sync buffer (DAT_006efc90) via Res_BunchFreadNow, and stores the
//   resource handle and frame-target in the lip-sync state globals.
//   Also sets Txt text mode to 1 and saves the previous mode.
//   Debug name: "set_speaking_sca_char__name_int_f"
void Player_SetSpeakingChar(const char *pszName, int nFrame);

// 0x004586c0  Lazily initialise the player event handles and critical sections.
//   Creates g_hPlayerSyncEvent, g_hPlayerAuxEvent, DAT_00703f60, and
//   g_hPlayerFrameEvent via CreateEventA, and calls InitializeCriticalSection
//   for g_nPlayerCS and DAT_00703f10.  No-op if g_nPlayerInitialized != 0.
void Player_InitEvents(void);

// 0x004587e0  Write a debug chunk file (CHNK_%04d.BIN) to disk.
//   Each call increments g_nPlayerDebugChunkIdx and creates a new numbered
//   file.  Used during development to capture SCM data chunks.
void Player_WriteDebugChunk(const void *pData, int nSize);

// 0x00458900  Play a single SCM entry.
//   Calls Player_ScmAddChar(param_1), then Player_ScmPlayList(param_2,
//   param_3, param_4), then Player_ScmInit().  Also restores the saved Txt
//   mode (g_szPlayerSavedTxtMode) if it is not -1.
//   param_1  SCM name string (character / cutscene identifier)
//   param_2  abort-check flags for Player_IsAbortPressed
//   param_3  let-it-break flag (0 = play to end, 1 = allow early exit)
//   param_4  extra param passed through to Player_ScmPlayList
void Player_PlayScm(const char *pszName, int nAbortFlags,
                    int bLetItBreak, int nExtra);

// 0x004589f0  OR param_1 into g_nPlayerVoiceMask.  Used to enable voice channels.
void Player_SetFlags(unsigned int nMask);

// 0x00458a80  AND param_1 into g_nPlayerVoiceMask.  Used to disable voice channels.
void Player_ClearFlags(unsigned int nMask);

// 0x00458b10  Reset g_nPlayerVoiceMask and g_nPlayerVoiceMaskDefault to param_1.
//   Called at start of playback to restore the saved default mask.
void Player_ResetFlags(int nMask);

// 0x00458bb0  Core SCM playlist playback engine.
//   param_1  bCheckAbort — non-zero to check for abort/escape key
//   param_2  nFrameLimit — maximum number of frames to play (0 = to end)
//   param_3  bLetItBreak — 0 = play all frames, 1 = stop at natural break point
//   Iterates all SCM entries in g_abPlayerScmNames, loads each resource
//   (type 0x10), and drives the streaming read loop:
//     - Reads 8-byte chunk headers to decode chunk type (short at +6) and
//       size (int at +0), then dispatches to the appropriate handler.
//     - Creates a background thread (Player_StreamThreadProc) for async reads.
//     - Calls Player_FlushVoices each frame loop iteration.
//     - Monitors Player_IsAbortPressed and g_nPlayerMissedFrames for cutoff.
//   Debug name: "scm_play_list_int_let_it_break_i"
void Player_ScmPlayList(int bCheckAbort, int nFrameLimit, int bLetItBreak);

// ---------------------------------------------------------------------------
// SCM streaming subsystem  (0x0045ace0 -- 0x0045c610)
// ---------------------------------------------------------------------------

// 0x0045ace0  Pop the next SCM name from g_abPlayerScmNames into param_1.
//   If in normal playback mode (g_nPlayerSpeakingFrameTarget == -10 and
//   g_nPlayerSharedMode == 0): pops from the stack (decrements
//   g_nPlayerScmCount, copies entry at new top).  If in shared or non-normal
//   mode: copies entry 0 (g_abPlayerScmNames[0]) without popping.
//   Sets *param_1 = '\0' if the stack is empty.
void Player_GetNextScmName(char *pszOut);

// 0x0045ade0  Background streaming thread procedure for SCM playback.
//   Sets up the sync timer via Theme_SetTimer(Player_StreamSyncCallback),
//   then loops: dispatching bunch-target audio to voice channels, handling
//   lip-sync frames, updating the screen palette, and waiting on
//   WaitForMultipleObjects.  Exits on g_hPlayerAuxEvent signal (WAIT_OBJECT_0+1).
//   Calls ExitThread(0) at end (no return).
void Player_StreamThreadProc(void);

// 0x0045b260  Render one SCM frame.
//   Increments g_nPlayerLastFrameTime, traces the frame timestamp, then
//   iterates all chunk entries for the current stream slot:
//     0x10 = video frame: blit via thunk_FUN_0042bd40 or thunk_FUN_0042d2a0
//     0x02 = palette: compare vs current palette; if different, update
//            g_abTargetPal and kick palette-transition machinery
//     0x100 = speech/control: dispatch sub-opcode (0=lips-off, 1=lips-on,
//             2=wait-N-frames, 3=wait-1-frame, 4=lips-off-quiet, 5=lips-on-quiet,
//             6=music-stop, 7=music-loop, 8=voice-flags, 9=voice-flags-2,
//             10=voice-active, 11=volume, 12=music-pause)
//     0x40..0x43 = music buffer swap
//     0x80..0x83 = bunch-audio (lip-sync sample data)
//     0x400 = lip-sync frame data (into DAT_006efc90)
//     0x401 = lip-sync SCI data (multi-channel lip data)
//     0x1000 = text subtitle (Txt_SetString)
//   Also draws the cover sprite and calls Txt_DrawBackground/Txt_Update.
//   Debug name: "void_sca_frame___"
void Player_RenderFrame(void);

// 0x0045c610  Start or continue a music loop for the current SCM track.
//   Copies the current music-track state (DAT_00629880 from DAT_00703f4c).
//   If no music-loop target is set (g_nPlayerMusicTarget == -1), calls
//   MIXER::Stop (thunk_FUN_00443df0).  Otherwise starts the music via
//   thunk_FUN_004427e0 with the buffered track data, then registers a
//   callback for the next loop point via thunk_FUN_00442f40.
void Player_StartMusicLoop(int bParam);
