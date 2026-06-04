// SCHED.cpp -- Process priority management + palette data layer
//
// This file implements two tightly coupled responsibilities:
//
//   1. Process priority control: Crux boosts its Win32 process priority to
//      HIGH_PRIORITY_CLASS (0x100) around timing-critical sections (such as
//      the palette flip) and drops it back to NORMAL_PRIORITY_CLASS (0x80)
//      when done. In debug mode (g_nSchedDebugMode != 0) the boost is
//      suppressed so the debugger can preempt normally.
//
//   2. Palette data layer: owns the three 768-byte raw-palette buffers
//      (active, target, adjusted), the per-entry 6-bit RGB encode/decode
//      routines, the border-fill helpers, and the main UpdatePalette pump
//      that detects changes and drives the hardware palette through
//      SetPal_PreChange (in SETPAL.cpp).
//
// The palette format is 256 entries × 3 bytes; each component is 6 bits
// (range 0–63).  Entry 0 and entries near 255 are "border" entries that are
// forced to black (or a sentinel pattern) depending on g_nPalBorderMode.
//
// Original source: C:\DevStudio\Projects\Crux\SCHED.cpp
// Address range:   0x0046c120 -- 0x0046ce10

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>   // timeGetTime
#include <string.h>
#include "SCHED.h"

// Forward declarations
extern "C" {
    void Debug_Assert(int nLine, const char *pszFile, DWORD dwTime);
    void FUN_004896d0(void *pDst, const void *pSrc, int nLen);  // memcpy
}

// Forward declaration for SETPAL side
void SetPal_PreChange(void);

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Process-priority guard (read-only after startup; set by command-line parser)
int  g_nSchedDebugMode = 0;     // 0x00629dd0  non-zero = suppress priority boost

// Palette data buffers (256 entries × 3 bytes, components 0–63)
byte g_abActivePal[768];        // 0x007c5010  current hardware palette
byte g_abTargetPal[768];        // 0x007d5c28  palette to transition towards
byte g_abAdjustedPal[768];      // 0x007c5360  target with borders clamped

// Palette state
int  g_nPalGeneration = 0;      // 0x007c56b0  incremented by SetActivePalette
int  g_nPalBorderMode = 0;      // 0x007c56b8  0 = 3-entry borders, !=0 = 30-entry
int  g_nPalGamma      = 0;      // 0x007c56bc  gamma correction value (0 = off)

// ---------------------------------------------------------------------------
// Priority management
// ---------------------------------------------------------------------------

// Sched_BeginHighPriority -- Boost to HIGH_PRIORITY_CLASS for timing-critical work.
// No-op in debug mode.
// 0x0046c120
void Sched_BeginHighPriority(void)
{
    if (g_nSchedDebugMode == 0)
    {
        HANDLE hProcess = GetCurrentProcess();
        SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);    // 0x100
    }
}

// Sched_EndHighPriority -- Drop back to NORMAL_PRIORITY_CLASS after high-priority work.
// Emits a timeGetTime assertion timestamp so the debug log shows the duration.
// No-op in debug mode.
// 0x0046c1c0
void Sched_EndHighPriority(void)
{
    if (g_nSchedDebugMode == 0)
    {
        DWORD dwTime = timeGetTime();
        // Line number stored at DAT_004d8a60+4; file path = "C:\DevStudio\Projects\Crux\SCHED.cpp"
        Debug_Assert(*(int *)(0x004d8a60 + 4),
                     "C:\\DevStudio\\Projects\\Crux\\SCHED.cpp",
                     dwTime);

        HANDLE hProcess = GetCurrentProcess();
        SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS); // 0x80
    }
}

// Sched_SetNormalPriority -- Unconditionally set NORMAL_PRIORITY_CLASS (0x20).
// Used at startup/shutdown to ensure the process does not hold an elevated class.
// 0x0046c290
void Sched_SetNormalPriority(void)
{
    HANDLE hProcess = GetCurrentProcess();
    SetPriorityClass(hProcess, 0x20);   // NORMAL_PRIORITY_CLASS alternate value
}

// Sched_SetAboveNormalPriority -- Set HIGH_PRIORITY_CLASS (0x40) if not in debug mode.
// 0x0046c320
void Sched_SetAboveNormalPriority(void)
{
    if (g_nSchedDebugMode == 0)
    {
        HANDLE hProcess = GetCurrentProcess();
        SetPriorityClass(hProcess, 0x40);
    }
}

// Sched_Stub1 / Sched_Stub2 -- Empty functions; SEH frame only.
// These appear to be placeholder stubs compiled into the CRT thunk table.
// 0x0046c3c0
void Sched_Stub1(void) {}

// 0x0046c440
void Sched_Stub2(void) {}

// ---------------------------------------------------------------------------
// Palette data layer
// ---------------------------------------------------------------------------

// Sched_GetPalEntryRaw -- Return the raw 6-bit RGB triple for palette entry nIdx,
// packed as (R<<2 | G<<10 | B<<18).
// 0x0046c4c0
unsigned int Sched_GetPalEntryRaw(int nIdx)
{
    byte bR = g_abActivePal[nIdx * 3 + 0];
    byte bG = g_abActivePal[nIdx * 3 + 1];
    byte bB = g_abActivePal[nIdx * 3 + 2];
    return ((bR & 0x3f) << 2) | ((bG & 0x3f) << 10) | ((bB & 0x3f) << 18);
}

// Sched_SetGamma -- Set the gamma correction value applied in SetPal_ApplyGamma.
// 0 = gamma off (linear).
// 0x0046c5a0
void Sched_SetGamma(int nGamma)
{
    g_nPalGamma = nGamma;
}

// Sched_GetGamma -- Return the current gamma correction value.
// 0x0046c630
int Sched_GetGamma(void)
{
    return g_nPalGamma;
}

// Sched_SetBorderMode -- Select narrow (0) or wide (!0) border strip mode.
// Narrow:  3 entries at each end of g_abActivePal (entries 0–2 and 253–255)
//          are treated as border (forced black / sentinel).
// Wide:    30 entries at each end (entries 0–29 and 226–255).
// 0x0046c6c0
void Sched_SetBorderMode(int nMode)
{
    g_nPalBorderMode = nMode;
}

// Sched_ComparePalettes -- Copy non-border region of pSrc into pDst using memcpy.
// The copy skips the border strip (3 or 30 entries) at each end.
// Used to snapshot the palette without touching the border entries.
// 0x0046c750
void Sched_ComparePalettes(int pDst, int pSrc)
{
    if (g_nPalBorderMode == 0)
    {
        // Narrow borders: skip 3-entry (9-byte) strip at each end;
        // copy 250 middle entries = 750 bytes
        memcmp((void *)(pDst + 3),  (void *)(pSrc + 3),  0x2fa);
    }
    else
    {
        // Wide borders: skip 30-entry (0x1e = 90-byte) strip at each end;
        // copy 196 middle entries = 0x2c4 bytes
        memcmp((void *)(pDst + 0x1e), (void *)(pSrc + 0x1e), 0x2c4);
    }
}

// Sched_FillPalBorders -- Write border sentinel values into pPal.
// Entry 0 bytes: filled from DAT_004d8bd0 (black = 0,0,0).
// Last entry:    filled from s__><(()_ (sentinel marker bytes).
// Width of border strip = 3 (narrow) or 30 (wide) entries.
// 0x0046c820
void Sched_FillPalBorders(int pPal)
{
    static const byte *k_pBlack    = (const byte *)0x004d8bd0; // 0,0,0
    static const byte *k_pSentinel = (const byte *)0x004d8bf0; // border sentinel

    if (g_nPalBorderMode == 0)
    {
        // Narrow: 3 entries = 9 bytes at head; 3 entries at tail (offset 0x2fd = 765)
        FUN_004896d0((void *)pPal,           k_pBlack,    3);
        FUN_004896d0((void *)(pPal + 0x2fd), k_pSentinel, 3);
    }
    else
    {
        // Wide: 30 entries = 90 bytes at head (0x1e); 30 at tail (offset 0x2e2 = 738)
        FUN_004896d0((void *)pPal,           k_pBlack,    0x1e);
        FUN_004896d0((void *)(pPal + 0x2e2), k_pSentinel, 0x1e);
    }
}

// Sched_UpdatePalette -- Main palette pump.
// Compares g_abTargetPal against the current hardware palette (g_abActivePal).
// For entries that differ: copies them from g_abTargetPal into g_abAdjustedPal.
// For entries that match:  zeros the g_abAdjustedPal entry (skip).
// Then forces border strips in g_abAdjustedPal and calls SetPal_PreChange
// to push the adjusted palette to the hardware.
//
// Called from thunks at 0x0046c920 (with one int parameter — appears to be
// a "force" flag, although the parameter is not used in the decompile body
// beyond stack setup; the thunk always passes 1).
// 0x0046c920
void Sched_UpdatePalette(void)
{
    int nStart = (g_nPalBorderMode != 0) ? 10  : 0;
    int nEnd   = (g_nPalBorderMode != 0) ? 246 : 256;

    if (memcmp(g_abTargetPal, g_abActivePal, 0x300) != 0)
    {
        for (int i = nStart; i < nEnd; i++)
        {
            if (g_abTargetPal[i*3+0] == g_abActivePal[i*3+0] &&
                g_abTargetPal[i*3+1] == g_abActivePal[i*3+1] &&
                g_abTargetPal[i*3+2] == g_abActivePal[i*3+2])
            {
                // No change — pass through active entry
                FUN_004896d0(&g_abAdjustedPal[i*3], &g_abActivePal[i*3], 3);
            }
            else
            {
                // Entry changed — zero adjusted entry (marks as dirty)
                memset(&g_abAdjustedPal[i*3], 0, 3);
            }
        }

        if (g_nPalBorderMode != 0)
        {
            // Wide border: force black at head, sentinel at tail
            FUN_004896d0(g_abAdjustedPal,           (void *)0x004d8bd0, 0x1e);
            FUN_004896d0(g_abAdjustedPal + 0x1e * 0x22 /*=642*/,
                         (void *)0x004d8bf0,          0x1e);
        }

        SetPal_PreChange(g_abTargetPal, /*nFlags=*/0);
    }
}

// Sched_SetActivePalette -- Install pNewPal as the active palette.
// Copies 768 bytes into g_abActivePal, applies wide-border mask if needed,
// increments g_nPalGeneration so callers can detect palette staleness.
// 0x0046cb30
void Sched_SetActivePalette(const void *pNewPal, int nFlags)
{
    FUN_004896d0(g_abActivePal, pNewPal, 0x300);

    if (g_nPalBorderMode != 0)
    {
        FUN_004896d0(g_abActivePal,           (void *)0x004d8bd0, 0x1e);
        FUN_004896d0(g_abActivePal + 0x2f2,   (void *)0x004d8bf0, 0x1e);
    }

    g_nPalGeneration = nFlags + 1;
}

// Sched_GetPalColor -- Return palette entry nIdx as a packed BGR DWORD
// (low byte = R, mid = G, high = B).
// 0x0046cc20
int Sched_GetPalColor(int nIdx)
{
    byte bR = g_abActivePal[nIdx * 3 + 0];
    byte bG = g_abActivePal[nIdx * 3 + 1];
    byte bB = g_abActivePal[nIdx * 3 + 2];
    return (unsigned int)bR + (unsigned int)bG * 0x100 + (unsigned int)bB * 0x10000;
}

// Sched_SetPalColor -- Write palette entry nIdx from a packed BGR value.
// Stores each component after sign-extending and masking to get the magnitude,
// effectively: store abs(component_byte).
// 0x0046ccf0
void Sched_SetPalColor(int nIdx, int nColor)
{
    // Extract R component (low byte of nColor), store magnitude
    byte bSign = (byte)(nColor >> 31);
    g_abActivePal[nIdx * 3 + 0] = ((((byte)nColor ^ bSign) - bSign) ^ bSign) - bSign;

    // Extract G component (byte 1)
    int nShifted = nColor + (nColor >> 31 & 0xff);
    bSign = (byte)(nShifted >> 31);
    g_abActivePal[nIdx * 3 + 1] = ((((byte)(nShifted >> 8) ^ bSign) - bSign) ^ bSign) - bSign;

    // Extract B component (byte 2)
    int nHigh = (nShifted >> 8) + (nShifted >> 31 & 0xff);
    bSign = (byte)(nHigh >> 31);
    g_abActivePal[nIdx * 3 + 2] = ((((byte)(nHigh >> 8) ^ bSign) - bSign) ^ bSign) - bSign;
}

// Sched_SavePaletteSnapshot -- Copy g_abActivePal into g_abSnapshotPal (0x007d5f38).
// Called before a fade so the fade functions know the pre-fade palette.
// 0x0046ce10
void Sched_SavePaletteSnapshot(void)
{
    extern byte g_abSnapshotPal[768];   // 0x007d5f38
    for (int i = 0; i < 256; i++)
    {
        g_abSnapshotPal[i * 3 + 0] = g_abActivePal[i * 3 + 0];
        g_abSnapshotPal[i * 3 + 1] = g_abActivePal[i * 3 + 1];
        g_abSnapshotPal[i * 3 + 2] = g_abActivePal[i * 3 + 2];
    }
}
