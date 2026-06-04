#ifndef THEMES_H
#define THEMES_H

// ---------------------------------------------------------------------------
// THEMES.h  —  Music theme / room-music streaming system
// Original: C:\DevStudio\Projects\Crux\THEMES.cpp
// RE offset: 0x004798e0 – 0x0047d5ef
// ---------------------------------------------------------------------------
// THEMES.cpp is the room-music streaming engine for Crux.  It owns:
//
//   - A circular PCM ring buffer (g_pThemeMemPool) shared with the MIXER.
//   - A background thread that sequences music segments (cues) from a
//     per-scene event table loaded by READRES.
//   - A music-event dispatcher (Theme_MusicEvent) that scripts call to
//     trigger named stings or transitions.
//   - Room-level music routing via Theme_SetRoom / Theme_GetRoom.
//   - Fade-out driven by a multimedia timer (timeSetEvent).
//   - A small multimedia-timer wrapper (Theme_SetTimer / KillTimer) and
//     an async-program registration table used by SCHED.cpp.
//
// "Segment" terminology:
//   A "seg" is a PCM music cue identified by a resource name.  The scene's
//   event table lists sequences of seg-ops that the background thread works
//   through.  The pool holds decoded PCM; the MIXER reads from it.
//
// Volume range: 0 (silence) .. 64 (maximum).
//
// Thread safety: g_nThemeCS guards room/segment state; g_nThemeMemPoolCS
// guards the PCM pool write head.  Both are Win32 CRITICAL_SECTIONs stored
// as plain int arrays (opaque to Ghidra/the RE).
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// System state
extern int   g_nThemeSystemEnabled;    // 0x007d3950  set by Theme_Init, cleared by Shutdown
extern int   g_nThemeCritSecInited;    // 0x007d3980  g_nThemeCS is valid
extern int   g_nThemeMusicActive;      // 0x007d3a8c  music is currently playing
extern int   g_nThemeSilentMode;       // 0x007d3a90  suppress background loads

// Thread / sync
extern HANDLE g_hThemeThread;          // 0x007c4bac  background music thread
extern int    g_nThemeThreadId;        // 0x007d0268  thread ID
extern HANDLE g_hThemeFinishedEvent;   // 0x007d3978  "FinishedThmSeg" event
extern HANDLE g_hThemeExecuteEvent;    // 0x007d397c  "Execute_command" event
extern int    g_nThemeCS;              // 0x007d3930  CRITICAL_SECTION (24 bytes)
extern int    g_nThemeMemPoolCS;       // 0x007cf428  pool CRITICAL_SECTION (24 bytes)

// Room / segment state
extern int   g_nThemeCurrentRoom;      // 0x004da788  active room ID (0=none)
extern int   g_nThemeSegCount;         // 0x007d3968  entries in the scene seg table
extern int   g_nThemeCurEventSeg;      // 0x007d3964  current event segment
extern int   g_nThemeSegStackTop;      // 0x007d3970  seg-op stack depth (max 51)
extern int   g_nThemeSegOpCursor;      // 0x004da76c  walk cursor in segment table
extern int   g_nThemeSegOpType;        // 0x007cebe4  current seg-op type (0-6)
extern int   g_nThemeActiveChan;       // 0x007cebe0  active mixer channel index
extern int   g_nThemePrevSegIdx;       // 0x007d3974  previous segment index
extern int   g_nThemePendingSegIdx;    // 0x004da778  next seg to play (-1=none)
extern int   g_nThemeLastLoadedSeg;    // 0x004da774  most recently loaded segment
extern int   g_nThemeLastMemSlot;      // 0x007cff34  slot from last SegToMem call
extern int   g_nThemeLoadCursor;       // 0x004da770  down-counter for load queue
extern int   g_nThemeChanLooping;      // 0x007cff38  non-zero when channel loops
extern int   g_nThemeMemFilling;       // 0x007d0264  re-entry guard for mem fill
extern int   g_nThemePendingCmd;       // 0x007d3a94  thread cmd (0=none 1=stop 2=xfade 3=endseg 4=event)

// PCM memory pool
extern void* g_pThemeMemPool;          // 0x007d0260  PCM ring buffer (silence = 0x80)
extern int   g_nThemePoolSize;         // 0x007c8198  pool size in bytes
extern int   g_nThemePoolWriteHead;    // 0x007d3948  write position (circular)
extern void* g_pThemePlayPtr;          // 0x007d347c  pointer to current play block
extern int   g_nThemePlaySize;         // 0x007d394c  size of current play block in bytes
extern int   g_nThemePlayChannelFlags; // 0x007d026c  channel format flags

// Track name buffers (char arrays at fixed addresses)
extern char  g_szThemeCurrentTrack[];  // 0x007d3988  currently playing track name
extern char  g_szThemeNextTrack[];     // 0x007d3ab0  next track name
extern char  g_szThemePendingTrack[];  // 0x007cf228  queued pending track name
extern char  g_szThemePendingExt[];    // 0x007cf328  pending track extension/variant

// Transition / fade
extern int   g_nThemeTransitionMode;   // 0x004da77c  1=cut 2=xfade 3=endseg 4=endseg+fade 5=endseg2 7=loop
extern int   g_nThemeFadeDuration;     // 0x004da784  fade ms (-1 = default 3000ms)
extern int   g_nThemeFadeStep;         // 0x007d3478  volume delta per fade tick
extern int   g_nThemeFadeTimerId;      // 0x007d3a88  active fade timer ID (0=none)

// Volume
extern int   g_nThemeVolume;           // 0x004da780  current volume 0..64

// Timer subsystem
extern int   g_nThemeTimerCount;       // 0x007d4c18  active timers (max 10)
extern int   g_nThemeTimerIds;         // 0x007d4bc8  array[10] of MMTIMER IDs
extern int   g_nThemeTimerFuncs;       // 0x007d4bf0  array[10] of callback ptrs (as int)

// Async program table
extern int   g_nThemeAsyncProgCount;   // 0x007d5020  registered async progs (max 100)
extern int   g_nThemeAsyncProgBase;    // 0x007d5668  base index/offset
extern int   g_nThemeAsyncProgFuncs;   // 0x007d5028  array[100] of prog ptrs (as int)

// ---------------------------------------------------------------------------
// Seg-op type constants (stored in seg-op stack entries, field[0])
// ---------------------------------------------------------------------------
#define THEME_SEGOP_PLAY        0  // play named segment
#define THEME_SEGOP_SILENCE     1  // fill with silence
#define THEME_SEGOP_EVENT       2  // execute event-cue callback
#define THEME_SEGOP_STOP        3  // stop music and exit thread loop
#define THEME_SEGOP_FADE        4  // set fade duration
#define THEME_SEGOP_XFADE       5  // crossfade to next file
#define THEME_SEGOP_SETFADE     6  // set g_nThemeFadeDuration

// Transition mode constants (g_nThemeTransitionMode)
#define THEME_TRANS_CUT         1  // immediate cut to next track
#define THEME_TRANS_XFADE       2  // crossfade
#define THEME_TRANS_ENDSEG      3  // wait for segment end, then crossfade (default 3000ms)
#define THEME_TRANS_ENDSEG_FADE 4  // wait for segment end, then fade (custom duration)
#define THEME_TRANS_ENDSEG2     5  // wait for segment end (no fade-out)
#define THEME_TRANS_LOOP        7  // loop current track then switch

// ---------------------------------------------------------------------------
// Pending command constants (g_nThemePendingCmd)
// ---------------------------------------------------------------------------
#define THEME_CMD_NONE          0
#define THEME_CMD_STOP          1  // stop music after fade completes
#define THEME_CMD_XFADE         2  // crossfade to pending track
#define THEME_CMD_ENDSEG        3  // wait-for-endseg then play
#define THEME_CMD_EVENT         4  // re-dispatch music event after mem fill

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Initialise the music system: allocate pool, spawn background thread.
// Call once at startup before any other Theme_ function.
void Theme_Init(void);

// Shut down the music system: terminate thread, close handles, delete CS.
void Theme_Shutdown(void);

// Set the active room ID to trigger the appropriate music.
// Pass 0 to leave the room (no music). Pass 1 = "same room, no change".
void Theme_SetRoom(int roomId);

// Return the current room ID (0 = none).
int  Theme_GetRoom(void);

// Dispatch a named music event to select/switch the current cue.
// Called by scripts; eventName matches entries in the scene seg table.
void Theme_MusicEvent(const char* eventName);

// Pre-fill the PCM pool synchronously and start any deferred decoders.
// Call when entering a new room before playback begins.
void Theme_FillMemAndStartStreams(void);

// Begin streaming the next segment into the pool; called by the music thread.
// loadNow=1 for synchronous load; 0 allows deferred/background load.
int  Theme_LoadSegs(int loadNow);

// Decode a named music file into a free PCM pool slot.
// Returns slot index (0..99), -1 on failure, -2 if file not found.
int  Theme_SegToMem(const char* name);

// Advance the segment-op stack by one entry from the scene event table.
// Returns new stack depth, -1 when exhausted.
int  Theme_PushSegOp(void);

// Set volume (0..64) and immediately apply to mixer channel 0.
void Theme_SetVolume(int volume);

// Return current volume (0..64).
int  Theme_GetVolume(void);

// Decrement volume by 1 (clamped at 0).
void Theme_VolumeDown(void);

// Increment volume by 1 (clamped at 64).
void Theme_VolumeUp(void);

// Apply g_nThemeVolume at 25% (quarter level).
void Theme_SetVolumeQuarter(void);

// Apply g_nThemeVolume at 100% (full level).
void Theme_SetVolumeFull(void);

// Begin a volume fade-out over durationMs milliseconds.
// Sets g_nThemePendingCmd=1 so the thread stops music when fade completes.
void Theme_FadeOut(int durationMs);

// Fade in from 0 to g_nThemeVolume over durationMs (synchronous).
void Theme_FadeIn(int durationMs);

// Return true while a fade-out timer is active.
bool Theme_IsFading(void);

// Stop music immediately (no fade). Clears g_nThemeMusicActive.
void Theme_StopMusic(void);

// Stop music and free any pending fade timer.
void Theme_StopMusicAndFree(void);

// Pause music (MIXER::Pause channel 0).
void Theme_PauseMusic(void);

// Resume music (MIXER::Resume channel 0).
void Theme_ResumeMusic(void);

// Restart the current track from the beginning if it has changed.
void Theme_RestartCurrentTrack(void);

// Set transition mode for next room switch (see THEME_TRANS_* constants).
void Theme_SetTransitionMode(int mode);

// Lock/unlock the PCM pool CRITICAL_SECTION for external access.
void Theme_LockMemPool(void);
void Theme_UnlockMemPool(void);

// Start any deferred PCM decoders (ONTHEFLY::Flush) for pending slots.
void Theme_StartPendingStreams(void);

// --- Timer subsystem --------------------------------------------------------

// Create a periodic timer at freqHz Hz calling callbackFn.
// Returns MMTIMER ID, or 0 on failure.
int  Theme_SetTimer(int callbackFn, int freqHz);

// Kill a timer by ID; removes it from the timer table.
void Theme_KillTimer(int timerId);

// Kill all active timers.
void Theme_KillAllTimers(void);

// Reset the async-program table (g_nThemeAsyncProgCount = 0).
void Theme_InitTimerTable(void);

// Register a function pointer as an async program (max 100).
void Theme_RegisterAsyncProg(int progFn);

// Placeholder callbacks — no-ops in this build.
void Theme_Nop1(void);
void Theme_Nop2(void);
void Theme_Nop3(void);
void Theme_Nop4(void);

// ---------------------------------------------------------------------------
// External dependencies (resolved when those modules are reversed)
// ---------------------------------------------------------------------------

// MIXER module — music playback channel
// MIXER::SetVolume(channel, volume, flags)  — thunk_FUN_00443060
// MIXER::GetVolume(channel)                 — thunk_FUN_00443510
// MIXER::Stop(channel)                      — thunk_FUN_00443df0
// MIXER::IsActive(channel)                  — thunk_FUN_00443d50
// MIXER::Pause(channel)                     — thunk_FUN_00443820
// MIXER::Resume(channel)                    — thunk_FUN_00443970
// MIXER::PlaySeg(trackPtr, extPtr)          — thunk_FUN_00478580
// MIXER::FillBuf(channel, timeout)          — thunk_FUN_00478380

// NOTE: These three thunks were previously labelled "ONTHEFLY" but are
// actually READRES.cpp functions — confirmed by decompilation:
//   thunk_FUN_0045d4e0 = Res_FindByNumChar     (READRES.cpp)
//   thunk_FUN_0045db60 = Res_BunchFreadLoadPtr  (READRES.cpp)
//   thunk_FUN_0045e6f0 = Res_WaitForEntry       (READRES.cpp)
//
// The actual ONTHEFLY.cpp module (0x004577a0–0x00457c90) is a spatial
// area-node subsystem (OTF_AllocSlot, OTF_AllocNodeList, OTF_AreaTip) —
// it creates dynamic on-the-fly clickable/hoverable area nodes at runtime.
// It has no audio decoding functionality.

// READRES module — resource reader
// READRES::OpenSeg(tag, name, codecHint)    — thunk_FUN_0041fad0

// ERRORS module
// Debug_Trace(line, file, msg, ...)
// Debug_Assert(line, file)

// SAFEHEAP module
// SafeAlloc(tag, size)                      — thunk_FUN_0046bcc0
// SafeFree(tag, file, ptr)                  — thunk_FUN_0046bd80

#endif // THEMES_H
