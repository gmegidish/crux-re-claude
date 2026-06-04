// FRMTIMER.cpp — Frame-rate pacing timer
//
// ============================================================================
//  HOW THE FRAME LIMITER WORKS
// ============================================================================
//  The engine paces itself to a target frames-per-second (fps) using a
//  free-running multimedia timer plus a real-time accumulator:
//
//    * FrmTimer_Init creates an auto-reset event (g_hFrmEvent) and starts a
//      multimedia timer (Theme_SetTimer) whose callback is FrmTimer_OnTick.
//      The timer is scheduled at fps*3 ms — i.e. it fires roughly 3x faster
//      than the desired frame rate, so the accumulator can resolve frames with
//      better-than-one-frame granularity.
//
//    * FrmTimer_Reset precomputes two budgets from the fps:
//          g_dwFrmThreshold = 930 / fps   (emit-frame threshold, ms)
//          g_dwFrmPeriodMs  = 1000 / fps  (true frame period, ms)
//      and seeds g_dwFrmLastTime with timeGetTime(), zeroing the accumulator.
//      The threshold (930) is deliberately a touch below the true period
//      (1000) so the limiter biases toward emitting a frame slightly early
//      rather than dropping one.
//
//    * FrmTimer_OnTick (called from the multimedia timer) measures the real
//      wall-clock delta since the last tick, adds it to g_dwFrmAccumMs, and
//      when the accumulator reaches g_dwFrmThreshold:
//          - subtracts one true period (g_dwFrmPeriodMs) from the accumulator
//            (carrying the remainder forward so timing does not drift),
//          - SetEvent(g_hFrmEvent) to release the loop waiting on this frame,
//          - advances the global frame counter (Res_TickFrameCounter) and
//            the lip-sync clock (SndMem_AdvanceLipsync).
//
//    * The main / render loop blocks on g_hFrmEvent, so it runs exactly once
//      per emitted frame — that is the actual rate limit.
//
//    * FrmTimer_SetFps changes the rate at runtime: it tears down the current
//      timer, optionally adopts a new fps (a negative argument keeps the
//      current one), then re-seeds and restarts via FrmTimer_Reset.
//
//    * FrmTimer_GetFps just returns the configured fps.
//
//  Original source: C:\DevStudio\Projects\Crux\FRMTIMER.cpp

#include "FRMTIMER.h"

// --- Cross-module helpers ---
extern unsigned int Theme_SetTimer(void (*pfnCallback)(void), int nPeriodMs);  // 0x...
extern void         Theme_KillTimer(unsigned int nTimerId);
extern void         Res_TickFrameCounter(void);
extern void         SndMem_AdvanceLipsync(void);

// --- State globals (see FRMTIMER.h for addresses) ---
DWORD  g_dwFrmTimerId;
DWORD  g_dwFrmAccumMs;
int    g_nFrmFps;
DWORD  g_dwFrmLastTime;
DWORD  g_dwFrmThreshold;
DWORD  g_dwFrmPeriodMs;
HANDLE g_hFrmEvent;

// ============================================================
//  FrmTimer_Reset  (0x0042a930)
//  (Re)seed the limiter for a given fps: snapshot the clock, zero the
//  accumulator, and recompute the per-frame threshold/period budgets.
// ============================================================
void FrmTimer_Reset(int nFps)
{
    g_dwFrmLastTime  = timeGetTime();
    g_dwFrmAccumMs   = 0;
    g_dwFrmThreshold = (DWORD)(930  / (long long)nFps);   // emit-frame threshold
    g_dwFrmPeriodMs  = (DWORD)(1000 / (long long)nFps);   // true frame period
}

// ============================================================
//  FrmTimer_OnTick  (0x0042a9f0)
//  Multimedia-timer callback. Accumulate real elapsed time; when a frame's
//  worth has built up, signal the frame event and advance the per-frame clocks.
// ============================================================
void FrmTimer_OnTick(void)
{
    DWORD dwNow = timeGetTime();
    g_dwFrmAccumMs += (dwNow - g_dwFrmLastTime);
    g_dwFrmLastTime = dwNow;

    if (g_dwFrmAccumMs >= g_dwFrmThreshold)
    {
        // Carry the remainder forward so the cadence does not drift.
        g_dwFrmAccumMs -= g_dwFrmPeriodMs;

        SetEvent(g_hFrmEvent);     // release the loop waiting on this frame
        Res_TickFrameCounter();    // advance global frame counter
        SndMem_AdvanceLipsync();   // advance lip-sync clock
    }
}

// ============================================================
//  FrmTimer_SetFps  (0x0042aaf0)
//  Change the frame rate at runtime. Kills any running timer, adopts the new
//  fps (a negative argument leaves the current fps unchanged), then re-seeds
//  and restarts the timer at fps*3 ms.
// ============================================================
void FrmTimer_SetFps(int nFps)
{
    if (g_dwFrmTimerId != 0)
    {
        Theme_KillTimer(g_dwFrmTimerId);
        g_dwFrmTimerId = 0;
    }

    if (nFps >= 0)
        g_nFrmFps = nFps;

    if (g_nFrmFps != 0)
    {
        FrmTimer_Reset(nFps);
        g_dwFrmTimerId = Theme_SetTimer(FrmTimer_OnTick, g_nFrmFps * 3);
    }
}

// ============================================================
//  FrmTimer_GetFps  (0x0042abf0)
//  Return the configured target frames-per-second.
// ============================================================
int FrmTimer_GetFps(void)
{
    return g_nFrmFps;
}
