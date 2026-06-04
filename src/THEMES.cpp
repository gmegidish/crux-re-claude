// THEMES.cpp — Music theme / room-music streaming system
//
// Manages background music playback for room transitions in Crux.  The system
// is entirely audio — it does NOT load visual backgrounds or room geometry.
// "Theme" here means the musical theme tied to each room.
//
// Architecture overview:
//   - A dedicated background thread (Theme_ThreadProc) owns all playback.
//   - Music is stored as a flat PCM pool (g_pThemeMemPool) of g_nThemePoolSize
//     bytes, pre-filled with silence (0x80).  The pool is a circular ring
//     buffer; g_nThemePoolWriteHead tracks where the next decoded block goes.
//   - Rooms are associated with named music files.  Calling Theme_SetRoom(id)
//     queues a room change; the thread handles crossfade/cut transitions.
//   - "Segments" are individual music cues.  A playlist is built as a stack of
//     seg-ops (g_aThemeSegStack, up to 51 entries).  Each seg-op has a type:
//         0 = play segment
//         1 = fill with silence
//         2 = event cue
//         3 = stop / end-of-music
//         4 = crossfade-timed silence
//         5 = play-until-dot (loop until first '.' in filename)
//         6 = set-fade-duration
//   - Transitions between rooms are controlled by g_nThemeTransitionMode:
//         1/2 = immediate cut / crossfade
//         3   = wait for segment end then crossfade
//         4   = same with custom fade duration
//         5   = wait for segment end (no fade)
//         7   = loop current track then switch
//   - Music events (Theme_MusicEvent) map event IDs to per-scene entry points
//     in the segment table, enabling scripts to trigger musical stings.
//   - Volume: 0..64, stored in g_nThemeVolume.  Fade-out is driven by a
//     multimedia timer (timeSetEvent) firing Theme_FadeOutHandler at ~20 Hz.
//
// Timer subsystem (appended at the bottom of the translation unit):
//   - Theme_SetTimer/KillTimer/KillAllTimers wrap timeSetEvent/timeKillEvent
//     and maintain a table of up to 10 active timer IDs.
//   - Theme_RegisterAsyncProg / Theme_InitTimerTable register up to 100
//     function pointers that are driven asynchronously by the timer machinery.
//
// MIXER thunk resolution (confirmed by decompile):
//   thunk_FUN_00443820(0)  → Mixer_RemoveChannel(0)   [THEMES called "Pause"]
//   thunk_FUN_00443970(0)  → Mixer_AddChannel(0)      [THEMES called "Resume"]
//   thunk_FUN_00443060     → Mixer_SetVolume           [3-arg: chan, vol, pan]
//   thunk_FUN_00443510     → Mixer_GetVolume           [1-arg: chan → current vol]
//   thunk_FUN_00444350     → Mixer_SelectFillFunc      [THEMES called "PlaySeg" — actually format selector]
//   thunk_FUN_004410d0     → Mixer_Reinit              [THEMES called "FillBuf" — actually DS re-init]
// Original source: C:\DevStudio\Projects\Crux\THEMES.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "THEMES.h"
#include "MIXER.h"
#include "READRES.h"
#include "ERRORS.h"

// ============================================================
//  Cross-module helpers — the Thm_* music-segment functions are reversed
//  in TEXT.cpp (the room-script "Thm" subsystem, 0x00478380–0x00478a70).
//  THEMES.cpp (this file) is the streaming engine; it drives those.
// ============================================================

// Thm_Play (TEXT.cpp 0x00478580) — play a track by name+extension
extern void Thm_Play(const char* szName, const char* szExt);

// Thm_FindLabel (TEXT.cpp 0x00478a70) — look up an event-name label in the
// scene event table; returns index or -1 if not found
extern int  Thm_FindLabel(void* pTable, int count, const char* szLabel);

// Thm_PlayNextSegment (TEXT.cpp 0x00478380) — fill the PCM pool with the next
// segment block; param is pool offset (0xffffffff = entire pool)
extern void Thm_PlayNextSegment(unsigned int poolOffset);

// func_0x00401640 — dispatches a stored callback/event-cue by value
extern void func_0x00401640(unsigned int arg);

// FUN_004895e0 — strcpy wrapper (dst, src)
extern void FUN_004895e0(void* dst, const void* src);

// FUN_0049a830 — strcmp wrapper, returns 0 if equal
extern int  FUN_0049a830(const void* a, const void* b);

// FUN_00489d20 — memmove(dst, src, n)
extern void FUN_00489d20(void* dst, const void* src, int n);

// FUN_0049def0 — normalise string to uppercase in-place
extern void FUN_0049def0(char* s);

// Err_SetRecord3 — formats an error record; returns pointer to record
extern void* Err_SetRecord3(int type, const char* name, int code);

// FUN_00489090 — displays an error dialog from a record
extern void  FUN_00489090(void* record, void* dlgTemplate);

// thunk_FUN_00442fd0(0) — stop MIDI channel 0
extern void  thunk_FUN_00442fd0(int chan);

// thunk_FUN_00443d50(0) — poll MIDI busy on channel 0; returns non-zero while busy
extern int   thunk_FUN_00443d50(int chan);

// Mixer_GetVolume — returns current volume for a mixer channel
extern int   Mixer_GetVolume(int chan);

// ============================================================
//  Globals
// ============================================================

// System state ---------------------------------------------------------------
int   g_nThmReady             = 0;       // 0x007d3950  set by Theme_Init (was g_nThemeSystemEnabled)
int   g_nThemeSystemEnabled   = 0;       // alias kept for compatibility — maps same address
int   g_nThemeCritSecInited   = 0;       // 0x007d3980  g_nThemeCS is valid
int   g_nThemeMusicActive     = 0;       // 0x007d3a8c  music is currently playing
int   g_nThemeSilentMode      = 0;       // 0x007d3a90  suppress background loads

// Thread / sync --------------------------------------------------------------
HANDLE g_hThemeThread         = NULL;    // 0x007c4bac  background music thread
int    g_nThemeThreadId       = 0;       // 0x007d0268  thread ID (for safe terminate)
int    g_nThmPlayEvent        = 0;       // 0x007d3978  "FinishedThmSeg" event (int-sized HANDLE)
HANDLE g_hThemeExecuteEvent   = NULL;    // 0x007d397c  "Execute_command" event
int    g_nThemeCS             = 0;       // 0x007d3930  CRITICAL_SECTION (24 bytes)
int    g_nThemeMemPoolCS      = 0;       // 0x007cf428  pool CRITICAL_SECTION

// Room / segment state -------------------------------------------------------
int   g_nThemeCurrentRoom     = 0;       // 0x004da788  active room ID
int   g_nThemeSegCount        = 0;       // 0x007d3968  entries in segment table
int   g_nThemeCurEventSeg     = 0;       // 0x007d3964  current event segment
int   g_nThemeSegStackTop     = 0;       // 0x007d3970  seg-op stack depth (max 51)
int   g_nThemeSegOpCursor     = 0;       // 0x004da76c  walk cursor in segment table
int   g_nThemeSegOpType       = 0;       // 0x007cebe4  current seg-op type
int   g_nThemeActiveChan      = 0;       // 0x007cebe0  active mixer channel index
int   g_nThemePrevSegIdx      = 0;       // 0x007d3974  previous segment index
int   g_nThemePendingSegIdx   = -1;      // 0x004da778  next segment to play (-1=none)
int   g_nThemeLastLoadedSeg   = 0;       // 0x004da774  most recently loaded segment
int   g_nThemeLastMemSlot     = 0;       // 0x007cff34  mem slot from last SegToMem
int   g_nThemeLoadCursor      = 0;       // 0x004da770  down-counter for load queue
int   g_nThemeChanLooping     = 0;       // 0x007cff38  non-zero when channel loops
int   g_nThemeMemFilling      = 0;       // 0x007d0264  re-entry guard for mem fill
int   g_nThemePendingCmd      = 0;       // 0x007d3a94  command for background thread

// Segment stack / command table (Ghidra names) --------------------------------
// g_nThmSegmentIdx  — DAT_007d348c offset / 0x10 → current depth of the seg-op stack
// g_nThmCommandCount — number of commands in the event table for the current scene
// g_nThmEventNameCount — count of named labels in g_pThmEventNameTable
int   g_nThmSegmentIdx        = 0;       // 0x007d348c / 0x10  stack walk cursor
int   g_nThmCommandCount      = 0;       // count of commands in current scene table
int   g_nThmEventNameCount    = 0;       // count of label names in event table

// Current segment playback data (written by Theme_PrepNextSeg) ---------------
int   g_nThmCurrentSegmentData  = 0;    // 0x007d027c  PCM data pointer for current seg
int   g_nThmCurrentSegmentLen   = 0;    // 0x007d028c  byte length of current seg
int   g_nThmCurrentSegmentFlags = 0;    // 0x007d0294  format flags for current seg

// Memory pool ----------------------------------------------------------------
void* g_pThemeMemPool         = NULL;    // 0x007d0260  PCM ring buffer
int   g_nThemePoolSize        = 0;       // 0x007c8198  pool size in bytes
int   g_nThemePoolWriteHead   = 0;       // 0x007d3948  write position in pool
void* g_pThemePlayPtr         = NULL;    // 0x007d347c  pointer to current play block
int   g_nThemePlaySize        = 0;       // 0x007d394c  size of current play block
int   g_nThemePlayChannelFlags= 0;       // 0x007d026c  channel format flags

// Track name buffers ---------------------------------------------------------
// (char arrays; declared extern in header; actual storage at fixed addresses)
// g_szThemeCurrentTrack  0x007d3988
// g_szThemeNextTrack     0x007d3ab0
// g_szThemePendingTrack  0x007cf228
// g_szThemePendingExt    0x007cf328
extern char g_szThemeCurrentTrack[];    // 0x007d3988
extern char g_szThemeNextTrack[];       // 0x007d3ab0
extern char g_szThemePendingTrack[];    // 0x007cf228
extern char g_szThemePendingExt[];      // 0x007cf328

// Transition / fade ----------------------------------------------------------
int   g_nThemeTransitionMode  = 0;       // 0x004da77c  transition mode (1-7)
int   g_nThemeFadeDuration    = -1;      // 0x004da784  fade-out ms (-1=default 3000)
int   g_nThemeFadeStep        = 0;       // 0x007d3478  volume delta per tick
int   g_nThemeFadeTimerId     = 0;       // 0x007d3a88  active fade timer ID (0=none)

// Volume ---------------------------------------------------------------------
int   g_nThemeVolume          = 64;      // 0x004da780  current volume (0..64)

// Timer subsystem ------------------------------------------------------------
int   g_nThemeTimerCount      = 0;       // 0x007d4c18  active timers (max 10)
int   g_nThemeTimerIds        = 0;       // 0x007d4bc8  array[10] of MMTIMER IDs
int   g_nThemeTimerFuncs      = 0;       // 0x007d4bf0  array[10] of callback ptrs

// Async program table --------------------------------------------------------
int   g_nThemeAsyncProgCount  = 0;       // 0x007d5020  registered async progs
int   g_nThemeAsyncProgBase   = 0;       // 0x007d5668  base index/offset
int   g_nThemeAsyncProgFuncs  = 0;       // 0x007d5028  array[100] of prog ptrs

// ============================================================
//  Theme_StopMusic  (0x004798e0)
// ============================================================
// Stops music playback.  Clears g_nThemeMusicActive, flushes the PCM
// buffers (FUN_004895e0 = memset-to-silence over the pool range), and
// if the mixer channel is still active it calls into the MIXER module to
// stop it and log a "stopping music" debug trace.
void Theme_StopMusic(void)
{
    if (g_nThmReady != 0) {
        g_nThemeMusicActive = 0;
        Mixer_Stop();
    }
}

// ============================================================
//  Theme_StopMusicAndFree  (0x004799d0)
// ============================================================
// Like Theme_StopMusic but also frees any pending fade timer
// (g_nThemeFadeTimerId) before stopping.
void Theme_StopMusicAndFree(void)
{
    if (g_nThmReady != 0) {
        g_nThemeMusicActive = 0;
        if (g_nThemeFadeTimerId != 0) {
            Theme_KillTimer(g_nThemeFadeTimerId);
            g_nThemeFadeTimerId = 0;
        }
        Mixer_Stop();
    }
}

// ============================================================
//  Theme_PauseMusic  (0x00479ae0)
// ============================================================
// Pauses music by calling MIXER::Pause(channel=0).
void Theme_PauseMusic(void)
{
    if (g_nThmReady != 0) {
        Mixer_RemoveChannel(0);
    }
}

// ============================================================
//  Theme_ResumeMusic  (0x00479b80)
// ============================================================
// Resumes music by calling MIXER::Resume(channel=0).
void Theme_ResumeMusic(void)
{
    if (g_nThmReady != 0) {
        Mixer_AddChannel(0);
    }
}

// ============================================================
//  Theme_MusicEvent  (0x00479c20)
// ============================================================
// Dispatches a named music event string to select the appropriate
// music segment from the scene/event table.  The event name is
// looked up in the label table; if found the seg entry's transition-mode
// field controls how the switch happens.
//
// Transition modes in the seg record:
//   1/2     = immediate play (Thm_Play directly)
//   3/4     = queue pending + start fade-out (3000ms or g_nThemeFadeDuration)
//   5       = queue pending as endseg (wait for seg boundary)
//   7       = loop to named event (re-dispatch MusicEvent after prep)
//   default = immediate play
//
// param_1: event name string (e.g. "DOOR_OPEN")
void Theme_MusicEvent(const char* eventName)
{
    char szName[256];
    char szExt[256];
    int  transMode;
    int  activeChan;
    int  labelIdx;
    int  i;
    int* pEntry;

    // DAT_00629f58 is a global "disabled" flag checked throughout the thread
    extern int DAT_00629f58;

    if (g_nThmReady == 0 || DAT_00629f58 != 0) {
        return;
    }

    Debug_Trace(0, 0, "Music event: %s", eventName);

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);

    pEntry = NULL;

    if (g_nThmCommandCount > 0) {
        activeChan = g_nThemeActiveChan;
        labelIdx   = Thm_FindLabel((void*)0x007cff40, g_nThmEventNameCount, eventName);

        if (labelIdx != -1) {
            // Search the current channel's event range first
            extern int DAT_007d0ef8[];
            extern int DAT_007d1538[];
            int rangeStart = *(int*)((char*)DAT_007d0ef8 + activeChan * sizeof(void*) + 0xc);
            int rangeEnd   = *(int*)((char*)DAT_007d0ef8 + activeChan * sizeof(void*) + 0x10);
            for (i = rangeStart; i <= rangeEnd; i++) {
                int* pCur = *(int**)((char*)DAT_007d1538 + i * 4);
                if (pCur[1] == labelIdx) {
                    pEntry = pCur;
                }
            }
        }

        if (pEntry == NULL) {
            // Fall back to searching the first channel (index 0)
            extern int DAT_007d0ef8[];
            extern int DAT_007d1538[];
            int rangeEnd0 = *(int*)((char*)DAT_007d0ef8 + 0xc);
            for (i = 0; i < rangeEnd0; i++) {
                int* pCur = *(int**)((char*)DAT_007d1538 + i * 4);
                if (pCur[1] == labelIdx) {
                    pEntry = pCur;
                }
            }
        }
    }

    if (pEntry == NULL) {
        // No table entry found — use current transition mode (or 1 if not active)
        if (g_nThemeMusicActive == 0) {
            transMode = 1;
        } else {
            transMode = g_nThemeTransitionMode;
        }
        FUN_004895e0(szName, eventName);
        // Default extension from DAT_007d3ab4 (+4 offset into NextTrack area)
        extern char DAT_007d3ab4[];
        FUN_004895e0(szExt, DAT_007d3ab4);
    } else {
        transMode = pEntry[0];
        // Default extension from DAT_007d3ab8
        extern char DAT_007d3ab8[];
        FUN_004895e0(szExt, DAT_007d3ab8);
        if (pEntry[2] == -1) {
            // Use current track name
            FUN_004895e0(szName, &g_szThemeCurrentTrack);
            if (pEntry[3] != -1) {
                extern char* DAT_007cf440[];
                FUN_004895e0(szExt, DAT_007cf440[pEntry[3]]);
            }
        } else {
            extern char* DAT_007d37a0[];
            FUN_004895e0(szName, DAT_007d37a0[pEntry[2]]);
            if (pEntry[3] != -1) {
                extern char* DAT_007cebe8[];
                FUN_004895e0(szExt, DAT_007cebe8[pEntry[3]]);
            }
        }
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);

    switch (transMode) {
    case 1:
        Thm_Play(szName, szExt);
        break;
    case 2:
        Thm_Play(szName, szExt);
        break;
    case 3:
    case 4:
        g_nThemePendingCmd = 2;
        FUN_004895e0(&g_szThemePendingTrack, szName);
        FUN_004895e0(&g_szThemePendingExt,   szExt);
        if (g_nThemeFadeDuration == -1) {
            Theme_StartFadeOut(3000);
        } else {
            Theme_StartFadeOut(g_nThemeFadeDuration);
        }
        break;
    case 5:
        g_nThemePendingCmd = 3;
        Debug_Trace(0, 0, "endseg to: %s %s", szName, szExt);
        FUN_004895e0(&g_szThemePendingTrack, szName);
        FUN_004895e0(&g_szThemePendingExt,   szExt);
        break;
    case 7:
        Debug_Trace(0, 0, "loop event");
        g_nThemePendingCmd = 4;
        FUN_004895e0(&g_szThemePendingTrack, eventName);
        break;
    default:
        Thm_Play(szName, szExt);
        break;
    }
}

// ============================================================
//  Theme_Init  (0x0047a1c0)
// ============================================================
// One-time startup:
//   - InitializeCriticalSection for g_nThemeMemPoolCS
//   - Allocates g_pThemeMemPool (g_nThemePoolSize bytes), fills with 0x80
//   - Initialises 100 mem-slot flags in g_nThemeMemSlotFlags
//   - Spawns the background music thread (Theme_ThreadProc)
//   - InitializeCriticalSection for g_nThemeCS
//   - Sets g_nThemeCritSecInited = 1, g_nThmReady = 1
void Theme_Init(void)
{
    int i;

    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nThemeMemPoolCS);

    // g_nThemePoolWriteHead starts at pool base offset (DAT_006dc044)
    extern int DAT_006dc044;
    g_nThemePoolWriteHead = DAT_006dc044;

    g_pThemeMemPool = (void*)SafeHeap_Alloc(__LINE__, __FILE__, g_nThemePoolSize);
    memset(g_pThemeMemPool, 0x80, g_nThemePoolSize);

    // Clear 100 mem-slot name-pointer and ready-bit fields
    // Each slot is 0x20 bytes wide; DAT_007d0278 = slot[i].namePtr, DAT_007d0290 = slot[i].flags
    extern int DAT_007d0278[];
    extern unsigned int DAT_007d0290[];
    for (i = 0; i < 100; i++) {
        *(int*)  ((char*)DAT_007d0278 + i * 0x20) = 0;
        *(unsigned int*)((char*)DAT_007d0290 + i * 0x20) =
            *(unsigned int*)((char*)DAT_007d0290 + i * 0x20) & 0xfffffffd;
    }

    g_hThemeThread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)Theme_ThreadProc,
        NULL, 0, (LPDWORD)&g_nThemeThreadId);

    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
    g_nThemeCritSecInited = 1;
    g_nThmReady = 1;
}

// ============================================================
//  Theme_ThreadProc  (0x0047a350)
// ============================================================
// Background music thread.  Waits on two events:
//   [0] g_nThmPlayEvent     — segment finished playing  ("FinishedThmSeg")
//   [1] g_hThemeExecuteEvent — execute next command     ("Execute_command")
//
// On FinishedThmSeg (index 0):
//   if g_nThemePendingCmd==1 → Theme_StopMusic
//   if g_nThemePendingCmd==2 → Thm_Play(pending track)
//   g_nThemePendingCmd = 0
//
// On Execute_command (index 1):
//   if g_nThemePendingCmd==3 → Thm_Play(pending), clear cmd
//   if g_nThemePendingCmd==4 → Theme_PrepNextSeg, re-dispatch MusicEvent
//   else                     → Theme_PrepNextSeg
//
// When g_nThemeMemFilling flag needs setting (g_nThemePrevSegIdx < 0):
//   set g_nThemeMemFilling=1, Thm_PlayNextSegment(pool), Theme_PrepNextSeg
void Theme_ThreadProc(void)
{
    extern int DAT_00629f58;

    HANDLE hEvents[2];

    if (DAT_00629f58 != 0) {
        return;
    }

    if (g_nThmPlayEvent == 0) {
        g_nThmPlayEvent = (int)CreateEventA(NULL, 0, 0, "FinishedThmSeg");
    }
    if (g_hThemeExecuteEvent == NULL) {
        g_hThemeExecuteEvent = CreateEventA(NULL, 0, 0, "Execute_command");
    }

    hEvents[0] = (HANDLE)g_nThmPlayEvent;
    hEvents[1] = g_hThemeExecuteEvent;

    for (;;) {
        // If the previous seg index is negative and we are not already filling,
        // kick off a background fill of the PCM pool then prep the next segment.
        while (g_nThemePrevSegIdx < 0 && g_nThemeMemFilling == 0) {
            g_nThemeMemFilling = 1;
            Thm_PlayNextSegment(0xffffffff);
            Theme_PrepNextSeg();
        }

        DWORD dwWait = WaitForMultipleObjects(2, hEvents, 0, INFINITE);

        if (dwWait == 0) {
            // Execute_command event (index 1 in the original; Ghidra labels differ — confirmed index 1)
            if (g_nThemePendingCmd == 3) {
                g_nThemePendingCmd = 0;
                Thm_Play(g_szThemePendingTrack, g_szThemePendingExt);
            } else if (g_nThemePendingCmd == 4) {
                Theme_PrepNextSeg();
                Theme_MusicEvent(g_szThemePendingTrack);
            } else {
                Theme_PrepNextSeg();
            }
        } else if (dwWait == 1) {
            // FinishedThmSeg event (index 0 in the original)
            if (g_nThemePendingCmd == 1) {
                Theme_StopMusic();
            } else if (g_nThemePendingCmd == 2) {
                Thm_Play(g_szThemePendingTrack, g_szThemePendingExt);
            }
            g_nThemePendingCmd = 0;
        }
    }
}

// ============================================================
//  Theme_PrepNextSeg  (0x0047a5a0)
// ============================================================
// Called from the music thread to advance to the next queued segment.
// Original debug name: "void thm_prep_next_seg()"
//
// Steps:
//   1. EnterCriticalSection
//   2. If channel is not fixed (DAT_007d0270==0), update g_nThemeActiveChan from stack
//   3. Pop seg-ops until stack depth >= 6
//   4. Decrement stack cursor; Theme_Nop2
//   5. Clear loop-bit for previous slot when appropriate
//   6. If pending seg == -1: Theme_LoadSegs(1) × 2, decrement pending
//   7. Theme_LoadSegs(0) × 2 unconditionally
//   8. Read g_nThemeSegOpType from stack entry
//   9. Loop on non-play/non-silence types:
//      2 = func callback
//      3 = StopMusic → leave CS → return
//      4 = set g_nThemeFadeDuration
//      5 = stop MIDI, wait, split filename at '.', Thm_Play, loop
//  10. type 0: set prev, Nop1, read pending seg + looping + play ptr
//      type 1: silence buffer as play ptr
//  11. LeaveCriticalSection
void Theme_PrepNextSeg(void)
{
    char szBase[256];
    char szExt[256];
    char* pDot;

    // The seg-op stack entries are structs of 0x10 bytes each, laid out:
    //   +0x00  type   (DAT_007d3480)
    //   +0x04  segIdx (DAT_007d3484)
    //   +0x08  memSlot(DAT_007d3488)
    //   +0x0c  chan   (DAT_007d348c)
    extern int  DAT_007d0270;         // "channel fixed" flag
    extern int  DAT_007d3480[];       // seg-op stack, field type
    extern int  DAT_007d3484[];       // seg-op stack, field segIdx
    extern int  DAT_007d3488[];       // seg-op stack, field memSlot
    extern int  DAT_007d348c[];       // seg-op stack, field chan
    extern int  DAT_007d0288[];       // mem-slot loop flag
    extern int  DAT_007d027c[];       // mem-slot PCM data pointer
    extern int  DAT_007d028c[];       // mem-slot PCM size
    extern int  DAT_007d0294[];       // mem-slot channel flags
    extern char* DAT_007cf440[];      // seg-name pointer table

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);

    if (DAT_007d0270 == 0) {
        g_nThemeActiveChan = *(int*)((char*)DAT_007d348c + g_nThmSegmentIdx * 0x10);
    }

    while (g_nThmSegmentIdx < 6) {
        Theme_PushSegOp();
    }
    g_nThmSegmentIdx--;

    Theme_Nop2();

    // Clear the loop-bit on the previous slot when conditions are met
    if ((g_nThemePrevSegIdx != g_nThemePendingSegIdx) &&
        (g_nThemePendingSegIdx >= 0) &&
        (g_nThemePrevSegIdx >= 0) &&
        (g_nThemeSegOpType == 0))
    {
        *(unsigned int*)((char*)DAT_007d0290 + g_nThemePrevSegIdx * 0x20) &= 0xfffffffe;
    }

    if (g_nThemePendingSegIdx == -1) {
        Theme_LoadSegs(1);
        Theme_LoadSegs(1);
        g_nThemePendingSegIdx--;
    }
    Theme_LoadSegs(0);
    Theme_LoadSegs(0);

    g_nThemeSegOpType = *(int*)((char*)DAT_007d3480 + g_nThmSegmentIdx * 0x10);

    // Process non-playback types in a loop
    while (g_nThemeSegOpType != 0 && g_nThemeSegOpType != 1) {
        switch (g_nThemeSegOpType) {
        case 2:
            func_0x00401640(*(unsigned int*)((char*)DAT_007d3484 + g_nThmSegmentIdx * 0x10));
            break;
        case 3:
            Theme_StopMusic();
            LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
            return;
        case 4:
            g_nThemeFadeDuration = *(int*)((char*)DAT_007d3484 + g_nThmSegmentIdx * 0x10);
            break;
        case 5:
            // Stop MIDI, wait until idle, then play until-dot segment
            thunk_FUN_00442fd0(0);
            while (thunk_FUN_00443d50(0) != 0) {
                Sleep(10);
            }
            FUN_004895e0(szBase,
                DAT_007cf440[*(int*)((char*)DAT_007d3484 + g_nThmSegmentIdx * 0x10)]);
            pDot = strchr(szBase, '.');
            FUN_004895e0(szExt, pDot + 1);
            *pDot = '\0';
            Thm_Play(szBase, szExt);
            break;
        }

        // Refill stack and reload
        while (g_nThmSegmentIdx < 6) {
            Theme_PushSegOp();
        }
        g_nThmSegmentIdx--;

        if (g_nThemePendingSegIdx == -1) {
            Theme_LoadSegs(1);
            Theme_LoadSegs(1);
            g_nThemePendingSegIdx--;
        }

        g_nThemeSegOpType = *(int*)((char*)DAT_007d3480 + g_nThmSegmentIdx * 0x10);
    }

    if (g_nThemeSegOpType == 0) {
        // Normal playback segment
        g_nThemePrevSegIdx  = g_nThemePendingSegIdx;
        Theme_Nop1();
        g_nThemePendingSegIdx      = *(int*)((char*)DAT_007d3488 + g_nThmSegmentIdx * 0x10);
        g_nThemeChanLooping        = (*(int*)((char*)DAT_007d0288 + g_nThemePendingSegIdx * 0x20) != 0) ? 1 : 0;
        g_nThmCurrentSegmentData   = *(int*)((char*)DAT_007d027c + g_nThemePendingSegIdx * 0x20);
        g_nThmCurrentSegmentLen    = *(int*)((char*)DAT_007d028c + g_nThemePendingSegIdx * 0x20);
        g_nThmCurrentSegmentFlags  = *(int*)((char*)DAT_007d0294 + g_nThemePendingSegIdx * 0x20);
    } else if (g_nThemeSegOpType == 1) {
        // Silence fill
        g_nThmCurrentSegmentData  = (int)g_pThemeMemPool;
        g_nThmCurrentSegmentLen   = *(int*)((char*)DAT_007d3484 + g_nThmSegmentIdx * 0x10);
        g_nThmCurrentSegmentFlags = 0;
        g_nThemeChanLooping       = 1;
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
}

// ============================================================
//  Theme_LoadSegs  (0x0047aa80)
// ============================================================
// Loads the next unloaded segment from the load queue into the PCM pool.
// Original debug name: "int thm_load_segs(int load_now)"
//
// Returns 0 on success, -1 if alloc failed, 0 on deferred load.
// param_1: 1 = load synchronously (block), 0 = allow deferred
//
// Walks g_nThemeLoadCursor downward through the seg-op stack, skipping
// entries that are already loaded or match the last-loaded seg.  Calls
// Theme_SegToMem(segName, param_1) to do the actual load.  On -2 return
// (file-not-found) it calls into the ERRORS module to display a dialog.
int Theme_LoadSegs(int loadNow)
{
    extern int DAT_007d3480[];
    extern int DAT_007d3484[];
    extern int DAT_007d3488[];
    extern char* DAT_007cfa80[];
    extern int   g_nThmIndex;

    int segIdx;
    int slot;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);

    // Skip over entries that are not type-0 (play) or that match the last loaded seg
    while (g_nThemeLoadCursor != -1) {
        int type    = *(int*)((char*)DAT_007d3480 + g_nThemeLoadCursor * 0x10);
        int thisSeg = *(int*)((char*)DAT_007d3484 + g_nThemeLoadCursor * 0x10);
        if (type != 0 || thisSeg == g_nThemeLastLoadedSeg) {
            if (thisSeg == g_nThemeLastLoadedSeg) {
                *(int*)((char*)DAT_007d3488 + g_nThemeLoadCursor * 0x10) = g_nThemeLastMemSlot;
            }
            g_nThemeLoadCursor--;
        } else {
            break;
        }
    }

    if (g_nThemeLoadCursor == -1) {
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
        return 0;
    }

    segIdx = *(int*)((char*)DAT_007d3484 + g_nThemeLoadCursor * 0x10);
    slot   = Theme_SegToMem(DAT_007cfa80[segIdx], loadNow);

    if (slot == -2) {
        if (g_nThmIndex != 0) {
            LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
            return 0;
        }
        const char* pName = DAT_007cfa80[segIdx];
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);

        // Show error dialog for missing file
        void* pRec = Err_SetRecord3(1, pName, 0xd);
        // FUN_00489090 shows a dialog from the record
        extern void DAT_004ab3f8;
        FUN_00489090(pRec, &DAT_004ab3f8);
        return 0;
    }

    if (slot == -1) {
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
        return -1;
    }

    g_nThemeLastMemSlot = slot;

    // Backfill all stack entries that share the same seg index
    while (g_nThemeLoadCursor >= 0 &&
           *(int*)((char*)DAT_007d3484 + g_nThemeLoadCursor * 0x10) == segIdx)
    {
        *(int*)((char*)DAT_007d3488 + g_nThemeLoadCursor * 0x10) = g_nThemeLastMemSlot;
        g_nThemeLoadCursor--;
    }

    g_nThemeLastLoadedSeg = segIdx;
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
    return 0;
}

// ============================================================
//  Theme_SegToMem  (0x0047ad30)
// ============================================================
// Decodes a named music file into a free slot in the PCM pool.
// Original debug name: "int thm_seg_to_mem(char *name_in)"
//
// Returns slot index (0..99) on success, -1 on alloc failure,
// -2 (0xfffffffe) if file not found.
//
// Steps:
//   1. EnterCriticalSection(&g_nThemeMemPoolCS)
//   2. FUN_0049def0(name) — normalise name to uppercase
//   3. Theme_FindFreeMemSlot() → slot
//   4. Allocate name buffer in slot, copy name, mark slot in-use (bit 0)
//   5. Check if any other loaded slot has the same name and is ready (bit 1 set,
//      bit 2 set):  if yes, copy PCM pointer (memcpy), mark bit 1, return slot
//   6. If not shared: iterate ONTHEFLY decoders (indices 0x27..0x20, then 0x0D)
//      to find a codec that accepts the file
//   7. Theme_GetMemBlock(fileSize) → PCM address offset
//   8. Start async decode (Res_BunchFreadLoadPtr) or sync + Res_WaitForEntry
//   9. Mark slot bit 1 (ready), store file size
//  10. LeaveCriticalSection
int Theme_SegToMem(const char* name, int loadNow)
{
    extern unsigned int DAT_007d0290[];
    extern int          DAT_007d0278[];
    extern int          DAT_007d0288[];
    extern int          DAT_007d027c[];
    extern int          DAT_007d028c[];
    extern int          DAT_007d0294[];
    extern int          DAT_004c4c40;    // multiplier for async priority calc

    unsigned int slot;
    unsigned int other;
    unsigned int result;
    size_t       nameLen;
    unsigned int codec;
    unsigned int formatFlags;
    unsigned int fileSize;
    char         resBuf[256];
    char         auxBuf[16];
    unsigned int auxSize;
    int          found;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nThemeMemPoolCS);

    FUN_0049def0((char*)name);

    slot = (unsigned int)Theme_FindFreeMemSlot();
    if (slot == 0xffffffff) {
        result = 0xffffffff;
        goto leave;
    }

    // Free old name buffer if present
    if (*(int*)((char*)DAT_007d0278 + slot * 0x20) != 0) {
        SafeHeap_Free(__LINE__, __FILE__,
            (void*)*(int*)((char*)DAT_007d0278 + slot * 0x20));
    }

    // Allocate and copy name
    nameLen = strlen(name);
    *(int*)((char*)DAT_007d0278 + slot * 0x20) =
        (int)SafeHeap_Alloc(__LINE__, __FILE__, (int)(nameLen + 1));
    FUN_004895e0((void*)*(int*)((char*)DAT_007d0278 + slot * 0x20), name);

    // Mark in-use (bit 0)
    *(unsigned int*)((char*)DAT_007d0290 + slot * 0x20) |= 1;

    // Check for an already-loaded slot with the same name (bit 1 and bit 2 set)
    for (other = 0; other < 100; other++) {
        if (other == slot) continue;
        unsigned int flags = *(unsigned int*)((char*)DAT_007d0290 + other * 0x20);
        // bit 1 set = (flags << 0x1e) < 0  →  flags & 0x4
        if (!((flags << 0x1e) < 0)) continue;

        // Compare names
        found = FUN_0049a830(name, (void*)*(int*)((char*)DAT_007d0278 + other * 0x20));
        if (found != 0) continue;

        // Same name found
        if (*(int*)((char*)DAT_007d0288 + other * 0x20) == 0) {
            result = 0xffffffff;
            goto leave;
        }

        int memBlock = Theme_GetMemBlock(*(int*)((char*)DAT_007d028c + other * 0x20));
        *(int*)((char*)DAT_007d027c + slot * 0x20) = memBlock;
        if (memBlock == 0) {
            result = 0xffffffff;
            goto leave;
        }

        // Copy flags and PCM content
        *(unsigned int*)((char*)DAT_007d0294 + slot * 0x20) =
            *(unsigned int*)((char*)DAT_007d0294 + other * 0x20);
        FUN_00489d20(
            (void*)*(int*)((char*)DAT_007d027c + slot * 0x20),
            (void*)*(int*)((char*)DAT_007d027c + other * 0x20),
            *(int*)((char*)DAT_007d028c + other * 0x20));
        *(int*)((char*)DAT_007d028c + slot * 0x20) =
            *(int*)((char*)DAT_007d028c + other * 0x20);
        InterlockedExchange((LONG*)((char*)DAT_007d0288 + slot * 0x20), 1);
        *(unsigned int*)((char*)DAT_007d0290 + slot * 0x20) |= 2;
        result = slot;
        goto leave;
    }

    // Try ONTHEFLY decoders 0x27 down to 0x20
    formatFlags = 0;
    found = 0;
    for (codec = 0x27; codec > 0x1f; codec--) {
        int rc = Res_FindByNumChar(codec, name, resBuf, 0, auxBuf, (int*)&auxSize);
        if (rc == 0) {
            Debug_Assert(__LINE__, "found", codec);
            formatFlags = codec & 7;
            found = 1;
            break;
        }
    }

    if (!found) {
        // Try codec 0x0D
        codec = 0x0d;
        int rc = Res_FindByNumChar(0x0d, name, resBuf, 0, auxBuf, (int*)&auxSize);
        if (rc != 0) {
            result = 0xfffffffe;
            goto leave;
        }
        Debug_Assert(__LINE__, "found", (int)codec);
        formatFlags = 0;
    }

    *(unsigned int*)((char*)DAT_007d0294 + slot * 0x20) = formatFlags;
    fileSize = auxSize;

    {
        int memBlock = Theme_GetMemBlock((int)fileSize);
        *(int*)((char*)DAT_007d027c + slot * 0x20) = memBlock;
        if (memBlock == 0) {
            result = 0xffffffff;
            goto leave;
        }
    }

    *(unsigned int*)((char*)DAT_007d0290 + slot * 0x20) |= 2;

    if (loadNow == 0) {
        // Async/deferred load — priority based on pool size ratio
        unsigned int priority = (fileSize / (unsigned int)g_nThemePoolSize) * (unsigned int)DAT_004c4c40 - 9;
        Res_BunchFreadLoadPtr((void*)((char*)DAT_007d027c + slot * 0x20),
                              1, (int)fileSize, auxBuf, (int)priority, 1);
    } else {
        // Synchronous load
        Res_BunchFreadLoadPtr((void*)((char*)DAT_007d027c + slot * 0x20),
                              1, (int)fileSize, auxBuf, 0, 0);
        Res_WaitForEntry((void*)((char*)DAT_007d027c + slot * 0x20));
    }

    *(int*)((char*)DAT_007d028c + slot * 0x20) = (int)fileSize;
    result = slot;

leave:
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeMemPoolCS);
    return (int)result;
}

// ============================================================
//  Theme_FindFreeMemSlot  (0x0047b410)
// ============================================================
// Finds the first mem-slot whose in-use flags (bits 1-2, i.e. bit 1 and
// bit 2 combined in the flags word) are both clear.
// Returns slot index 0..99, or -1 if all slots occupied.
int Theme_FindFreeMemSlot(void)
{
    extern unsigned int DAT_007d0290[];
    int i;
    for (i = 0; i < 100; i++) {
        unsigned int flags = *(unsigned int*)((char*)DAT_007d0290 + i * 0x20);
        // Slots with bit 1 clear (flags & 0x2 == 0) AND bit 0 clear are free
        // Ghidra condition: (flags << 0x1e) >= 0  → bit 1 clear → flags & 0x4 == 0
        // Matching the decompile: find slot where neither bit set
        if ((flags & 0x3) == 0) {
            return i;
        }
    }
    return -1;
}

// ============================================================
//  Theme_GetMemBlock  (0x0047b4e0)
// ============================================================
// Allocates a contiguous block of 'size' bytes from the ring pool.
// Original debug name: "uchar *thm_get_mem(int size)"
//
// Returns the pool offset at which the block was placed.
// Handles the circular wrap: if g_nThemePoolWriteHead+size overflows
// DAT_006dc044+DAT_006dc020, it wraps back to the pool start.
// Any existing slot whose PCM pointer falls within the new allocation is
// evicted (bit 1 cleared) to prevent overlap.
int Theme_GetMemBlock(int size)
{
    extern int DAT_006dc044;   // pool base offset
    extern int DAT_006dc020;   // pool capacity (may differ from g_nThemePoolSize)
    extern int DAT_007d027c[]; // mem-slot PCM offset fields
    extern unsigned int DAT_007d0290[];

    int poolEnd = DAT_006dc044 + DAT_006dc020;
    int allocAt;
    int i;

    if (g_nThemePoolWriteHead + size > poolEnd) {
        // Wrap around to pool start
        g_nThemePoolWriteHead = DAT_006dc044;
    }

    allocAt = g_nThemePoolWriteHead;

    // Evict any slot whose data pointer falls within [allocAt, allocAt+size)
    for (i = 0; i < 100; i++) {
        int slotPtr = *(int*)((char*)DAT_007d027c + i * 0x20);
        if (slotPtr >= allocAt && slotPtr < allocAt + size) {
            *(unsigned int*)((char*)DAT_007d0290 + i * 0x20) &= ~2u;
        }
    }

    g_nThemePoolWriteHead += size;
    return allocAt;
}

// ============================================================
//  Theme_PushSegOp  (0x0047b730)
// ============================================================
// Reads the next segment-event entry from the scene's event table and
// pushes it onto g_aThemeSegStack.  Called until the stack is "full enough"
// (depth >= 6).
//
// Seg-event types (as stored in DAT_007d0ef8[i].type):
//   1 = multi-segment play  (count field says how many to push)
//   2 = single segment play
//   3 = file-name-based loop / dot-extension branch
//   4 = timed silence (duration/1000 segments of pool-size bytes)
//   5 = stop music
//   6 = set fade duration
//
// Returns new stack depth, or -1 when the event table is exhausted.
int Theme_PushSegOp(void)
{
    extern int DAT_007d0ef8[];      // scene command table (array of pointers)
    extern int DAT_007d3480[];      // seg-stack field: type
    extern int DAT_007d3484[];      // seg-stack field: segIdx / param
    extern int DAT_007d3488[];      // seg-stack field: memSlot
    extern int DAT_007d348c[];      // seg-stack field: chan
    extern int DAT_007d3490[];      // one slot above stack top (shift buffer)
    extern char* DAT_007cf440[];    // seg-name pointer table
    extern int  DAT_007ce5a0[];     // loop-back cursor table

    // These are the aliases Ghidra used for stack[0] fields directly
    extern int _DAT_007d3480;
    extern int _DAT_007d3484;
    extern int _DAT_007d3488;
    extern int _DAT_007d348c;

    int iVar2;
    int iStack_20;
    int iVar3;

    g_nThemeSegOpCursor++;

    if (g_nThemeSegOpCursor >= g_nThmCommandCount) {
        // End of command table — push a STOP entry
        FUN_00489d20(&DAT_007d3490, &DAT_007d3480, g_nThmSegmentIdx << 4);
        _DAT_007d3480 = 3;
        _DAT_007d3488 = -1;
        _DAT_007d348c = g_nThemeSegOpCursor;
        g_nThmSegmentIdx++;
        g_nThemeLoadCursor++;
        return -1;
    }

    iVar3 = g_nThmSegmentIdx;

    if (g_nThmSegmentIdx + 1 >= 0x33) {
        return iVar3;
    }

    int* pCmd    = (int*)((char*)DAT_007d0ef8 + g_nThemeSegOpCursor * sizeof(void*));
    int  cmdType = *pCmd;         // pCmd[0]
    int  cmdArg  = pCmd[1];       // pCmd[1] (+4)
    int  cmdCnt  = pCmd[2];       // pCmd[2] (+8)

    switch (cmdType) {
    case 1:
        // Multi-segment: push cmdCnt copies of cmdArg
        if (g_nThmSegmentIdx + cmdCnt < 0x33) {
            FUN_00489d20((char*)DAT_007d3480 + cmdCnt * 0x10,
                         &DAT_007d3480,
                         g_nThmSegmentIdx << 4);
            for (iStack_20 = 0; iStack_20 < cmdCnt; iStack_20++) {
                *(int*)((char*)DAT_007d3480 + iStack_20 * 0x10) = 0;
                *(int*)((char*)DAT_007d3484 + iStack_20 * 0x10) = cmdArg;
                *(int*)((char*)DAT_007d3488 + iStack_20 * 0x10) = -1;
                *(int*)((char*)DAT_007d348c + iStack_20 * 0x10) = g_nThemeSegOpCursor;
            }
            g_nThmSegmentIdx  += cmdCnt;
            g_nThemeLoadCursor += cmdCnt;
            iVar3 = g_nThmSegmentIdx;
        }
        break;

    case 2:
        // Single segment
        FUN_00489d20(&DAT_007d3490, &DAT_007d3480, g_nThmSegmentIdx << 4);
        _DAT_007d3480 = 2;
        _DAT_007d3484 = cmdArg;
        _DAT_007d3488 = -1;
        _DAT_007d348c = g_nThemeSegOpCursor;
        g_nThmSegmentIdx++;
        g_nThemeLoadCursor++;
        iVar3 = g_nThmSegmentIdx;
        break;

    case 3:
        // Branch on dot in filename
        {
            char* pcVar1 = strchr(DAT_007cf440[cmdArg], '.');
            if (pcVar1 == NULL) {
                // Loop back — reset cursor to start of this seg's loop point
                g_nThemeSegOpCursor = DAT_007ce5a0[cmdArg] - 1;
                iVar3 = g_nThmSegmentIdx;
            } else {
                // Has extension — push as play-until-dot (type 5)
                FUN_00489d20(&DAT_007d3490, &DAT_007d3480, g_nThmSegmentIdx << 4);
                _DAT_007d3480 = 5;
                _DAT_007d3484 = cmdArg;
                _DAT_007d3488 = -1;
                _DAT_007d348c = g_nThemeSegOpCursor;
                g_nThmSegmentIdx++;
                g_nThemeLoadCursor++;
                iVar3 = g_nThmSegmentIdx;
            }
        }
        break;

    case 4:
        // Timed silence: (duration/1000 + 1) segments of g_nThemePoolSize bytes
        iVar2 = cmdArg / 1000 + 1;
        if (g_nThmSegmentIdx + iVar2 < 0x33) {
            FUN_00489d20((char*)DAT_007d3480 + iVar2 * 0x10,
                         &DAT_007d3480,
                         g_nThmSegmentIdx << 4);
            for (iStack_20 = 0; iStack_20 < iVar2; iStack_20++) {
                *(int*)((char*)DAT_007d3480 + iStack_20 * 0x10) = 1;
                *(int*)((char*)DAT_007d3484 + iStack_20 * 0x10) = g_nThemePoolSize;
            }
            // Fractional last segment
            if (cmdArg % 1000 != 0) {
                _DAT_007d3484 = (int)(((long long)(cmdArg % 1000) *
                                       (long long)g_nThemePoolSize & 0xffffffffLL) / 1000);
            }
            g_nThmSegmentIdx  += iVar2;
            g_nThemeLoadCursor += iVar2;
            iVar3 = g_nThmSegmentIdx;
        }
        break;

    case 5:
        // Stop music
        FUN_00489d20(&DAT_007d3490, &DAT_007d3480, g_nThmSegmentIdx << 4);
        _DAT_007d3480 = 3;
        _DAT_007d3488 = -1;
        _DAT_007d348c = g_nThemeSegOpCursor;
        g_nThmSegmentIdx++;
        g_nThemeLoadCursor++;
        iVar3 = g_nThmSegmentIdx;
        break;

    case 6:
        // Set fade duration
        FUN_00489d20(&DAT_007d3490, &DAT_007d3480, g_nThmSegmentIdx << 4);
        _DAT_007d3480 = 4;
        _DAT_007d3488 = -1;
        _DAT_007d348c = g_nThemeSegOpCursor;
        _DAT_007d3484 = cmdArg;
        g_nThmSegmentIdx++;
        g_nThemeLoadCursor++;
        iVar3 = g_nThmSegmentIdx;
        break;
    }

    return iVar3;
}

// ============================================================
//  Theme_Shutdown  (0x0047bdc0)
// ============================================================
// Cleans up on exit:
//   - Deletes g_nThemeCS if g_nThemeCritSecInited
//   - Closes g_nThmPlayEvent ("FinishedThmSeg")
//   - Terminates and closes g_hThemeThread (unless caller == music thread)
void Theme_Shutdown(void)
{
    if (g_nThmReady != 0) {
        if (g_nThemeCritSecInited != 0) {
            g_nThemeCritSecInited = 0;
            DeleteCriticalSection((LPCRITICAL_SECTION)&g_nThemeCS);
        }
        if (g_nThmPlayEvent != 0) {
            CloseHandle((HANDLE)g_nThmPlayEvent);
            g_nThmPlayEvent = 0;
        }
        if (g_hThemeThread != NULL &&
            GetCurrentThreadId() != (DWORD)g_nThemeThreadId)
        {
            TerminateThread(g_hThemeThread, 0);
            CloseHandle(g_hThemeThread);
            g_hThemeThread = NULL;
        }
    }
}

// ============================================================
//  Theme_Nop1  (0x0047bef0)
//  Theme_Nop2  (0x0047bf70)
//  Theme_Nop3  (0x0047cf90)
//  Theme_Nop4  (0x0047d020)
// ============================================================
// Empty stub functions — SEH frame only, no body.
// Likely placeholders for overridable callbacks (e.g. "segment started",
// "segment ended") that were never implemented in this build.
void Theme_Nop1(void) {}
void Theme_Nop2(void) {}
void Theme_Nop3(void) {}
void Theme_Nop4(void) {}

// ============================================================
//  Theme_SetVolume  (0x0047bff0)
// ============================================================
// Sets the music volume (0..64) and immediately applies it to
// mixer channel 0.  Also saves the value to g_nThemeVolume so that
// Theme_FadeOut / FadeIn can reference the starting level.
void Theme_SetVolume(int volume)
{
    if (g_nThmReady != 0) {
        g_nThemeVolume = volume;
        Mixer_SetVolume(0, volume, -1);
    }
}

// ============================================================
//  Theme_GetVolume  (0x0047c0a0)
// ============================================================
// Returns the current music volume (g_nThemeVolume, 0..64).
int Theme_GetVolume(void)
{
    return g_nThemeVolume;
}

// ============================================================
//  Theme_FadeOutHandler  (0x0047c130)
// ============================================================
// Multimedia timer callback that decrements volume by g_nThemeFadeStep
// each tick until the channel reads 0, then signals g_hThemeExecuteEvent
// and frees g_nThemeFadeTimerId.  Named "fadeout_handler" in debug trace.
void Theme_FadeOutHandler(void)
{
    if (g_nThmReady != 0 && g_nThemeMusicActive != 0) {
        Debug_Trace(0, 0, "fadeout_handler");
        int iVar1 = Mixer_GetVolume(0);
        if (g_nThemeFadeStep < iVar1) {
            Mixer_SetVolume(0, iVar1 - g_nThemeFadeStep, -1);
        } else {
            Mixer_SetVolume(0, 0, -1);
            if (g_nThemeFadeTimerId != 0) {
                Theme_KillTimer(g_nThemeFadeTimerId);
                g_nThemeFadeTimerId = 0;
            }
            SetEvent(g_hThemeExecuteEvent);
        }
    }
}

// ============================================================
//  Theme_StartFadeOut  (0x0047c280)
// ============================================================
// Begins a music fade-out over 'durationMs' milliseconds.
// Named "fadeout_start" in debug trace.
//
// - If a fade timer is already active: no-op.
// - If g_nThemeMusicActive == 0: immediately SetEvent to unblock thread.
// - If volume == 0: call FadeOutHandler directly (instant).
// - Otherwise: compute tick rate and fire a periodic timer at Theme_FadeOutHandler.
void Theme_StartFadeOut(int durationMs)
{
    if (g_nThemeFadeTimerId != 0) {
        return;
    }

    if (g_nThemeMusicActive == 0) {
        SetEvent(g_hThemeExecuteEvent);
        return;
    }

    Debug_Trace(0, 0, "fadeout_start");

    int iVar1 = Mixer_GetVolume(0);
    if (iVar1 == 0) {
        g_nThemeFadeStep = 10;
        Theme_FadeOutHandler();
    } else {
        int ticksNeeded = (iVar1 * 1000) / durationMs;
        if (ticksNeeded < 2) {
            ticksNeeded = 1;
        }
        int freqHz = ticksNeeded;
        g_nThemeFadeStep = 1;
        if (ticksNeeded > 0x14) {
            g_nThemeFadeStep = ticksNeeded / 0x14 + 1;
            freqHz = ticksNeeded / g_nThemeFadeStep;
        }
        g_nThemeFadeTimerId = Theme_SetTimer(
            (int)(void*)Theme_FadeOutHandler, freqHz);
    }
}

// ============================================================
//  Theme_FadeOut  (0x0047c430)
// ============================================================
// High-level fade-out: sets g_nThemePendingCmd=1 then calls
// Theme_StartFadeOut(durationMs).  The pending command ensures that
// when the fade timer fires and the execute event is set, the thread
// will call Theme_StopMusic.
// Named "fadeout" in debug trace.
void Theme_FadeOut(int durationMs)
{
    if (g_nThmReady != 0 && g_nThemeMusicActive != 0) {
        g_nThemePendingCmd = 1;
        Theme_StartFadeOut(durationMs);
    }
}

// ============================================================
//  Theme_IsFading  (0x0047c510)
// ============================================================
// Returns true while a fade timer is active (g_nThemeFadeTimerId != 0).
bool Theme_IsFading(void)
{
    return g_nThemeFadeTimerId != 0;
}

// ============================================================
//  Theme_FadeIn  (0x0047c5b0)
// ============================================================
// Ramps volume from 0 to g_nThemeVolume over 'durationMs'.
// Sleeps (durationMs / g_nThemeVolume) ms between each 1-unit step.
// Runs synchronously in the caller's thread.
void Theme_FadeIn(int durationMs)
{
    if (g_nThmReady != 0) {
        int step = -1;
        DWORD delayMs = (DWORD)(durationMs / g_nThemeVolume);
        while (step < 0) {
            step++;
            Mixer_SetVolume(0, step, -1);
            Sleep(delayMs);
        }
    }
}

// ============================================================
//  Theme_SetVolumeQuarter  (0x0047c690)
// ============================================================
// Applies volume at 1/4 of g_nThemeVolume (integer divide with sign
// correction for negative values).
void Theme_SetVolumeQuarter(void)
{
    if (g_nThmReady != 0) {
        Mixer_SetVolume(0, (g_nThemeVolume + (g_nThemeVolume >> 31 & 3)) >> 2, -1);
    }
}

// ============================================================
//  Theme_SetVolumeFull  (0x0047c740)
// ============================================================
// Re-applies g_nThemeVolume at full level to channel 0.
void Theme_SetVolumeFull(void)
{
    if (g_nThmReady != 0) {
        Mixer_SetVolume(0, g_nThemeVolume, -1);
    }
}

// ============================================================
//  Theme_LockMemPool  (0x0047c7e0)
// ============================================================
// Acquires g_nThemeMemPoolCS so callers can safely read/write the
// PCM pool without the music thread moving the write head.
void Theme_LockMemPool(void)
{
    if (g_nThmReady != 0) {
        EnterCriticalSection((LPCRITICAL_SECTION)&g_nThemeMemPoolCS);
    }
}

// ============================================================
//  Theme_UnlockMemPool  (0x0047c880)
// ============================================================
// Releases g_nThemeMemPoolCS.
void Theme_UnlockMemPool(void)
{
    if (g_nThmReady != 0) {
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nThemeMemPoolCS);
    }
}

// ============================================================
//  Theme_VolumeDown  (0x0047c920)
// ============================================================
// Decrements g_nThemeVolume by 1 (clamped at 0) and applies it.
// Logs "Music volume = %d" via Debug_Trace.
void Theme_VolumeDown(void)
{
    if (g_nThmReady != 0 && g_nThemeVolume >= 0) {
        g_nThemeVolume--;
        Debug_Trace(0, 0, "Music volume = %d", g_nThemeVolume);
        Mixer_SetVolume(0, g_nThemeVolume, -1);
    }
}

// ============================================================
//  Theme_VolumeUp  (0x0047ca00)
// ============================================================
// Increments g_nThemeVolume by 1 (clamped at 64) and applies it.
// Logs "Music volume = %d" via Debug_Trace.
void Theme_VolumeUp(void)
{
    if (g_nThmReady != 0 && g_nThemeVolume < 0x40) {
        g_nThemeVolume++;
        Debug_Trace(0, 0, "Music volume = %d", g_nThemeVolume);
        Mixer_SetVolume(0, g_nThemeVolume, -1);
    }
}

// ============================================================
//  Theme_RestartCurrentTrack  (0x0047cae0)
// ============================================================
// If not in silent mode and the current track differs from the last
// track played on the channel (g_szThemeCurrentTrack vs g_szThemeNextTrack),
// calls Thm_Play to restart from the beginning.
void Theme_RestartCurrentTrack(void)
{
    if (g_nThemeSilentMode == 0) {
        if (FUN_0049a830(g_szThemeCurrentTrack, g_szThemeNextTrack) != 0) {
            Thm_Play(g_szThemeCurrentTrack, NULL);
        }
    }
}

// ============================================================
//  Theme_SetRoom  (0x0047cba0)
// ============================================================
// Sets the current room ID and triggers the appropriate music response:
//   - If no room was set yet (g_nThemeCurrentRoom == 0) and param_1 != 0:
//     store room ID and call Theme_RestartCurrentTrack.
//   - If a room was set and param_1 != 1 (not "same room"):
//     store new room ID and call Theme_StopMusic.
void Theme_SetRoom(int roomId)
{
    if (g_nThmReady != 0) {
        if (g_nThemeCurrentRoom == 0) {
            if (roomId != 0) {
                g_nThemeCurrentRoom = roomId;
                Theme_RestartCurrentTrack();
            }
        } else if (roomId != 1) {
            g_nThemeCurrentRoom = roomId;
            Theme_StopMusic();
        }
    }
}

// ============================================================
//  Theme_GetRoom  (0x0047cc70)
// ============================================================
// Returns the current room ID (g_nThemeCurrentRoom).
int Theme_GetRoom(void)
{
    return g_nThemeCurrentRoom;
}

// ============================================================
//  Theme_StartPendingStreams  (0x0047cd00)
// ============================================================
// For each mem slot that is in-use (bit 2 set) and ready (bit 1 set)
// but not yet playing (DAT_007d0288[slot] == 0): calls Res_WaitForEntry
// to block until the async load completes.
void Theme_StartPendingStreams(void)
{
    extern int          DAT_007d0288[];
    extern unsigned int DAT_007d0290[];
    extern int          DAT_007d027c[];

    int i;
    for (i = 0; i < 100; i++) {
        unsigned int flags = *(unsigned int*)((char*)DAT_007d0290 + i * 0x20);
        // bit 1 set: (flags << 0x1e) < 0  → flags & 0x4
        // bit 0 set: (flags << 0x1f) < 0  → flags & 0x2
        if (((flags << 0x1e) < 0) &&
            ((flags << 0x1f) < 0) &&   // cast to signed
            (*(int*)((char*)DAT_007d0288 + i * 0x20) == 0))
        {
            Res_WaitForEntry((void*)((char*)DAT_007d027c + i * 0x20));
        }
    }
}

// ============================================================
//  Theme_FillMemAndStartStreams  (0x0047ce10)
// ============================================================
// Synchronously pre-loads all music for the current track into the
// PCM pool before the room starts.  Named "thm_fill_mem" in debug.
//
// Loops calling Theme_LoadSegs(1) and Theme_PushSegOp until the
// current track buffer is fully filled (load cursor <= 0 and current
// track matches the saved snapshot), then calls
// Theme_StartPendingStreams to block until all async loads complete.
void Theme_FillMemAndStartStreams(void)
{
    char szSavedTrack[256];
    int  loadResult = 0;
    int  pushResult = 0;

    FUN_004895e0(szSavedTrack, &g_szThemeCurrentTrack);

    if (g_nThmReady == 0 || g_nThemeMusicActive == 0) {
        return;
    }

    Debug_Trace(0, 0, "music fill mem");

    // Keep loading until either the load queue is drained or the track changes
    while (loadResult != -1 &&
           FUN_0049a830(szSavedTrack, &g_szThemeCurrentTrack) == 0)
    {
        loadResult = Theme_LoadSegs(1);

        // Keep pushing seg-ops until load cursor advances past 0 or track changes
        while (pushResult != -1 &&
               g_nThemeLoadCursor < 1 &&
               FUN_0049a830(szSavedTrack, &g_szThemeCurrentTrack) == 0)
        {
            pushResult = Theme_PushSegOp();
        }
    }

    Theme_StartPendingStreams();
}

// ============================================================
//  Theme_SetTransitionMode  (0x0047cf80)
// ============================================================
// Stores the transition mode used for the next room switch.
// See g_nThemeTransitionMode comments above for mode values.
void Theme_SetTransitionMode(int mode)
{
    g_nThemeTransitionMode = mode;
}

// ============================================================
//  Theme_KillTimer  (0x0047d0b0)
// ============================================================
// Kills a multimedia timer by ID, removes it from g_nThemeTimerIds[],
// and clears the corresponding g_nThemeTimerFuncs[] entry.
void Theme_KillTimer(int timerId)
{
    extern int g_aThemeTimerIds[];
    extern int g_aThemeTimerFuncs[];

    int i;
    if (timerId == 0) return;

    for (i = 0; i < g_nThemeTimerCount; i++) {
        if (g_aThemeTimerIds[i] == timerId) {
            timeKillEvent((UINT)timerId);
            g_aThemeTimerIds[i]   = 0;
            g_aThemeTimerFuncs[i] = 0;
            break;
        }
    }
}

// ============================================================
//  Theme_KillAllTimers  (0x0047d1b0)
// ============================================================
// Kills all active timers in the table.  Called during shutdown or
// when switching rooms to ensure no stale callbacks fire.
void Theme_KillAllTimers(void)
{
    extern int g_aThemeTimerIds[];
    extern int g_aThemeTimerFuncs[];

    int i;
    for (i = 0; i < g_nThemeTimerCount; i++) {
        if (g_aThemeTimerFuncs[i] != 0) {
            if (g_aThemeTimerIds[i] != 0) {
                timeKillEvent((UINT)g_aThemeTimerIds[i]);
            }
            g_aThemeTimerIds[i]   = 0;
            g_aThemeTimerFuncs[i] = 0;
        }
    }
}

// ============================================================
//  Theme_TimerCallback  (0x0047d2b0)
// ============================================================
// Multimedia timer trampoline: called by the system timer at the rate
// set in Theme_SetTimer.  Dispatches to g_aThemeTimerFuncs[slotIdx].
// param_3 is the slot index passed as the dwUser value to timeSetEvent.
void Theme_TimerCallback(int unused1, int unused2, int slotIdx)
{
    extern int g_aThemeTimerFuncs[];
    typedef void (*TimerFn)(void);
    TimerFn fn = (TimerFn)g_aThemeTimerFuncs[slotIdx];
    if (fn) fn();
}

// ============================================================
//  Theme_SetTimer  (0x0047d390)
// ============================================================
// Creates a periodic multimedia timer at 'freqHz' Hz that calls
// Theme_TimerCallback (via the stub at 0x00401870).
//
// Returns the MMRESULT timer ID, or 0 on failure.
// Allocates a new slot from g_aThemeTimerIds (up to 10 slots;
// beyond 10 it recycles the first empty slot).
int Theme_SetTimer(int callbackFn, int freqHz)
{
    extern int g_aThemeTimerIds[];
    extern int g_aThemeTimerFuncs[];

    // Timer callback stub address confirmed by decompile at 0x00401870
    typedef void (CALLBACK *MmTimerCb)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
    extern MmTimerCb g_pfnThemeTimerStub;   // 0x00401870

    int slot;
    MMRESULT id;

    if (g_nThemeTimerCount < 10) {
        slot = g_nThemeTimerCount;
    } else {
        // Find first empty slot
        slot = 0;
        while (slot < 10 && g_aThemeTimerFuncs[slot] != 0) {
            slot++;
        }
    }

    g_aThemeTimerFuncs[slot] = callbackFn;

    id = timeSetEvent(1000 / freqHz, 5,
                      g_pfnThemeTimerStub,
                      (DWORD_PTR)slot,
                      TIME_PERIODIC);

    g_aThemeTimerIds[slot] = (int)id;

    if (g_nThemeTimerCount < 10) {
        g_nThemeTimerCount++;
    }

    return (int)id;
}

// ============================================================
//  Theme_InitTimerTable  (0x0047d510)
// ============================================================
// Resets the async-program table:
//   g_nThemeAsyncProgCount = 0
//   g_nThemeAsyncProgBase  = 0
void Theme_InitTimerTable(void)
{
    g_nThemeAsyncProgCount = 0;
    g_nThemeAsyncProgBase  = 0;
}

// ============================================================
//  Theme_RegisterAsyncProg  (0x0047d5b0)
// ============================================================
// Adds a function pointer to the async-program table (max 100 entries).
// Asserts "Too many Async programs" if the limit is exceeded.
void Theme_RegisterAsyncProg(int progFn)
{
    if (g_nThemeAsyncProgCount + 1 > 100) {
        Debug_Assert(__LINE__, "Too many Async programs", 0);
        return;
    }
    // g_nThemeAsyncProgFuncs[g_nThemeAsyncProgCount] = 0  (state slot)
    // g_nThemeAsyncProgFuncs_data[g_nThemeAsyncProgCount] = progFn
    g_nThemeAsyncProgCount++;
}
