#pragma once
// SCHED.cpp -- Process priority management + palette data layer
// Address range: 0x0046c120 -- 0x0046ce10

// ---------------------------------------------------------------------------
// Globals (palette data layer)
// ---------------------------------------------------------------------------
extern byte g_abActivePal[768];     // 0x007c5010  current hardware palette (256×3, 6-bit)
extern byte g_abTargetPal[768];     // 0x007d5c28  fade target palette
extern byte g_abAdjustedPal[768];   // 0x007c5360  target with border entries clamped
extern byte g_abSnapshotPal[768];   // 0x007d5f38  snapshot saved before fade-out

extern int  g_nPalGeneration;       // 0x007c56b0  incremented by SetActivePalette
extern int  g_nPalBorderMode;       // 0x007c56b8  0 = 3-entry borders, !=0 = 30-entry
extern int  g_nPalGamma;            // 0x007c56bc  gamma value (0 = off)
extern int  g_nSchedDebugMode;      // 0x00629dd0  suppress priority boost when set

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

// Priority management
void         Sched_BeginHighPriority(void);
void         Sched_EndHighPriority(void);
void         Sched_SetNormalPriority(void);
void         Sched_SetAboveNormalPriority(void);
void         Sched_Stub1(void);
void         Sched_Stub2(void);

// Palette data access
unsigned int Sched_GetPalEntryRaw(int nIdx);
void         Sched_SetGamma(int nGamma);
int          Sched_GetGamma(void);
void         Sched_SetBorderMode(int nMode);
void         Sched_ComparePalettes(int pDst, int pSrc);
void         Sched_FillPalBorders(int pPal);
void         Sched_UpdatePalette(void);
void         Sched_SetActivePalette(const void *pNewPal, int nFlags);
int          Sched_GetPalColor(int nIdx);
void         Sched_SetPalColor(int nIdx, int nColor);
void         Sched_SavePaletteSnapshot(void);
