#pragma once

// GI.cpp — Graphics Interface layer
// Sits between high-level game logic and DDRAWI.cpp (raw DirectDraw).
// Manages a double-buffered 640x480 (0x280x0x1E0) 8-bpp surface pipeline,
// a GDI clip region, palette remapping helpers, a 2-D draw library
// (pixel read/write, line, flood-fill), and the GV toolbar button update.
//
// Draw modes (g_nGIDrawMode):
//   0 = overlay surface (g_dwGIOverlaySurf)
//   1 = back-buffer  (g_apGIBackbufs[g_nGIPageIndex ^ 1])
//   2 = front-buffer (g_apGIBackbufs[g_nGIPageIndex])

// -------------------------------------------------------------------------
// Globals
// -------------------------------------------------------------------------

// Draw-mode selector (0/1/2)
extern int       g_nGIDrawMode;

// Double-buffer surface handles
extern uint      g_apGIBackbufs[2];
extern int       g_nGIPageIndex;       // current front-buffer index into g_apGIBackbufs
extern int       g_nGILastBltPageIndex;

// Overlay and dev surfaces
extern uint      g_dwGIOverlaySurf;
extern uint      g_dwGIDevSurf;
extern int       g_nGIDevSurfEnabled;

// Surface currently held locked (for the unified Unlock wrappers)
extern uint      g_dwGICurrentLockedSurf;

// Page-flip control
extern int       g_nGIFlipPending;

// Screen window offsets (client-to-screen translation)
extern int       g_nGIScreenOffsetX;
extern int       g_nGIScreenOffsetY;

// GDI clip region
extern uint      g_dwGIClipRgn;        // HRGN

// Clipper rectangle (screen-space)
extern int       g_nGIClipperActive;
extern int       g_nGIClipX1;
extern int       g_nGIClipY1;
extern int       g_nGIClipX2;
extern int       g_nGIClipY2;

// Readiness gate
extern int       g_nGIReady;

// 1-scanline black surface used by GI_ClearBorder (lazy-init, -1 = not yet created)
extern int       g_nGIClearLineSurf;

// Palette tables
extern byte      g_abGIGeneralPal[768];    // GENERAL.PAL as RGB triplets
extern int       g_anGISnapshotPalMap[256];// snapshot→active palette index map

// Flood-fill state
extern int       g_nGIFloodFillStackDepth;
extern byte      g_abGIFloodFillTargetColor; // color at seed point (to replace)
extern byte      g_abGIFloodFillColor;        // replacement fill color
extern byte      g_abGIFloodFillMode;          // 0 = match target, 1 = match fill
extern short     g_anGIFloodFillStackX[512];
extern short     g_anGIFloodFillStackY[512];

// -------------------------------------------------------------------------
// Window / mode setup
// -------------------------------------------------------------------------

void  GI_CalcRegion(void);               // build GDI clip region for main DC
void  GI_WaitForReady(void);             // spin-wait on g_nGIReady
void  GI_SetDrawMode(char mode);         // set g_nGIDrawMode (0/1/2)
void  GI_SetPageFlip(int pending);       // set g_nGIFlipPending; toggle page if was 0

// -------------------------------------------------------------------------
// Surface locking (select active surface into DDI_SurfLock)
// -------------------------------------------------------------------------

void  GI_LockActiveSurf(void);           // 0x0042bd40 — canonical implementation
void  GI_LockActiveSurf_Thunk(void);     // 0x0042c0e0 — thin forwarding thunk
void  GI_LockActiveSurf_Debug(void);     // 0x0042c180 — debug-mode variant
void  GI_LockActiveSurf_v2(void);        // 0x0042d130
void  GI_LockActiveSurf_v3(void);        // 0x0042d2a0
void  GI_LockActiveSurf_v4(void);        // 0x0042d730
void  GI_LockActiveSurf_v5(void);        // 0x0042dad0
void  GI_LockActiveSurf_v6(void);        // 0x0042ec30
void  GI_LockActiveSurf_v7(void);        // 0x0042ee10
void  GI_LockActiveSurf_v8(void);        // 0x0042f6a0
void  GI_LockActiveSurf_v9(void);        // 0x0042f860
void  GI_LockActiveSurf_v10(void);       // 0x0042fa30
void  GI_LockActiveSurf_v11(void);       // 0x0042fbe0

// -------------------------------------------------------------------------
// Clipper
// -------------------------------------------------------------------------

void  GI_SetClipper(int x1, int y1, int x2, int y2); // enable clip rect (client coords)
void  GI_ClearClipper(void);                           // disable clip rect

// -------------------------------------------------------------------------
// Blit / flip pipeline
// -------------------------------------------------------------------------

void  GI_PutImgToScreen(int x, int y, uint hResource); // blit resource at (x,y)
void  GI_BlitResource(uint p1, uint p2, uint p3, uint p4, uint p5, uint p6);
void  GI_FlipToScreen(int y1, int y2);    // main page-flip + DDI_BltToScreen
void  GI_BltRegionToScreen(int x1, int y1, int x2, int y2); // blit rect to screen
void  GI_CopyBackbufToOverlay(void);      // DDI_BltSurfToSurf backbuf→overlay (full frame)

// -------------------------------------------------------------------------
// Surface accessor wrappers
// (back-buffer = g_apGIBackbufs[g_nGIPageIndex^1], front = g_apGIBackbufs[g_nGIPageIndex])
// -------------------------------------------------------------------------

void  GI_LockBackbuf(uint pitch, uint height);
void  GI_UnlockBackbuf(uint ptr);
void  GI_LockFrontbuf(uint pitch, uint height);
void  GI_UnlockFrontbuf(uint ptr);
void  GI_LockDevSurf(uint pitch, uint height);
void  GI_UnlockDevSurf(uint ptr);
void  GI_LockOverlaySurf(uint pitch, uint height);
uint  GI_GetOverlaySurf(void);            // return g_dwGIOverlaySurf
void  GI_UnlockOverlaySurf(uint ptr);
void  GI_UnlockCurrentSurf(uint ptr);     // unlock g_dwGICurrentLockedSurf
uint  GI_GetBackbufSurf(void);            // return current back-buffer handle

void  GI_GetBackbufDC(void);
void  GI_ReleaseBackbufDC(uint hdc);
void  GI_GetFrontbufDC(void);
void  GI_ReleaseFrontbufDC(uint hdc);
void  GI_GetDevSurfDC(void);
void  GI_ReleaseDevSurfDC(uint hdc);

// -------------------------------------------------------------------------
// Surface clear helpers
// -------------------------------------------------------------------------

void  GI_ClearSeenSurf(void);             // zero screen surface + front-buffer
void  GI_ClearDevSurf(void);              // zero dev/debug surface

// -------------------------------------------------------------------------
// Dev-surface toggle
// -------------------------------------------------------------------------

void  GI_EnableDevSurf(void);             // g_nGIDevSurfEnabled = 1
void  GI_DisableDevSurf(void);            // g_nGIDevSurfEnabled = 0
void  GI_ToggleDevSurf(void);             // g_nGIDevSurfEnabled ^= 1

// -------------------------------------------------------------------------
// 2-D pixel read/write
// -------------------------------------------------------------------------

byte  GI_GetPixel(int x, int y);                  // read pixel from active surface
void  GI_PlotPixel(int x, int y, byte color);     // write pixel to active surface
void  GI_SetBufPixel(int buf, int x, int y, byte color, int pitch);
byte  GI_ReadMemPixel(int buf, int x, int y, int pitch);

// -------------------------------------------------------------------------
// 2-D draw primitives
// -------------------------------------------------------------------------

int   GI_CalcRectSize(int x1, int y1, int x2, int y2);  // returns (w+1)*(h+1)*2
void  GI_ClearBorder(int sx, int sy, int ex, int ey);    // blit-clear with black scanline
void  GI_FillRect(int sx, int sy, int ex, int ey, uint color); // solid rectangle fill
void  GI_DrawLine(uint buf, uint pitch, uint dummy,
                  int x1, int y1, int x2, int y2, byte color); // Bresenham line
void  GI_FloodFill(uint buf, uint pitch, uint dummy, uint dummy2,
                   int seedX, char fillColor);                   // scanline flood-fill
void  GI_FillHorizontalRow(uint buf, int x, int y, int pitch, int maxX); // flood-fill span helper

// -------------------------------------------------------------------------
// Statistics / debug
// -------------------------------------------------------------------------

void  GI_WriteStatistics(void);           // dump frame colour/compression stats via Debug_Trace
bool  GI_IsPowerOfTwo(uint n);            // true if exactly one bit set in bits 0..8
int   GI_PercentOfWidth(int x);           // (x * 100) / 640

// -------------------------------------------------------------------------
// Palette utilities
// -------------------------------------------------------------------------

byte  GI_FindNearestPalColor(uint nColors, int palPtr, uint r, uint g, uint b);
void  GI_BuildColorRemapTable(int tableOut, int palIn);
uint  GI_FindFarthestSnapshotColor(uint r, uint g, uint b);
void  GI_BuildSnapshotPalMap(void);
void  GI_ApplyGeneralPalToTarget(int targetBuf);
void  GI_LoadGeneralPal(void);            // load GENERAL.PAL, register with SetPal, apply to target

// -------------------------------------------------------------------------
// GV toolbar
// -------------------------------------------------------------------------

void  GV_UpdateButtons(void);             // update Graninv toolbar button bitmaps via SendMessage
