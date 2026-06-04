#ifndef CURSORS_H
#define CURSORS_H

// ---------------------------------------------------------------------------
// CURSORS.h  —  Software sprite cursor system
// Original: C:\DevStudio\Projects\Crux\CURSORS.cpp
// RE offset: 0x00418640 – 0x0041a7b0
// ---------------------------------------------------------------------------
// The cursor system maintains a bank of animated sprite cursors drawn in
// software over the DirectDraw back buffer.  Each cursor has an animation
// sequence, a hotspot, and Win32 HCURSOR fallback for dialog boxes.
// Thread safety is provided by g_nCursorCS (CRITICAL_SECTION) and a Win32
// auto-reset event (g_hCursorSyncEvent) that synchronises PutOnPage /
// RestoreFromPage across the render thread.
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Cursor state ---------------------------------------------------------------
extern int g_nCurrentCursor;       // 0x004c8768  active cursor index (-1=none)
extern int g_nCursorSystemEnabled; // 0x00647c18  non-zero when system is active
extern int g_nCursorMode;          // 0x00646754  1=arrow 2=wait 3=cross 4=custom else=object
extern int g_nCursorDrawEnabled;   // 0x004c8774  allow software draw
extern int g_nWin32CursorVisible;  // 0x004c8770  mirrors ShowCursor state
extern int g_nCursorMode4Id;       // 0x0064674c  cursor ID for mode 4

// Geometry -------------------------------------------------------------------
extern int g_nCursorX;             // 0x00646734  screen X (hotspot-relative)
extern int g_nCursorY;             // 0x00646730  screen Y (hotspot-relative)
extern int g_nCursorWidth;         // 0x00646760  sprite width in pixels
extern int g_nCursorHeight;        // 0x0064675c  sprite height in pixels
extern int g_nCursorImgW;          // 0x00646744  image width (from sprite header)
extern int g_nCursorImgH;          // 0x0064673c  image height (from sprite header)
extern int g_nCursorHotX;          // 0x00646710  hotspot X (frame-adjusted)
extern int g_nCursorHotY;          // 0x00646738  hotspot Y (frame-adjusted)
extern int g_nCursorOffsetX;       // 0x00647c20  extra draw offset X
extern int g_nCursorOffsetY;       // 0x00647c24  extra draw offset Y

// Blit clip region (computed per frame) -------------------------------------
extern int g_nCursorSrcX;          // 0x00647c38  source X clip offset
extern int g_nCursorSrcY;          // 0x00647c3c  source Y clip offset
extern int g_nCursorDstX;          // 0x00647c0c  destination X on screen
extern int g_nCursorDstY;          // 0x00647c08  destination Y on screen
extern int g_nCursorBlitW;         // 0x00647c10  blit width (after clipping)
extern int g_nCursorBlitH;         // 0x00647c14  blit height (after clipping)

// DirectDraw surfaces -------------------------------------------------------
extern int* g_pCursorSurface;      // 0x00646758  software cursor surface
extern int* g_pCursorBgSurface;    // 0x00646750  background save surface

// Dirty / visible state -----------------------------------------------------
extern int g_nCursorDirty;         // 0x00647c28  background saved, needs restore
extern int g_nCursorWasVisible;    // 0x00647c34  cursor was blitted last frame
extern int g_nLastCursorSprite;    // 0x00646740  last sprite blitted (skip dupe draws)

// Win32 handles -------------------------------------------------------------
extern HANDLE g_hCursorSyncEvent;  // 0x00647c2c  sync event for PutOnPage/RestoreFromPage
extern int g_nWin32CursorIdx;      // 0x004c8e3c  current Win32 HCURSOR index
extern int g_nWin32Cursor1;        // 0x006468cc  HCURSOR mode 1 (arrow)
extern int g_nWin32Cursor2;        // 0x0064677c  HCURSOR mode 2 (hourglass)
extern int g_nWin32CursorWait;     // 0x006467ac  HCURSOR busy/wait

// Callbacks -----------------------------------------------------------------
extern int* g_pfnCursorPos;        // 0x00647c1c  position transform callback
extern int* g_pfnCursorOverride;   // 0x00647c30  cursor ID override callback

// Thread safety -------------------------------------------------------------
extern int g_nCursorCS;            // 0x00646718  CRITICAL_SECTION (opaque to Ghidra)

// Shared with input system --------------------------------------------------
extern int g_nMouseX;              // 0x006dc4f0  current mouse X
extern int g_nMouseY;              // 0x006dc4f4  current mouse Y

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Init the debug log file and its CRITICAL_SECTION.  Call once at startup.
void Curs_InitLog();

// Load a cursor resource, selecting between two types based on file existence.
void Curs_LoadCursorSelect(int cursorId, int resId, int x, int y);

// Set the active cursor by ID, blit it to g_pCursorSurface.
void Curs_SetCursor(int cursorId);

// High-level cursor setter: dispatches based on g_nCursorMode.
void Curs_SetCursorByMode(int cursorId);

// Advance the animation frame for the current cursor.
void Curs_UpdateAnimFrame();

// Per-tick animation driver: advance frame and update cursor if changed.
void Curs_Animate();

// Per-frame tick: animate + update position from g_nMouseX/Y.
void Curs_Tick();

// Recompute g_nCursorX/Y from raw position and current hotspot.
void Curs_SetPosition(int x, int y);

// Update cursor position and blit; called with raw mouse coordinates.
void Curs_Update(int x, int y);

// Set draw offset added on top of cursor position.
void Curs_SetOffset(int x, int y);

// Get the draw surface, optionally transforming via g_pfnCursorPos.
int Curs_GetSurface(int x, int y);

// Restore the background behind the cursor (erase old cursor position).
void Curs_Restore(int invalidate);

// Blit cursor onto a page surface; saves background first.
void Curs_PutOnPage(int surface);

// Restore background from saved surface; signal g_hCursorSyncEvent.
void Curs_RestoreFromPage(int surface);

// Wait for sync event then shut down cursor rendering.
void Curs_Shutdown();

// Show the Win32 system cursor (ShowCursor TRUE).
void Curs_ShowWin32();

// Hide the Win32 system cursor (ShowCursor FALSE).
void Curs_HideWin32();

// Return cursor buffer dimensions: (width, height, bufW=150, bufH=150).
void Curs_GetSize(int* width, int* height, int* bufW, int* bufH);

// Set Win32 HCURSOR based on current mode and supplied index.
void Curs_SetWin32Cursor(int idx);

// Set Win32 busy/wait cursor immediately.
void Curs_SetWaitCursor();

// Force a cursor background restore (Curs_Restore(1)).
void Curs_ForceRestore();

// Disable software cursor rendering.
void Curs_DisableDraw();

// Enable software cursor rendering.
void Curs_EnableDraw();

// Register a position-transform callback (called by Curs_GetSurface).
void Curs_SetPosCallback(void* fn);

// Register a cursor-ID override callback (called by Curs_SetCursor).
void Curs_SetOverrideCallback(void* fn);

// Create a DirectDraw offscreen surface of given width × height.
// Returns surface slot index, or -1 on failure.
int DDI_CreateOffscreenSurf(int width, int height, int colorKey, int isVideo);

// ---------------------------------------------------------------------------
// Debug stubs (no-ops in release build — macros stripped)
// ---------------------------------------------------------------------------
void Debug_Assert(int line, const char* file, int value);
void Debug_AssertFatal(int line, const char* file);
void Debug_Trace(int line, const char* file, const char* msg);
void Debug_TraceVal(int line, const char* file, const char* msg, int val);

// ---------------------------------------------------------------------------
// Mouse input handler (0x00451e90 – 0x004528c0)
// ---------------------------------------------------------------------------

// --- Mouse state globals ---
extern int    g_nMouseDblClickTimer;   // 0x004d5058  dbl-click timer ID (-1 none)
extern int    g_nMouseButtons;         // 0x006dc4f8  logical button/click state
extern int    g_nMouseClickX;          // 0x006dc4fc  X at last button-down
extern int    g_nMouseClickY;          // 0x006dc500  Y at last button-down
extern void*  g_pMouseCS;              // 0x006dc508  CRITICAL_SECTION (opaque)
extern int    g_nMouseScreenX;         // 0x006dc520  GetCursorPos POINT.x
extern int    g_nMouseScreenY;         // 0x006dc524  GetCursorPos POINT.y
extern int    g_nMouseBtnDownMask;     // 0x006dc528  raw button bitmask
extern HANDLE g_pMouseStateEvent;      // 0x006dc52c  state-changed event
extern HANDLE g_pMouseMoveEvent;       // 0x006dc530  mouse-move event
extern int    g_nMouseDblClickEnabled; // 0x006dc534  from [Mouse]DoubleClick INI
extern int    g_nMouseOriginX;         // 0x006dc538  origin offset X
extern int    g_nMouseOriginY;         // 0x006dc53c  origin offset Y

// Cancel a pending double-click timer and commit the press as a single click.
void Curs_CancelDblClickTimer();

// Set the origin offset subtracted from raw WM_MOUSEMOVE coordinates.
void Curs_SetMouseOrigin(int x, int y);

// Main mouse message handler (WM_MOUSEMOVE / WM_*BUTTON* 0x200-0x205).
int  Curs_HandleMouseMsg(unsigned int msg, unsigned int wParam, unsigned int lParam);

// Initialise mouse subsystem: critical section, events, DoubleClick INI.
int  Curs_InitMouse();

// Close the two mouse sync event handles.
void Curs_CloseMouseEvents();

// Return current screen cursor position and logical button state.
void Curs_GetMouseState(int* outX, int* outY, int* outButtons);

// Empty SEH-only stubs (debug helpers stripped in the release build).
void Curs_Nop1();
void Curs_Nop2();
void Curs_Nop3();
void Curs_Nop4();
void Curs_Nop5();

#endif // CURSORS_H
