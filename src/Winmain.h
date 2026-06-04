#ifndef WINMAIN_H
#define WINMAIN_H

// ---------------------------------------------------------------------------
// Winmain.h -- WinMain entry point and top-level window management
// Original: C:\DevStudio\Projects\Crux\Winmain.cpp
// RE offset: 0x004824a0 -- 0x00486ef0
// ---------------------------------------------------------------------------
// This module is the Win32 application entry point.  It owns window class
// registration ("CRUXGIWinClass" and "CRUXCmptblClass"), the main message
// pump, the game-logic execution thread, and all clean-exit infrastructure.
//
// Threading model:
//   Main (message) thread  -- Win_Main -> Win_MessageLoop (GetMessage loop)
//   Execution thread       -- Win_ExecutionThread (game logic, room loop)
//
// The two threads synchronise via g_hMsgThreadInitEvent: the execution
// thread calls WaitForSingleObject on it at startup; the message thread
// calls SetEvent after Win_BringToFront completes.
//
// CRT thunk table entries originating here:
//   thunk_FUN_00483ac0  (Win_PostCallback)   -- posts a callback to msg thread
//   thunk_FUN_004861c0  (Win_IsAppActive)    -- returns g_nAppActive
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

extern HANDLE g_hInstance;            // 0x007d6af0  application HINSTANCE
extern int    g_nWindowed;            // 0x007d6b84  non-zero = windowed mode
extern int    g_nAppActive;           // 0x004ddb64  non-zero = app is foreground
extern HANDLE g_hMsgThreadInitEvent;  // 0x007d6b78  "MessageThreadInit" sync event
extern DWORD  g_dwExecThreadId;       // 0x007d6b6c  execution thread ID
extern int    g_nExitHandlerCount;    // 0x007d6dd0  registered exit handler count
extern int    g_nScreenWidth;         // 0x007d6a84  logical game width  (640)
extern int    g_nScreenHeight;        // 0x007d6a8c  logical game height (480)

// Shared with ERRORS.h / DDRAWI.h
extern int    g_nHwndMain;            // 0x007d6a94  main window HWND (as int)

// ---------------------------------------------------------------------------
// Path utilities
// ---------------------------------------------------------------------------

// Win_GetModuleDir -- Get directory of the running .EXE (strips filename).
// 0x00483050
void Win_GetModuleDir(char *pszOutDir);

// Win_EnsureTrailingSlash -- Append '\\' to pszPath if not already present.
// 0x00482f90
void Win_EnsureTrailingSlash(char *pszPath);

// Win_BuildSubDir -- Strip last path component of pszPath, append pszSubDir.
// 0x00482e30
void Win_BuildSubDir(char *pszPath, const char *pszSubDir);

// ---------------------------------------------------------------------------
// Startup helpers
// ---------------------------------------------------------------------------

// Win_ParseCmdLine -- Parse command-line switches; call Win_GetWorkingDirs.
// 0x004824a0
void Win_ParseCmdLine(const char *pszCmdLine);

// Win_GetWorkingDirs -- Resolve g_abAdventDir, g_abSaveGameDir from INI/CD/EXE.
// 0x00482980
void Win_GetWorkingDirs(void);

// Win_ReadIni -- Read ADVENT.INI [Status] Inifile validation key.
// 0x00483eb0
void Win_ReadIni(void);

// Win_QueryDisplayMetrics -- Sample desktop DC caps and system metrics.
// 0x00485e90
void Win_QueryDisplayMetrics(void);

// ---------------------------------------------------------------------------
// Exit handler registry
// ---------------------------------------------------------------------------

// Win_RegisterExitHandler -- Register a function to be called on Win_CleanExit.
// 0x00483200
void Win_RegisterExitHandler(void (*pfnHandler)(void));

// Win_UnregisterExitHandler -- Remove a previously registered exit handler.
// 0x004832d0
void Win_UnregisterExitHandler(void (*pfnHandler)(void));

// Win_RunExitHandlers -- Fire all registered exit handlers (reverse order).
// 0x004833e0
void Win_RunExitHandlers(void);

// Win_CloseEventHandle -- Close g_hMsgThreadInitEvent via Win_CloseHandle.
// 0x00483820
void Win_CloseEventHandle(void);

// Win_CloseHandle -- Safe CloseHandle wrapper; nulls *phHandle.
// 0x00483130
void Win_CloseHandle(int *phHandle);

// Win_CleanExit -- Ordered engine teardown (EnterCS, handlers, Mixer, DDI...).
// 0x00483490
void Win_CleanExit(int nLine, const char *pszFile);

// Win_Quit -- Win_CleanExit(0,0) then ExitProcess.
// 0x004831d0
void Win_Quit(void);

// ---------------------------------------------------------------------------
// Window management
// ---------------------------------------------------------------------------

// Win_CreateHiddenWindow -- Register and create "CRUXCmptblClass" hidden window.
// 0x00484e20
void Win_CreateHiddenWindow(HINSTANCE hInstance);

// Win_CreateMainWindow -- Register "CRUXGIWinClass", CreateWindowEx, store g_nHwndMain.
// 0x004849c0
void Win_CreateMainWindow(HINSTANCE *phInstance);

// Win_RepositionWindow -- Centre g_nHwndMain on the desktop via SetWindowPos.
// 0x00483c70
void Win_RepositionWindow(void);

// Win_FixWindowZOrder -- Toggle TOPMOST/NOTOPMOST to force window to front.
// 0x00486100
void Win_FixWindowZOrder(void);

// Win_BringToFront -- SetForegroundWindow + ShowWindow(SW_NORMAL).
// 0x00486d80
void Win_BringToFront(HWND hWnd);

// Win_IsWindowed -- Return g_nWindowed (non-zero = windowed mode active).
// 0x00483e20
int  Win_IsWindowed(void);

// Win_IsAppActive -- Return g_nAppActive (non-zero = app in foreground).
// 0x004861c0
int  Win_IsAppActive(void);

// Win_ConfirmExit -- Prompt user on exit; returns 0 if confirmed, -1 if cancelled.
// 0x00485ff0
int  Win_ConfirmExit(void);

// Win_WndProc -- Main window procedure ("FAR PASCAL GIWndProc").
// 0x00486250
LRESULT CALLBACK Win_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// Message pump
// ---------------------------------------------------------------------------

// Win_PeekMsg -- One PeekMessage pass; routes through IsDialogMessage if needed.
// 0x004838b0
void Win_PeekMsg(void);

// Win_MessageLoop -- Main-thread GetMessage pump (signals init event first).
// 0x00484f60
void Win_MessageLoop(void);

// Win_PostCallback -- Post a callback function pointer to the message thread.
// 0x00483ac0
void Win_PostCallback(void (*pfnCallback)(void));

// ---------------------------------------------------------------------------
// Input / cursor
// ---------------------------------------------------------------------------

// Win_UpdateInput -- Read mouse state into game globals, tick scheduler.
// 0x00483990
void Win_UpdateInput(void);

// Win_UpdateCursor -- Area_FindAt + Curs_SetCursorByMode + Curs_Tick.
// 0x00486cd0
void Win_UpdateCursor(void);

// Win_WriteThreadTimes -- Record kernel/user thread times via Debug_Assert.
// 0x00483b60
void Win_WriteThreadTimes(int nLine, const char *pszFile);

// ---------------------------------------------------------------------------
// Cursor mask builder
// ---------------------------------------------------------------------------

// Win_BuildCursorMask -- Convert char[][] to packed 1-bpp AND-mask.
// 0x00486e20
void *Win_BuildCursorMask(int pSrc, int nRows, int nCols);

// ---------------------------------------------------------------------------
// Threads
// ---------------------------------------------------------------------------

// Win_ExecutionThread -- Game logic thread: waits on init event, runs room loop.
// 0x004850c0
DWORD WINAPI Win_ExecutionThread(LPVOID lpParam);

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

// Win_Main -- Actual WinMain: mutex guard, init, window create, DirectDraw,
// spawn execution thread, run message loop, clean exit.
// 0x00484040
int PASCAL Win_Main(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow);

#endif // WINMAIN_H
