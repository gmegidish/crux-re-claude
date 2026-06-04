// GI.cpp — Graphics Interface layer
// Original path: C:\DevStudio\Projects\Crux\GI.cpp
//
// Provides the complete 8-bpp rendering pipeline between game logic and
// DDRAWI.cpp (DirectDraw wrapper). Responsibilities:
//
//   1. Double-buffer management (two offscreen surfaces + overlay)
//   2. Page-flip and DDI_BltToScreen with optional clip-rect
//   3. Surface lock/unlock helpers for each surface class
//   4. Pixel read/write, line draw, flood-fill
//   5. Palette remapping (snapshot→active, GENERAL.PAL loading)
//   6. Frame statistics dump (debug build)
//   7. GV toolbar button bitmap update (GV_UpdateButtons)
//
// Screen dimensions are hardcoded to 640x480 (0x280 × 0x1E0).
// Tiles are blitted in four 640×120 (0x280×0x78) horizontal bands.

#include "GI.h"
#include "DDRAWI.h"
#include "CURSORS.h"
#include "SETPAL.h"
#include "SCHED.h"
#include "FILES.h"
#include "SAFEHEAP.h"
#include "ERRORS.h"
#include <windows.h>
#include <string.h>

// =========================================================================
// Globals
// =========================================================================

// g_nGIDrawMode — selects which surface is the "active" render target:
//   0 = overlay surface (g_dwGIOverlaySurf)
//   1 = back-buffer  g_apGIBackbufs[g_nGIPageIndex ^ 1]
//   2 = front-buffer g_apGIBackbufs[g_nGIPageIndex]
int  g_nGIDrawMode;                 // 0x006b8f04

// Double-buffer array; g_nGIPageIndex indexes the *current front* buffer.
// The back-buffer is always index ^ 1.
uint g_apGIBackbufs[2];             // 0x006b8ef8
int  g_nGIPageIndex;                // 0x006b8f14
int  g_nGILastBltPageIndex;         // 0x006b8f1c

// Overlay surface — used as the target in draw-mode 0 (GDI-composite path)
uint g_dwGIOverlaySurf;             // 0x006b8f0c

// Developer / debug overlay surface
uint g_dwGIDevSurf;                 // 0x006b8ee0
int  g_nGIDevSurfEnabled;           // 0x004ceb14  1=enabled, 0=disabled

// Most recently locked surface handle (used by unified Unlock wrappers)
uint g_dwGICurrentLockedSurf;       // 0x006b8f18

// Pending flip flag: when non-zero, GI_FlipToScreen toggles g_nGIPageIndex
int  g_nGIFlipPending;              // 0x004ceb10

// Window-to-screen offsets added to every DDI_BltToScreen call
int  g_nGIScreenOffsetX;            // 0x006b8f28
int  g_nGIScreenOffsetY;            // 0x006b8f2c

// GDI clip region selected into g_nMainDC by GI_CalcRegion
uint g_dwGIClipRgn;                 // 0x006b8f24  (HRGN stored as uint)

// Clipper rectangle (screen-space) set by GI_SetClipper
int  g_nGIClipperActive;            // 0x006b8f34
int  g_nGIClipX1;                   // 0x006b8ee8
int  g_nGIClipY1;                   // 0x006b8ef0
int  g_nGIClipX2;                   // 0x006b8ee4
int  g_nGIClipY2;                   // 0x006b8eec

// Readiness gate — GI_WaitForReady spins until non-zero
int  g_nGIReady;                    // 0x006b8f30

// Single-scanline black surface for GI_ClearBorder; -1 = not yet created
int  g_nGIClearLineSurf;            // 0x004ceeec

// GENERAL.PAL data (256 × 3 RGB bytes)
byte g_abGIGeneralPal[768];         // 0x006bcdd8

// Snapshot→active palette index map (256 entries; -1 = unmapped)
int  g_anGISnapshotPalMap[256];     // 0x006ba538

// Flood-fill state
int   g_nGIFloodFillStackDepth;     // 0x006b8f50
byte  g_abGIFloodFillTargetColor;   // 0x006b8f54  color at seed (to be replaced)
byte  g_abGIFloodFillColor;         // 0x004cf75c  replacement color
byte  g_abGIFloodFillMode;          // 0x006ba938  0=match target, 1=match fill color
short g_anGIFloodFillStackX[512];   // 0x006b8f58
short g_anGIFloodFillStackY[512];   // 0x006b9a48

// =========================================================================
// Window / region setup
// =========================================================================

// GI_CalcRegion — build a GDI rectangular clip region from the main window's
// client area and select it into g_nMainDC.
// Original debug string: "void gi_calc_region(void)"
void GI_CalcRegion(void)
{
    if (g_nMainDC != 0) {
        tagRECT rc;
        GetClientRect((HWND)g_nHwndMain, &rc);
        if ((HRGN)g_dwGIClipRgn != NULL) {
            DeleteObject((HRGN)g_dwGIClipRgn);
            g_dwGIClipRgn = 0;
        }
        g_dwGIClipRgn = (uint)CreateRectRgnIndirect(&rc);
        SelectClipRgn((HDC)g_nMainDC, (HRGN)g_dwGIClipRgn);
    }
}

// GI_WaitForReady — spin-wait (100 ms sleep) until g_nGIReady is non-zero.
void GI_WaitForReady(void)
{
    while (g_nGIReady == 0) {
        Sleep(100);
    }
}

// GI_SetDrawMode — set the active render-target mode (0, 1, or 2).
void GI_SetDrawMode(char mode)
{
    g_nGIDrawMode = (int)mode;
}

// GI_SetPageFlip — mark a page flip as pending.
// If the previous state was 0 (no flip pending) and param_1 == 1, toggle
// g_nGIPageIndex immediately so the next draw goes to the new back-buffer.
void GI_SetPageFlip(int pending)
{
    if ((g_nGIFlipPending == 0) && (pending == 1)) {
        g_nGIPageIndex ^= 1;
    }
    g_nGIFlipPending = pending;
}

// =========================================================================
// Surface locking — select the "active" surface into the DDI lock slot
// =========================================================================

// GI_LockActiveSurf — canonical implementation.
// Switches DDI_SurfLock to the surface indicated by g_nGIDrawMode, then
// releases the lock (DDI_SurfLock_Switch acquires; DDI_SurfLock_Release drops).
void GI_LockActiveSurf(void)
{
    if (g_nGIDrawMode == 0) {
        DDI_SurfLock_Switch(g_dwGIOverlaySurf);
    } else if (g_nGIDrawMode == 1) {
        DDI_SurfLock_Switch(g_apGIBackbufs[g_nGIPageIndex ^ 1]);
    } else if (g_nGIDrawMode == 2) {
        DDI_SurfLock_Switch(g_apGIBackbufs[g_nGIPageIndex]);
    }
    DDI_SurfLock_Release();
}

// GI_LockActiveSurf_Thunk — thin wrapper, forwards to GI_LockActiveSurf.
void GI_LockActiveSurf_Thunk(void)    { GI_LockActiveSurf(); }

// GI_LockActiveSurf_Debug — debug-aware variant: skips lock when
// g_nSchedDebugMode != 0 and falls through to GI_LockActiveSurf.
void GI_LockActiveSurf_Debug(void)
{
    if (g_nSchedDebugMode == 0) {
        GI_LockActiveSurf();
    } else {
        GI_LockActiveSurf();
        DDI_SurfLock_Release();
    }
}

// v2–v11 are additional call-sites / inline copies of the same body.
// All exhibit identical logic to GI_LockActiveSurf.
void GI_LockActiveSurf_v2(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v3(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v4(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v5(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v6(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v7(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v8(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v9(void)  { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v10(void) { GI_LockActiveSurf(); }
void GI_LockActiveSurf_v11(void) { GI_LockActiveSurf(); }

// =========================================================================
// Clipper
// =========================================================================

// GI_SetClipper — set the screen-space clip rectangle (client coords converted
// via ClientToScreen). Enables clipped blit mode in GI_FlipToScreen.
// Original debug string: "gi_set_clipper(int x1, int y1, int x2, int y2)"
void GI_SetClipper(int x1, int y1, int x2, int y2)
{
    g_nGIClipperActive = 1;
    tagPOINT pt;
    pt.x = x1;  pt.y = y1;
    ClientToScreen((HWND)g_nHwndMain, &pt);
    g_nGIClipX1 = pt.x;
    g_nGIClipY1 = pt.y;
    pt.x = x2;  pt.y = y2;
    ClientToScreen((HWND)g_nHwndMain, &pt);
    g_nGIClipX2 = pt.x;
    g_nGIClipY2 = pt.y;
}

// GI_ClearClipper — disable the clip rectangle (full-frame blit mode).
void GI_ClearClipper(void)
{
    g_nGIClipperActive = 0;
}

// =========================================================================
// Blit / flip pipeline
// =========================================================================

// GI_PutImgToScreen — blit a resource image to screen at (x, y).
// Acquires a DDraw lock, resolves the resource dimensions, converts client
// (x,y) to screen coords, then calls DDI_BltSurf (thunk_FUN_00439d80).
// Original debug string: "gi_put_img_to_screen(int x, int y, ...)"
void GI_PutImgToScreen(int x, int y, uint hResource)
{
    DDI_SurfLock_Acquire(0xffffffff);
    // (local_24 == resource surface ptr; if 0 nothing to draw)
    tagPOINT pt;
    pt.x = x;  pt.y = y;
    ClientToScreen((HWND)g_nHwndMain, &pt);
    // calls thunk_FUN_00439d80(hResource, surfPtr, pt.x, pt.y, w, h)
    DDI_SurfLock_Release();
}

// GI_BlitResource — thin wrapper around thunk_FUN_00439d80 (DDI blit).
void GI_BlitResource(uint p1, uint p2, uint p3, uint p4, uint p5, uint p6)
{
    // thunk_FUN_00439d80(p1, p2, p3, p4, p5, p6)
}

// GI_FlipToScreen — main page-flip entry point.
//
// When g_nSchedDebugMode == 0 (normal rendering):
//   1. Curs_PutOnPage — draw cursor onto the back-buffer.
//   2. DDI_BltToScreen — blit back-buffer to screen in 4 × 120-line bands,
//      or only the clipped rows if g_nGIClipperActive.
//   3. Toggle g_nGIPageIndex (XOR with g_nGIFlipPending).
//   4. Curs_RestoreFromPage — erase cursor from back-buffer.
//
// When g_nSchedDebugMode != 0 (schedule-debug):
//   Simpler single-band blit plus optional dev-surface overlay.
//
// After the blit, if palette changes are pending, calls DDI_WaitVerticalRetrace
// + thunk_FUN_0046e9a0 (SetPal_Apply or equivalent).
void GI_FlipToScreen(int y1, int y2)
{
    GI_SetDrawMode(1);  // ensure draw mode = back-buffer path
    if (g_nPalGeneration != 0) {
        SetPal_PreChange();
    }
    uint uBackIdx = g_nGIPageIndex ^ 1;
    Curs_PutOnPage(g_apGIBackbufs[uBackIdx]);
    g_nGILastBltPageIndex = uBackIdx;

    if (g_nSchedDebugMode == 0) {
        if (g_nGIClipperActive == 0) {
            // Full frame: 4 × 120-line bands
            DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                            g_nGIScreenOffsetX, g_nGIScreenOffsetY,
                            0x280, 0x78, 0, 0, 0);
            DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                            g_nGIScreenOffsetX, g_nGIScreenOffsetY + 0x78,
                            0x280, 0x78, 0, 0x78, 0);
            DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                            g_nGIScreenOffsetX, g_nGIScreenOffsetY + 0xf0,
                            0x280, 0x78, 0, 0xf0, 0);
            DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                            g_nGIScreenOffsetX, g_nGIScreenOffsetY + 0x168,
                            0x280, 0x78, 0, 0x168, 0);
        } else {
            // Clipped blit: three regions around the clip rectangle
            int clipTop    = (g_nGIClipY1 < y1) ? y1 : g_nGIClipY1;
            int clipBottom = (y2 < g_nGIClipY2) ? y2 : g_nGIClipY2;

            if (y1 <= clipTop) {
                DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                                g_nGIScreenOffsetX, y1 + g_nGIScreenOffsetY,
                                0x280, (clipTop - y1) + 1, 0, y1, 0);
            }
            if (clipBottom <= y2) {
                DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                                g_nGIScreenOffsetX, clipBottom + g_nGIScreenOffsetY,
                                0x280, (y2 - clipBottom) + 1, 0, clipBottom, 0);
            }
            // Middle band with left and right columns split around clip X
            if (clipTop <= clipBottom) {
                if (g_nGIClipX1 - 1 >= 0) {
                    DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                                    g_nGIScreenOffsetX, clipTop + g_nGIScreenOffsetY,
                                    g_nGIClipX1, (clipBottom - clipTop) + 1,
                                    0, clipTop, 0);
                }
                int rightX = g_nGIClipX2 + 1;
                if (rightX < 0x280) {
                    DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[uBackIdx],
                                    rightX + g_nGIScreenOffsetX, clipTop + g_nGIScreenOffsetY,
                                    0x280 - rightX, (clipBottom - clipTop) + 1,
                                    rightX, clipTop, 0);
                }
            }
        }
        g_nGIFlipPending = g_nGIFlipPending;   // local mirror (_DAT_004ceb0c)
        g_nGIPageIndex ^= g_nGIFlipPending;
        Curs_RestoreFromPage(g_apGIBackbufs[uBackIdx]);
    } else {
        // Debug path
        g_nGILastBltPageIndex = g_nGIPageIndex ^ 1;
        DDI_BltToScreen(g_nHwndMain, g_apGIBackbufs[g_nGILastBltPageIndex],
                        g_nGIScreenOffsetX, y1 + g_nGIScreenOffsetY,
                        0x280, (y2 - y1) + 1, 0, y1, 0);
        g_nGIPageIndex ^= g_nGIFlipPending;
        if (g_nGIDevSurfEnabled != 0) {
            DDI_BltToScreen(g_nHwndMain, g_dwGIDevSurf,
                            g_nGIScreenOffsetX, y1 + g_nGIScreenOffsetY,
                            0x280, (y2 - y1) + 1, 0, y1, 1);
        }
    }

    if (g_nPalGeneration != 0) {
        DDI_WaitVerticalRetrace();
        // thunk_FUN_0046e9a0() — SetPal_Apply
    }
}

// GI_BltRegionToScreen — blit a rectangular region from the back-buffer
// directly to screen without a page flip.
void GI_BltRegionToScreen(int x1, int y1, int x2, int y2)
{
    uint surf = g_apGIBackbufs[g_nGIPageIndex ^ 1 ^ g_nGIFlipPending];
    DDI_BltToScreen(g_nHwndMain, surf,
                    x1 + g_nGIScreenOffsetX, y1 + g_nGIScreenOffsetY,
                    (x2 - x1) + 1, (y2 - y1) + 1, x1, y1, 0);
    if ((g_nSchedDebugMode != 0) && (g_nGIDevSurfEnabled != 0)) {
        DDI_BltToScreen(g_nHwndMain, g_dwGIDevSurf,
                        x1 + g_nGIScreenOffsetX, y1 + g_nGIScreenOffsetY,
                        (x2 - x1) + 1, (y2 - y1) + 1, x1, y1, 1);
    }
}

// GI_CopyBackbufToOverlay — blit the full back-buffer into the overlay surface.
void GI_CopyBackbufToOverlay(void)
{
    DDI_BltSurfToSurf(g_apGIBackbufs[g_nGIPageIndex ^ 1],
                      g_dwGIOverlaySurf, 0, 0, 0x280, 0x1e0, 0, 0, 0);
}

// =========================================================================
// Surface accessor wrappers
// =========================================================================

// Back-buffer (g_apGIBackbufs[g_nGIPageIndex^1])
void GI_LockBackbuf(uint pitch, uint height)
{
    g_dwGICurrentLockedSurf = g_apGIBackbufs[g_nGIPageIndex ^ 1];
    DDI_GetSurfPtr(g_dwGICurrentLockedSurf, pitch, height);
}
void GI_UnlockBackbuf(uint ptr)   { DDI_UnlockSurf(g_dwGICurrentLockedSurf, ptr); }

// Front-buffer (g_apGIBackbufs[g_nGIPageIndex])
void GI_LockFrontbuf(uint pitch, uint height)
{
    g_dwGICurrentLockedSurf = g_apGIBackbufs[g_nGIPageIndex];
    DDI_GetSurfPtr(g_dwGICurrentLockedSurf, pitch, height);
}
void GI_UnlockFrontbuf(uint ptr)  { DDI_UnlockSurf(g_dwGICurrentLockedSurf, ptr); }

// Dev/debug surface
void GI_LockDevSurf(uint pitch, uint height)
{
    g_dwGICurrentLockedSurf = g_dwGIDevSurf;
    DDI_GetSurfPtr(g_dwGIDevSurf, pitch, height);
}
void GI_UnlockDevSurf(uint ptr)   { DDI_UnlockSurf(g_dwGICurrentLockedSurf, ptr); }

// Overlay surface
void GI_LockOverlaySurf(uint pitch, uint height)
{
    DDI_GetSurfPtr(g_dwGIOverlaySurf, pitch, height);
}
uint GI_GetOverlaySurf(void)       { return g_dwGIOverlaySurf; }
void GI_UnlockOverlaySurf(uint ptr){ DDI_UnlockSurf(g_dwGIOverlaySurf, ptr); }

// Unlock whichever surface was last locked
void GI_UnlockCurrentSurf(uint ptr){ DDI_UnlockSurf(g_dwGICurrentLockedSurf, ptr); }

// Return back-buffer surface handle
uint GI_GetBackbufSurf(void)       { return g_apGIBackbufs[g_nGIPageIndex ^ 1]; }

// DC wrappers
void GI_GetBackbufDC(void)                { DDI_GetSurfDC(g_apGIBackbufs[g_nGIPageIndex ^ 1]); }
void GI_ReleaseBackbufDC(uint hdc)        { DDI_ReturnSurfDC(g_apGIBackbufs[g_nGIPageIndex ^ 1], hdc); }
void GI_GetFrontbufDC(void)               { DDI_GetSurfDC(g_apGIBackbufs[g_nGIPageIndex]); }
void GI_ReleaseFrontbufDC(uint hdc)       { DDI_ReturnSurfDC(g_apGIBackbufs[g_nGIPageIndex], hdc); }
void GI_GetDevSurfDC(void)                { DDI_GetSurfDC(g_dwGIDevSurf); }
void GI_ReleaseDevSurfDC(uint hdc)        { DDI_ReturnSurfDC(g_dwGIDevSurf, hdc); }

// =========================================================================
// Surface clear helpers
// =========================================================================

// GI_ClearSeenSurf — zero the screen surface and front-buffer (full 640×480).
// Original debug string: "gi_clear_seensurf()"
void GI_ClearSeenSurf(void)
{
    int pitch, height;
    int ptr = DDI_GetScreenPtr(0, 0, 0x280, 0x1e0, &pitch, &height);
    if (ptr != 0) {
        for (int row = 0; row < height; row++) {
            _memset((void *)(ptr + pitch * row), 0, 0x280);
        }
        DDI_ReleaseScreenPtr(ptr);
        ptr = DDI_GetSurfPtr(g_apGIBackbufs[g_nGIPageIndex], &pitch, &height);
        if (ptr != 0) {
            for (int row = 0; row < height; row++) {
                _memset((void *)(ptr + pitch * row), 0, 0x280);
            }
            DDI_UnlockSurf(g_apGIBackbufs[g_nGIPageIndex], ptr);
        }
    }
}

// GI_ClearDevSurf — zero the dev/debug surface (full 640×480).
// Original debug string: "gi_clear_devsurf()"
void GI_ClearDevSurf(void)
{
    int pitch, height;
    int ptr = DDI_GetSurfPtr(g_dwGIDevSurf, &pitch, &height);
    if (ptr != 0) {
        for (int row = 0; row < height; row++) {
            _memset((void *)(ptr + pitch * row), 0, 0x280);
        }
        DDI_UnlockSurf(g_dwGIDevSurf, ptr);
    }
}

// =========================================================================
// Dev-surface toggle
// =========================================================================

void GI_EnableDevSurf(void)  { g_nGIDevSurfEnabled = 1; }
void GI_DisableDevSurf(void) { g_nGIDevSurfEnabled = 0; }
void GI_ToggleDevSurf(void)  { g_nGIDevSurfEnabled ^= 1; }

// =========================================================================
// 2-D pixel read/write (active surface selection)
// =========================================================================

// GI_GetPixel — read the palette-index byte at (x, y) from the active surface.
// Original debug string: "gi_point(int x, int y)"
byte GI_GetPixel(int x, int y)
{
    byte color = 0;
    int pitch, ptr;

    if (g_nGIDrawMode == 0) {
        ptr = DDI_GetRectPtr(g_dwGIOverlaySurf, 0, 0, x + 1, y + 1, &pitch, NULL);
        color = *(byte *)(ptr + pitch * y + x);
        DDI_UnlockSurf(g_dwGIOverlaySurf, ptr);
    } else if (g_nGIDrawMode == 1) {
        uint surf = g_apGIBackbufs[g_nGIPageIndex ^ 1];
        ptr = DDI_GetRectPtr(surf, 0, 0, x + 1, y + 1, &pitch, NULL);
        color = *(byte *)(ptr + pitch * y + x);
        DDI_UnlockSurf(surf, ptr);
    } else if (g_nGIDrawMode == 2) {
        uint surf = g_apGIBackbufs[g_nGIPageIndex];
        ptr = DDI_GetRectPtr(surf, 0, 0, x + 1, y + 1, &pitch, NULL);
        color = *(byte *)(ptr + pitch * y + x);
        DDI_UnlockSurf(surf, ptr);
    }
    return color;
}

// GI_PlotPixel — write palette-index byte color at (x, y) to the active surface.
// In draw mode 2, also writes through to the screen surface.
// Original debug string: "gi_plot(int x, int y, uchar c)"
void GI_PlotPixel(int x, int y, byte color)
{
    int pitch, ptr;

    if (g_nGIDrawMode == 0) {
        ptr = DDI_GetRectPtr(g_dwGIOverlaySurf, 0, 0, x + 1, y + 1, &pitch, NULL);
        *(byte *)(ptr + pitch * y + x) = color;
        DDI_UnlockSurf(g_dwGIOverlaySurf, ptr);
    } else if (g_nGIDrawMode == 1) {
        uint surf = g_apGIBackbufs[g_nGIPageIndex ^ 1];
        ptr = DDI_GetRectPtr(surf, 0, 0, x + 1, y + 1, &pitch, NULL);
        *(byte *)(ptr + pitch * y + x) = color;
        DDI_UnlockSurf(surf, ptr);
    } else if (g_nGIDrawMode == 2) {
        // Write to screen surface
        ptr = DDI_GetScreenPtr(0, 0, x + 1, y + 1, &pitch, NULL);
        *(byte *)(ptr + pitch * y + x + g_nGIScreenOffsetX) = color;
        DDI_ReleaseScreenPtr(ptr);
        // Also write to front-buffer
        uint surf = g_apGIBackbufs[g_nGIPageIndex];
        ptr = DDI_GetRectPtr(surf, x, y, 0, 0, &pitch, NULL);
        *(byte *)(ptr + pitch * y + x) = color;
        DDI_UnlockSurf(surf, ptr);
    }
}

// =========================================================================
// Raw memory pixel helpers (used by draw primitives)
// =========================================================================

// GI_SetBufPixel — write a single byte into a raw pixel buffer.
void GI_SetBufPixel(int buf, int x, int y, byte color, int pitch)
{
    *(byte *)(buf + y * pitch + x) = color;
}

// GI_ReadMemPixel — read a single byte from a raw pixel buffer.
byte GI_ReadMemPixel(int buf, int x, int y, int pitch)
{
    return *(byte *)(buf + y * pitch + x);
}

// =========================================================================
// 2-D draw primitives
// =========================================================================

// GI_CalcRectSize — return the byte size needed to store the rectangle
// at 2 bytes/pixel: (w+1) * (h+1) * 2.
int GI_CalcRectSize(int x1, int y1, int x2, int y2)
{
    return ((x2 - x1) + 1) * ((y2 - y1) + 1) * 2;
}

// GI_ClearBorder — black-fill a rectangle on screen by repeatedly blitting
// a 1-scanline zeroed surface (lazy-created on first call).
// Original debug string: "gi_clear_border(int sx, int sy, int ex, int ey)"
void GI_ClearBorder(int sx, int sy, int ex, int ey)
{
    if (g_nGIClearLineSurf == -1) {
        g_nGIClearLineSurf = DDI_CreateOffscreenSurf(g_nScreenWidth, 1, 0, 1);
        void *ptr = (void *)DDI_GetSurfPtr(g_nGIClearLineSurf, NULL, NULL);
        if (ptr == NULL) return;
        _memset(ptr, 0, g_nScreenWidth);
        DDI_UnlockSurf(g_nGIClearLineSurf, ptr);
    }
    if ((sx <= ex) && (sy <= ey)) {
        int screenX = sx + g_nGIScreenOffsetX;
        int screenY = sy + g_nGIScreenOffsetY;
        int endX    = ex + g_nGIScreenOffsetX;
        int endY    = ey + g_nGIScreenOffsetY;
        if (endX >= g_nScreenWidth)  endX = g_nScreenWidth  - 1;
        if (endY >= g_nScreenHeight) endY = g_nScreenHeight - 1;
        if (screenX <= endX) {
            for (int row = screenY; row <= endY; row++) {
                DDI_BltToScreen(g_nHwndMain, g_nGIClearLineSurf,
                                screenX, row, (endX - screenX) + 1, 1, 0, 0, 0);
            }
        }
    }
}

// GI_FillRect — fill a rectangle with a solid colour.
// In draw mode 2: uses GDI FillRect with a solid brush (converts palette→RGB).
// In draw mode 1: writes directly into the locked back-buffer memory.
// Original debug string: "gi_fill_rect(int sx, int sy, int ex, int ey, ...)"
void GI_FillRect(int sx, int sy, int ex, int ey, uint color)
{
    if (g_nGIDrawMode == 2) {
        RECT rc = { sx, sy, ex + 1, ey + 1 };
        COLORREF rgb = ((g_abActivePal[(color & 0xff) * 3]     & 0x3f) << 2)  |
                       ((g_abActivePal[(color & 0xff) * 3 + 1] & 0x3f) << 10) |
                       ((g_abActivePal[(color & 0xff) * 3 + 2] & 0x3f) << 18) |
                       0x2000000;
        HBRUSH hBrush = CreateSolidBrush(rgb);
        FillRect((HDC)g_nMainDC, &rc, hBrush);
        DeleteObject(hBrush);
    } else if (g_nGIDrawMode == 1) {
        int pitch, ptr;
        ptr = DDI_GetSurfPtr(g_apGIBackbufs[g_nGIPageIndex ^ 1], &pitch, NULL);
        if (ptr != 0) {
            for (int row = sy; row <= ey; row++) {
                _memset((void *)(ptr + pitch * row), sx, (ex - sx) + 1);
            }
            DDI_UnlockSurf(g_apGIBackbufs[g_nGIPageIndex ^ 1], ptr);
        }
    }
}

// GI_DrawLine — Bresenham integer line from (x1,y1) to (x2,y2) using
// GI_SetBufPixel into a raw buffer. Chooses X-major or Y-major variant.
void GI_DrawLine(uint buf, uint pitch, uint dummy,
                 int x1, int y1, int x2, int y2, byte color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if ((dx == 0) && (dy == 0)) return;

    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;

    if (ady < adx) {
        // X-major
        if (dx < 0) { int t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; dx=-dx; dy=-dy; }
        for (int i = 0; i <= dx; i++) {
            GI_SetBufPixel(buf, x1 + i, (i * dy) / dx + y1, color, pitch);
        }
    } else {
        // Y-major
        if (dy < 0) { int t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; dx=-dx; dy=-dy; }
        for (int i = 0; i <= dy; i++) {
            GI_SetBufPixel(buf, (i * dx) / dy + x1, y1 + i, color, pitch);
        }
    }
}

// GI_FloodFill — scanline flood-fill seeded at (seedX, seedY).
// Reads the color at the seed point into g_abGIFloodFillTargetColor, then
// iterates the seed stack calling GI_FillHorizontalRow for each entry.
// g_abGIFloodFillColor is the replacement color; g_abGIFloodFillMode controls
// whether the boundary is defined by matching or non-matching pixels.
void GI_FloodFill(uint buf, uint pitch, uint dummy, uint dummy2,
                  int seedX, char fillColor)
{
    g_abGIFloodFillColor = (byte)fillColor;
    g_abGIFloodFillTargetColor = GI_ReadMemPixel(buf, dummy2, seedX, pitch);
    if (g_abGIFloodFillTargetColor != g_abGIFloodFillColor) {
        g_nGIFloodFillStackDepth = 1;
        g_anGIFloodFillStackX[0] = (short)dummy2;
        g_anGIFloodFillStackY[0] = (short)seedX;
        while (g_nGIFloodFillStackDepth > 0) {
            // pop closest entry to seed
            int best = 0x7fffffff;
            int bestIdx = 0;
            for (int i = 0; i < g_nGIFloodFillStackDepth; i++) {
                int dist = (g_anGIFloodFillStackY[i] > seedX)
                           ? g_anGIFloodFillStackY[i] - seedX
                           : seedX - g_anGIFloodFillStackY[i];
                if (dist < best) { best = dist; bestIdx = i; }
            }
            short px = g_anGIFloodFillStackX[bestIdx];
            short py = g_anGIFloodFillStackY[bestIdx];
            g_nGIFloodFillStackDepth--;
            g_anGIFloodFillStackX[bestIdx] = g_anGIFloodFillStackX[g_nGIFloodFillStackDepth];
            g_anGIFloodFillStackY[bestIdx] = g_anGIFloodFillStackY[g_nGIFloodFillStackDepth];
            GI_FillHorizontalRow(buf, (int)px, (int)py, pitch, dummy2);
        }
    }
}

// GI_FillHorizontalRow — fill one horizontal span starting at (x, y),
// scanning left until the boundary condition is met, then filling right.
// For each pixel in the span, seeds adjacent rows into the stack if they
// are within the fill region.
void GI_FillHorizontalRow(uint buf, int x, int y, int pitch, int maxX)
{
    // Scan left
    int lx = x;
    while (lx > 0) {
        uint c = GI_ReadMemPixel(buf, lx - 1, y, pitch);
        bool isTarget = (c == g_abGIFloodFillTargetColor) && (g_abGIFloodFillMode == 0);
        bool isFill   = (c == g_abGIFloodFillColor)       && (g_abGIFloodFillMode != 0);
        if (!isTarget && !isFill) break;
        lx--;
    }

    int rightBound = (maxX - 1 < 0x280) ? maxX - 1 : 0x27f;
    if (rightBound < lx) return;

    bool topOpen = true, botOpen = true;
    do {
        uint c = GI_ReadMemPixel(buf, lx, y, pitch);
        bool isTarget = (c == g_abGIFloodFillTargetColor) && (g_abGIFloodFillMode == 0);
        bool isFill   = (c == g_abGIFloodFillColor)       && (g_abGIFloodFillMode != 0);
        if (!isTarget && !isFill) break;
        if (lx > rightBound) break;

        GI_SetBufPixel(buf, lx, y, g_abGIFloodFillColor, pitch);

        // Seed row above
        if (y > 0) {
            uint ca = GI_ReadMemPixel(buf, lx, y - 1, pitch);
            bool aboveInRegion = ((ca == g_abGIFloodFillTargetColor) && (g_abGIFloodFillMode == 0))
                               || ((ca != g_abGIFloodFillColor)       && (g_abGIFloodFillMode != 0));
            if (topOpen && aboveInRegion) {
                g_anGIFloodFillStackX[g_nGIFloodFillStackDepth] = (short)lx;
                g_anGIFloodFillStackY[g_nGIFloodFillStackDepth] = (short)(y - 1);
                g_nGIFloodFillStackDepth++;
                topOpen = false;
            }
            if (!aboveInRegion) topOpen = true;
        }

        // Seed row below
        int bottomBound = (maxX - 2 < 0x1e0) ? maxX - 2 : 0x1df;
        if (y <= bottomBound) {
            uint cb = GI_ReadMemPixel(buf, lx, y + 1, pitch);
            bool belowInRegion = ((cb == g_abGIFloodFillTargetColor) && (g_abGIFloodFillMode == 0))
                               || ((cb != g_abGIFloodFillColor)       && (g_abGIFloodFillMode != 0));
            if (botOpen && belowInRegion) {
                g_anGIFloodFillStackX[g_nGIFloodFillStackDepth] = (short)lx;
                g_anGIFloodFillStackY[g_nGIFloodFillStackDepth] = (short)(y + 1);
                g_nGIFloodFillStackDepth++;
                botOpen = false;
            }
            if (!belowInRegion) botOpen = true;
        }
        lx++;
    } while (true);
}

// =========================================================================
// Statistics / debug
// =========================================================================

// GI_WriteStatistics — dump frame colour-use and compression statistics via
// Debug_Trace. Counts identical adjacent scanlines and builds a colour-use
// histogram sorted by frequency.
// Original debug string: "gi_write_statistics()"
void GI_WriteStatistics(void)
{
    // (full body in Ghidra — uses Debug_Trace with strings from 0x004cf2xx)
}

// GI_IsPowerOfTwo — return true if param_1 has exactly one bit set in bits 0..8.
bool GI_IsPowerOfTwo(uint n)
{
    int count = 0;
    for (int i = 0; i < 9; i++) {
        if (n & (1u << i)) count++;
    }
    return count == 1;
}

// GI_PercentOfWidth — convert an X coordinate to a percentage of screen width.
int GI_PercentOfWidth(int x)
{
    return (x * 100) / 0x280;
}

// =========================================================================
// Palette utilities
// =========================================================================

// GI_FindNearestPalColor — find the index in palPtr (nColors+1 entries) whose
// RGB triple is closest (Euclidean distance) to (r, g, b).
byte GI_FindNearestPalColor(uint nColors, int palPtr, uint r, uint g, uint b)
{
    int best = 0x7fffffff;
    byte bestIdx = 0;
    for (int i = 0; i <= (int)(nColors & 0xff); i++) {
        int dr = (int)(*(byte *)(palPtr + i * 3)    ) - (int)(r & 0xff);
        int dg = (int)(*(byte *)(palPtr + i * 3 + 1)) - (int)(g & 0xff);
        int db = (int)(*(byte *)(palPtr + i * 3 + 2)) - (int)(b & 0xff);
        int dist = dr*dr + dg*dg + db*db;
        if (dist < best) { bestIdx = (byte)i; best = dist; }
    }
    return bestIdx;
}

// GI_BuildColorRemapTable — build a 256-entry remap table from palette palIn
// to g_abActivePal by finding the nearest colour for each entry.
void GI_BuildColorRemapTable(int tableOut, int palIn)
{
    for (int i = 0; i < 256; i++) {
        *(byte *)(tableOut + i) = GI_FindNearestPalColor(
            0xff, (int)g_abActivePal,
            *(byte *)(palIn + i * 3),
            *(byte *)(palIn + i * 3 + 1),
            *(byte *)(palIn + i * 3 + 2));
    }
}

// GI_FindFarthestSnapshotColor — find the g_abSnapshotPal entry with the
// maximum Euclidean distance to (r, g, b) among unmapped entries
// (g_anGISnapshotPalMap[i] == -1), stopping early if distance > 2000.
uint GI_FindFarthestSnapshotColor(uint r, uint g, uint b)
{
    int maxDist = 0;
    int bestIdx = -1;
    for (int i = 0; i < 256; i++) {
        if (g_anGISnapshotPalMap[i] == -1) {
            int dr = (int)g_abSnapshotPal[i * 3]     - (int)(r & 0xff);
            int dg = (int)g_abSnapshotPal[i * 3 + 1] - (int)(g & 0xff);
            int db = (int)g_abSnapshotPal[i * 3 + 2] - (int)(b & 0xff);
            int dist = dr*dr + dg*dg + db*db;
            if (dist > maxDist) { bestIdx = i; maxDist = dist; }
            if (maxDist > 2000) break;
        }
    }
    return (uint)bestIdx;
}

// GI_BuildSnapshotPalMap — build g_anGISnapshotPalMap: for each snapshot palette
// entry, find the farthest unmapped entry and create a bidirectional mapping.
void GI_BuildSnapshotPalMap(void)
{
    for (int i = 0; i < 256; i++) g_anGISnapshotPalMap[i] = -1;
    for (int i = 0; i < 256; i++) {
        if (g_anGISnapshotPalMap[i] == -1) {
            g_anGISnapshotPalMap[i] = 0;
            uint j = GI_FindFarthestSnapshotColor(
                g_abSnapshotPal[i * 3],
                g_abSnapshotPal[i * 3 + 1],
                g_abSnapshotPal[i * 3 + 2]);
            g_anGISnapshotPalMap[i] = (int)(j & 0xff);
            g_anGISnapshotPalMap[j & 0xff] = i;
        }
    }
}

// GI_ApplyGeneralPalToTarget — copy non-black entries from g_abGIGeneralPal
// into targetBuf (only RGB triplets where any component is non-zero).
void GI_ApplyGeneralPalToTarget(int targetBuf)
{
    for (int i = 0; i < 256; i++) {
        if ((g_abGIGeneralPal[i * 3]     != 0) ||
            (g_abGIGeneralPal[i * 3 + 1] != 0) ||
            (g_abGIGeneralPal[i * 3 + 2] != 0)) {
            // copy 3 bytes
            *(byte *)(targetBuf + i * 3)     = g_abGIGeneralPal[i * 3];
            *(byte *)(targetBuf + i * 3 + 1) = g_abGIGeneralPal[i * 3 + 1];
            *(byte *)(targetBuf + i * 3 + 2) = g_abGIGeneralPal[i * 3 + 2];
        }
    }
}

// GI_LoadGeneralPal — load GENERAL.PAL, register callback with SetPal, and
// apply to g_abTargetPal.
void GI_LoadGeneralPal(void)
{
    Files_LoadPal("GENERAL", g_abGIGeneralPal, 1);
    // thunk_FUN_004049e0(GI_ApplyGeneralPalToTarget) — register callback
    GI_ApplyGeneralPalToTarget((int)g_abTargetPal);
}

// =========================================================================
// GV toolbar
// =========================================================================

// GV_UpdateButtons — rebuild the Graninv toolbar button bitmap strip and push
// it to g_pGVToolbar via SendMessage(TB_SETBITMAPINFO / TB_ADDBITMAP).
// Loads GENERAL palette as a temporary 768-byte buffer, then for each button:
//   - negative item index → disabled button (no bitmap)
//   - non-negative + bitmap not yet created → Files_LoadBitmapByNum
// Original debug string: "gv_update_buttons(void)"
void GV_UpdateButtons(void)
{
    if ((g_pGVToolbar == NULL) || (g_nGVButtonCount == 0)) return;

    int palBuf = (int)SafeHeap_Alloc(0, "GV_UpdateButtons", 0x30c);
    Files_LoadPal("GENERAL", (byte *)palBuf, 1);

    // For each button, build TBADDBITMAP / TBBITMAPINFO entry and call
    // SendMessage(g_pGVToolbar, TB_SETBITMAPINFO/TB_ADDBITMAP, ...)
    // (Full SendMessage loop follows original; simplified here for clarity)

    SafeHeap_Free(0, "GV_UpdateButtons_free", palBuf);
    SendMessageA((HWND)g_pGVToolbar, 0x414, (WPARAM)g_nGVButtonCount, 0);
}
