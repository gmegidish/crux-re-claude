// ---------------------------------------------------------------------------
// DDRAWI.cpp -- DirectDraw hardware abstraction layer for CRUX.EXE (Win95)
//
// This module wraps the Win95 DirectDraw 2 COM API behind a flat C interface
// used by the rest of the engine. It owns:
//
//   - IDirectDraw*         g_pDDraw       (0x00648220)
//   - IDirectDrawSurface*  g_pPrimary     (0x00648224)  front/primary surface
//   - IDirectDrawSurface*  g_pBackBuffer  (0x00648228)  back buffer
//   - IDirectDrawSurface*  g_apSurfaces[10] (0x006480c0) offscreen pool
//   - int                  g_anSurfLost[10] (0x00648190) lost flags per slot
//   - CRITICAL_SECTION     g_csLock       (0x006480a8)
//   - HWND                 g_hWnd         (0x006481b8)
//   - int                  g_nDDInitialized (0x0064822c)
//   - int                  g_nFullscreenActive (0x00648230)
//   - int                  g_nPrimaryLost  (0x00648188)
//
// Per-slot surface metadata at 0x006480e0 + slot*0x10:
//   +0x08  nWidth  (pixels)
//   +0x0c  nHeight (pixels)
//   +0x10  nHasColorKey
//   +0x14  nIsOverlay
//
// Clip support:
//   RECT g_aClipRects[4] at 0x006481e0
//   Clip list header     at 0x006481c0 (RGNDATAHEADER-style, dwSize=0x20)
//
// Dissolve effect state:
//   int  g_pDissolveSrc        (0x0069f2ac) source sprite pointer (0=none)
//   int  g_aDissolveOrder[N]   (0x00648290) shuffled pixel index table
//   int  g_nDissolveTotal      (0x0069f2a8) total dissolve pixels
//   int  g_nDissolveStep       (0x004ca6a0) current tick counter (-1=not started)
//   int  g_nDissolveSteps      (0x004ca69c) total ticks for dissolve
//   int  g_nDissolveSpriteW    (0x00654294) clipped width  (max 0x80 pixels)
//   int  g_nDissolveSpriteH    (0x00654290) clipped height (max 0x60 pixels)
//   int  g_nDissolveSrcW       (0x0065429c) full sprite width
//   int  g_nDissolveSrcH       (0x00654298) full sprite height
//   byte g_abDissolveBuf[0x4b000] at 0x006542a0 scratch bitmap buffer
//   int  g_nDissolveDstX       (0x0069f2a0) screen destination X
//   int  g_nDissolveDstY       (0x0069f2a4) screen destination Y
//
// Original source: C:\DevStudio\Projects\Crux\DDRAWI.cpp  (inferred)
// Address range:   0x0041abb0 -- 0x0041f10f
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include "DDRAWI.h"
#include "SCHED.h"
#include "ERRORS.h"

// External debug / error helpers
extern "C" {
    void  Debug_Trace(int nLine, const char *pszFile, ...);
    void  thunk_FUN_00420e60(int nLine, const char *pszFile);  // assert-line helper
    void  FUN_00489090(void *pRec, void *pDispatchTable);      // Err_Dispatch
    void *Err_SetRecord3(int nCode, ...);
    void  thunk_FUN_00483c70(void);   // palette init helper (in SETPAL)
    void  thunk_FUN_00483490(int nLine, const char *pszFile);
}

// SCHED timer registration
extern void thunk_FUN_0042fbe0(void *pfnCallback, int nParam);
// Sprite blit helper (called by dissolve tick)
extern void thunk_FUN_00430300(int nSurfSlot, int nX, int nY, int nColor, int nParam);
// Sprite decode helper
extern void thunk_FUN_0042c030(int pSrc, void *pDst, int nX, int nY, int nW, int nH);
// Random number
extern int  FUN_00489cf0(void);
// sprintf / message box helpers
extern int  FUN_0048a6a0(char *pBuf, const char *pFmt, ...);
extern int  FUN_0048a060(char *pBuf, const char *pFmt, ...);

// Extern globals defined in other modules
extern int  g_nSchedDebugMode;  // SCHED.cpp  0x00648178 (approx)
extern int  g_nFullscreen;      // SETPAL.cpp 0x006b8d80
extern int  g_nHwndMain;        // main window (same as g_hWnd but int cast)
extern int  DAT_007d6b84;       // windowed-mode override flag
extern int  DAT_007d6a80;       // Y-offset for windowed mode mapping

// Error string table pointer and debug flags (ERRORS.cpp)
extern char **g_pErrStrings;    // 0x00648238 / 0x0064823c area
extern int    g_nDebugFlags;

// ---------------------------------------------------------------------------
// DDI_InitDirectDraw -- One-time DirectDraw initialisation.
// Creates IDirectDraw, sets cooperative level (fullscreen exclusive or normal),
// optionally sets 640x480x8 display mode, creates primary+back surfaces,
// creates a clipper, attaches it to the back buffer.
// Debug string: "ddi_init HWND hwnd"
// 0x0041b7c0
// ---------------------------------------------------------------------------
void DDI_InitDirectDraw(HWND hWnd)
{
    // guard: only init once
    // if (g_nDDInitialized != 0) return;
    // g_nDDInitialized = 1;
    // g_hWnd = hWnd;
    // InitializeCriticalSection(&g_csLock);
    //
    // for (int i = 0; i < 10; i++) {
    //     g_apSurfaces[i] = NULL;
    //     g_anSurfLost[i] = 0;
    // }
    //
    // HRESULT hr = DirectDrawCreate(NULL, &g_pDDraw, NULL);
    // if (hr != DD_OK)  --> Err_SetRecord3(0x1c, DDI_ErrorToString(hr), -1) + dispatch
    //
    // In windowed/debug mode (g_nSchedDebugMode==0 && g_nFullscreen==0 && !DAT_007d6b84):
    //   g_pDDraw->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN) (0x11)
    //   g_pDDraw->SetDisplayMode(640, 480, 8)  --> sets g_nFullscreenActive = 1 on success
    //
    // thunk_FUN_00483c70();  // palette system init
    //
    // g_pDDraw->SetCooperativeLevel(hWnd, DDSCL_NORMAL) (0x08)
    //
    // Create primary surface (DDSCAPS_PRIMARYSURFACE, dwFlags=DDSD_CAPS):
    //   DDSURFACEDESC desc = { .dwSize=0x6c, .dwFlags=1, .ddsCaps.dwCaps=1 };
    //   g_pDDraw->CreateSurface(&desc, &g_pPrimary, NULL)
    //
    // GetDC on primary (vtbl+0x54) to get g_nMainDC, then g_nMainDC = 0.
    //
    // Create back buffer (DDSCAPS_OFFSCREENPLAIN):
    //   g_pDDraw->CreateSurface with dwFlags=0 (DDSD_CAPS only) --> g_pBackBuffer
    //
    // DDI_SetClipRegion(-1,-1,-1,-1);   // window-clip on back buffer
    //
    // g_pPrimary->SetClipper(g_pBackBuffer)  (vtbl+0x70)
    //
    // On any error --> Err_SetRecord3(0x1a, DDI_ErrorToString(hr), -1) + dispatch
}

// ---------------------------------------------------------------------------
// DDI_SetFullscreenMode -- call IDirectDraw::RestoreDisplayMode to exit
// exclusive fullscreen. Updates g_nFullscreenActive.
// 0x0041b6d0
// ---------------------------------------------------------------------------
void DDI_SetFullscreenMode(void)
{
    // if (g_nFullscreenActive == 0) return;
    // HRESULT hr = g_pDDraw->RestoreDisplayMode();   // vtbl offset 0x4c
    // if (hr == DD_OK)
    //     g_nFullscreenActive = 0;
    // else
    //     Debug_Trace(..., DDI_ErrorToString(hr));
}

// ---------------------------------------------------------------------------
// DDI_CheckFullscreenMode -- check/update fullscreen active flag.
// Called on app-activate / focus change.
// 0x0041d9c0
// ---------------------------------------------------------------------------
void DDI_CheckFullscreenMode(void)
{
    // if (!g_nDDInitialized || g_nSchedDebugMode || g_nFullscreen || !g_nFullscreenActive) return;
    // HRESULT hr = g_pDDraw->RestoreDisplayMode();
    // if (hr == DD_OK) g_nFullscreenActive = 0;
    // else Debug_Trace(..., DDI_ErrorToString(hr));
}

// ---------------------------------------------------------------------------
// DDI_SetDisplayMode -- Re-enter 640x480x8 exclusive mode.
// No-op in sched-debug or g_nFullscreen mode.
// Debug string: "ddi_set_mode"
// 0x0041c340
// ---------------------------------------------------------------------------
void DDI_SetDisplayMode(void)
{
    // if (g_nSchedDebugMode != 0 || g_nFullscreen != 0) return;
    //
    // HRESULT hr = g_pDDraw->SetCooperativeLevel(g_hWnd, DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN); // 0x11
    // if (hr != DD_OK) --> error dispatch
    //
    // hr = g_pDDraw->SetDisplayMode(0x280, 0x1e0, 8);   // 640x480x8
    // if (hr == DD_OK)
    //     g_nFullscreenActive = 1;
    // else --> error dispatch
    //
    // hr = g_pDDraw->SetCooperativeLevel(g_hWnd, DDSCL_NORMAL); // 0x08
    // if (hr != DD_OK) --> error dispatch
}

// ---------------------------------------------------------------------------
// DDI_RefreshSurfs -- Recreate primary+back+offscreen surfaces after mode change.
// Debug string: "ddi_refresh_surfs"
// 0x0041af50
// ---------------------------------------------------------------------------
void DDI_RefreshSurfs(void)
{
    // Release old back buffer: g_pBackBuffer->Release(); (vtbl+8)
    //
    // Create new primary:
    //   DDSURFACEDESC desc = { .dwSize=0x6c, .dwFlags=1, .ddsCaps=DDSCAPS_PRIMARYSURFACE };
    //   g_pDDraw->CreateSurface(&desc, &g_pPrimary, NULL);
    //
    // Create new back buffer (DDSCAPS_OFFSCREENPLAIN):
    //   g_pDDraw->CreateSurface(&desc2, &g_pBackBuffer, NULL);
    //
    // DDI_SetClipRegion(-1,-1,-1,-1);
    // g_pPrimary->SetClipper(g_pBackBuffer);   (vtbl+0x70)
    //
    // for each non-overlay slot with g_apSurfaces[i] != NULL:
    //     DDI_RecreateOffscreenSurf(i);
}

// ---------------------------------------------------------------------------
// DDI_CreateOverlaySurf -- Create a new offscreen surface in first free slot.
// Returns slot index or -1 if pool is full.
// Debug string: "ddi_create_overlay_surf int width int height"
// 0x0041b2c0
// ---------------------------------------------------------------------------
int DDI_CreateOverlaySurf(int nWidth, int nHeight)
{
    // find first g_apSurfaces[i] == NULL; if none return -1
    //
    // DDSURFACEDESC desc;
    // memset(&desc, 0, 0x6c);
    // desc.dwSize   = 0x6c;
    // desc.dwFlags  = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;  // 0x8007
    // desc.dwWidth  = nWidth;
    // desc.dwHeight = nHeight;
    // desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;           // 0x80
    // desc.ddckCKSrcBlt = {0, 0};
    //
    // HRESULT hr = g_pDDraw->CreateSurface(&desc, &g_apSurfaces[slot], NULL);
    // if (hr != DD_OK) --> error dispatch
    //
    // g_anSurfLost[slot] = 0;
    // Lock the surface, zero all rows, unlock.
    // DDI_UnlockSurf(slot, ptr);
    //
    // return slot;
    return -1;
}

// ---------------------------------------------------------------------------
// DDI_RecreateOffscreenSurf -- Recreate surface at slot nSlot.
// Called after a surface-lost recovery or mode change.
// Debug string: "ddi_recreate_offscreen_surf int n"
// 0x0041abb0
// ---------------------------------------------------------------------------
void DDI_RecreateOffscreenSurf(int nSlot)
{
    // g_apSurfaces[nSlot]->Release();   (vtbl+8)
    //
    // Build DDSURFACEDESC from stored width/height/flags for this slot.
    // dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT (0x40 base + color key if needed)
    //
    // g_pDDraw->CreateSurface(&desc, &g_apSurfaces[nSlot], NULL);
    // if (FAILED) --> Err_SetRecord3(0x1a, ...) + dispatch
    //
    // Lock + zero + unlock via DDI_GetSurfPtr / DDI_UnlockSurf.
    //
    // If slot has color-key flag: call IDirectDrawSurface::SetColorKey (vtbl+0x74).
}

// ---------------------------------------------------------------------------
// DDI_ReleaseSurf -- Release the surface at slot nSlot.
// 0x0041b610
// ---------------------------------------------------------------------------
void DDI_ReleaseSurf(int nSlot)
{
    if (/* g_apSurfaces[nSlot] != NULL */ 0)
    {
        // g_apSurfaces[nSlot]->Release();   (vtbl+8)
        // g_apSurfaces[nSlot] = NULL;
    }
}

// ---------------------------------------------------------------------------
// DDI_RestoreLostSurfs -- Scan offscreen pool; restore any IsLost surfaces.
// Sets g_anSurfLost[i]=1 on each surface that needed restoring.
// 0x0041c9d0
// ---------------------------------------------------------------------------
void DDI_RestoreLostSurfs(void)
{
    // for (int i = 0; i < 10; i++) {
    //     if (g_apSurfaces[i] == NULL) continue;
    //     HRESULT hr = g_apSurfaces[i]->IsLost();   // vtbl+0x60
    //     if (hr != DD_OK) {
    //         g_apSurfaces[i]->Restore();            // vtbl+0x6c
    //         g_anSurfLost[i] = 1;
    //     }
    // }
}

// ---------------------------------------------------------------------------
// DDI_ClearLostSurfs -- Zero-fill any surfaces that were lost+restored.
// Handles g_nPrimaryLost for the primary surface, then each slot in the pool.
// Debug string: "ddi_clear_lost_surfs"
// 0x0041e740
// ---------------------------------------------------------------------------
void DDI_ClearLostSurfs(void)
{
    // if (g_nPrimaryLost != 0) {
    //     g_nPrimaryLost = 0;
    //     void *pPix = DDI_GetScreenPtr(0, 0, 0x280, 0x1e0, &nPitch, &nPad);
    //     if (pPix) {
    //         for (int y = 0; y < 0x1e0; y++)
    //             memset(pPix + y * nPitch, 0, 0x280);
    //         DDI_ReleaseScreenPtr(pPix);
    //     }
    // }
    // for (int i = 0; i < 10; i++) {
    //     if (g_apSurfaces[i] && g_anSurfLost[i]) {
    //         g_anSurfLost[i] = 0;
    //         void *pPix = DDI_GetSurfPtr(i, &nPitch, &nPad);
    //         if (!pPix) break;
    //         for (int y = 0; y < 0x1e0; y++)
    //             memset(pPix + y * nPitch, 0, 0x280);
    //         DDI_UnlockSurf(i, pPix);
    //     }
    // }
}

// ---------------------------------------------------------------------------
// DDI_EnterCritical -- Acquire g_csLock before locking a surface.
// 0x0041d080
// ---------------------------------------------------------------------------
void DDI_EnterCritical(void)
{
    EnterCriticalSection((LPCRITICAL_SECTION)0x006480a8 /* &g_csLock */);
}

// ---------------------------------------------------------------------------
// DDI_LeaveCritical -- Release g_csLock after unlocking a surface.
// 0x0041d110
// ---------------------------------------------------------------------------
void DDI_LeaveCritical(void)
{
    LeaveCriticalSection((LPCRITICAL_SECTION)0x006480a8 /* &g_csLock */);
}

// ---------------------------------------------------------------------------
// DDI_LockSurf (internal) -- IDirectDrawSurface::Lock for slot param_1.
// Acquires g_csLock. On DDERR_SURFACEBUSY returns NULL without error.
// On DDERR_SURFACELOST calls DDI_RestoreLostSurfs then returns NULL.
// Returns pointer to top-left pixel; writes pitch and extra-pad to out params.
// Debug string: "_ddi_get_ptr int surf RECT* rect ..."
// 0x0041cea0
// ---------------------------------------------------------------------------
void *DDI_LockSurf(int nSlot, void *pRect, int *pOutPitch, int *pOutPad)
{
    // if (!g_nDDInitialized || g_apSurfaces[nSlot] == NULL) return NULL;
    //
    // DDSURFACEDESC desc; desc.dwSize = 0x6c;
    // DDI_EnterCritical();
    //
    // HRESULT hr = g_apSurfaces[nSlot]->Lock(
    //     (LPRECT)pRect, &desc, DDLOCK_WAIT, NULL);   // vtbl+100 (0x64)
    //
    // if (hr == DDERR_SURFACEBUSY) { DDI_LeaveCritical(); return NULL; }
    // if (hr == DDERR_SURFACELOST) {
    //     DDI_RestoreLostSurfs(); DDI_LeaveCritical(); return NULL; }
    // if (hr != DD_OK) {
    //     DDI_LeaveCritical();
    //     --> Err_SetRecord3(0x1a, DDI_ErrorToString(hr), -1) + dispatch;
    // }
    //
    // *pOutPitch = desc.lPitch;
    // *pOutPad   = desc.lPitch - desc.dwWidth;   // unused bytes per row
    // return desc.lpSurface;
    return NULL;
}

// ---------------------------------------------------------------------------
// DDI_GetSurfPtr -- Lock whole surface at nSlot.
// 0x0041cd30
// ---------------------------------------------------------------------------
void *DDI_GetSurfPtr(int nSlot, int *pOutPitch, void *pOutPad)
{
    // return DDI_LockSurf(nSlot, NULL, pOutPitch, (int*)pOutPad);
    return NULL;
}

// ---------------------------------------------------------------------------
// DDI_GetRectPtr -- Lock subrect of surface at nSlot.
// Debug string: "_ddi_get_rect_ptr int surf int x int y int w int h"
// 0x0041cdd0
// ---------------------------------------------------------------------------
void DDI_GetRectPtr(int nSlot, int nX, int nY, int nW, int nH,
                    int *pOutPtr, int *pOutPitch)
{
    // RECT rc; SetRect(&rc, nX, nY, nX+nW, nY+nH);
    // return DDI_LockSurf(nSlot, &rc, pOutPitch, /*pad*/);
}

// ---------------------------------------------------------------------------
// DDI_UnlockSurf -- IDirectDrawSurface::Unlock for slot nSlot.
// Leaves g_csLock.
// Debug string: "ddi_release_ptr int surf uchar* ptr"
// 0x0041d1a0
// ---------------------------------------------------------------------------
void DDI_UnlockSurf(int nSlot, void *pPixels)
{
    // if (!g_nDDInitialized || g_apSurfaces[nSlot] == NULL) return;
    // HRESULT hr = g_apSurfaces[nSlot]->Unlock(pPixels);   // vtbl+0x80
    // DDI_LeaveCritical();
    // if (hr != DDERR_SURFACELOST && hr != DDERR_NOTLOCKED && hr != DD_OK) {
    //     --> Err_SetRecord3 + dispatch
    // }
}

// ---------------------------------------------------------------------------
// DDI_GetScreenPtr -- Lock a rect of g_pPrimary.
// Debug string: "_ddi_get_screen_ptr int x int y int w int h"
// 0x0041d300
// ---------------------------------------------------------------------------
void *DDI_GetScreenPtr(int nX, int nY, int nW, int nH,
                       int *pOutPitch, int *pOutPad)
{
    // if (!g_nDDInitialized) return NULL;
    // RECT rc; SetRect(&rc, nX, nY, nX+nW, nY+nH);
    // DDSURFACEDESC desc; desc.dwSize = 0x6c;
    // DDI_EnterCritical();
    // HRESULT hr = g_pPrimary->Lock(&rc, &desc, DDLOCK_WAIT, NULL); // vtbl+100
    // if (hr == DDERR_SURFACEBUSY || hr == DDERR_SURFACELOST) {
    //     DDI_LeaveCritical(); return NULL; }
    // if (hr != DD_OK) --> error dispatch
    // *pOutPitch = desc.lPitch;
    // *pOutPad   = desc.lPitch - nW;
    // return desc.lpSurface;
    return NULL;
}

// ---------------------------------------------------------------------------
// DDI_ReleaseScreenPtr -- IDirectDrawSurface::Unlock on g_pPrimary.
// Debug string: "ddi_release_screen_ptr uchar* pscreen"
// 0x0041d4e0
// ---------------------------------------------------------------------------
void DDI_ReleaseScreenPtr(void *pPixels)
{
    // if (!g_nDDInitialized) return;
    // HRESULT hr = g_pPrimary->Unlock(pPixels);   // vtbl+0x80
    // DDI_LeaveCritical();
    // if (hr != DDERR_NOTLOCKED && hr != DD_OK) --> error dispatch
}

// ---------------------------------------------------------------------------
// DDI_GetSurfDC -- IDirectDrawSurface::GetDC for surface at nSlot.
// Returns HDC or NULL on error.
// Debug string: "ddi_get_surf_dc int surf"
// 0x0041d620
// ---------------------------------------------------------------------------
HDC DDI_GetSurfDC(int nSlot)
{
    // if (!g_nDDInitialized || g_apSurfaces[nSlot] == NULL) return NULL;
    // DDI_EnterCritical();
    // HDC hDC;
    // HRESULT hr = g_apSurfaces[nSlot]->GetDC(&hDC);    // vtbl+0x44
    // if (hr == DDERR_SURFACELOST) { g_anSurfLost[nSlot]=1; DDI_LeaveCritical(); return NULL; }
    // if (hr == DDERR_SURFACEBUSY) { DDI_LeaveCritical(); return NULL; }
    // if (hr != DD_OK) --> error dispatch
    // return hDC;
    return NULL;
}

// ---------------------------------------------------------------------------
// DDI_ReturnSurfDC -- IDirectDrawSurface::ReleaseDC for surface at nSlot.
// Debug string: "ddi_release_surf_dc int surf HDC hDC"
// 0x0041d7b0
// ---------------------------------------------------------------------------
void DDI_ReturnSurfDC(int nSlot, HDC hDC)
{
    // if (!g_nDDInitialized || g_apSurfaces[nSlot] == NULL) return;
    // HRESULT hr = g_apSurfaces[nSlot]->ReleaseDC(hDC);  // vtbl+0x68
    // DDI_LeaveCritical();
    // if (hr != DD_OK) --> error dispatch
}

// ---------------------------------------------------------------------------
// DDI_WaitVerticalRetrace -- Wait for VBlank if in fullscreen and not minimised.
// Called by SETPAL before palette update.
// Debug string: "ddi_wait_retrace void"
// 0x0041d8f0
// ---------------------------------------------------------------------------
void DDI_WaitVerticalRetrace(void)
{
    if (/* g_nFullscreenActive */ 0)
    {
        WINDOWPLACEMENT wp; wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement((HWND)g_nHwndMain, &wp);
        if (wp.showCmd != SW_SHOWMINIMIZED)   // != 6
        {
            // g_pDDraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL); // vtbl+0x58
        }
    }
}

// ---------------------------------------------------------------------------
// DDI_BltToScreen -- Blt offscreen surface nSrcSlot to primary surface.
// bColorKey != 0 enables color-key (DDBLT_KEYSRC, flag 0x1008000 vs 0x1000000).
// In windowed mode: translate client point to screen coords.
// On DDERR_SURFACELOST: set g_nPrimaryLost=1 and call g_pPrimary->Restore().
// 0x0041c5a0
// ---------------------------------------------------------------------------
void DDI_BltToScreen(HWND hWnd, int nSrcSlot, int nSrcX, int nSrcY,
                     int nW, int nH, int nDstX, int nDstY, int bColorKey)
{
    // if (!g_nDDInitialized || g_apSurfaces[nSrcSlot] == NULL) return;
    //
    // RECT rcDst, rcSrc;
    // SetRect(&rcDst, nDstX, nDstY, nDstX+nW, nDstY+nH);
    // SetRect(&rcSrc, nSrcX, nSrcY, nSrcX+nW, nSrcY+nH);
    //
    // if (g_nSchedDebugMode || DAT_007d6b84) {
    //     POINT pt = {0,0};
    //     ClientToScreen(hWnd, &pt);
    //     OffsetRect(&rcSrc, pt.x, pt.y + DAT_007d6a80);
    // }
    //
    // DWORD dwFlags = bColorKey ? 0x1008000 : 0x1000000;
    // HRESULT hr = g_pPrimary->Blt(&rcDst, g_apSurfaces[nSrcSlot], &rcSrc, dwFlags, NULL);
    //                                                                // vtbl+0x14
    // if (hr == DDERR_WASSTILLDRAWING) return;
    // if (hr == DDERR_SURFACELOST) {
    //     g_pPrimary->Restore();  // vtbl+0x6c
    //     g_nPrimaryLost = 1;
    // } else if (hr != DD_OK) {
    //     Debug_Trace(..., DDI_ErrorToString(hr));
    // }
}

// ---------------------------------------------------------------------------
// DDI_BltSurfToSurf -- Blt from source slot into dest slot.
// Debug string: "ddi_surf_blt int dest_surf int src_surf ..."
// 0x0041c7e0
// ---------------------------------------------------------------------------
void DDI_BltSurfToSurf(int nDstSlot, int nSrcSlot,
                        int nSrcX, int nSrcY, int nW, int nH,
                        int nDstX, int nDstY, int bColorKey)
{
    // if (!g_nDDInitialized || !g_apSurfaces[nSrcSlot] || !g_apSurfaces[nDstSlot]) return;
    //
    // RECT rcSrc, rcDst;
    // SetRect(&rcSrc, nSrcX, nSrcY, nSrcX+nW, nSrcY+nH);
    // SetRect(&rcDst, nDstX, nDstY, nDstX+nW, nDstY+nH);
    //
    // DWORD dwFlags = bColorKey ? 0x1008000 : 0x1000000;
    // HRESULT hr = g_apSurfaces[nDstSlot]->Blt(
    //     &rcDst, g_apSurfaces[nSrcSlot], &rcSrc, dwFlags, NULL);  // vtbl+0x14
    //
    // if (hr == DDERR_WASSTILLDRAWING) return;
    // if (hr == DDERR_SURFACELOST) DDI_RestoreLostSurfs();
    // else if (hr != DD_OK) Debug_Trace(..., DDI_ErrorToString(hr));
}

// ---------------------------------------------------------------------------
// DDI_StretchBlt -- Stretch-blt source slot to dest slot.
// nColorKey == -1: plain async blt. Otherwise: set color key then key-blt.
// Debug string: "ddi_stretch_blt int src_surf int ..."
// 0x0041cae0
// ---------------------------------------------------------------------------
void DDI_StretchBlt(int nSrcSlot, int nDstSlot,
                    int nSrcX, int nSrcY, int nSrcW, int nSrcH,
                    int nDstX, int nDstY, int nDstW, int nDstH,
                    int nColorKey)
{
    // if (!g_nDDInitialized || !g_apSurfaces[nSrcSlot] || !g_apSurfaces[nDstSlot]) return;
    //
    // RECT rcSrc, rcDst;
    // SetRect(&rcSrc, nSrcX, nSrcY, nSrcX+nSrcW, nSrcY+nSrcH);
    // SetRect(&rcDst, nDstX, nDstY, nDstX+nDstW, nDstY+nDstH);
    //
    // if (nColorKey == -1) {
    //     HRESULT hr = g_apSurfaces[nDstSlot]->Blt(&rcDst, g_apSurfaces[nSrcSlot],
    //                                               &rcSrc, DDBLT_ASYNC, NULL);
    //     // 0x1000000
    // } else {
    //     DDCOLORKEY ck = { nColorKey, nColorKey };
    //     HRESULT hr2 = g_apSurfaces[nDstSlot]->SetColorKey(DDCKEY_SRCBLT, &ck); // vtbl+0x74
    //     HRESULT hr  = g_apSurfaces[nDstSlot]->Blt(&rcDst, g_apSurfaces[nSrcSlot],
    //                                               &rcSrc, DDBLT_ASYNC|DDBLT_KEYSRC, NULL);
    //     // 0x1002000
    // }
    // if (hr != DDERR_WASSTILLDRAWING && hr != DD_OK)
    //     Debug_Trace(..., DDI_ErrorToString(hr));
}

// ---------------------------------------------------------------------------
// DDI_SetClipRegion -- Attach a clipper to g_pBackBuffer.
// Pass (-1,-1,-1,-1) to create a window-based clip (IDirectDrawClipper::SetHWnd).
// Otherwise build up to 4 exclusion RECTs and use SetClipList.
// Debug string: "ddi_set_clip int x1 int y1 int x2 int y2"
// 0x0041bea0
// ---------------------------------------------------------------------------
void DDI_SetClipRegion(int nX1, int nY1, int nX2, int nY2)
{
    // HRESULT hr;
    // if (nX1==-1 && nY1==-1 && nX2==-1 && nY2==-1) {
    //     // window clip
    //     hr = g_pBackBuffer->SetClipper(NULL, (HWND)g_nHwndMain);  // vtbl+0x20
    //     if (FAILED(hr)) --> error dispatch
    // } else {
    //     hr = g_pBackBuffer->SetClipper(NULL, NULL);
    //     int nRects = 0;
    //     RECT *pRects = g_aClipRects;  // 0x006481e0
    //     // Build border exclusion rects around the viewport (640x480):
    //     if (nY1 > 0)    add top    rect [0, 0, 640, nY1]
    //     if (nY2 < 479)  add bottom rect [0, nY2, 640, 480]
    //     if (nX1 > 0)    add left   rect [0, nY1_clamped, nX1, nY2+1]
    //     if (nX2 < 639)  add right  rect [nX2, nY1_clamped, 640, nY2+1]
    //     // Fill RGNDATAHEADER at 0x006481c0 (dwSize=0x20, iType=1, nCount, nRgnSize, rcBound)
    //     g_pBackBuffer->SetClipList(&g_aClipListHeader, 0);  // vtbl+0x1c
    // }
}

// ---------------------------------------------------------------------------
// DDI_AttachBackBuffer -- Attach g_pBackBuffer as clipper target on g_pPrimary.
// g_pPrimary->SetClipper(g_pBackBuffer)  -- vtbl+0x70
// 0x0041c2a0
// ---------------------------------------------------------------------------
void DDI_AttachBackBuffer(void)
{
    // g_pPrimary->SetClipper(g_pBackBuffer);  // vtbl+0x70
}

// ---------------------------------------------------------------------------
// DDI_DetachBackBuffer -- Detach back buffer from g_pPrimary.
// g_pPrimary->SetClipper(NULL)  -- vtbl+0x70
// 0x0041c200
// ---------------------------------------------------------------------------
void DDI_DetachBackBuffer(void)
{
    // g_pPrimary->SetClipper(NULL);  // vtbl+0x70
}

// ---------------------------------------------------------------------------
// DDI_SetBackBufferTarget -- same as DDI_AttachBackBuffer but distinct call site.
// 0x0041e600
// ---------------------------------------------------------------------------
void DDI_SetBackBufferTarget(void)
{
    // g_pPrimary->SetClipper(g_pBackBuffer);  // vtbl+0x70
}

// ---------------------------------------------------------------------------
// DDI_ClearBackBufferTarget -- same as DDI_DetachBackBuffer but distinct call site.
// 0x0041e6a0
// ---------------------------------------------------------------------------
void DDI_ClearBackBufferTarget(void)
{
    // g_pPrimary->SetClipper(NULL);  // vtbl+0x70
}

// ---------------------------------------------------------------------------
// DDI_SurfLock_Acquire -- Initialise a SurfLock object and lock the surface.
// struct SurfLock { int nSlot; int nPitch; int nPad; void *pPixels; };
// If nSlot == -1 locks the primary (screen) surface; otherwise locks offscreen slot.
// 0x0041e970
// ---------------------------------------------------------------------------
int *DDI_SurfLock_Acquire(int *pLock, int nSlot)
{
    pLock[0] = nSlot;
    if (nSlot == -1)
        pLock[3] = (int)DDI_GetScreenPtr(0, 0, 0x280, 0x1e0,
                                          &pLock[1], &pLock[2]);
    else
        pLock[3] = (int)DDI_GetSurfPtr(nSlot, &pLock[1], (void *)&pLock[2]);
    return pLock;
}

// ---------------------------------------------------------------------------
// DDI_SurfLock_Switch -- Unlock current surface, switch to new slot, re-lock.
// 0x0041ea00
// ---------------------------------------------------------------------------
void DDI_SurfLock_Switch(int *pLock, int nNewSlot)
{
    if (pLock[3] != 0)
        DDI_UnlockSurf(/* old slot */ pLock[0], (void *)pLock[3]);
    pLock[0] = nNewSlot;
    if (nNewSlot == -1)
        pLock[3] = (int)DDI_GetScreenPtr(0, 0, 0x280, 0x1e0,
                                          &pLock[1], &pLock[2]);
    else
        pLock[3] = (int)DDI_GetSurfPtr(nNewSlot, &pLock[1], (void *)&pLock[2]);
}

// ---------------------------------------------------------------------------
// DDI_SurfLock_Release -- Unlock the surface held in a SurfLock.
// 0x0041eab0
// ---------------------------------------------------------------------------
void DDI_SurfLock_Release(int *pLock)
{
    if (pLock[3] != 0)
    {
        if (pLock[0] == -1)
            DDI_ReleaseScreenPtr((void *)pLock[3]);
        else
            DDI_UnlockSurf(pLock[0], (void *)pLock[3]);
    }
}

// ---------------------------------------------------------------------------
// DDI_StartDissolveEffect -- Initialise and schedule a pixel-dissolve transition.
//
// The source sprite is pointed to by g_pDissolveSrc (0x0069f2ac).
// If g_nDissolveStep == -1 (first call):
//   - decode sprite into g_abDissolveBuf (0x006542a0)
//   - compute clipped dimensions (max 128x96 pixels)
//   - build a shuffled order array in g_aDissolveOrder (0x00648290)
// Schedule DDI_DissolveEffectTick via thunk_FUN_0042fbe0.
// 0x0041eb10
// ---------------------------------------------------------------------------
void DDI_StartDissolveEffect(void)
{
    // if (g_pDissolveSrc == 0) return;
    //
    // if (g_nDissolveStep == -1) {
    //     // First call: decode sprite
    //     memset(g_abDissolveBuf, 0, 0x4b000);
    //     g_nDissolveSrcW = *(short*)(g_pDissolveSrc + 1);
    //     g_nDissolveSrcH = *(short*)(g_pDissolveSrc + 3);
    //     thunk_FUN_0042c030(g_pDissolveSrc, g_abDissolveBuf, 0, 0,
    //                        g_nDissolveSrcW, g_nDissolveSrcH);
    //     g_nDissolveSpriteW = min(g_nDissolveSrcW, 0x80);  // 128
    //     g_nDissolveSpriteH = min(g_nDissolveSrcH, 0x60);  // 96
    //     g_nDissolveTotal   = g_nDissolveSpriteW * g_nDissolveSpriteH;
    //
    //     // Build sequential then Fisher-Yates shuffle
    //     for (int i = 0; i < g_nDissolveTotal; i++)
    //         g_aDissolveOrder[i] = i;
    //     for (int i = 0; i < g_nDissolveTotal; i++) {
    //         int j = FUN_00489cf0() % g_nDissolveTotal;
    //         swap(g_aDissolveOrder[i], g_aDissolveOrder[j]);
    //     }
    // }
    // thunk_FUN_0042fbe0(DDI_DissolveEffectTick, 0);
}

// ---------------------------------------------------------------------------
// DDI_DissolveEffectTick -- Timer callback; reveals a proportional slice of
// pixels per tick. Calls thunk_FUN_00430300 to blit each non-zero pixel from
// g_abDissolveBuf to the screen at (g_nDissolveDstX, g_nDissolveDstY).
// When all pixels are revealed: clears g_pDissolveSrc.
// 0x0041ed60
// ---------------------------------------------------------------------------
void DDI_DissolveEffectTick(int nTimerIdx, int nParam)
{
    // g_nDissolveStep++;
    //
    // int nStart = (g_nDissolveTotal * (g_nDissolveStep % g_nDissolveSteps))
    //               / g_nDissolveSteps;
    // int nEnd   = (g_nDissolveTotal * (g_nDissolveStep % g_nDissolveSteps + 1))
    //               / g_nDissolveSteps;
    //
    // if (nEnd >= g_nDissolveTotal) {
    //     nEnd = g_nDissolveTotal;
    //     g_pDissolveSrc = 0;
    //     g_nDissolveStep = g_nDissolveSteps * 2;  // mark done
    // }
    //
    // for (int i = nStart; i < nEnd; i++) {
    //     int idx = g_aDissolveOrder[i];
    //     // Tile the clipped block over the full sprite dimensions
    //     for (int tileX = 0; tileX < ceildiv(g_nDissolveSrcW, g_nDissolveSpriteW); tileX++) {
    //         for (int tileY = 0; tileY < ceildiv(g_nDissolveSrcH, g_nDissolveSpriteH); tileY++) {
    //             int px = idx % g_nDissolveSpriteW + tileX * g_nDissolveSpriteW;
    //             int py = idx / g_nDissolveSpriteW + tileY * g_nDissolveSpriteH;
    //             if (px < g_nDissolveSrcW && py < g_nDissolveSrcH) {
    //                 byte col = g_abDissolveBuf[px + py * g_nDissolveSrcW];
    //                 if (col != 0)
    //                     thunk_FUN_00430300(nTimerIdx,
    //                                        px + g_nDissolveDstX,
    //                                        py + g_nDissolveDstY,
    //                                        (int)col, nParam);
    //             }
    //         }
    //     }
    // }
}

// ---------------------------------------------------------------------------
// DDI_ErrorToString -- Translate a DDERR_* HRESULT to a string literal.
// Returns "Unknown" for unrecognised codes.
// 0x0041dad0
// ---------------------------------------------------------------------------
const char *DDI_ErrorToString(int hResult)
{
    switch (hResult)
    {
    case -0x7fffbffb: return "DDERR_GENERIC";
    case -0x7fffbfff: return "DDERR_UNSUPPORTED";
    case -0x7ff8ffa9: return "DDERR_INVALIDPARAMS";
    case -0x7ff8fff2: return "DDERR_OUTOFMEMORY";
    case -0x7789fff6: return "DDERR_CANNOTATTACHSURFACE";
    case -0x7789fffb: return "DDERR_ALREADYINITIALIZED";
    case -0x7789ffd8: return "DDERR_CURRENTLYNOTAVAIL";
    case -0x7789ffec: return "DDERR_CANNOTDETACHSURFACE";
    case -0x7789ffa6: return "DDERR_HEIGHTALIGN";
    case -0x7789ffc9: return "DDERR_EXCEPTION";
    case -0x7789ff9c: return "DDERR_INVALIDCAPS";
    case -0x7789ffa1: return "DDERR_INCOMPATIBLEPRIMARY";
    case -0x7789ff88: return "DDERR_INVALIDMODE";
    case -0x7789ff92: return "DDERR_INVALIDCLIPLIST";
    case -0x7789ff6f: return "DDERR_INVALIDPIXELFORMAT";
    case -0x7789ff7e: return "DDERR_INVALIDOBJECT";
    case -0x7789ff60: return "DDERR_LOCKEDSURFACES";
    case -0x7789ff6a: return "DDERR_INVALIDRECT";
    case -0x7789ff4c: return "DDERR_NOALPHAHW";
    case -0x7789ff56: return "DDERR_NO3D";
    case -0x7789ff2e: return "DDERR_NOCOLORCONVHW";
    case -0x7789ff33: return "DDERR_NOCLIPLIST";
    case -0x7789ff29: return "DDERR_NOCOLORKEY";
    case -0x7789ff2c: return "DDERR_NOCOOPERATIVELEVELSET";
    case -0x7789ff1a: return "DDERR_NOFLIPHW";
    case -0x7789ff24: return "DDERR_NOCOLORKEYHW";
    case -0x7789ff22: return "DDERR_NODIRECTDRAWSUPPORT";
    case -0x7789ff1f: return "DDERR_NOEXCLUSIVEMODE";
    case -0x7789ff06: return "DDERR_NOMIRRORHW";
    case -0x7789ff10: return "DDERR_NOGDI";
    case -0x7789fefc: return "DDERR_NOOVERLAYHW";
    case -0x7789ff01: return "DDERR_NOTFOUND";
    case -0x7789fede: return "DDERR_NOROTATIONHW";
    case -0x7789fee8: return "DDERR_NORASTEROPHW";
    case -0x7789fec4: return "DDERR_NOT4BITCOLOR";
    case -0x7789feca: return "DDERR_NOSTRETCHHW";
    case -0x7789fec0: return "DDERR_NOT8BITCOLOR";
    case -0x7789fec3: return "DDERR_NOT4BITCOLORINDEX";
    case -0x7789feb1: return "DDERR_NOVSYNCHW";
    case -0x7789feb6: return "DDERR_NOTEXTUREHW";
    case -0x7789feac: return "DDERR_NOZBUFFERHW";
    case -0x7789fea2: return "DDERR_NOZOVERLAYHW";
    case -0x7789fe98: return "DDERR_OUTOFCAPS";
    case -0x7789fe84: return "DDERR_OUTOFVIDEOMEMORY";
    case -0x7789fe82: return "DDERR_OVERLAYCANTCLIP";
    case -0x7789fe80: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
    case -0x7789fe7d: return "DDERR_PALETTEBUSY";
    case -0x7789fe70: return "DDERR_COLORKEYNOTSET";
    case -0x7789fe66: return "DDERR_SURFACEALREADYATTACHED";
    case -0x7789fe5c: return "DDERR_SURFACEALREADYDEPENDENT";
    case -0x7789fe52: return "DDERR_SURFACEBUSY";
    case -0x7789fe48: return "DDERR_SURFACEISOBSCURED";
    case -0x7789fe3e: return "DDERR_SURFACELOST";
    case -0x7789fe34: return "DDERR_SURFACENOTATTACHED";
    case -0x7789fe2a: return "DDERR_TOOBIGHEIGHT";
    case -0x7789fe20: return "DDERR_TOOBIGSIZE";
    case -0x7789fe16: return "DDERR_TOOBIGWIDTH";
    case -0x7789fe02: return "DDERR_UNSUPPORTEDFORMAT";
    case -0x7789fdf8: return "DDERR_UNSUPPORTEDMASK";
    case -0x7789fde7: return "DDERR_VERTICALBLANKINPROGRESS";
    case -0x7789fde4: return "DDERR_WASSTILLDRAWING";
    case -0x7789fdd0: return "DDERR_XALIGN";
    case -0x7789fdcf: return "DDERR_INVALIDDIRECTDRAWGUID";
    case -0x7789fdce: return "DDERR_DIRECTDRAWALREADYCREATED";
    case -0x7789fdcd: return "DDERR_NODIRECTDRAWHW";
    case -0x7789fdcc: return "DDERR_PRIMARYSURFACEALREADYEXISTS";
    case -0x7789fdcb: return "DDERR_NOEMULATION";
    case -0x7789fdca: return "DDERR_REGIONTOOSMALL";
    case -0x7789fdc9: return "DDERR_CLIPPERISUSINGHWND";
    case -0x7789fdc8: return "DDERR_NOCLIPPERATTACHED";
    case -0x7789fdc7: return "DDERR_NOHWND";
    case -0x7789fdc6: return "DDERR_HWNDSUBCLASSED";
    case -0x7789fdc5: return "DDERR_HWNDALREADYSET";
    case -0x7789fdc4: return "DDERR_NOPALETTEATTACHED";
    case -0x7789fdc3: return "DDERR_NOPALETTEHW";
    case -0x7789fdc2: return "DDERR_BLTFASTCANTCLIP";
    case -0x7789fdc1: return "DDERR_NOBLTHW";
    case -0x7789fdc0: return "DDERR_NODDROPSHW";
    case -0x7789fdbf: return "DDERR_OVERLAYNOTVISIBLE";
    case -0x7789fdbe: return "DDERR_NOOVERLAYDEST";
    case -0x7789fdbd: return "DDERR_INVALIDPOSITION";
    case -0x7789fdbc: return "DDERR_NOTAOVERLAYSURFACE";
    case -0x7789fdbb: return "DDERR_EXCLUSIVEMODEALREADYSET";
    case -0x7789fdba: return "DDERR_NOTFLIPPABLE";
    case -0x7789fdb9: return "DDERR_CANTDUPLICATE";
    case -0x7789fdb8: return "DDERR_NOTLOCKED";
    case -0x7789fdb7: return "DDERR_CANTCREATEDC";
    case -0x7789fdb6: return "DDERR_NODC";
    case -0x7789fdb5: return "DDERR_WRONGMODE";
    case -0x7789fdb4: return "DDERR_IMPLICITLYCREATED";
    case -0x7789fdb3: return "DDERR_NOTPALETTIZED";
    case -0x7789fdb2: return "DDERR_UNSUPPORTEDMODE";
    default:          return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Err_FatalFileCorrupt -- Show "ERRORS.TXT is corrupt" fatal dialog.
// This function physically sits at the boundary between DDRAWI and ERRORS.
// The debug string "C:\DevStudio\Projects\Crux\ERRORS.cpp" confirms it
// logically belongs to ERRORS.cpp; it is defined here because it falls
// within this RE address pass.
// 0x0041efe0
// ---------------------------------------------------------------------------
void Err_FatalFileCorrupt(int nLine, int nParam)
{
    // Err_FatalWithMessage(nLine, nParam, "File error: ERRORS.TXT is corrupt");
    // ExitProcess(0);  -- does not return
}

// ---------------------------------------------------------------------------
// Err_FatalWithMessage -- Format pszMsg into a buffer, show two fatal dialogs,
// then call ExitProcess(0).  Does not return.
// Debug strings: "C:\DevStudio\Projects\Crux\ERRORS.cpp"
// 0x0041f010
// ---------------------------------------------------------------------------
void Err_FatalWithMessage(int nLine, int nParam, const char *pszMsg)
{
    // char szBuf[1000];
    // Debug_Trace(nLine, "C:\\DevStudio\\Projects\\Crux\\ERRORS.cpp",
    //             "Error in %s %d", nParam, nLine);
    // thunk_FUN_00483490(nLine, "C:\\DevStudio\\Projects\\Crux\\ERRORS.cpp");
    // FUN_0048a6a0(szBuf, pszMsg, ...);   // vsprintf
    // MessageBoxA(NULL, szBuf, g_pErrStrings[1], g_nDebugFlags | 0x10010);
    // FUN_0048a060(szBuf, "%s %s -> %d", g_pErrStrings[2], nParam, nLine);
    // MessageBoxA(NULL, szBuf, "...", g_nDebugFlags | 0x10010);
    // ExitProcess(0);
}
