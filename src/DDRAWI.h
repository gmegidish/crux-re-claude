#pragma once
// ---------------------------------------------------------------------------
// DDRAWI.h -- DirectDraw hardware abstraction layer for CRUX.EXE (Win95)
//
// This module owns the IDirectDraw object, primary surface, back buffer,
// offscreen surface pool (up to 10 slots), clipper, and the surface
// critical section used to synchronise Lock/Unlock across threads.
//
// The game runs 320x200 (displayed as 640x480) in exclusive fullscreen
// DirectDraw mode; surfaces are 8-bit palettised (DDSCAPS_OFFSCREENPLAIN).
//
// Address range: 0x0041abb0 -- 0x0041f10f
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>

// ---------------------------------------------------------------------------
// Globals (defined in DDRAWI.cpp)
// ---------------------------------------------------------------------------

// DirectDraw COM objects
extern IDirectDraw        *g_pDDraw;         // 0x00648220  IDirectDraw*
extern IDirectDrawSurface *g_pPrimary;       // 0x00648224  front/primary surface
extern IDirectDrawSurface *g_pBackBuffer;    // 0x00648228  back buffer surface

// Offscreen surface pool  [0..9]
// g_apSurfaces[i] = IDirectDrawSurface*   at 0x006480c0 + i*4
// g_anSurfLost[i] = lost flag (1=needs restore) at 0x00648190 + i*4
// Per-slot surface descriptors at 0x006480e0 + i*0x10:
//   +0x08  nWidth    (pixels)
//   +0x0c  nHeight   (pixels)
//   +0x10  nHasColorKey (non-zero = surface uses color keying)
//   +0x14  nIsOverlay   (non-zero = overlay surface)

// State
extern int                 g_nDDInitialized; // 0x0064822c  non-zero after DDI_InitDirectDraw
extern int                 g_nFullscreenActive; // 0x00648230  non-zero = 640x480 exclusive mode active
extern int                 g_nPrimaryLost;   // 0x00648188  non-zero = primary surface lost
extern HWND                g_hWnd;           // 0x006481b8  main window HWND

// Clip rect array used by DDI_SetClipRegion
// RECT  g_aClipRects[4]  at 0x006481e0  (4 * sizeof(RECT) = 64 bytes)
// Clip list header at 0x006481c0 (DDRGN_RGNDATA-style, size=0x20)

// Critical section protecting Lock/Unlock
// CRITICAL_SECTION  g_csLock  at 0x006480a8

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// DDI_InitDirectDraw -- One-time init. Calls DirectDrawCreate, sets cooperative
// level, sets display mode (640x480x8), creates primary+back surfaces and
// clipper. param_1 = HWND of the main window.
// Debug string: "ddi_init HWND hwnd"
// 0x0041b7c0
void DDI_InitDirectDraw(HWND hWnd);

// DDI_SetDisplayMode -- (Re-)apply SetCooperativeLevel(DDSCL_EXCLUSIVE) +
// SetDisplayMode(640, 480, 8) for the stored hWnd. No-op in sched-debug or
// fullscreen mode. Sets g_nFullscreenActive on success.
// Debug string: "ddi_set_mode"
// 0x0041c340
void DDI_SetDisplayMode(void);

// DDI_SetFullscreenMode -- Restore cooperative level to DDSCL_NORMAL
// (IDirectDraw::RestoreDisplayMode). Clears g_nFullscreenActive on success.
// 0x0041b6d0
void DDI_SetFullscreenMode(void);

// DDI_CheckFullscreenMode -- Re-check whether fullscreen is still active;
// calls RestoreDisplayMode if needed. No-op in sched-debug / non-fullscreen.
// 0x0041d9c0
void DDI_CheckFullscreenMode(void);

// ---------------------------------------------------------------------------
// Surface management
// ---------------------------------------------------------------------------

// DDI_CreateOverlaySurf -- Allocate a new slot in g_apSurfaces[], create an
// offscreen (non-overlay-flag) DirectDraw surface of param_1 x param_2 pixels,
// lock it and zero the pixels, store pointer. Returns slot index or -1 on
// failure.
// Debug string: "ddi_create_overlay_surf int width int height"
// 0x0041b2c0
int  DDI_CreateOverlaySurf(int nWidth, int nHeight);

// DDI_RecreateOffscreenSurf -- Re-create the surface for slot param_1
// (called after a surface-lost event). Releases old surface, creates new one,
// zeroes pixels.
// Debug string: "ddi_recreate_offscreen_surf int n"
// 0x0041abb0
void DDI_RecreateOffscreenSurf(int nSlot);

// DDI_ReleaseSurf -- Release (IUnknown::Release) the surface at slot param_1
// and zero the pointer.
// 0x0041b610
void DDI_ReleaseSurf(int nSlot);

// DDI_RefreshSurfs -- Re-create primary + back buffer after a display-mode
// change. Releases old surfaces, creates new primary with attached back
// buffer, re-clips, then iterates all non-overlay slots and calls
// DDI_RecreateOffscreenSurf.
// Debug string: "ddi_refresh_surfs"
// 0x0041af50
void DDI_RefreshSurfs(void);

// DDI_RestoreLostSurfs -- Scan g_apSurfaces[]; for any slot where IsLost()
// returns non-zero: call Restore() and mark g_anSurfLost[i]=1.
// 0x0041c9d0
void DDI_RestoreLostSurfs(void);

// DDI_ClearLostSurfs -- Check g_nPrimaryLost; if set, lock+zero the primary
// surface. Then scan g_anSurfLost[]; for any flagged slot, lock+zero and
// clear the flag. Used during scene transitions to repaint recovered surfaces.
// Debug string: "ddi_clear_lost_surfs"
// 0x0041e740
void DDI_ClearLostSurfs(void);

// ---------------------------------------------------------------------------
// Lock / Unlock (offscreen surfaces)
// ---------------------------------------------------------------------------

// DDI_LockSurf -- Lock surface at slot param_1. Wrapper: calls DDI_GetSurfPtr.
// Returns pixel pointer; *pOutPitch = pitch in bytes; *pOutPad = stride-width.
// Debug string: "_ddi_get_ptr int surf RECT* rect ..."
// 0x0041cd30
void *DDI_GetSurfPtr(int nSlot, int *pOutPitch, void *pOutPad);

// DDI_GetRectPtr -- Lock a subrect of surface slot param_1.
// Builds a RECT from (x, y, w, h) then calls DDI_LockSurf.
// Debug string: "_ddi_get_rect_ptr int surf int x int y int w int h"
// 0x0041cdd0
void DDI_GetRectPtr(int nSlot, int nX, int nY, int nW, int nH,
                    int *pOutPtr, int *pOutPitch);

// DDI_LockSurf (internal) -- IDirectDrawSurface::Lock with retry on DDERR_SURFACEBUSY.
// On DDERR_SURFACELOST calls DDI_RestoreLostSurfs. Acquires g_csLock.
// 0x0041cea0
void *DDI_LockSurf(int nSlot, void *pRect, int *pOutPitch, int *pOutPad);

// DDI_UnlockSurf -- IDirectDrawSurface::Unlock for slot param_1. Leaves g_csLock.
// Debug string: "ddi_release_ptr int surf uchar* ptr"
// 0x0041d1a0
void DDI_UnlockSurf(int nSlot, void *pPixels);

// ---------------------------------------------------------------------------
// Lock / Unlock (primary / screen)
// ---------------------------------------------------------------------------

// DDI_GetScreenPtr -- Lock a rect of the primary surface g_pPrimary.
// Returns pointer to top-left pixel. *pOutPitch = pitch; *pOutPad = extra.
// Debug string: "_ddi_get_screen_ptr int x int y int w int h"
// 0x0041d300
void *DDI_GetScreenPtr(int nX, int nY, int nW, int nH,
                       int *pOutPitch, int *pOutPad);

// DDI_ReleaseScreenPtr -- IDirectDrawSurface::Unlock on g_pPrimary.
// Debug string: "ddi_release_screen_ptr uchar* pscreen"
// 0x0041d4e0
void DDI_ReleaseScreenPtr(void *pPixels);

// ---------------------------------------------------------------------------
// GDI DC access
// ---------------------------------------------------------------------------

// DDI_GetSurfDC -- IDirectDrawSurface::GetDC for slot param_1.
// Returns HDC or 0 on error.
// Debug string: "ddi_get_surf_dc int surf"
// 0x0041d620
HDC  DDI_GetSurfDC(int nSlot);

// DDI_ReturnSurfDC -- IDirectDrawSurface::ReleaseDC for slot param_1.
// Debug string: "ddi_release_surf_dc int surf HDC hDC"
// 0x0041d7b0
void DDI_ReturnSurfDC(int nSlot, HDC hDC);

// ---------------------------------------------------------------------------
// Blit operations
// ---------------------------------------------------------------------------

// DDI_BltToScreen -- Blt offscreen surface param_2 to the primary surface
// (g_pPrimary), with optional color-keying (param_9 != 0 enables DDBLT_KEYSRC).
// In windowed/debug mode, translates client coords to screen coords first.
// 0x0041c5a0
void DDI_BltToScreen(HWND hWnd, int nSrcSlot, int nSrcX, int nSrcY,
                     int nW, int nH, int nDstX, int nDstY, int bColorKey);

// DDI_BltSurfToSurf -- Blt from surface slot param_2 into slot param_1.
// param_9 != 0 enables color-keying. On DDERR_SURFACELOST calls
// DDI_RestoreLostSurfs.
// Debug string: "ddi_surf_blt int dest_surf int src_surf ..."
// 0x0041c7e0
void DDI_BltSurfToSurf(int nDstSlot, int nSrcSlot,
                        int nSrcX, int nSrcY, int nW, int nH,
                        int nDstX, int nDstY, int bColorKey);

// DDI_StretchBlt -- Stretch-blt from slot param_1 into slot param_2.
// If param_11 == -1 uses DDBLT_ASYNC; otherwise sets color key via
// IDirectDrawSurface::SetColorKey then uses DDBLT_ASYNC|DDBLT_KEYSRC.
// Debug string: "ddi_stretch_blt int src_surf int ..."
// 0x0041cae0
void DDI_StretchBlt(int nSrcSlot, int nDstSlot,
                    int nSrcX, int nSrcY, int nSrcW, int nSrcH,
                    int nDstX, int nDstY, int nDstW, int nDstH,
                    int nColorKey);

// ---------------------------------------------------------------------------
// Clip management
// ---------------------------------------------------------------------------

// DDI_SetClipRegion -- Build an exclusion region around the viewport and
// attach it to g_pBackBuffer via IDirectDrawSurface::SetClipper.
// Pass (-1,-1,-1,-1) to attach a window-clip (IDirectDrawClipper using hWnd).
// Debug string: "ddi_set_clip int x1 int y1 int x2 int y2"
// 0x0041bea0
void DDI_SetClipRegion(int nX1, int nY1, int nX2, int nY2);

// DDI_AttachBackBuffer -- Attach g_pBackBuffer to g_pPrimary via SetClipper.
// 0x0041c2a0
void DDI_AttachBackBuffer(void);

// DDI_DetachBackBuffer -- Detach back buffer (SetClipper(NULL) on g_pPrimary).
// 0x0041c200
void DDI_DetachBackBuffer(void);

// DDI_SetBackBufferTarget -- SetClipper on g_pPrimary, targeting g_pBackBuffer.
// 0x0041e600
void DDI_SetBackBufferTarget(void);

// DDI_ClearBackBufferTarget -- SetClipper on g_pPrimary with NULL target.
// 0x0041e6a0
void DDI_ClearBackBufferTarget(void);

// ---------------------------------------------------------------------------
// Sync
// ---------------------------------------------------------------------------

// DDI_WaitVerticalRetrace -- IDirectDraw::WaitForVerticalBlank if
// g_nFullscreenActive and window is not minimised.
// Debug string: "ddi_wait_retrace void"
// 0x0041d8f0
void DDI_WaitVerticalRetrace(void);

// DDI_EnterCritical -- EnterCriticalSection(&g_csLock). Called before Lock.
// 0x0041d080
void DDI_EnterCritical(void);

// DDI_LeaveCritical -- LeaveCriticalSection(&g_csLock). Called after Unlock.
// 0x0041d110
void DDI_LeaveCritical(void);

// ---------------------------------------------------------------------------
// SurfLock helper object (struct { int nSlot; int nPitch; int nPad; void *pPixels; })
// Used by higher-level modules to keep a lock alive across a function call.
// ---------------------------------------------------------------------------

// DDI_SurfLock_Acquire -- Init a SurfLock struct for slot param_2 and lock.
// 0x0041e970
int *DDI_SurfLock_Acquire(int *pLock, int nSlot);

// DDI_SurfLock_Switch -- Unlock current, then lock new slot param_2.
// 0x0041ea00
void DDI_SurfLock_Switch(int *pLock, int nSlot);

// DDI_SurfLock_Release -- Unlock and release the SurfLock.
// 0x0041eab0
void DDI_SurfLock_Release(int *pLock);

// ---------------------------------------------------------------------------
// Dissolve effect
// ---------------------------------------------------------------------------

// DDI_StartDissolveEffect -- Initialise and kick off a pixel-dissolve transition.
// Reads the source sprite from g_pDissolveSrc (0x0069f2ac), builds a
// shuffled index array g_aDissolveOrder (0x00648290) then schedules
// DDI_DissolveEffectTick via the SCHED timer system.
// 0x0041eb10
void DDI_StartDissolveEffect(void);

// DDI_DissolveEffectTick -- Timer callback; reveals a proportional batch of
// pixels each tick. Calls thunk_FUN_00430300 to blit each pixel from the
// source. Stops and clears g_pDissolveSrc when all pixels are revealed.
// 0x0041ed60
void DDI_DissolveEffectTick(int nTimerIdx, int nParam);

// ---------------------------------------------------------------------------
// Error helpers (tail of DDRAWI.cpp; these two belong to ERRORS module)
// ---------------------------------------------------------------------------

// DDI_ErrorToString -- Translate a DDERR_* HRESULT into a human-readable
// C string. Returns "Unknown" for unrecognised codes.
// 0x0041dad0
const char *DDI_ErrorToString(int hResult);

// Err_FatalFileCorrupt -- Show "File error: ERRORS.TXT is corrupt" message
// and call ExitProcess(0). Wrapper around Err_FatalWithMessage.
// 0x0041efe0
void Err_FatalFileCorrupt(int nLine, int nParam);

// Err_FatalWithMessage -- Format param_3 into a 1000-byte buffer, show two
// MessageBoxA dialogs (user-facing + technical), then ExitProcess(0).
// Debug strings reference "C:\DevStudio\Projects\Crux\ERRORS.cpp".
// NOTE: This function physically lives at the boundary with ERRORS.cpp
//       (next function is 0x0041f120 = Err_LoadStrings). The debug string
//       "C:\DevStudio\Projects\Crux\ERRORS.cpp" confirms it belongs to
//       ERRORS, not DDRAWI. It is declared here because it falls within
//       the address range assigned to this RE pass.
// 0x0041f010
void Err_FatalWithMessage(int nLine, int nParam, const char *pszMsg);
