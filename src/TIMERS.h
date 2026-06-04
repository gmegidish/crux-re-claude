#ifndef TIMERS_H
#define TIMERS_H

// ---------------------------------------------------------------------------
// TIMERS.h  —  Frame-counted game timer / async-program scheduler
// Original: C:\DevStudio\Projects\Crux\TIMERS.cpp
// RE offset: 0x0047d6a0 – 0x0047ecf0  (21 functions)
// ---------------------------------------------------------------------------
//
// This module implements the game's high-level timer layer, sitting on top
// of the multimedia-timer infrastructure in THEMES.cpp.  Rather than wall-
// clock milliseconds, all timers here count down in "frames" — discrete ticks
// delivered by Timer_Tick / Timer_Untick which are called from the main game
// loop (registered via thunk_FUN_004065e0 / thunk_FUN_00411760).
//
// Architecture:
//
//   Timer table  (g_aTimers, max TIMER_MAX = 30 entries, stride 20 bytes)
//   -----------------------------------------------------------------------
//   Each entry:
//     [+0x00] int  callback   — function pointer to fire when countdown hits 0
//     [+0x04] int  frames     — remaining frames (decremented by Timer_Tick)
//     [+0x08] int  pauseCount — suspend depth (incremented by Tick on pause,
//                               decremented by Untick; fire only when == 0)
//     [+0x0c] int  flags      — 0 = async (fire via Timer_AddAsyncProg),
//                               1 = sync  (fire via Timer_AddSyncProg)
//     [+0x10] int  resetVal   — original interval for self-resetting timers
//                               (non-zero only for Timer_AddWithReset entries)
//   g_nTimerCount  (0x007d4c30) — number of active timers
//
//   Async-program queue  (re-uses THEMES globals, max 100 entries)
//   -----------------------------------------------------------------------
//   g_nThemeAsyncProgCount  (0x007d5020) — queue depth
//   g_aAsyncProgFn[]        (0x007d5028) — function pointers (=g_nThemeAsyncProgFuncs)
//   g_aAsyncProgData[]      (0x007d51b8) — extra data per entry
//   g_aAsyncProgWait[]      (0x007d5670) — per-entry wait counter
//
//   Sync-program queue  (max 100 entries)
//   -----------------------------------------------------------------------
//   g_nThemeAsyncProgBase   (0x007d5668) — queue depth
//   g_aSyncProgFn[]         (0x007d54d8) — function pointers
//   g_aSyncProgData[]       (0x007d4c38) — extra data per entry
//   g_aSyncProgWait[]       (0x007d5348) — per-entry wait counter
//
// Tick / Untick:
//   Timer_Tick   — increments all pause counters (timer table + both prog
//                  queues).  Registered via the game loop hook table.
//   Timer_Untick — decrements pause counters (clamped at 0) for all three
//                  structures.  Also a registered game loop hook.
//
// Timer lifecycle (normal, async):
//   Timer_AddAsync(frames, callback)
//     → writes entry in g_aTimers[], flags=0, pauseCount=0, resetVal=0
//     → if this is the first timer, registers Timer_TickCallback with the
//       game loop (thunk_FUN_004065e0) and Timer_Reset with the shutdown
//       hook (thunk_FUN_00411760)
//   Timer_TickCallback — called every game frame from the loop:
//     → for each timer: if pauseCount < 1, decrement frames
//       → if frames < 1: fire → if flags==0 call Theme_RegisterAsyncProg(cb)
//                                else call Timer_AddSyncProg(cb)
//         compact the table; if empty, deregister hooks
//   Timer_Kill(callback) — remove timer by callback pointer
//   Timer_Reset — deregister all hooks and clear all three structures
//
// "FireTimer" (called from Graninv.cpp / SPEECH.cpp):
//   FireTimer(handle) in Graninv.cpp corresponds to Timer_AddAsync or
//   Timer_AddSync — the handle is an (frames, callback) pair.  The actual
//   "fire a ready trigger immediately" path goes through
//   Theme_RegisterAsyncProg (which is thunk_FUN_0047d5b0).  The function
//   that Graninv calls as FireTimer(g_nGranTriggerHandle) is Timer_AddAsync
//   (0x0047e3a0) — it enqueues the callback for async dispatch.
//
//   For the SPEECH side: SPEECH.cpp calls thunk_FUN_0047d5b0 directly =
//   Theme_RegisterAsyncProg, not any function in this file.
//
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define TIMER_MAX           30   // maximum simultaneous game timers
#define TIMER_PROG_MAX     100   // maximum async/sync programs per queue

// flags field in TimerEntry
#define TIMER_FLAG_ASYNC    0    // fire via async prog queue
#define TIMER_FLAG_SYNC     1    // fire via sync prog queue

// ---------------------------------------------------------------------------
// Timer table entry (stride 0x14 = 20 bytes at 0x007d4dc8)
// ---------------------------------------------------------------------------

struct TimerEntry {
    int  callback;    // +0x00  function pointer to invoke
    int  frames;      // +0x04  frames remaining before fire
    int  pauseCount;  // +0x08  suspend depth (0 = running)
    int  flags;       // +0x0c  TIMER_FLAG_ASYNC / TIMER_FLAG_SYNC
    int  resetVal;    // +0x10  non-zero = restart with this frame count after fire
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Timer table
extern TimerEntry g_aTimers[];      // 0x007d4dc8  array[TIMER_MAX] of TimerEntry
extern int        g_nTimerCount;    // 0x007d4c30  active timer count

// Async program queue (shared with THEMES counters)
extern int  g_nThemeAsyncProgCount; // 0x007d5020  (declared in THEMES.h)
extern int  g_nThemeAsyncProgBase;  // 0x007d5668  (declared in THEMES.h)

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Add a one-shot async timer: fires callback after 'frames' game ticks.
// Async = callback is enqueued via Theme_RegisterAsyncProg for deferred exec.
// This is the function called as FireTimer() from Graninv.cpp.
void Timer_AddAsync(int frames, int callback);

// Add a one-shot sync timer: fires callback via the sync-program queue.
void Timer_AddSync(int frames, int callback);

// Add a self-resetting sync timer: re-arms itself with the same interval
// after each fire. Stores original interval in TimerEntry.resetVal.
void Timer_AddWithReset(int frames, int callback);

// Remove the timer associated with 'callback'. Compacts the table.
// Deregisters game-loop hooks if the table becomes empty.
void Timer_Kill(int callback);

// Clear all timers and program queues; deregister all hooks.
// Registered as a shutdown hook via thunk_FUN_00411760.
void Timer_Reset(void);

// Reset countdown counters for all self-resetting timers
// (restores TimerEntry.frames from TimerEntry.resetVal where non-zero).
void Timer_ResetCounters(void);

// Increment all pause counters by 1.
// Registered as a game-loop tick hook (thunk_FUN_004065e0 at rate 0, priority 3).
void Timer_Tick(void);

// Decrement all pause counters by 1 (clamped at 0).
// Registered as a game-loop tick hook (thunk_FUN_004065e0 at rate 0, priority 3).
void Timer_Untick(void);

// Game-loop callback: advance all timer countdowns and fire any that expire.
// Called every frame while at least one timer is active.
void Timer_TickCallback(void);

// ---------------------------------------------------------------------------
// Async / Sync program queue internals (called by Timer_TickCallback)
// ---------------------------------------------------------------------------

// Add a function pointer to the async-program queue (max 100).
// Asserts "Too many Async programs" on overflow.
void Timer_AddAsyncProg(int progFn, int data);

// Add a function pointer to the sync-program queue (no data, max 100).
// Asserts "Too many Sync programs" on overflow.
void Timer_AddSyncProg(int progFn);

// Add a function pointer + data to the sync-program queue (max 100).
// Asserts "Too many Sync programs" on overflow.
void Timer_AddSyncProgWithData(int progFn, int data);

// Check whether the async queue has at least one ready (wait==0) entry.
// Returns 1 if yes, 0 if all entries are still waiting.
int Timer_HasPendingAsyncProg(void);

// Check whether the sync queue has at least one ready (wait==0) entry.
// Returns 1 if yes, 0 if all entries are still waiting.
int Timer_HasPendingSyncProg(void);

// Pop the first ready async entry, invoke it, and remove it from the queue.
void Timer_DispatchAsyncProg(void);

// Pop the first ready sync entry, invoke it, and remove it from the queue.
void Timer_DispatchProg(void);

// Internal: pop and return the fn pointer + data for the first ready async entry.
// Compacts the async queue. Returns fn ptr; writes data to *pData.
int Timer_PopAsyncProg(int* pData);

// Internal: pop and return the fn ptr for the first ready sync entry.
// Compacts the sync queue. Returns fn ptr; writes data to *pData.
int Timer_PopSyncProg(int* pData);

// Run one async prog: save context, pop and call the ready entry, restore context.
void Timer_RunAsyncProg(void);

// Run one sync prog: save context, pop and call the ready entry, restore context.
void Timer_RunSyncProg(void);

// ---------------------------------------------------------------------------
// Trigger object constructor (0x0047ecf0)
// ---------------------------------------------------------------------------
// Initialises a trigger/timer object at param_1 from a string name param_2.
// Hashes the name (thunk_FUN_004098c0), allocates a name buffer, copies the
// string, and sets up a trigger record in DAT_005b10b0[hash * 0x58].
// __thiscall: param_1 is 'this'.
int Timer_TriggerInit(int pThis, const char* pszName);

#endif // TIMERS_H
