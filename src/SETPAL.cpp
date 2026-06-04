// SETPAL.cpp -- Windows palette creation, fade effects, and system-color management
//
// This module bridges the game's internal 6-bit 256-colour palette (owned by
// SCHED.cpp) and the Win95 GDI/DirectDraw palette objects.
//
// Responsibilities:
//   - SetPal_Init:           one-time init; snapshots system palette entries and
//                            system UI colours; creates a manual-reset Win32 event
//                            (g_hPalEvent) used to synchronise palette-set callbacks.
//   - SetPal_PreChange /
//     SetPal_FillLogPalette: convert the internal 6-bit palette to a LOGPALETTE
//                            (8-bit, gamma-corrected) and call CreatePalette /
//                            RealizePalette to push it to GDI.
//   - SetPal_ApplyGamma:     single-component gamma curve via floating-point.
//   - SetPal_SetPalette:     high-level "apply palette now" — signals g_hPalEvent,
//                            copies system PALETTEENTRY data into g_abTargetPal,
//                            calls SetPal_PreChange + thunk_FUN_0041d8f0 + flush.
//   - Fade functions:        FadeOut, QuickFadeToBlack, SmoothFadeToBlack,
//                            FadeToTarget, FadeInFromBlack, FadeInSnapshot —
//                            all step through 64 brightness levels (0–63) with a
//                            20 ms Sleep between frames and an optional per-frame
//                            callback (g_nPalCallback).
//   - SetPal_SetCallbackEnabled: enables/disables per-step callback invocation.
//   - SetSysColor helpers:   RestoreSysColors / ClearSysColors save and restore
//                            the 19 Windows system UI colours (e.g. button face,
//                            caption bar) that the game must blacken while it has
//                            exclusive DirectDraw focus.
//   - SetPal_FindNearestColor: squared-distance nearest-colour search (palette
//                            indices 0–9 and 246–255 reserved/skipped).
//   - SetPal_GetSysColorRef: returns the COLORREF for a given system palette index.
//
// Original source: C:\DevStudio\Projects\Crux\SETPAL.cpp
// Address range:   0x0046cf10 -- 0x00470780

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "SETPAL.h"
#include "SCHED.h"

// Forward declarations for cross-module helpers
extern "C" {
    void Debug_Assert(int nLine, const char *pszFile, DWORD dwTime);
    void FUN_004896d0(void *pDst, const void *pSrc, int nLen);  // memcpy
    void FUN_0048b5c0(double x);    // fpu push
    int  __ftol(void);              // float-to-long
}

// thunk entry points referenced by SETPAL
extern void thunk_FUN_0046c920(int);  // Sched_UpdatePalette (with force flag)
extern void thunk_FUN_0046ce10(void); // Sched_SavePaletteSnapshot
extern void thunk_FUN_0046c290(void); // Sched_SetNormalPriority
extern void thunk_FUN_0046c1c0(void); // Sched_EndHighPriority
extern void thunk_FUN_0046e9a0(void); // SetPal_WaitOrRealizeIfNeeded (message-pump tick / palette gate)
extern void thunk_FUN_0041d8f0(void); // (GI / blit flush)
extern void thunk_FUN_0046ea40(void); // SetPal_RealizePalette (realize palette on vsync)
extern void thunk_FUN_0046e810(void); // SetPal_RestoreAndReset (restore sys colors + SYSPAL_NOSTATIC)

// thunk entry points resolved in this extension
extern void thunk_FUN_0046f730(int, int, int); // Snd_PlayPanned
extern void thunk_FUN_0046f890(int, int, int, int, int); // Snd_PlayFull
extern int  thunk_FUN_004861c0(void);           // DDI check-for-fullscreen/active
extern int  thunk_FUN_00472340(int, char*, int, int, char*); // SndMem_Load variant
extern void thunk_FUN_004427e0(int, int*, int, int, int, int, int, int); // Mixer audio start
extern void thunk_FUN_00443b80(int);            // Mixer_SelectFillFunc(1)
extern void thunk_FUN_00443df0(int);            // Mixer_StopChannel
extern void thunk_FUN_00443d50(int);            // Mixer_IsChannelIdle
extern void thunk_FUN_00443fe0(int);            // Mixer_StopChannel (alt)
extern void thunk_FUN_00443ee0(void);           // Mixer_StopAll
extern void thunk_FUN_00417df0(const char*, int); // Debug_Printf / trace helper

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// System palette captured at init
byte    g_abSysPalEntries[1024];    // 0x007c4bc0  256 PALETTEENTRY = 256×4 bytes
int     g_nSysColorCount    = 0;    // 0x007c4bb8  number of system colors (set to 19)
byte    g_abSavedSysColors[76];     // 0x007c4fc0  19 saved COLORREF values
byte    g_abSysColorTextRefs[76];   // 0x007c5660  19 replacement text COLORREFs

// Snapshot palette buffer (filled by Sched_SavePaletteSnapshot)
byte    g_abSnapshotPal[768];       // 0x007d5f38

// Runtime state
int     g_nPalCallbackEnabled = 0;  // 0x007c56c0  non-zero = invoke g_nPalCallback
int     g_nPalCallback        = 0;  // 0x005f3328  fn pointer: void(*)(byte *pPal)
HANDLE  g_hPalEvent           = NULL; // 0x007c56c4 manual-reset event from SetPal_Init

// Shared with GI.cpp
int     g_nMainDC             = 0;  // 0x006b8f20  main device context (HDC)
int     g_nFullscreen         = 0;  // 0x006b8d80  non-zero when in exclusive fullscreen

// Globals used by palette generation tracker (shared with GI.cpp)
// g_nPalGeneration is defined in GI.cpp (0x007c56b0); see GI.h for extern
// g_nGdiPalette   (HPALETTE stored as int)     0x007c56b4
// g_nBlankWnd     (HWND stored as int)         0x007c56c8

// Sound channel volume globals
int     g_nSndSpeechVol       = 0;  // 0x004d9590  speech channel volume (0-63)
int     g_nSndSfxVol          = 0;  // 0x004d958c  SFX/music channel volume (0-63)

// Sound system mode flags (set by Sound_Init from CRUX.INI [Sound] Pmode)
int     g_nSndPmodeFlag1      = 0;  // 0x007c5908
int     g_nSndPmodeFlag2      = 0;  // 0x007c5904

// Sound channel table: 20 slots × 0x30 bytes, used by Snd_ResetChannelTable / Slider_Add
// Base at 0x007c5910; the active-channel ID table (Mixer) is at 0x006dc3d8

// INI file path (global C string), used by Sound_Init
// g_abIniPath at 0x006299c0

// Static data: black entry and border sentinel (in .rdata)
static const byte *k_pBlack    = (const byte *)0x004d8bd0;  // {0,0,0}
static const byte *k_pSentinel = (const byte *)0x004d8bf0;  // {' ','>','<',...}
// System color index table (19 indices, in .rdata)
static const INT  *k_aSysColorIdx = (const INT *)0x004d8dd0;

// Double constant used by gamma: 1.0 / 64.0 stored at 0x004aa0e0
static const double *k_p1over64 = (const double *)0x004aa0e0;

// ---------------------------------------------------------------------------
// SetPal_ApplyGamma -- Apply gamma correction to a single 6-bit palette component.
// Returns the adjusted component (range 0–63).
// Formula: result = round( -comp * gamma / 64.0 - (-gamma) ) = comp attenuated.
// 0x0046d200
// ---------------------------------------------------------------------------
static int SetPal_ApplyGamma(int nComp)
{
    if (g_nPalGamma != 0 && nComp != 0)
    {
        // Compute: -(nComp * g_nPalGamma) / 64.0
        FUN_0048b5c0(-(double)nComp * (double)g_nPalGamma * (*k_p1over64));
        FUN_0048b5c0((double)(-g_nPalGamma));
        nComp = __ftol();
    }
    return nComp;
}

// ---------------------------------------------------------------------------
// SetPal_FillLogPalette -- Convert 6-bit palette at pPal768 into a Win32 LOGPALETTE.
// pLogPal must point to a buffer of at least sizeof(LOGPALETTE)+255*sizeof(PALETTEENTRY).
// Each 6-bit component is shifted left by 2 (→ 8-bit) after gamma correction.
// Entries 0 and 255 are forced to pure black and pure white respectively.
// 0x0046d050
// ---------------------------------------------------------------------------
void SetPal_FillLogPalette(const byte *pPal768, LOGPALETTE *pLogPal)
{
    pLogPal->palVersion    = 0x300;
    pLogPal->palNumEntries = 256;

    for (int i = 0; i < 256; i++)
    {
        char bR = (char)(SetPal_ApplyGamma(pPal768[i*3+0]) << 2);
        char bG = (char)(SetPal_ApplyGamma(pPal768[i*3+1]) << 2);
        char bB = (char)(SetPal_ApplyGamma(pPal768[i*3+2]) << 2);

        pLogPal->palPalEntry[i].peRed   = bR;
        pLogPal->palPalEntry[i].peGreen = bG;
        pLogPal->palPalEntry[i].peBlue  = bB;
        pLogPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE; // 1
    }

    // Force entry 255 = white, entry 0 = black
    pLogPal->palPalEntry[255].peRed   = 0xff;
    pLogPal->palPalEntry[255].peGreen = 0xff;
    pLogPal->palPalEntry[255].peBlue  = 0xff;
    pLogPal->palPalEntry[255].peFlags = 0;

    pLogPal->palPalEntry[0].peRed   = 0;
    pLogPal->palPalEntry[0].peGreen = 0;
    pLogPal->palPalEntry[0].peBlue  = 0;
    pLogPal->palPalEntry[0].peFlags = 0;
}

// ---------------------------------------------------------------------------
// SetPal_PreChange -- Build a LOGPALETTE from g_abAdjustedPal (0x007c5360),
// call CreatePalette / SelectPalette / RealizePalette to push it to GDI.
// Debug string: "pal_pre_change"
// 0x0046cf10
// ---------------------------------------------------------------------------
void SetPal_PreChange(void)
{
    // Stack-allocated LOGPALETTE for 256 entries (128 LOGPALETTE "pairs" in Ghidra view)
    byte logPalBuf[sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pLogPal = (LOGPALETTE *)logPalBuf;

    memset(logPalBuf, 0, sizeof(logPalBuf));

    SetPal_FillLogPalette(g_abAdjustedPal, pLogPal);

    HPALETTE hPal = CreatePalette(pLogPal);
    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2
    SelectPalette((HDC)g_nMainDC, hPal, FALSE);
    RealizePalette((HDC)g_nMainDC);
}

// ---------------------------------------------------------------------------
// SetPal_SetCallbackEnabled -- Enable or disable the per-frame palette callback.
// When enabled and g_nPalCallback != NULL, the fade functions invoke
//   ((void(*)(byte*))g_nPalCallback)(pPalBuf) after each step.
// 0x0046d310
// ---------------------------------------------------------------------------
void SetPal_SetCallbackEnabled(int nEnabled)
{
    g_nPalCallbackEnabled = nEnabled;
}

// ---------------------------------------------------------------------------
// SetPal_FadeOut -- Fade the snapshot palette (g_abSnapshotPal) out to black.
// 64 steps, each subtracting 2 from every component then clamping at 0.
// 20 ms Sleep between steps (set by thunk before loop — see SetPal_SmoothFadeToBlack).
// 0x0046d3a0
// ---------------------------------------------------------------------------
void SetPal_FadeOut(void)
{
    thunk_FUN_0046ce10();   // Sched_SavePaletteSnapshot

    for (int nStep = 0; nStep < 64; nStep += 2)
    {
        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                g_abTargetPal[i*3+c] = g_abSnapshotPal[i*3+c] - (byte)nStep;
                if ((byte)g_abTargetPal[i*3+c] > 0x3f)
                    g_abTargetPal[i*3+c] = 0;
            }
        }

        if (g_nPalCallbackEnabled && g_nPalCallback != 0)
        {
            typedef void (*PalCB)(byte *);
            ((PalCB)g_nPalCallback)(g_abTargetPal);
        }

        thunk_FUN_0046c920(1);  // Sched_UpdatePalette(force)
        Sleep(20);
    }
}

// ---------------------------------------------------------------------------
// SetPal_QuickFadeToBlack -- Fade g_abActivePal to black at a caller-specified
// percentage speed (param_1 out of 100).  Stops early if all entries are zero.
// 0x0046d540
// ---------------------------------------------------------------------------
void SetPal_QuickFadeToBlack(int nSpeedPct)
{
    int nSteps = (nSpeedPct << 6) / 100;   // scale 0–100% to 0–64 steps

    for (int nStep = 0; nStep < nSteps; nStep++)
    {
        int nZeroCount = 0;

        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                g_abActivePal[i*3+c]--;
                if ((byte)g_abActivePal[i*3+c] > 0x3f || g_abActivePal[i*3+c] == 0)
                {
                    g_abActivePal[i*3+c] = 0;
                    nZeroCount++;
                }
            }
        }

        if (g_nPalCallbackEnabled && g_nPalCallback != 0)
        {
            typedef void (*PalCB)(byte *);
            ((PalCB)g_nPalCallback)(g_abActivePal);
        }

        thunk_FUN_0046e9a0();
        Sleep(20);

        if (nZeroCount > 0x2ff)     // all 768 bytes are zero
            break;
    }
}

// ---------------------------------------------------------------------------
// SetPal_SmoothFadeToBlack -- Smooth 64-step fade to black with configurable delay.
// Debug string: "pal_smooth_fade_toblack__int_speed"
// Takes a snapshot of g_abActivePal at the start, then each step computes
//   pal[i] = snapshot[i] - step  (clamped to 0).
// param_1 = Sleep delay in ms per step (0 = no Sleep).
// 0x0046d720
// ---------------------------------------------------------------------------
void SetPal_SmoothFadeToBlack(DWORD dwDelayMs)
{
    byte abStartPal[768];
    FUN_004896d0(abStartPal, g_abActivePal, 0x300);

    for (int nStep = 0; nStep < 64; nStep++)
    {
        int nZeroCount = 0;

        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                g_abActivePal[i*3+c] = abStartPal[i*3+c] - (byte)nStep;
                if ((byte)g_abActivePal[i*3+c] > 0x3f)
                {
                    g_abActivePal[i*3+c] = 0;
                    nZeroCount++;
                }
            }
        }

        if ((int)dwDelayMs > 0)
            Sleep(dwDelayMs);

        if (g_nPalCallbackEnabled && g_nPalCallback != 0)
        {
            typedef void (*PalCB)(byte *);
            ((PalCB)g_nPalCallback)(g_abActivePal);
        }

        thunk_FUN_0046e9a0();

        if (nZeroCount > 0x2ff)
            break;
    }
}

// ---------------------------------------------------------------------------
// SetPal_FadeToTarget -- Fade g_abActivePal down towards g_abTargetPal.
// Each of 64 steps subtracts (step) from each component; if the result would
// go below the target, the target value is used instead (no overshoot).
// Stops early when all entries have reached their target.
// 0x0046d900
// ---------------------------------------------------------------------------
void SetPal_FadeToTarget(void)
{
    for (int nStep = 64; nStep > 0; nStep--)
    {
        int nAtTarget  = 0;
        int nDoneCount = 0;

        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                byte bNew;
                if ((int)((unsigned int)(byte)g_abTargetPal[i*3+c] - nStep) < 0)
                {
                    bNew = 0;
                }
                else
                {
                    bNew = g_abTargetPal[i*3+c] - (byte)nStep;
                }

                g_abActivePal[i*3+c] = bNew;

                // Clamp to target if we overshot
                if ((byte)g_abTargetPal[i*3+c] <= (byte)g_abActivePal[i*3+c])
                {
                    g_abActivePal[i*3+c] = g_abTargetPal[i*3+c];
                    nDoneCount++;
                }

                if (g_abActivePal[i*3+c] == 0)
                    nAtTarget++;
            }
        }

        if (nAtTarget < 0x300)
        {
            if (g_nPalCallbackEnabled && g_nPalCallback != 0)
            {
                typedef void (*PalCB)(byte *);
                ((PalCB)g_nPalCallback)(g_abActivePal);
            }
            thunk_FUN_0046e9a0();
        }

        if (nDoneCount > 0x2ff)
            break;
    }
}

// ---------------------------------------------------------------------------
// SetPal_FadeInFromBlack -- Fade g_abActivePal up from black towards g_abTargetPal.
// Each of 64 steps sets each component to (step); if the target is below
// step the target value is used (no overshoot).
// Stops early when all entries have reached their target.
// 0x0046db50
// ---------------------------------------------------------------------------
void SetPal_FadeInFromBlack(void)
{
    for (int nStep = 0; nStep < 64; nStep++)
    {
        int nDoneCount = 0;

        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                g_abActivePal[i*3+c] = (byte)nStep;

                if ((byte)g_abTargetPal[i*3+c] < (byte)g_abActivePal[i*3+c])
                {
                    nDoneCount++;
                    g_abActivePal[i*3+c] = g_abTargetPal[i*3+c];
                }
            }
        }

        if (g_nPalCallbackEnabled && g_nPalCallback != 0)
        {
            typedef void (*PalCB)(byte *);
            ((PalCB)g_nPalCallback)(g_abActivePal);
        }

        thunk_FUN_0046e9a0();
        Sleep(20);

        if (nDoneCount > 0x2ff)
            break;
    }
}

// ---------------------------------------------------------------------------
// SetPal_FadeInSnapshot -- Fade g_abTargetPal up from black towards g_abSnapshotPal.
// Each of 32 steps (step += 2) sets each component to min(step, snapshotVal).
// Calls Sched_UpdatePalette(1) each step (via thunk) instead of the direct
// callback path — pushes through the full adjusted-palette pipeline.
// 0x0046dd10
// ---------------------------------------------------------------------------
void SetPal_FadeInSnapshot(void)
{
    for (int nStep = 0; nStep < 64; nStep += 2)
    {
        for (int i = 0; i < 256; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                g_abTargetPal[i*3+c] = (byte)nStep;
                if ((byte)g_abSnapshotPal[i*3+c] < (byte)g_abTargetPal[i*3+c])
                    g_abTargetPal[i*3+c] = g_abSnapshotPal[i*3+c];
            }
        }

        if (g_nPalCallbackEnabled && g_nPalCallback != 0)
        {
            typedef void (*PalCB)(byte *);
            ((PalCB)g_nPalCallback)(g_abTargetPal);
        }

        thunk_FUN_0046c920(1);  // Sched_UpdatePalette(force)
        Sleep(20);
    }
}

// ---------------------------------------------------------------------------
// SetPal_RestoreSysColors -- Re-apply the saved Windows system colors
// (captured in SetPal_Init) so the desktop looks correct after the game
// exits fullscreen or transitions to a window.
// Skipped in fullscreen mode (g_nFullscreen != 0).
// 0x0046deb0
// ---------------------------------------------------------------------------
void SetPal_RestoreSysColors(void)
{
    if (g_nFullscreen == 0)
    {
        thunk_FUN_0046c290();   // Sched_SetNormalPriority

        SetSysColors(g_nSysColorCount,
                     k_aSysColorIdx,
                     (COLORREF *)g_abSavedSysColors);

        thunk_FUN_0046c1c0();   // Sched_EndHighPriority
    }
}

// ---------------------------------------------------------------------------
// SetPal_ClearSysColors -- Zero-out all Windows system colors (blacken the desktop
// chrome) so they don't bleed through DirectDraw's exclusive surface.
// Debug string: "syspal_clear"
// 0x0046df60
// ---------------------------------------------------------------------------
void SetPal_ClearSysColors(void)
{
    COLORREF aBlack[20];
    for (int i = 0; i < 20; i++)
        aBlack[i] = 0;

    if (g_nFullscreen == 0)
    {
        thunk_FUN_0046c290();   // Sched_SetNormalPriority

        SetSysColors(g_nSysColorCount,
                     k_aSysColorIdx,
                     aBlack);

        thunk_FUN_0046c1c0();   // Sched_EndHighPriority
    }
}

// ---------------------------------------------------------------------------
// SetPal_Init -- One-time palette system initialisation.
// - Captures 256 PALETTEENTRY values from the system palette.
// - Enumerates 19 Windows system UI colors, saves them and computes
//   contrast replacement text colors (black or white based on luminance).
// - Sets SYSPAL_NOSTATIC so game palette entries aren't reserved by the OS.
// - Creates a manual-reset Win32 event (g_hPalEvent) for palette-set sync.
// 0x0046e040
// ---------------------------------------------------------------------------
void SetPal_Init(void)
{
    GetSystemPaletteEntries((HDC)g_nMainDC, 0, 256, (LPPALETTEENTRY)g_abSysPalEntries);

    g_nSysColorCount = 19;  // 0x13

    for (int i = 0; i < g_nSysColorCount; i++)
    {
        DWORD dwColor = GetSysColor(k_aSysColorIdx[i]);
        *(DWORD *)(g_abSavedSysColors + i * 4) = dwColor;

        DWORD dwR = (dwColor & 0xff);
        DWORD dwG = (dwColor & 0xff00) >> 8;
        DWORD dwB = (dwColor & 0xff0000) >> 16;

        // If luminance < sqrt(0x18001) ≈ 98 then use black text, else white
        if (dwR*dwR + dwG*dwG + dwB*dwB < 0x18001)
            *(DWORD *)(g_abSysColorTextRefs + i * 4) = 0x000000;
        else
            *(DWORD *)(g_abSysColorTextRefs + i * 4) = 0xffffff;
    }

    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2

    g_hPalEvent = CreateEventA(NULL, TRUE, FALSE, NULL);    // manual-reset, non-signalled
}

// ---------------------------------------------------------------------------
// SetPal_FindNearestColor -- Find the palette index in g_abActivePal closest
// (squared Euclidean distance in 6-bit RGB space) to the given 8-bit RGB triple.
// Palette indices 0–9 and 246–255 are reserved (skipped).
// Returns the COLORREF stored in g_abSysPalEntries for the matched index.
// 0x0046e210
// ---------------------------------------------------------------------------
unsigned int SetPal_FindNearestColor(int nR8, int nG8, int nB8)
{
    int nBestDist = 0x7fffffff;
    int nBestIdx  = 0;

    for (int i = 0; i < 256; i++)
    {
        if (i == 10)
            i = 246;    // skip reserved range

        int dR = (int)(byte)g_abActivePal[i*3+0] - (nR8 >> 2);
        int dG = (int)(byte)g_abActivePal[i*3+1] - (nG8 >> 2);
        int dB = (int)(byte)g_abActivePal[i*3+2] - (nB8 >> 2);
        int nDist = dR*dR + dG*dG + dB*dB;

        if (nDist < nBestDist)
        {
            nBestIdx  = i;
            nBestDist = nDist;
        }
    }

    // Return the COLORREF from the system palette entry for that index
    byte bR = g_abSysPalEntries[nBestIdx * 4 + 0];
    byte bG = g_abSysPalEntries[nBestIdx * 4 + 1];
    byte bB = g_abSysPalEntries[nBestIdx * 4 + 2];
    return (unsigned int)(bR | (bG << 8) | (bB << 16));
}

// ---------------------------------------------------------------------------
// SetPal_GetSysColorRef -- Return the COLORREF from g_abSysPalEntries for index nIdx.
// 0x0046e3d0
// ---------------------------------------------------------------------------
unsigned int SetPal_GetSysColorRef(int nIdx)
{
    byte bR = g_abSysPalEntries[nIdx * 4 + 0];
    byte bG = g_abSysPalEntries[nIdx * 4 + 1];
    byte bB = g_abSysPalEntries[nIdx * 4 + 2];
    return (unsigned int)(bR | (bG << 8) | (bB << 16));
}

// ---------------------------------------------------------------------------
// SetPal_SetPalette -- High-level palette install called when a scene or
// inventory cutscene establishes a new palette.
// Sequence (all steps timed by Debug_Assert with timeGetTime):
//   1. Signal g_hPalEvent (wake any waiter).
//   2. Copy system PALETTEENTRY data into g_abTargetPal.
//   3. Sched_UpdatePalette(force=1) to push through the adjusted-pal pipeline.
//   4. SetPal_PreChange to rebuild the GDI LOGPALETTE.
//   5. thunk_FUN_0041d8f0 (GI flush / blit).
//   6. thunk_FUN_0046ea40 (page flip or wait).
//   7. SetSystemPaletteUse(SYSPAL_NOSTATIC).
//   8. thunk_FUN_0046e810 (cursor/overlay blit).
// Debug strings reference "C:\DevStudio\Projects\Crux\SETPAL.cpp".
// 0x0046e490
// ---------------------------------------------------------------------------
void SetPal_SetPalette(void)
{
    if (g_nMainDC == 0)
        return;

    DWORD dwTime;

    if (g_hPalEvent != NULL)
        SetEvent(g_hPalEvent);

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x22),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    FUN_004896d0(g_abTargetPal, g_abSysPalEntries, 0x300);

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x24),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046c920(1);  // Sched_UpdatePalette(force)

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x26),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046cf10();   // SetPal_PreChange (via thunk)

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x28),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0041d8f0();   // GI flush

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x2a),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046ea40();   // page flip / wait

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x2c),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x2e),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046e810();   // SetPal_RestoreAndReset

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d8ebc + 0x30),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);
}

// ===========================================================================
// Extended SETPAL functions (0x0046e6d0 – 0x00470780)
//
// This section covers three subsystems that share the SETPAL translation unit:
//
//   1. Palette continuation helpers (SetPal_*):
//      RestoreSysColorsRaw, SetSyspalNostatic, RestoreAndReset,
//      WaitOrRealizeIfNeeded, RealizePalette, RemapSysColorTextRefs,
//      SetBlack, DestroyBlankWindow, CreateBlankWindow.
//
//   2. Sound channel dispatcher (Snd_*):
//      Sound_Init reads CRUX.INI [Sound] section for Stereo/16bit/Reverse/Pmode,
//      calls Mixer_Init, and sets g_nSndPmodeFlag1/2.
//      Snd_PlayCore (0x0046ff70) is the central sound dispatcher — routes to the
//      right Mixer channel based on a mode flag.
//      Snd_PlayCentered (0x0046f7f0) and Snd_PlayPanned (0x0046f730) are the
//      speech-channel entry points called from SOUNDMEM.cpp and MOVEMENT.cpp.
//
//   3. UI slider (Slider_Add 0x00470780):
//      Allocates and initialises a slot in the 20-entry sound channel table
//      (g_nSndChannelTable at 0x007c5910) for a UI animation slider widget.
//      Unexpected — this function is entirely animation/UI, not audio.
// ===========================================================================

// ---------------------------------------------------------------------------
// SetPal_RestoreSysColorsRaw -- Restore saved Windows system colours and set
// SYSPAL_NOSTATIC (mode 1).  Lightweight version of SetPal_RestoreSysColors
// without the Sched_SetNormalPriority / EndHighPriority guards.
// 0x0046e6d0
// ---------------------------------------------------------------------------
void SetPal_RestoreSysColorsRaw(void)
{
    SetSysColors(g_nSysColorCount, k_aSysColorIdx, (COLORREF *)g_abSavedSysColors);
    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 1
}

// ---------------------------------------------------------------------------
// SetPal_SetSyspalNostatic -- Set SYSPAL_NOSTATIC (mode 2) on the main DC.
// Called to release the two static palette entries reserved by Windows.
// 0x0046e780
// ---------------------------------------------------------------------------
void SetPal_SetSyspalNostatic(void)
{
    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2
}

// ---------------------------------------------------------------------------
// SetPal_RestoreAndReset -- Full palette teardown after exclusive use:
//   1. Sched_SetNormalPriority
//   2. SetSysColors with saved system colours (if windowed)
//   3. SetSystemPaletteUse(DC, SYSPAL_NOSTATIC=1)
//   4. Sched_EndHighPriority
// Debug strings reference "C:\DevStudio\Projects\Crux\SETPAL.cpp".
// Called from SetPal_SetPalette via thunk_FUN_0046e810.
// 0x0046e810
// ---------------------------------------------------------------------------
void SetPal_RestoreAndReset(void)
{
    DWORD dwTime;

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d9034 + 2),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046c290();  // Sched_SetNormalPriority

    if (g_nFullscreen == 0)
    {
        dwTime = timeGetTime();
        Debug_Assert(*(int *)(0x004d9034 + 6),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);
        SetSysColors(g_nSysColorCount, k_aSysColorIdx, (COLORREF *)g_abSavedSysColors);
    }

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d9034 + 9),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 1

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d9034 + 0xb),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);

    thunk_FUN_0046c1c0();  // Sched_EndHighPriority

    dwTime = timeGetTime();
    Debug_Assert(*(int *)(0x004d9034 + 0xd),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", dwTime);
}

// ---------------------------------------------------------------------------
// SetPal_WaitOrRealizeIfNeeded -- Palette synchronisation gate.
// WaitForSingleObject(g_hPalEvent, 0): if the event is not yet signalled
// (WAIT_TIMEOUT), calls SetPal_RealizePalette to push the active palette.
// If already signalled, returns immediately (palette is fresh).
// Called every frame from fade loops as thunk_FUN_0046e9a0.
// 0x0046e9a0
// ---------------------------------------------------------------------------
void SetPal_WaitOrRealizeIfNeeded(void)
{
    DWORD dwRet = WaitForSingleObject(g_hPalEvent, 0);
    if (dwRet != 0)
    {
        thunk_FUN_0046ea40();  // SetPal_RealizePalette
    }
}

// ---------------------------------------------------------------------------
// SetPal_RealizePalette -- Rebuild GDI palette from g_abActivePal on vsync.
// Debug string: "realize_palette_2"
// Steps:
//   1. DDI_WaitVerticalRetrace
//   2. SetPal_FillLogPalette(g_abActivePal, logPalBuf)
//   3. g_nGdiPalette = CreatePalette(logPalBuf)
//   4. SetSystemPaletteUse(DC, SYSPAL_NOSTATIC=2)
//   5. ho = SelectPalette; DeleteObject(ho)  [delete old palette]
//   6. RealizePalette
//   7. If g_nPalGeneration == 2: SetPal_RemapSysColorTextRefs + SetPal_RestoreSysColors
//   8. g_nPalGeneration = 0
// Called from SetPal_WaitOrRealizeIfNeeded (and directly during transitions).
// 0x0046ea40
// ---------------------------------------------------------------------------
void SetPal_RealizePalette(void)
{
    // Stack-allocated LOGPALETTE for 256 entries
    byte logPalBuf[sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pLogPal = (LOGPALETTE *)logPalBuf;

    HPALETTE ho;

    pLogPal->palVersion    = 0x300;
    pLogPal->palNumEntries = 256;

    PALETTEENTRY *pPE = pLogPal->palPalEntry;
    for (int i = 256; i != 0; i--)
    {
        pPE->peRed   = 0;
        pPE->peGreen = 0;
        pPE->peBlue  = 0;
        pPE->peFlags = 0;
        pPE++;
    }

    if (thunk_FUN_004861c0() != 0)  // DDI_IsActiveAndFullscreen
    {
        DDI_WaitVerticalRetrace();
        SetPal_FillLogPalette(g_abActivePal, pLogPal);
        g_nGdiPalette = (int)CreatePalette(pLogPal);
        SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2
        ho = SelectPalette((HDC)g_nMainDC, (HPALETTE)g_nGdiPalette, FALSE);
        DeleteObject(ho);
        RealizePalette((HDC)g_nMainDC);

        if (g_nPalGeneration == 2)
        {
            SetPal_RemapSysColorTextRefs(g_abActivePal);
            SetPal_RestoreSysColors();
        }
        g_nPalGeneration = 0;
    }
}

// ---------------------------------------------------------------------------
// SetPal_RemapSysColorTextRefs -- For each saved system colour, find the
// nearest entry in the supplied palette (param_1 = pPal768) and store the
// corresponding system-palette COLORREF in g_abSysColorTextRefs.
// Used so that text drawn over the game palette uses the correct on-screen colour.
// Called from SetPal_RealizePalette when g_nPalGeneration == 2.
// 0x0046ebc0
// ---------------------------------------------------------------------------
void SetPal_RemapSysColorTextRefs(const byte *pPal768)
{
    for (int i = 0; i < g_nSysColorCount; i++)
    {
        // Extract saved colour components, shift to 6-bit
        DWORD dwSaved = *(DWORD *)(g_abSavedSysColors + i * 4);
        int nR = (int)( dwSaved        & 0xff) >> 2;
        int nG = (int)((dwSaved >> 8)  & 0xff) >> 2;
        int nB = (int)((dwSaved >> 16) & 0xff) >> 2;

        int nBestDist = 0x7fffffff;
        int nBestIdx  = 0;

        for (int j = 0; j < 256; j++)
        {
            if (j == 10)
                j = 0xf6;   // skip reserved range

            int dR = (int)(byte)pPal768[j*3+0] - nR;
            int dG = (int)(byte)pPal768[j*3+1] - nG;
            int dB = (int)(byte)pPal768[j*3+2] - nB;
            int nDist = dR*dR + dG*dG + dB*dB;

            if (nDist < nBestDist)
            {
                nBestIdx  = j;
                nBestDist = nDist;
            }
        }

        *(DWORD *)(g_abSysColorTextRefs + i * 4) =
            (DWORD)(g_abSysPalEntries[nBestIdx*4+0]) |
            ((DWORD)(g_abSysPalEntries[nBestIdx*4+1]) << 8) |
            ((DWORD)(g_abSysPalEntries[nBestIdx*4+2]) << 16);
    }
}

// ---------------------------------------------------------------------------
// SetPal_SetBlack -- Set the GDI palette to pure black (all zeros) and
// call WaitForVerticalRetrace if param_1 != 0.
// Debug string: "pal_zero_int_wait"
// Used before a fade-to-black to ensure the hardware palette is zeroed.
// 0x0046edf0
// ---------------------------------------------------------------------------
void SetPal_SetBlack(int bWaitVbl)
{
    byte logPalBuf[sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pLogPal = (LOGPALETTE *)logPalBuf;
    byte abBlack[768];

    pLogPal->palVersion    = 0x300;
    pLogPal->palNumEntries = 256;

    PALETTEENTRY *pPE = pLogPal->palPalEntry;
    for (int i = 256; i != 0; i--)
    {
        pPE->peRed = pPE->peGreen = pPE->peBlue = pPE->peFlags = 0;
        pPE++;
    }

    if (g_nMainDC != 0)
    {
        Debug_Assert(*(int *)(0x004d913c + 0x10),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        if (bWaitVbl != 0)
            DDI_WaitVerticalRetrace();

        Debug_Assert(*(int *)(0x004d913c + 0x16),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        memset(abBlack, 0, 768);

        Debug_Assert(*(int *)(0x004d913c + 0x19),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        SetPal_FillLogPalette(abBlack, pLogPal);

        Debug_Assert(*(int *)(0x004d913c + 0x1b),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        g_nGdiPalette = (int)CreatePalette(pLogPal);

        Debug_Assert(*(int *)(0x004d913c + 0x1d),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        SetSystemPaletteUse((HDC)g_nMainDC, SYSPAL_NOSTATIC);  // 2

        Debug_Assert(*(int *)(0x004d913c + 0x1f),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        SelectPalette((HDC)g_nMainDC, (HPALETTE)g_nGdiPalette, FALSE);

        Debug_Assert(*(int *)(0x004d913c + 0x21),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

        RealizePalette((HDC)g_nMainDC);

        Debug_Assert(*(int *)(0x004d913c + 0x23),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);
    }
}

// ---------------------------------------------------------------------------
// SetPal_DestroyBlankWindow -- Destroy the blank overlay window (g_nBlankWnd).
// Calls CloseWindow then DestroyWindow and sets g_nBlankWnd = NULL.
// 0x0046f060
// ---------------------------------------------------------------------------
void SetPal_DestroyBlankWindow(void)
{
    Debug_Assert(*(int *)(0x004d9298 + 2),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

    thunk_FUN_004832d0(&LAB_00401c99);  // thread safety / lock

    Debug_Assert(*(int *)(0x004d9298 + 4),
                 "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);

    if (g_nBlankWnd != 0)
    {
        Debug_Assert(*(int *)(0x004d9298 + 9),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);
        CloseWindow((HWND)g_nBlankWnd);
        Debug_Assert(*(int *)(0x004d9298 + 0xb),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);
        DestroyWindow((HWND)g_nBlankWnd);
        Debug_Assert(*(int *)(0x004d9298 + 0xd),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);
        g_nBlankWnd = 0;
        Debug_Assert(*(int *)(0x004d9298 + 0xf),
                     "C:\\DevStudio\\Projects\\Crux\\SETPAL.cpp", 0);
    }
}

// ---------------------------------------------------------------------------
// SetPal_CreateBlankWindow -- Create a full-screen black WS_POPUP window
// (class "CRUXBlnkWndClass") and push it to the top of the Z-order.
// Debug string: "dt_blank"
// The window is used to black out the desktop before exclusive DirectDraw.
// Registers the class exactly once (DAT_004d9140 gates the RegisterClassA call).
// 0x0046f1f0
// ---------------------------------------------------------------------------
void SetPal_CreateBlankWindow(void)
{
    extern int  g_nBlankWndClassRegistered; // DAT_004d9140  0 = needs registration
    extern int  g_nHwndApp;                 // DAT_007d6af0  application HINSTANCE
    extern int  g_nHwndMain;               // main window HWND (int)

    if (g_nBlankWnd == 0)
    {
        if (g_nBlankWndClassRegistered != 0)
        {
            g_nBlankWndClassRegistered = 0;

            WNDCLASSA wc;
            wc.style         = CS_HREDRAW | CS_VREDRAW;   // 3
            wc.lpfnWndProc   = DefWindowProcA;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = 0;
            wc.hInstance     = (HINSTANCE)g_nHwndApp;
            wc.hIcon         = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
            wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);   // 4
            wc.lpszMenuName  = NULL;
            wc.lpszClassName = "CRUXBlnkWndClass";
            RegisterClassA(&wc);
        }

        thunk_FUN_00483200(&LAB_00401c99);  // thread safety / unlock

        g_nBlankWnd = (int)CreateWindowExA(
            0,
            "CRUXBlnkWndClass",
            "",
            WS_POPUP,           // 0x80000000
            0, 0, 0, 0,
            NULL, NULL,
            (HINSTANCE)g_nHwndApp,
            NULL);

        ShowWindow((HWND)g_nBlankWnd, SW_MAXIMIZE);  // 3
        SetWindowPos((HWND)g_nBlankWnd,  HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);  // 0x53
        SetWindowPos((HWND)g_nHwndMain, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
}

// ===========================================================================
// Sound channel dispatcher subsystem  (Snd_* / Sound_Init)
//
// These functions sit in the SETPAL translation unit but form a distinct
// sound-dispatch layer between SOUNDMEM.cpp/MOVEMENT.cpp callers and the
// low-level MIXER module.  The central dispatcher is Snd_PlayCore.
// ===========================================================================

// ---------------------------------------------------------------------------
// Snd_SetSpeechVol -- Set the speech channel volume register (g_nSndSpeechVol).
// Range: any int (callers pass 0-63).
// 0x0046f3c0
// ---------------------------------------------------------------------------
void Snd_SetSpeechVol(int nVol)
{
    g_nSndSpeechVol = nVol;
}

// ---------------------------------------------------------------------------
// Snd_GetSpeechVol -- Return the current speech channel volume.
// 0x0046f450
// ---------------------------------------------------------------------------
int Snd_GetSpeechVol(void)
{
    return g_nSndSpeechVol;
}

// ---------------------------------------------------------------------------
// Snd_SetSfxVol -- Set the SFX/music channel volume (g_nSndSfxVol).
// 0x0046f4e0
// ---------------------------------------------------------------------------
void Snd_SetSfxVol(int nVol)
{
    g_nSndSfxVol = nVol;
}

// ---------------------------------------------------------------------------
// Sound_Init -- Initialise the sound system from CRUX.INI.
// Reads [Sound] section: SampleRate, Stereo, 16bit, Reverse, Pmode.
// Pmode values:
//   0 = both flags off
//   1 = both flags on (g_nSndPmodeFlag1=1, g_nSndPmodeFlag2=1)
//   2 = flag1 off, flag2 on
// Calls Mixer_Init(sampleRate, stereo, 16bit, reverse).
// Calls thunk_FUN_00443b80(1) = Mixer_SelectFillFunc(1) (pick mono fill kernel).
// 0x0046f570
// ---------------------------------------------------------------------------
void Sound_Init(void)
{
    extern byte g_abIniPath[];   // 0x006299c0  CRUX.INI path string

    UINT nSampleRate = GetPrivateProfileIntA("Sound", "",       0x5622, (LPCSTR)g_abIniPath);
    UINT nStereo     = GetPrivateProfileIntA("Sound", "Stereo", 1,      (LPCSTR)g_abIniPath);
    UINT n16bit      = GetPrivateProfileIntA("Sound", "16bit",  1,      (LPCSTR)g_abIniPath);
    UINT nReverse    = GetPrivateProfileIntA("Sound", "Reverse",0,      (LPCSTR)g_abIniPath);
    UINT nPmode      = GetPrivateProfileIntA("Sound", "Pmode",  1,      (LPCSTR)g_abIniPath);

    if (nPmode == 0)
    {
        g_nSndPmodeFlag1 = 0;
        g_nSndPmodeFlag2 = 0;
    }
    else if (nPmode == 1)
    {
        g_nSndPmodeFlag1 = 1;
        g_nSndPmodeFlag2 = 1;
    }
    else if (nPmode == 2)
    {
        g_nSndPmodeFlag1 = 0;
        g_nSndPmodeFlag2 = 1;
    }

    Mixer_Init((int)nSampleRate, (int)nStereo, (int)n16bit, (int)nReverse);
    thunk_FUN_00443b80(1);   // Mixer_SelectFillFunc(mono source)
}

// ---------------------------------------------------------------------------
// Snd_PlayPanned -- Play a sound on a specific channel with optional pan.
// param_1 = sound handle / sentence id
// param_2 = channel selector (1 = speech channel, other = SFX channel)
// param_3 = pan position (0-100; 50=centre)
//
// Volume is looked up from g_nSndSpeechVol (channel==1) or g_nSndSfxVol.
// Calls Snd_PlayCore(handle, channel, vol, 0, pan, -1).
// 0x0046f730
// ---------------------------------------------------------------------------
void Snd_PlayPanned(int nHandle, int nChannel, int nPan)
{
    int nVol;

    if (nChannel == 1)
        nVol = g_nSndSpeechVol;
    else
        nVol = g_nSndSfxVol;

    Snd_PlayCore((int*)&nHandle, nChannel, nVol, 0, nPan, 0xffffffff);
}

// ---------------------------------------------------------------------------
// Snd_PlayCentered -- Play a sound centred (pan=50) on the given channel.
// Calls Snd_PlayPanned(handle, channel, 0x32=50).
// This is the entry point called from SOUNDMEM.cpp (speech: centred/mono)
// and from MOVEMENT.cpp (walk sound).
// 0x0046f7f0
// ---------------------------------------------------------------------------
void Snd_PlayCentered(int nHandle, int nChannel)
{
    thunk_FUN_0046f730(nHandle, nChannel, 0x32);  // Snd_PlayPanned(h, ch, 50)
}

// ---------------------------------------------------------------------------
// Snd_PlayFull -- Play a sound with explicit volume, pan, and callback.
// Calls Snd_PlayCore with mode=2 (full parameter set).
// param_1 = handle, param_2 = channel, param_3 = volume, param_4 = pan,
// param_5 = done-callback or -1
// 0x0046f890
// ---------------------------------------------------------------------------
void Snd_PlayFull(int nHandle, int nChannel, int nVol, int nPan, int nCallback)
{
    Snd_PlayCore((int*)&nHandle, nChannel, nVol, 2, nPan, (unsigned int)nCallback);
}

// ---------------------------------------------------------------------------
// Snd_PlayFull2 -- Snd_PlayFull variant with -1 as pan (centre/no pan).
// Calls Snd_PlayFull(p1, p2, p3, -1, p4).
// In the CRT thunk table — called from startup path.
// 0x0046f940
// ---------------------------------------------------------------------------
void Snd_PlayFull2(int nHandle, int nChannel, int nVol, int nCallback)
{
    thunk_FUN_0046f890(nHandle, nChannel, nVol, 0xffffffff, nCallback);
}

// ---------------------------------------------------------------------------
// Snd_Nop1 -- Empty stub (SEH frame only).  Placeholder.
// 0x0046f9e0
// ---------------------------------------------------------------------------
void Snd_Nop1(void) {}

// ---------------------------------------------------------------------------
// Snd_Stop -- Stop a sound channel.
// Calls thunk_FUN_00443df0(param_1) = Mixer_StopChannel(chan).
// Called from SOUNDMEM.cpp as thunk_FUN_0046fa60 (stop speech channel).
// 0x0046fa60
// ---------------------------------------------------------------------------
void Snd_Stop(int nChannel)
{
    thunk_FUN_00443df0(nChannel);
}

// ---------------------------------------------------------------------------
// Snd_GetSfxVol -- Return current SFX volume (g_nSndSfxVol).
// 0x0046faf0
// ---------------------------------------------------------------------------
int Snd_GetSfxVol(void)
{
    return g_nSndSfxVol;
}

// ---------------------------------------------------------------------------
// Snd_SetSfxVolClamped -- Set SFX volume, clamping to range 0-63.
// Negative values are clamped to 0; values > 63 are clamped to 63.
// 0x0046fb80
// ---------------------------------------------------------------------------
void Snd_SetSfxVolClamped(int nVol)
{
    int nClamped = nVol;
    if (nVol > 0x3f)
        nClamped = 0x3f;
    g_nSndSfxVol = (nClamped < 0) ? 0 : nClamped;
}

// ---------------------------------------------------------------------------
// Snd_IsIdle -- Check whether a channel is idle (not playing).
// Calls thunk_FUN_00443d50(param_1) = Mixer_IsChannelIdle(chan).
// Returns non-zero if the channel is free.
// Called from SOUNDMEM.cpp as thunk_FUN_0046fc40 (check speech idle).
// 0x0046fc40
// ---------------------------------------------------------------------------
int Snd_IsIdle(int nChannel)
{
    thunk_FUN_00443d50(nChannel);
    return 0;  // return value passed through from thunk
}

// ---------------------------------------------------------------------------
// Snd_Nop2 / Snd_Nop3 / Snd_Nop4 / Snd_Nop5 -- Empty stubs.
// 0x0046fcd0, 0x0046fd50, 0x0046fdd0, 0x0046fe50
// ---------------------------------------------------------------------------
void Snd_Nop2(void) {}
void Snd_Nop3(void) {}
void Snd_Nop4(void) {}
void Snd_Nop5(void) {}

// ---------------------------------------------------------------------------
// Snd_Play -- Play a sound with handle, channel, volume, callback.
// Calls Snd_PlayCore(handle, channel, vol, mode=0, callback, -1).
// 0x0046fed0
// ---------------------------------------------------------------------------
void Snd_Play(int nHandle, int nChannel, int nVol, int nCallback)
{
    Snd_PlayCore((int*)&nHandle, nChannel, nVol, 0, nCallback, 0xffffffff);
}

// ---------------------------------------------------------------------------
// Snd_PlayCore -- Central sound dispatch. Routes a sound to the Mixer.
// param_1 = sound data pointer (or pointer-to-handle)
// param_2 = channel id
// param_3 = volume
// param_4 = play mode:
//   0 = load via SndMem_Load (thunk_FUN_00472340), then play
//   1 = no-op (reserved path)
//   2,3 = param_1 already decoded (ptr+1 is data ptr, *param_1 is size)
//   default = null play (channel stopped)
// param_5 = pan position
// param_6 = loop-count mask or -1
//
// Early-out: if g_nSndSubtitleOnly (DAT_00629b04) != 0, skip all audio.
// 0x0046ff70
// ---------------------------------------------------------------------------
void Snd_PlayCore(int *pHandle, int nChannel, int nVol, int nMode,
                  int nPan, unsigned int nLoopMask)
{
    extern int g_nSndSubtitleOnly;  // 0x00629b04  subtitle-only flag

    int   nDataSize = 0;
    int  *pDataPtr  = NULL;

    if (g_nSndSubtitleOnly != 0)
        return;

    switch (nMode)
    {
    case 0:
        // Load via SndMem_Load, store decoded buffer pointer in local_1c/local_20
        pDataPtr = (int *)thunk_FUN_00472340(
                       (int)pHandle, &nDataSize, 0, 2,
                       (char*)&nLoopMask);
        break;
    case 1:
        // No-op path
        break;
    case 2:
    case 3:
        // Already decoded: pHandle[0]=size, pHandle+1=data
        pDataPtr = pHandle + 1;
        nDataSize = *pHandle;
        break;
    default:
        pDataPtr  = NULL;
        nDataSize = 0;
        break;
    }

    if (pDataPtr != NULL)
    {
        // Start Mixer channel playback
        thunk_FUN_004427e0(
            nChannel,
            pDataPtr,
            nDataSize,
            (int)(nLoopMask & 2) >> 1,      // stereo flag
            (int)(nLoopMask & 1) * 8 + 8,   // volume shift
            (int)((nLoopMask & 4) >> 2) * 0x5622 + 0x5622,  // sample rate
            nVol,
            nPan);
    }
}

// ---------------------------------------------------------------------------
// Snd_PauseChannel -- Pause a Mixer channel (remove from active list).
// Wraps Mixer_RemoveChannel(param_1).
// 0x00470180
// ---------------------------------------------------------------------------
void Snd_PauseChannel(int nChannel)
{
    Mixer_RemoveChannel(nChannel);
}

// ---------------------------------------------------------------------------
// Snd_ResumeChannel -- Resume a Mixer channel (add back to active list).
// Wraps Mixer_AddChannel(param_1).
// 0x00470210
// ---------------------------------------------------------------------------
void Snd_ResumeChannel(int nChannel)
{
    Mixer_AddChannel(nChannel);
}

// ---------------------------------------------------------------------------
// Snd_VolumeUp -- Increment SFX volume by 1 (max 0x40), log "sound_vol + %d".
// 0x004702a0
// ---------------------------------------------------------------------------
void Snd_VolumeUp(void)
{
    if (g_nSndSfxVol < 0x40)
    {
        g_nSndSfxVol++;
        thunk_FUN_00417df0("sound_vol + %d", g_nSndSfxVol);
    }
}

// ---------------------------------------------------------------------------
// Snd_VolumeDown -- Decrement SFX volume by 1 (min 0), log "sound_vol - %d".
// 0x00470350
// ---------------------------------------------------------------------------
void Snd_VolumeDown(void)
{
    if (g_nSndSfxVol != 0)
    {
        g_nSndSfxVol--;
        thunk_FUN_00417df0("sound_vol - %d", g_nSndSfxVol);
    }
}

// ---------------------------------------------------------------------------
// Snd_StopChannel -- Stop a single Mixer channel.
// Calls thunk_FUN_00443fe0(param_1) = Mixer_StopChannel(chan).
// 0x00470400
// ---------------------------------------------------------------------------
void Snd_StopChannel(int nChannel)
{
    thunk_FUN_00443fe0(nChannel);
}

// ---------------------------------------------------------------------------
// Snd_StopAll -- Stop all Mixer channels.
// Calls thunk_FUN_00443ee0() = Mixer_StopAll().
// In the CRT thunk table (thunk_FUN_00470490).
// 0x00470490
// ---------------------------------------------------------------------------
void Snd_StopAll(void)
{
    thunk_FUN_00443ee0();
}

// ---------------------------------------------------------------------------
// Snd_SetChannelPan -- Set pan position on a channel (volume unchanged).
// Calls Mixer_SetVolume(channel, -1, pan).  The -1 volume means "keep current".
// 0x00470520
// ---------------------------------------------------------------------------
void Snd_SetChannelPan(int nChannel, int nPan)
{
    Mixer_SetVolume(nChannel, 0xffffffff, nPan);
}

// ---------------------------------------------------------------------------
// Snd_GetActiveChanId -- Return the Mixer active-channel ID at slot nIdx.
// Reads from g_nMixerActiveChanIds[nIdx] (base at 0x006dc3d8).
// 0x004705c0
// ---------------------------------------------------------------------------
int Snd_GetActiveChanId(int nIdx)
{
    extern int g_nMixerActiveChanIds;  // 0x006dc3d8  int[]
    return *(int *)((byte *)&g_nMixerActiveChanIds + nIdx * 4);
}

// ---------------------------------------------------------------------------
// Snd_ResetChannelTable -- Initialise all 20 entries of g_nSndChannelTable.
// Per slot (stride 0x30):
//   +0x00  flags   = clear bits 0 and 1 (not-playing, not-looping)
//   +0x04  id      = -1 (inactive)
//   +0x08  x (dst) = 100
//   +0x0c  x (src) = 100
//   +0x10..+0x18 = 0
//   +0x1c  = 0
//   +0x20  = 0
//   +0x24..+0x28 = 0
// In the CRT thunk table (thunk_FUN_004705e0).
// 0x004705e0
// ---------------------------------------------------------------------------
void Snd_ResetChannelTable(void)
{
    extern int g_nSndChannelTable;  // 0x007c5910  base of 20-slot table

    for (int i = 0; i < 20; i++)
    {
        byte *pSlot = (byte *)&g_nSndChannelTable + i * 0x30;

        *(int *)(pSlot + 0x00) &= ~1;   // clear "active" bit
        *(int *)(pSlot + 0x00) &= ~2;   // clear "looping" bit
        *(int *)(pSlot + 0x04) = -1;    // channel id = inactive
        *(int *)(pSlot + 0x08) = 100;   // dst x (or volume scale)
        *(int *)(pSlot + 0x0c) = 100;   // src x
        *(int *)(pSlot + 0x10) = 0;
        *(int *)(pSlot + 0x14) = 0;
        *(int *)(pSlot + 0x18) = 0;
        *(int *)(pSlot + 0x1c) = 0;
        *(int *)(pSlot + 0x20) = 0;
        *(int *)(pSlot + 0x24) = 0;
        *(int *)(pSlot + 0x28) = 0;
    }
}

// ===========================================================================
// UI Slider subsystem
// Note: this is *not* an audio slider — it drives animated UI widgets.
// The channel table (g_nSndChannelTable) is dual-purposed: sound dispatch
// above and slider-widget state below.
// ===========================================================================

// ---------------------------------------------------------------------------
// Slider_Add -- Allocate a free slot in g_nSndChannelTable for an animation
// slider widget and initialise it.
// Debug string: "slider_add__int_ani_slot__int_dir"
//
// Finds the first slot where bit 0 of flags is 0 (inactive).
// If all 20 slots are full, calls Err_SetRecord3(0x1d, "Sliders", -1) and aborts.
// Initialises the slot with the animation slot index and direction flag.
// Returns the allocated slot index.
//
// param_1 = Anim slot index
// param_2 = direction flag (bit 0)
// 0x00470780
// ---------------------------------------------------------------------------
int Slider_Add(int nAnimSlot, unsigned int nDir)
{
    extern int  g_nSndChannelTable;     // 0x007c5910
    extern int  g_anAnimFrameCount[];   // animation frame count table
    extern int  g_anAnimSlotX[];        // animation slot X coords
    extern int  g_anAnimSlotY[];        // animation slot Y coords

    int nSlot = 0;

    // Find first inactive slot
    while (nSlot < 20 &&
           (*(int *)((byte *)&g_nSndChannelTable + nSlot * 0x30) & 1))
    {
        nSlot++;
    }

    if (nSlot == 20)
    {
        // Overflow: show error dialog
        Debug_Assert(*(int *)(0x004d9a30 + 0xe),
                     "C:\\DevStudio\\Projects\\Crux\\SLIDER.cpp");

        void *pRec = Err_SetRecord3(0x1d, "Sliders", 0xffffffff);
        int rec[3];
        rec[0] = *(int *)pRec;
        rec[1] = *((int *)pRec + 1);
        rec[2] = *((int *)pRec + 2);
        FUN_00489090(rec, (void *)0x004ab3f8);
    }

    byte *pSlot = (byte *)&g_nSndChannelTable + nSlot * 0x30;

    // Mark active + set direction bit
    *(int *)(pSlot + 0x00) |= 1;
    *(int *)(pSlot + 0x00) = (*(int *)(pSlot + 0x00) & ~2) | ((nDir & 1) << 1);

    *(int *)(pSlot + 0x04) = nAnimSlot;
    *(int *)(pSlot + 0x08) = g_anAnimFrameCount[nAnimSlot];

    Anim_SetFrameStep(nAnimSlot, 0);
    Anim_SetCurrentFrame(nAnimSlot, 0);

    *(int *)(pSlot + 0x2c) = 3;   // state = 3

    // Compute initial position from animation slot origin + frame top-left
    int nFrmX, nFrmY;
    Anim_GetFrameTopLeft(nAnimSlot, &nFrmX, &nFrmY);

    int nX = g_anAnimSlotX[nAnimSlot] + nFrmX;
    int nY = g_anAnimSlotY[nAnimSlot] + nFrmY;

    *(int *)(pSlot + 0x14) = nX;   // current x
    *(int *)(pSlot + 0x0c) = nX;   // target x (same at start)
    *(int *)(pSlot + 0x18) = nY;   // current y
    *(int *)(pSlot + 0x08) = nY;   // target y (same at start)

    return nSlot;
}
