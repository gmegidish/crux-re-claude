#pragma once
// SETPAL.cpp -- Windows palette creation, fade effects, system-color management,
//               sound channel dispatcher, and UI slider subsystem
// Address range: 0x0046cf10 -- 0x00470780

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
extern byte    g_abSysPalEntries[1024]; // 0x007c4bc0  256 PALETTEENTRY (R,G,B,flags)
extern int     g_nSysColorCount;        // 0x007c4bb8  19 Windows system color slots
extern byte    g_abSavedSysColors[76];  // 0x007c4fc0  19 × COLORREF saved at init
extern byte    g_abSysColorTextRefs[76];// 0x007c5660  19 × COLORREF (black or white)
extern byte    g_abSnapshotPal[768];    // 0x007d5f38  palette snapshot (pre-fade)
extern int     g_nPalCallbackEnabled;   // 0x007c56c0  non-zero = call g_nPalCallback
extern int     g_nPalCallback;          // 0x005f3328  fn ptr: void(*)(byte *pPal)
extern HANDLE  g_hPalEvent;             // 0x007c56c4  manual-reset event for sync
extern int     g_nMainDC;              // 0x006b8f20  device context (HDC as int)
extern int     g_nFullscreen;          // 0x006b8d80  non-zero = exclusive fullscreen

// ---------------------------------------------------------------------------
// Globals (extended)
// ---------------------------------------------------------------------------
extern int  g_nSndSpeechVol;         // 0x004d9590  speech channel volume (0-63)
extern int  g_nSndSfxVol;            // 0x004d958c  SFX/music channel volume (0-63)
extern int  g_nSndPmodeFlag1;        // 0x007c5908  INI [Sound] Pmode flag 1
extern int  g_nSndPmodeFlag2;        // 0x007c5904  INI [Sound] Pmode flag 2
extern int  g_nGdiPalette;           // 0x007c56b4  current GDI HPALETTE (int)
extern int  g_nBlankWnd;             // 0x007c56c8  CRUXBlnkWndClass HWND (int)
extern int  g_nSndChannelTable;      // 0x007c5910  20-slot channel/slider table base

// ---------------------------------------------------------------------------
// Functions — palette creation / fade (original range)
// ---------------------------------------------------------------------------
void         SetPal_PreChange(void);
void         SetPal_FillLogPalette(const byte *pPal768, LOGPALETTE *pLogPal);
void         SetPal_SetCallbackEnabled(int nEnabled);
void         SetPal_FadeOut(void);
void         SetPal_QuickFadeToBlack(int nSpeedPct);
void         SetPal_SmoothFadeToBlack(DWORD dwDelayMs);
void         SetPal_FadeToTarget(void);
void         SetPal_FadeInFromBlack(void);
void         SetPal_FadeInSnapshot(void);
void         SetPal_RestoreSysColors(void);
void         SetPal_ClearSysColors(void);
void         SetPal_Init(void);
unsigned int SetPal_FindNearestColor(int nR8, int nG8, int nB8);
unsigned int SetPal_GetSysColorRef(int nIdx);
void         SetPal_SetPalette(void);

// ---------------------------------------------------------------------------
// Functions — palette continuation helpers (0x0046e6d0 – 0x0046f1f0)
// ---------------------------------------------------------------------------
void  SetPal_RestoreSysColorsRaw(void);          // 0x0046e6d0
void  SetPal_SetSyspalNostatic(void);            // 0x0046e780
void  SetPal_RestoreAndReset(void);              // 0x0046e810  (cursor/overlay restore)
void  SetPal_WaitOrRealizeIfNeeded(void);        // 0x0046e9a0
void  SetPal_RealizePalette(void);               // 0x0046ea40
void  SetPal_RemapSysColorTextRefs(const byte *pPal768); // 0x0046ebc0
void  SetPal_SetBlack(int bWaitVbl);             // 0x0046edf0
void  SetPal_DestroyBlankWindow(void);           // 0x0046f060
void  SetPal_CreateBlankWindow(void);            // 0x0046f1f0

// ---------------------------------------------------------------------------
// Functions — sound channel dispatcher (0x0046f3c0 – 0x00470520)
// ---------------------------------------------------------------------------
void  Sound_Init(void);                          // 0x0046f570  init from CRUX.INI

void  Snd_SetSpeechVol(int nVol);                // 0x0046f3c0
int   Snd_GetSpeechVol(void);                    // 0x0046f450
void  Snd_SetSfxVol(int nVol);                   // 0x0046f4e0
int   Snd_GetSfxVol(void);                       // 0x0046faf0
void  Snd_SetSfxVolClamped(int nVol);            // 0x0046fb80

// Play entry points (used by SOUNDMEM.cpp and MOVEMENT.cpp)
void  Snd_PlayCentered(int nHandle, int nChannel);               // 0x0046f7f0
void  Snd_PlayPanned(int nHandle, int nChannel, int nPan);       // 0x0046f730
void  Snd_PlayFull(int nHandle, int nChannel, int nVol,
                   int nPan, int nCallback);                      // 0x0046f890
void  Snd_PlayFull2(int nHandle, int nChannel, int nVol,
                    int nCallback);                               // 0x0046f940
void  Snd_Play(int nHandle, int nChannel, int nVol,
               int nCallback);                                    // 0x0046fed0
void  Snd_PlayCore(int *pHandle, int nChannel, int nVol,
                   int nMode, int nPan, unsigned int nLoopMask); // 0x0046ff70

// Channel control
void  Snd_Stop(int nChannel);                    // 0x0046fa60
int   Snd_IsIdle(int nChannel);                  // 0x0046fc40
void  Snd_PauseChannel(int nChannel);            // 0x00470180
void  Snd_ResumeChannel(int nChannel);           // 0x00470210
void  Snd_StopChannel(int nChannel);             // 0x00470400
void  Snd_StopAll(void);                         // 0x00470490
void  Snd_SetChannelPan(int nChannel, int nPan); // 0x00470520
int   Snd_GetActiveChanId(int nIdx);             // 0x004705c0

// Volume
void  Snd_VolumeUp(void);                        // 0x004702a0
void  Snd_VolumeDown(void);                      // 0x00470350

// Table management
void  Snd_ResetChannelTable(void);               // 0x004705e0

// Stubs
void  Snd_Nop1(void);  // 0x0046f9e0
void  Snd_Nop2(void);  // 0x0046fcd0
void  Snd_Nop3(void);  // 0x0046fd50
void  Snd_Nop4(void);  // 0x0046fdd0
void  Snd_Nop5(void);  // 0x0046fe50

// ---------------------------------------------------------------------------
// Functions — UI slider subsystem (0x00470780)
// ---------------------------------------------------------------------------
int   Slider_Add(int nAnimSlot, unsigned int nDir); // 0x00470780
