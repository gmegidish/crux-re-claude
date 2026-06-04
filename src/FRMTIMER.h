#pragma once
// FRMTIMER.cpp — Frame-rate pacing timer
//
// A timeGetTime()-based frame limiter. A multimedia timer fires several times
// per second; on each fire FrmTimer_OnTick accumulates the real elapsed time and,
// once a frame's worth of time has passed, signals g_hFrmEvent (the main/render
// loop waits on it) and ticks the per-frame counters. Target rate is configured
// in frames-per-second by FrmTimer_Init / FrmTimer_SetFps.

#include <windows.h>

// --- State globals ---
extern DWORD  g_dwFrmTimerId;    // 0x006b8d84  Theme_SetTimer ID (0 = inactive)
extern DWORD  g_dwFrmAccumMs;    // 0x006b8d88  accumulated elapsed ms
extern int    g_nFrmFps;         // 0x006b8d8c  configured target FPS
extern DWORD  g_dwFrmLastTime;   // 0x006b8d90  timeGetTime() at previous tick
extern DWORD  g_dwFrmThreshold;  // 0x006b8d94  emit-frame threshold = 930/fps ms
extern DWORD  g_dwFrmPeriodMs;   // 0x006b8d98  true frame period   = 1000/fps ms
extern HANDLE g_hFrmEvent;       // 0x006b8d9c  auto-reset event, signalled per frame

// --- API ---
void FrmTimer_Init(int nFps);   // 0x0042a7f0  create event + start timer at nFps
void FrmTimer_Reset(int nFps);  // 0x0042a930  (re)compute period/threshold, zero accumulator
void FrmTimer_OnTick(void);     // 0x0042a9f0  multimedia-timer callback: accumulate + emit frame
void FrmTimer_SetFps(int nFps); // 0x0042aaf0  kill + restart timer at a new fps (nFps<0 keeps current)
int  FrmTimer_GetFps(void);     // 0x0042abf0  return configured fps
