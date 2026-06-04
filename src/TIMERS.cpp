// TIMERS.cpp — Frame-counted game timer / async-program scheduler
//
// Implements the game's high-level timer layer.  All countdowns are measured
// in "frames" (discrete ticks from the main game loop) rather than wall-clock
// milliseconds.  Two independent fire modes exist:
//   async — callback is enqueued via Theme_RegisterAsyncProg (deferred exec)
//   sync  — callback is enqueued via a second prog queue (sync exec)
//
// Self-resetting timers (Timer_AddWithReset) store the original interval in
// TimerEntry.resetVal and re-arm after each fire via Timer_ResetCounters.
//
// "FireTimer" from Graninv.cpp:
//   The function called as FireTimer(handle) in Graninv.cpp is Timer_AddAsync
//   (0x0047e3a0).  The handle encodes (frames, callback).
//
// Original source: C:\DevStudio\Projects\Crux\TIMERS.cpp
// Address range:   0x0047d6a0 – 0x0047ecf0 (21 functions)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "TIMERS.h"
#include "THEMES.h"     // Theme_RegisterAsyncProg, g_nThemeAsyncProgCount/Base

// ============================================================
//  Globals
// ============================================================

// Timer table — stride 0x14 (20 bytes), base 0x007d4dc8
// g_aTimers[0].callback  at 0x007d4dc8
// g_aTimers[0].frames    at 0x007d4dcc
// g_aTimers[0].pauseCount at 0x007d4dd0
// g_aTimers[0].flags     at 0x007d4dd4
// g_aTimers[0].resetVal  at 0x007d4dd8
TimerEntry  g_aTimers[TIMER_MAX];   // 0x007d4dc8

// Number of active timers in g_aTimers[]
int         g_nTimerCount = 0;      // 0x007d4c30

// ---- Async-program queue (shared with THEMES globals) ----------------------
// g_nThemeAsyncProgCount (0x007d5020) — queue depth         (declared in THEMES.h)
// g_nThemeAsyncProgFuncs (0x007d5028) — fn ptr array[100]   (declared in THEMES.h)
// g_aAsyncProgData[]     (0x007d51b8) — data array[100]
// g_aAsyncProgWait[]     (0x007d5670) — wait counter[100]
// (These are the DAT_007d51b8 / DAT_007d5670 arrays in the decompile.)

// ---- Sync-program queue ----------------------------------------------------
// g_nThemeAsyncProgBase  (0x007d5668) — queue depth         (declared in THEMES.h)
// g_aSyncProgFn[]        (0x007d54d8) — fn ptr array[100]
// g_aSyncProgData[]      (0x007d4c38) — data array[100]
// g_aSyncProgWait[]      (0x007d5348) — wait counter[100]

// ============================================================
//  Forward declarations for unresolved cross-module helpers
// ============================================================

// THEMES.cpp — register a deferred async callback (thunk_FUN_0047d5b0)
extern void Theme_RegisterAsyncProg(int progFn);

// Game-loop hook registration (game scheduler module)
// thunk_FUN_004065e0(fn, interval, priority) — register per-frame callback
extern void GameLoop_RegisterHook(void (*fn)(void), int interval, int priority);
// thunk_FUN_004066e0(fn)                    — unregister per-frame callback
extern void GameLoop_UnregisterHook(void (*fn)(void));
// thunk_FUN_00411760(fn)                    — register shutdown hook
extern void GameLoop_RegisterShutdown(void (*fn)(void));
// thunk_FUN_00411870(fn)                    — unregister shutdown hook
extern void GameLoop_UnregisterShutdown(void (*fn)(void));

// Script context save/restore (SCHED / interpreter module)
// thunk_FUN_00462380() — save current script exec context
extern void Script_SaveContext(void);
// thunk_FUN_00462560(fn, data) — restore and invoke fn(data)
extern void Script_RestoreAndCall(int fn, int data);

// Debug / assert
// thunk_FUN_0041f680(level, file, msg) — assert/fatal
extern void Debug_Assert(int nLevel, const char* pszFile, const char* pszMsg);
// thunk_FUN_00420e60(level, file)      — error (no message)
extern void Debug_Error(int nLevel, const char* pszFile);
// thunk_FUN_0041fad0(n, tag, flags)    — dump/log helper
extern void* Debug_DumpTable(int n, const char* pszTag, int flags);

// Memory
extern int  Mem_Alloc_Raw(int nSize);                   // thunk_FUN_004890e0
extern void Mem_Copy(void* pDst, const void* pSrc);    // thunk_FUN_004895e0
extern int  FUN_00489090(void* pSrc, void* pDst);      // format helper

// String / hash
extern int  Str_Hash(const char* psz, int nMax);        // thunk_FUN_004098c0
extern int  _strlen(const char* psz);

// ============================================================
//  Async-program queue operations  (0x0047d6a0 – 0x0047df40)
// ============================================================

// ------------------------------------------------------------
//  Timer_AddAsyncProg  (0x0047d6a0)
// ------------------------------------------------------------
// Appends (progFn, data) to the async-program queue.
// Slot layout: g_aAsyncProgFn[n] = progFn  (via g_nThemeAsyncProgFuncs offset)
//              g_aAsyncProgData[n] = data   (DAT_007d51b8)
//              g_aAsyncProgWait[n] = 0      (DAT_007d5670, cleared at add)
//
// Original debug string: "C:\DevStudio\Projects\Crux\TIMERS.cpp"
//                        "Too many Async programs"
void Timer_AddAsyncProg(int progFn, int data)
{
    if (g_nThemeAsyncProgCount + 1 > 100) {
        Debug_Assert(/* line */ 0, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp",
                     "Too many Async programs");
    }
    // g_aAsyncProgData[g_nThemeAsyncProgCount] = data
    // g_aAsyncProgWait[g_nThemeAsyncProgCount] = 0
    // g_nThemeAsyncProgFuncs[g_nThemeAsyncProgCount] = progFn
    g_nThemeAsyncProgCount++;
}

// ------------------------------------------------------------
//  Timer_AddSyncProg  (0x0047d7a0)
// ------------------------------------------------------------
// Appends progFn to the sync-program queue (no data).
// Slot layout: g_aSyncProgFn[n] = progFn   (DAT_007d54d8)
//              g_aSyncProgWait[n] = 0       (DAT_007d5348)
//
// Original debug string: "Too many Sync programs"
void Timer_AddSyncProg(int progFn)
{
    if (g_nThemeAsyncProgCount + 1 > 100) {
        Debug_Assert(/* line */ 0, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp",
                     "Too many Sync programs");
    }
    // g_aSyncProgWait[g_nThemeAsyncProgBase] = 0
    // g_aSyncProgFn[g_nThemeAsyncProgBase] = progFn
    g_nThemeAsyncProgBase++;
}

// ------------------------------------------------------------
//  Timer_AddSyncProgWithData  (0x0047d890)
// ------------------------------------------------------------
// Appends (progFn, data) to the sync-program queue.
// Unlike Timer_AddSyncProg, also stores extra data in g_aSyncProgData[].
//
// Original debug string: "Too many Sync programs"
void Timer_AddSyncProgWithData(int progFn, int data)
{
    if (g_nThemeAsyncProgCount + 1 > 100) {
        Debug_Assert(/* line */ 0, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp",
                     "Too many Sync programs");
    }
    // g_aSyncProgData[g_nThemeAsyncProgBase] = data
    // g_aSyncProgFn[g_nThemeAsyncProgBase] = 0   (note: cleared then set)
    // g_aSyncProgFn[g_nThemeAsyncProgBase] = progFn
    g_nThemeAsyncProgBase++;
}

// ------------------------------------------------------------
//  Timer_DispatchAsyncProg  (0x0047d990)
// ------------------------------------------------------------
// If the async queue has a ready entry (wait==0), pop and run it.
void Timer_DispatchAsyncProg(void)
{
    if (Timer_HasPendingAsyncProg()) {
        Timer_RunAsyncProg();
    }
}

// ------------------------------------------------------------
//  Timer_RunAsyncProg  (0x0047da20)
// ------------------------------------------------------------
// Pops the first ready async entry, saves the script execution context,
// invokes the entry via Script_RestoreAndCall, then restores the context.
//
// Original debug string: "void async_run()"
void Timer_RunAsyncProg(void)
{
    int data;
    // Save current DAT_0070e130 / DAT_00629f50 context
    // Script_SaveContext()
    int fn = Timer_PopAsyncProg(&data);
    // Script_RestoreAndCall(fn, data)
    // Restore context
    (void)fn;
    (void)data;
}

// ------------------------------------------------------------
//  Timer_PopAsyncProg  (0x0047db10)
// ------------------------------------------------------------
// Finds the first async queue entry with wait==0, removes it (compacting
// the queue), and returns its fn ptr + data via *pData.
//
// Scan: find first i where g_aAsyncProgWait[i] == 0
//       fn = g_nThemeAsyncProgFuncs[i]
//       *pData = g_aAsyncProgData[i]
//       decrement g_nThemeAsyncProgCount
//       shift remaining entries left
int Timer_PopAsyncProg(int* pData)
{
    // Find first ready slot (wait == 0)
    // Extract fn + data
    // Compact queue
    (void)pData;
    return 0; // placeholder
}

// ------------------------------------------------------------
//  Timer_DispatchProg  (0x0047dc60)
// ------------------------------------------------------------
// Dispatcher: checks whether a sync prog is ready first; if so, run it;
// otherwise if an async prog is ready, run it.
void Timer_DispatchProg(void)
{
    if (!Timer_HasPendingSyncProg()) {
        // no sync ready — check async
        Timer_DispatchAsyncProg();
    } else {
        Timer_RunSyncProg();
    }
}

// ------------------------------------------------------------
//  Timer_RunSyncProg  (0x0047dd00)
// ------------------------------------------------------------
// Pops the first ready sync entry, saves context, invokes it via
// Script_RestoreAndCall, then restores context.
//
// Original debug string: "void sync_run()"
void Timer_RunSyncProg(void)
{
    int data;
    // Save context
    int fn = Timer_PopSyncProg(&data);
    // Script_RestoreAndCall(fn, data)
    // Restore context
    (void)fn;
    (void)data;
}

// ------------------------------------------------------------
//  Timer_PopSyncProg  (0x0047ddf0)
// ------------------------------------------------------------
// Finds the first sync queue entry with wait==0, removes it (compacting
// the queue), and returns its fn ptr + data via *pData.
//
// Scan: find first i where g_aSyncProgWait[i] == 0
//       fn = g_aSyncProgFn[i]
//       *pData = g_aSyncProgData[i]
//       decrement g_nThemeAsyncProgBase
//       shift remaining entries left
int Timer_PopSyncProg(int* pData)
{
    // Find first ready slot (wait == 0)
    // Extract fn + data
    // Compact queue
    (void)pData;
    return 0; // placeholder
}

// ------------------------------------------------------------
//  Timer_HasPendingAsyncProg  (0x0047df40)
// ------------------------------------------------------------
// Scans the async queue for any entry with wait == 0.
// Returns 1 if found, 0 if all entries are still waiting.
int Timer_HasPendingAsyncProg(void)
{
    for (int i = 0; i < g_nThemeAsyncProgCount; i++) {
        // if (g_aAsyncProgWait[i] == 0) return 1;
    }
    return 0;
}

// ------------------------------------------------------------
//  Timer_HasPendingSyncProg  (0x0047e000)
// ------------------------------------------------------------
// Scans the sync queue for any entry with wait == 0.
// Returns 1 if found, 0 if all entries are still waiting.
int Timer_HasPendingSyncProg(void)
{
    for (int i = 0; i < g_nThemeAsyncProgBase; i++) {
        // if (g_aSyncProgWait[i] == 0) return 1;
    }
    return 0;
}

// ============================================================
//  Timer_Tick  (0x0047e0c0)
// ============================================================
// Increments all pause counters by 1:
//   - For each active timer:          g_aTimers[i].pauseCount++
//   - For each async prog queue entry: g_aAsyncProgWait[i]++
//   - For each sync  prog queue entry: g_aSyncProgWait[i]++
//
// Registered as a game-loop hook via thunk_FUN_004065e0.
// Appears in the startup CRT thunk table.
void Timer_Tick(void)
{
    for (int i = 0; i < g_nTimerCount; i++) {
        g_aTimers[i].pauseCount++;
    }
    for (int i = 0; i < g_nThemeAsyncProgCount; i++) {
        // g_aAsyncProgWait[i]++
    }
    for (int i = 0; i < g_nThemeAsyncProgBase; i++) {
        // g_aSyncProgWait[i]++
    }
}

// ============================================================
//  Timer_Untick  (0x0047e210)
// ============================================================
// Decrements all pause counters by 1, clamped at 0:
//   - For each active timer:           if > 0: g_aTimers[i].pauseCount--
//   - For each async prog queue entry: if > 0: g_aAsyncProgWait[i]--
//   - For each sync  prog queue entry: if > 0: g_aSyncProgWait[i]--
//
// Registered as a game-loop hook via thunk_FUN_004065e0.
// Appears in the startup CRT thunk table.
void Timer_Untick(void)
{
    for (int i = 0; i < g_nTimerCount; i++) {
        if (g_aTimers[i].pauseCount > 0)
            g_aTimers[i].pauseCount--;
    }
    for (int i = 0; i < g_nThemeAsyncProgCount; i++) {
        // if (g_aAsyncProgWait[i] > 0) g_aAsyncProgWait[i]--
    }
    for (int i = 0; i < g_nThemeAsyncProgBase; i++) {
        // if (g_aSyncProgWait[i] > 0) g_aSyncProgWait[i]--
    }
}

// ============================================================
//  Timer_AddAsync  (0x0047e3a0)
// ============================================================
// Adds a one-shot async timer: fires callback after 'frames' game ticks.
//
//   g_aTimers[n].callback  = callback
//   g_aTimers[n].frames    = frames
//   g_aTimers[n].pauseCount = 0
//   g_aTimers[n].flags      = TIMER_FLAG_ASYNC (0)
//   g_aTimers[n].resetVal   = 0
//
// If this is the first timer (g_nTimerCount was 0):
//   - registers Timer_TickCallback as a per-frame hook (priority 3)
//   - registers Timer_Reset as a shutdown hook
//
// Asserts "Too many timers" if TIMER_MAX (30) is exceeded.
// Original debug string: "Too many timers"
//
// This is the function called as FireTimer() from Graninv.cpp
// (handles g_nGVInitCallback, g_nGVDestroyCallback, g_nGranTriggerHandle).
void Timer_AddAsync(int frames, int callback)
{
    if (g_nTimerCount > 0x1d) {   // 0x1d = 29 → > 29 means 30+
        Debug_Assert(/* line */ 4, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp",
                     "Too many timers");
    }
    g_aTimers[g_nTimerCount].callback   = callback;
    g_aTimers[g_nTimerCount].frames     = frames;
    g_aTimers[g_nTimerCount].pauseCount = 0;
    g_aTimers[g_nTimerCount].flags      = TIMER_FLAG_ASYNC;
    g_aTimers[g_nTimerCount].resetVal   = 0;
    if (g_nTimerCount == 0) {
        GameLoop_RegisterHook(Timer_TickCallback, 0, 3);
        GameLoop_RegisterShutdown(Timer_Reset);
    }
    g_nTimerCount++;
}

// ============================================================
//  Timer_TickCallback  (0x0047e500)
// ============================================================
// Main per-frame timer engine.  Called every game frame while the timer
// table is non-empty.
//
// For each active timer:
//   1. Skip if pauseCount >= 1 (suspended).
//   2. Decrement frames.
//   3. If frames < 1 (countdown reached zero):
//      - if flags == TIMER_FLAG_ASYNC: Theme_RegisterAsyncProg(callback)
//      - if flags == TIMER_FLAG_SYNC : Timer_AddSyncProg(callback)
//      - Decrement g_nTimerCount.
//      - If table now empty: deregister Timer_TickCallback + Timer_Reset hooks.
//      - Compact table (memmove remaining entries).
//      - Back the loop index by 1 to re-check the slot.
void Timer_TickCallback(void)
{
    for (int i = 0; i < g_nTimerCount; ) {
        if (g_aTimers[i].pauseCount < 1) {
            g_aTimers[i].frames--;
            if (g_aTimers[i].frames < 1) {
                int cb    = g_aTimers[i].callback;
                int flags = g_aTimers[i].flags;

                if (flags == TIMER_FLAG_ASYNC) {
                    Theme_RegisterAsyncProg(cb);
                } else {
                    Timer_AddSyncProg(cb);
                }
                g_nTimerCount--;
                if (g_nTimerCount == 0) {
                    GameLoop_UnregisterHook(Timer_TickCallback);
                    GameLoop_UnregisterShutdown(Timer_Reset);
                    return;
                }
                // Compact: copy [i+1..g_nTimerCount] over [i..g_nTimerCount-1]
                for (int j = i; j < g_nTimerCount; j++) {
                    g_aTimers[j] = g_aTimers[j + 1];
                }
                i--;  // re-check same index (now holds next entry)
            }
        }
        i++;
    }
}

// ============================================================
//  Timer_Reset  (0x0047e6f0)
// ============================================================
// Resets all timer state to empty and deregisters the tick callback.
//   g_nTimerCount         = 0
//   g_nThemeAsyncProgBase = 0
//   g_nThemeAsyncProgCount = 0
//   GameLoop_UnregisterHook(Timer_TickCallback)
//
// Registered as a shutdown hook via thunk_FUN_00411760.
// Appears in the startup CRT thunk table.
void Timer_Reset(void)
{
    g_nTimerCount          = 0;
    g_nThemeAsyncProgBase  = 0;
    g_nThemeAsyncProgCount = 0;
    GameLoop_UnregisterHook(Timer_TickCallback);
}

// ============================================================
//  Timer_AddSync  (0x0047e7b0)
// ============================================================
// Adds a one-shot sync timer: fires callback after 'frames' game ticks
// via the sync-program queue.
//
//   g_aTimers[n].flags = TIMER_FLAG_SYNC (1)
//   All other fields identical to Timer_AddAsync.
//
// Asserts "Too many timers" if TIMER_MAX is exceeded.
// Original debug string: "Too many timers"
void Timer_AddSync(int frames, int callback)
{
    if (g_nTimerCount > 0x1d) {
        Debug_Assert(/* line */ 4, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp",
                     "Too many timers");
    }
    g_aTimers[g_nTimerCount].callback   = callback;
    g_aTimers[g_nTimerCount].frames     = frames;
    g_aTimers[g_nTimerCount].pauseCount = 0;
    g_aTimers[g_nTimerCount].flags      = TIMER_FLAG_SYNC;
    g_aTimers[g_nTimerCount].resetVal   = 0;
    if (g_nTimerCount == 0) {
        GameLoop_RegisterHook(Timer_TickCallback, 0, 3);
        GameLoop_RegisterShutdown(Timer_Reset);
    }
    g_nTimerCount++;
}

// ============================================================
//  Timer_Kill  (0x0047e910)
// ============================================================
// Searches the timer table for the entry whose callback == param_1,
// removes it, and compacts the table.
//
// If the table becomes empty, deregisters Timer_TickCallback and Timer_Reset.
void Timer_Kill(int callback)
{
    for (int i = 0; i < g_nTimerCount; i++) {
        if (g_aTimers[i].callback == callback) {
            g_nTimerCount--;
            // Compact: copy [i+1..g_nTimerCount] over [i..g_nTimerCount-1]
            for (int j = i; j < g_nTimerCount; j++) {
                g_aTimers[j] = g_aTimers[j + 1];
            }
            if (g_nTimerCount == 0) {
                GameLoop_UnregisterHook(Timer_TickCallback);
                GameLoop_UnregisterShutdown(Timer_Reset);
            }
            return;
        }
    }
}

// ============================================================
//  Timer_AddWithReset  (0x0047ea70)
// ============================================================
// Adds a self-resetting sync timer.  After each fire, Timer_ResetCounters
// restores frames from resetVal so the timer automatically re-arms.
//
//   g_aTimers[n].flags    = TIMER_FLAG_SYNC (1)
//   g_aTimers[n].resetVal = frames   (non-zero marks this as self-resetting)
//
// Asserts "Too many timers" if TIMER_MAX is exceeded.  On overflow it also
// calls Debug_DumpTable to log the current timer table state ("Timer progs").
// Original debug string: "Too many timers" / "Timer progs"
void Timer_AddWithReset(int frames, int callback)
{
    if (g_nTimerCount > 0x1d) {
        Debug_Error(/* line */ 4, "C:\\DevStudio\\Projects\\Crux\\TIMERS.cpp");
        // Debug_DumpTable(0x1d, "Timer progs", 0xffffffff) — logs the table
    }
    g_aTimers[g_nTimerCount].callback   = callback;
    g_aTimers[g_nTimerCount].frames     = frames;
    g_aTimers[g_nTimerCount].pauseCount = 0;
    g_aTimers[g_nTimerCount].flags      = TIMER_FLAG_SYNC;
    g_aTimers[g_nTimerCount].resetVal   = frames;  // stores original for reset
    if (g_nTimerCount == 0) {
        GameLoop_RegisterHook(Timer_TickCallback, 0, 3);
        GameLoop_RegisterShutdown(Timer_Reset);
    }
    g_nTimerCount++;
}

// ============================================================
//  Timer_ResetCounters  (0x0047ec10)
// ============================================================
// For all active timers where resetVal != 0 (self-resetting),
// restores frames from resetVal.  Called to re-arm periodic timers
// without removing and re-adding them.
void Timer_ResetCounters(void)
{
    for (int i = 0; i < g_nTimerCount; i++) {
        if (g_aTimers[i].resetVal != 0) {
            g_aTimers[i].frames = g_aTimers[i].resetVal;
        }
    }
}

// ============================================================
//  Timer_TriggerInit  (0x0047ecf0)
// ============================================================
// __thiscall — initialises a named trigger/timer object.
//
// Steps:
//   1. Hash pszName via Str_Hash() → store hash in this[+8]
//   2. Allocate strlen(pszName)+1 bytes → store ptr in this[+4]
//   3. Assert on alloc failure ("C:\DevStudio\Projects\Crux\Tusht..." assert file)
//   4. Copy pszName into allocated buffer
//   5. Set bit 0x200 in trigger record: DAT_005b10b0[hash * 0x58].flags |= 0x200
//   6. Zero-init fields at this[+0xac], this[+0xc4], this[+0xc0]
//   7. Set sentinel fields: this[+0xbc] = -1, this[+0xb4] = -1,
//                           this[+0xb0] = -1, this[+0xb8] = 0
//
// Returns pThis (the initialised object pointer).
int Timer_TriggerInit(int pThis, const char* pszName)
{
    // *(int*)(pThis + 8) = Str_Hash(pszName, 0xffffffff)
    // int nameLen = _strlen(pszName)
    // *(int*)(pThis + 4) = Mem_Alloc_Raw(nameLen + 1)
    // if (*(int*)(pThis + 4) == 0) Debug_Error / Debug_DumpTable
    // Mem_Copy(*(void**)(pThis + 4), pszName)
    // DAT_005b10b0[*(int*)(pThis+8) * 0x58].flags |= 0x200
    // *(int*)(pThis + 0xac) = 0
    // *(int*)(pThis + 0xc4) = 0
    // *(int*)(pThis + 0xc0) = 0
    // *(int*)(pThis + 0xbc) = -1
    // *(int*)(pThis + 0xb4) = -1
    // *(int*)(pThis + 0xb0) = -1
    // *(int*)(pThis + 0xb8) = 0
    (void)pszName;
    return pThis;
}
