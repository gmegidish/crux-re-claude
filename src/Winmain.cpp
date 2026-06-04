// ---------------------------------------------------------------------------
// Winmain.cpp -- WinMain entry point and top-level window management for CRUX.EXE (Win95)
//
// This module is the application entry point and top-level window host.
// It owns:
//
//   - HANDLE  g_hInstance            (0x007d6af0)  application HINSTANCE
//   - int     g_nHwndMain            (0x007d6a94)  main HWND (cast as int)
//   - int     g_nWindowed            (0x007d6b84)  non-zero = windowed mode
//   - int     g_nAppActive           (0x004ddb64)  non-zero = app in foreground
//   - HANDLE  g_hMsgThreadInitEvent  (0x007d6b78)  "MessageThreadInit" event
//   - DWORD   g_dwExecThreadId       (0x007d6b6c)  execution thread ID
//   - int     g_nExitHandlerCount    (0x007d6dd0)  number of registered exit handlers
//   - int     g_nScreenWidth         (0x007d6a84)  logical screen width  (640)
//   - int     g_nScreenHeight        (0x007d6a8c)  logical screen height (480)
//
// Window classes:
//   "CRUXGIWinClass"   (0x004dcb10)  main game window (WndProc = Win_WndProc)
//   "CRUXCmptblClass"  (0x004dcc74)  hidden compatibility/quest window
//
// The two CRT thunks in the startup table:
//   thunk_FUN_00483ac0  (0x00483ac0)  = Win_PostCallback
//   thunk_FUN_004861c0  (0x004861c0)  = Win_IsAppActive
//
// Startup sequence (Win_Main at 0x00484040):
//   1. CreateMutex("CRUX_MUTEX")          -- single-instance guard
//   2. InitializeCriticalSection           -- global engine CS
//   3. Sched_BeginHighPriority
//   4. Win_QueryDisplayMetrics             -- read desktop DC caps
//   5. Files_FindWatcomDebugger
//   6. Win_ParseCmdLine                    -- parse argv switches
//   7. Win_ReadIni                         -- read ADVENT.INI
//   8. Err_ReadDebugLevel
//   9. Win_CreateHiddenWindow              -- register CRUXCmptblClass
//  10. Win_CreateMainWindow               -- register CRUXGIWinClass, CreateWindowEx
//  11. DDI_InitDirectDraw(g_nHwndMain)    -- DirectDraw init
//  12. Theme_Init / SndMem_Reset          -- theme + sound init
//  13. ShowWindow(g_nHwndMain, SW_SHOW)
//  14. Anim_Init, SndMem_InitLipsync, FrmTimer_Init, Anim_GameInit
//  15. CreateEvent("MessageThreadInit")
//  16. CreateThread(Win_ExecutionThread)  -- game logic thread
//  17. Win_MessageLoop                    -- message pump (main thread)
//  18. Win_CleanExit                      -- ordered teardown
//  19. PostQuitMessage(0)
//
// Original source: C:\DevStudio\Projects\Crux\Winmain.cpp
// Address range:   0x004824a0 -- 0x00486ef0
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "Winmain.h"
#include "ERRORS.h"
#include "DDRAWI.h"
#include "CURSORS.h"
#include "SCHED.h"
#include "THEMES.h"
#include "SOUNDMEM.h"
#include "PLAYER.h"
#include "READRES.h"
#include "FILES.h"
#include "TIMERS.h"
#include "Advanim.h"
#include "AREAS.h"
#include "SETPAL.h"
#include "MIXER.h"
#include "GI.h"
#include "SAFEHEAP.h"

// ---------------------------------------------------------------------------
// Globals (definitions)
// ---------------------------------------------------------------------------

HANDLE  g_hInstance           = NULL;    // 0x007d6af0  application HINSTANCE
int     g_nWindowed           = 0;       // 0x007d6b84  non-zero = windowed mode
int     g_nAppActive          = 0;       // 0x004ddb64  non-zero = app is foreground
HANDLE  g_hMsgThreadInitEvent = NULL;    // 0x007d6b78  "MessageThreadInit" auto-reset event
DWORD   g_dwExecThreadId      = 0;       // 0x007d6b6c  execution thread ID
int     g_nExitHandlerCount   = 0;       // 0x007d6dd0  number of registered exit callbacks
int     g_nScreenWidth        = 0x280;   // 0x007d6a84  logical game width  (640 px)
int     g_nScreenHeight       = 0x1e0;   // 0x007d6a8c  logical game height (480 px)

// Exit handler table -- up to 10 function pointers (0x007d6ac8..0x007d6aef)
static void (*g_apExitHandlers[10])(void); // 0x007d6ac8

// Internal per-window-class globals
// 0x007d6a78  modeless dialog HWND (IsDialogMessage target)
// 0x007d6a80  menu bar height when g_nSchedDebugMode != 0  (0x1c pixels)
// 0x007d6a84  desktop width sampled at startup
// 0x007d6a8c  desktop height sampled at startup
// 0x007d6a90  SM_CYCAPTION   (caption bar height)
// 0x007d6ab8  SM_CYMENU      (menu bar height)
// 0x007d6afc  SM_CXFRAME     (resize border width)
// 0x007d6abc  SM_CYFRAME     (resize border height)
// 0x007d6b00  desktop height (second copy from GetDeviceCaps)
// 0x007d6b04  hidden compat window HWND ("Quest window")
// 0x007d6b70  bit depth reported by GetDeviceCaps(hdc, BITSPIXEL)
// 0x007d6b78  g_hMsgThreadInitEvent
// 0x007d6b80  non-zero = suppress exit dialog (-x switch)
// 0x007d6b84  g_nWindowed
// 0x007d6aa0  global engine CRITICAL_SECTION
// 0x007d6ba8  pending callback function pointer (Win_PostCallback / Win_MessageLoop)
// 0x007d6dbc  non-zero = CD-ROM adventure dir override was specified (-H switch)
// 0x007d6dd4  non-zero = window has been shown at least once
// 0x007d6dd8  non-zero = fullscreen (not windowed, not debug)
// 0x007d6de0  dirty palette flag (set in WM_RENDERALLFORMATS / clipboard)
// 0x007d6de4  WM_COMMAND received flag
// 0x007c4bb0  duplicated main-thread HANDLE (for GetThreadTimes)
// 0x007c4ba0  execution thread HANDLE

// ---------------------------------------------------------------------------
// Win_GetModuleDir  (0x00483050)
// ---------------------------------------------------------------------------
// Get the directory of the running executable by stripping the filename from
// the module path.  Writes result into param_1 (max 0x104 bytes).
void Win_GetModuleDir(char *pszOutDir)
{
    size_t nLen;
    GetModuleFileNameA((HINSTANCE)g_hInstance, pszOutDir, 0x104);
    for (nLen = strlen(pszOutDir); pszOutDir[nLen - 1] != '\\'; nLen--) {
    }
    pszOutDir[nLen] = '\0';
}

// ---------------------------------------------------------------------------
// Win_EnsureTrailingSlash  (0x00482f90)
// ---------------------------------------------------------------------------
// Appends a backslash to pszPath if the last character is not already one.
void Win_EnsureTrailingSlash(char *pszPath)
{
    size_t nLen = strlen(pszPath);
    if (pszPath[nLen - 1] != '\\') {
        pszPath[nLen]     = '\\';
        pszPath[nLen + 1] = '\0';
    }
}

// ---------------------------------------------------------------------------
// Win_BuildSubDir  (0x00482e30)
// ---------------------------------------------------------------------------
// Strip any trailing backslashes from pszPath, then strip back to the last
// remaining backslash, then strip those too, then append pszSubDir.
// Net effect: replace the last path component with the given subdirectory.
void Win_BuildSubDir(char *pszPath, const char *pszSubDir)
{
    size_t nLen;

    // Remove trailing backslashes
    while ((nLen = strlen(pszPath)) > 0 && pszPath[nLen - 1] == '\\')
        pszPath[nLen - 1] = '\0';

    // Strip back to separator
    while ((nLen = strlen(pszPath)) > 0 && pszPath[nLen - 1] != '\\')
        pszPath[nLen - 1] = '\0';

    // Remove that separator too
    while ((nLen = strlen(pszPath)) > 0 && pszPath[nLen - 1] == '\\')
        pszPath[nLen - 1] = '\0';

    // Append separator + subdirectory name (pszSubDir already includes leading '\')
    // FUN_004895f0 = str_append; DAT_004dc44c = "\\"
    // FUN_004895f0(pszPath, "\\");
    // FUN_004895f0(pszPath, pszSubDir);
}

// ---------------------------------------------------------------------------
// Win_ParseCmdLine  (0x004824a0)
// ---------------------------------------------------------------------------
// Parse the command line (via FUN_0048bb40 tokeniser) and apply switches:
//   -H<path>  override adventure directory
//   -h[0]     CD-ROM mode (0 = secondary)
//   -ii<path> inventory index file
//   -im<path> map file
//   -in<name> both files (auto-extension)
//   -m<n>     heap size in KB
//   -n        unknown (thunk)
//   -pplayer  enable player mode
//   -q        toggle quiet mode
//   -s<name>  starting scene override (8 chars)
//   -v        show version MessageBox then exit
//   -w        toggle windowed mode
//   -x        suppress exit dialog
void Win_ParseCmdLine(const char *pszCmdLine)
{
    // Parses command-line args; initialises working dirs when done.
    // See full decompile for switch table.
    Err_LoadStrings();
    Win_GetWorkingDirs();
}

// ---------------------------------------------------------------------------
// Win_GetWorkingDirs  (0x00482980)
// ---------------------------------------------------------------------------
// Resolve adventure directory, save-game directory, and resource paths.
// Reads LANGUAGE_ID from the INI, auto-detects CD-ROM (DriveType == 5),
// builds g_abAdventDir and g_abSaveGameDir.
void Win_GetWorkingDirs(void)
{
    // Implementation resolves working directories from module path and INI.
    // See decompile: builds g_abAdventDir, g_abSaveGameDir, g_nActiveDiskRooms.
}

// ---------------------------------------------------------------------------
// Win_CloseHandle  (0x00483130)
// ---------------------------------------------------------------------------
// Safe CloseHandle wrapper: closes *phHandle and sets it to 0.
void Win_CloseHandle(int *phHandle)
{
    if (*phHandle != 0) {
        CloseHandle((HANDLE)*phHandle);
        *phHandle = 0;
    }
}

// ---------------------------------------------------------------------------
// Win_RegisterExitHandler  (0x00483200)
// ---------------------------------------------------------------------------
// Register a function pointer to be called on Win_CleanExit.
// Maximum 10 handlers (asserts on overflow).
void Win_RegisterExitHandler(void (*pfnHandler)(void))
{
    if (g_nExitHandlerCount > 9) {
        // Err_BadResEntry(...)
        return;
    }
    g_apExitHandlers[g_nExitHandlerCount] = pfnHandler;
    g_nExitHandlerCount++;
}

// ---------------------------------------------------------------------------
// Win_UnregisterExitHandler  (0x004832d0)
// ---------------------------------------------------------------------------
// Remove a previously registered exit handler by pointer value.
void Win_UnregisterExitHandler(void (*pfnHandler)(void))
{
    int i;
    for (i = 0; i < g_nExitHandlerCount; i++) {
        if (g_apExitHandlers[i] == pfnHandler) {
            // Shift remaining entries down
            int j;
            for (j = i; j < g_nExitHandlerCount - 1; j++) {
                g_apExitHandlers[j] = g_apExitHandlers[j + 1];
            }
            g_nExitHandlerCount--;
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Win_RunExitHandlers  (0x004833e0)
// ---------------------------------------------------------------------------
// Call all registered exit handlers in reverse registration order.
void Win_RunExitHandlers(void)
{
    while (g_nExitHandlerCount > 0) {
        g_nExitHandlerCount--;
        g_apExitHandlers[g_nExitHandlerCount]();
    }
}

// ---------------------------------------------------------------------------
// Win_CloseEventHandle  (0x00483820)
// ---------------------------------------------------------------------------
// Close the MessageThreadInit event handle via Win_CloseHandle.
void Win_CloseEventHandle(void)
{
    // Win_CloseHandle(&g_hMsgThreadInitEvent as int*);
}

// ---------------------------------------------------------------------------
// Win_CleanExit  (0x00483490)
// ---------------------------------------------------------------------------
// Ordered engine teardown called from WM_CLOSE / Win_Quit.
// EnterCriticalSection, run exit handlers, Mixer_Stop, hide window,
// Curs_Shutdown, DDI_CheckFullscreenMode, SetPal_SetPalette,
// Player_Shutdown, Win_CloseEventHandle, Theme_KillAllTimers,
// Theme_Shutdown, Res_BunchShutdown, Sched_SetNormalPriority,
// LeaveCriticalSection.
void Win_CleanExit(int nLine, const char *pszFile)
{
    // Full ordered teardown — see decompile for complete call sequence.
}

// ---------------------------------------------------------------------------
// Win_Quit  (0x004831d0)
// ---------------------------------------------------------------------------
// Trigger clean exit with no file/line context then terminate.
void Win_Quit(void)
{
    Win_CleanExit(0, 0);
    // FUN_0048b8e0(0);  -- ExitProcess wrapper
}

// ---------------------------------------------------------------------------
// Win_PeekMsg  (0x004838b0)
// ---------------------------------------------------------------------------
// Single PeekMessage pass.  If a modeless dialog is active, routes the
// message through IsDialogMessage; otherwise Translate+Dispatch.
void Win_PeekMsg(void)
{
    MSG msg;
    // PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD)
    // if dialog: IsDialogMessage(g_hDlg, &msg)
    // else: TranslateMessage + DispatchMessage
}

// ---------------------------------------------------------------------------
// Win_UpdateInput  (0x00483990)
// ---------------------------------------------------------------------------
// Read current mouse position and button state into game globals, then
// drive the scheduler tick (FUN_00489ce0).
void Win_UpdateInput(void)
{
    // thunk_FUN_00452770(&g_nMouseX, &g_nMouseY, &DAT_006dc4f8)
    // copy to DAT_007d668c / 007d6690 / 007d5b8c
    // FUN_00489ce0(timeGetTime())
}

// ---------------------------------------------------------------------------
// Win_PostCallback  (0x00483ac0)
// ---------------------------------------------------------------------------
// Store a callback function pointer then post WM_USER+42 (0x002a) to the
// main window; Win_MessageLoop drains it via InterlockedExchange.
void Win_PostCallback(void (*pfnCallback)(void))
{
    // _DAT_007d6ba8 = pfnCallback;
    // PostMessageA((HWND)g_nHwndMain, 0x2a, 0, 0);
}

// ---------------------------------------------------------------------------
// Win_WriteThreadTimes  (0x00483b60)
// ---------------------------------------------------------------------------
// Retrieve kernel and user thread times for the current thread and record
// them via Debug_Assert (profiling aid).
void Win_WriteThreadTimes(int nLine, const char *pszFile)
{
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    HANDLE hThread = GetCurrentThread();
    GetThreadTimes(hThread, &ftCreate, &ftExit, &ftKernel, &ftUser);
    // Debug_Assert(nLine, pszFile, ftKernel.dwLowDateTime >> 1);
    // Debug_Assert(nLine, pszFile, ftKernel.dwHighDateTime >> 1);
    // Debug_Assert(nLine, pszFile, ftUser.dwLowDateTime >> 1);
    // Debug_Assert(nLine, pszFile, ftUser.dwHighDateTime >> 1);
}

// ---------------------------------------------------------------------------
// Win_RepositionWindow  (0x00483c70)
// ---------------------------------------------------------------------------
// Re-centre the main window on the desktop using current screen metrics.
// Computes position as ((desktopW - 640) / 2, (desktopH - 480) / 2).
void Win_RepositionWindow(void)
{
    // SetWindowPos((HWND)g_nHwndMain, NULL,
    //     (DAT_007d6a84 - 0x280) / 2, (DAT_007d6a8c - 0x1e0) / 2,
    //     DAT_007d6afc*2 + 0x280,
    //     DAT_007d6a90 + 0x1e0 + DAT_007d6ab8 + g_nMenuBarH + DAT_007d6abc*2, 0);
}

// ---------------------------------------------------------------------------
// Win_IsWindowed  (0x00483e20)
// ---------------------------------------------------------------------------
// Return g_nWindowed flag (non-zero = running in a window, not fullscreen).
int Win_IsWindowed(void)
{
    return g_nWindowed;
}

// ---------------------------------------------------------------------------
// Win_ReadIni  (0x00483eb0)
// ---------------------------------------------------------------------------
// Read ADVENT.INI [Status] Inifile key to validate the installation.
// Looks first in the module directory, then in the adventure directory.
// Calls Err_SetRecord3 if the key is absent (fatal install error).
void Win_ReadIni(void)
{
    // GetPrivateProfileIntA("Status","Inifile",0,<path>\\ADVENT.INI)
    // If 0: Err_SetRecord3(0x14, "ADVENT.INI", -1)
}

// ---------------------------------------------------------------------------
// Win_QueryDisplayMetrics  (0x00485e90)
// ---------------------------------------------------------------------------
// Sample the desktop device context to read colour depth, pixel dimensions,
// and system metrics (caption, menu, border sizes).
// Sets g_nScreenWidth=640, g_nScreenHeight=480 as the game's logical size.
void Win_QueryDisplayMetrics(void)
{
    HWND hDesktop = GetDesktopWindow();
    HDC  hdc      = GetWindowDC(hDesktop);

    // g_nBitDepth    = GetDeviceCaps(hdc, BITSPIXEL);
    // g_nDesktopW    = GetDeviceCaps(hdc, HORZRES);
    // g_nDesktopH    = GetDeviceCaps(hdc, VERTRES);
    g_nScreenWidth  = 0x280;   // 640
    g_nScreenHeight = 0x1e0;   // 480

    ReleaseDC(hDesktop, hdc);

    // SM_CYCAPTION, SM_CYMENU, SM_CXFRAME, SM_CYFRAME
}

// ---------------------------------------------------------------------------
// Win_FixWindowZOrder  (0x00486100)
// ---------------------------------------------------------------------------
// Force the main window to the front by toggling HWND_TOPMOST then
// HWND_NOTOPMOST (SWP_NOMOVE|SWP_NOSIZE on both calls).
void Win_FixWindowZOrder(void)
{
    SetWindowPos((HWND)g_nHwndMain, HWND_TOPMOST,    0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos((HWND)g_nHwndMain, HWND_NOTOPMOST,  0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

// ---------------------------------------------------------------------------
// Win_IsAppActive  (0x004861c0)
// ---------------------------------------------------------------------------
// Return g_nAppActive flag (non-zero = app is in the foreground).
int Win_IsAppActive(void)
{
    return g_nAppActive;
}

// ---------------------------------------------------------------------------
// Win_BringToFront  (0x00486d80)
// ---------------------------------------------------------------------------
// Set hWnd as the foreground window and restore it from minimised state.
void Win_BringToFront(HWND hWnd)
{
    SetForegroundWindow(hWnd);
    ShowWindow(hWnd, SW_NORMAL);
}

// ---------------------------------------------------------------------------
// Win_UpdateCursor  (0x00486cd0)
// ---------------------------------------------------------------------------
// Per-frame cursor update: find the area under the mouse, set cursor shape,
// then advance the cursor animation tick.
void Win_UpdateCursor(void)
{
    // int nArea = Area_FindAt(g_nMouseX, g_nMouseY);
    // Curs_SetCursorByMode(nArea);
    // Curs_Tick();
}

// ---------------------------------------------------------------------------
// Win_ConfirmExit  (0x00485ff0)
// ---------------------------------------------------------------------------
// Handle user-initiated exit (WM_CLOSE or Escape).  Minimises window,
// switches to fullscreen mode, shows Err_AskDialog.  If the user confirms,
// restores windowed mode and returns 0; otherwise returns -1.
int Win_ConfirmExit(void)
{
    // Player_StartStream();
    // ShowWindow((HWND)g_nHwndMain, SW_MINIMIZE);
    // DDI_SetFullscreenMode();
    // Sched_SetAboveNormalPriority();
    // if (g_nSuppressExitDialog || Err_AskDialog() != IDNO) return -1;
    // DDI_SetDisplayMode();
    // ShowWindow((HWND)g_nHwndMain, SW_RESTORE);
    // Win_FixWindowZOrder();
    // Sched_EndHighPriority();
    return 0;
}

// ---------------------------------------------------------------------------
// Win_BuildCursorMask  (0x00486e20)
// ---------------------------------------------------------------------------
// Convert a rectangular character array (rows x cols, 0=opaque/1=transparent)
// into a packed 1-bpp Windows cursor AND-mask bitmap.
// Returned buffer is SafeHeap_Alloc'd; rows are byte-aligned.
void *Win_BuildCursorMask(int pSrc, int nRows, int nCols)
{
    // Allocates ((nCols+7)/8 + 1) * nRows bytes, fills MSB-first bitmask.
    size_t nRowBytes = ((nCols + 7) >> 3) + 1;
    size_t nSize     = nRowBytes * nRows;
    void  *pDst      = SafeHeap_Alloc(0, NULL, nSize);
    memset(pDst, 0, nSize);
    // Pack bits from pSrc char array into pDst, MSB first per row.
    return pDst;
}

// ---------------------------------------------------------------------------
// Win_CreateHiddenWindow  (0x00484e20)
// ---------------------------------------------------------------------------
// Register "CRUXCmptblClass" and create a hidden WS_OVERLAPPED window
// labelled "Quest window".  Used as a compatibility target for legacy
// window-message routing.
void Win_CreateHiddenWindow(HINSTANCE hInstance)
{
    WNDCLASSA wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = DefWindowProcA;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)6;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "CRUXCmptblClass";
    RegisterClassA(&wc);

    // g_hCompatWnd = CreateWindowExA(0, "CRUXCmptblClass", "Quest window",
    //     0x88000000, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
}

// ---------------------------------------------------------------------------
// Win_CreateMainWindow  (0x004849c0)
// ---------------------------------------------------------------------------
// Register "CRUXGIWinClass" (WndProc = Win_WndProc) and create the main
// game window.  In fullscreen mode uses WS_POPUP|WS_VISIBLE (0x82000000);
// in windowed mode uses WS_OVERLAPPEDWINDOW (0x000a0000).
// Stores the result in g_nHwndMain, calls Win_RepositionWindow, UpdateWindow.
void Win_CreateMainWindow(HINSTANCE *phInstance)
{
    WNDCLASSA wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;  // 0x2b
    wc.lpfnWndProc   = (WNDPROC)Win_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = (HINSTANCE)*phInstance;
    wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = (g_nSchedDebugMode ? "MainMenu" : NULL);
    wc.lpszClassName = "CRUXGIWinClass";
    RegisterClassA(&wc);

    // Fullscreen:  WS_POPUP | WS_VISIBLE at (0,0) desktop size
    // Windowed:    WS_OVERLAPPEDWINDOW at (0,0) 640x(480 + borders)
    // g_nHwndMain = (int)CreateWindowExA(0, "CRUXGIWinClass", <title>,
    //     dwStyle, 0, 0, nW, nH, NULL, NULL, (HINSTANCE)*phInstance, NULL);

    Win_RepositionWindow();
    UpdateWindow((HWND)g_nHwndMain);
    Sleep(1000);
}

// ---------------------------------------------------------------------------
// Win_MessageLoop  (0x00484f60)
// ---------------------------------------------------------------------------
// Message-thread pump.  Runs on the main (UI) thread after the execution
// thread has been spawned.  Calls Win_BringToFront, signals the init event,
// then drives a standard GetMessage loop.  Each iteration drains any pending
// Win_PostCallback via InterlockedExchange.
void Win_MessageLoop(void)
{
    MSG msg;
    HANDLE hThread = GetCurrentThread();
    SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

    Win_BringToFront((HWND)g_nHwndMain);
    SetEvent(g_hMsgThreadInitEvent);

    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        // Drain pending callback posted by Win_PostCallback
        // void (*pfn)(void) = (void(*)(void))InterlockedExchange(&g_pfnPendingCallback, 0);
        // if (pfn) pfn();
    }
}

// ---------------------------------------------------------------------------
// Win_ExecutionThread  (0x004850c0)
// ---------------------------------------------------------------------------
// Game-logic thread entry point.  Waits on g_hMsgThreadInitEvent then
// drives the room/scene loop: Res_BunchInit, Files_LoadInvMain, then
// the inner area-transition do-while loop that calls thunk_FUN_004101f0
// (the main RUNPROG/area-run function) and handles exit codes:
//   +1  = restart from beginning
//   -1  = normal exit
//   -2  = load save game
//   -3  = load autosave
//   -4  = load named save
//   -5  = load from buffer (DAT_00629b08)
//   -6  = hard quit
//   -7  = puzzle mode (no save)
//   -8  = restart this area
//   -9  = full restart
DWORD WINAPI Win_ExecutionThread(LPVOID)
{
    WaitForSingleObject(g_hMsgThreadInitEvent, INFINITE);
    // GI_WaitForReady();
    // Res_BunchInit(...);
    // Files_LoadInvMain();
    // ... area loop ...
    return 0;
}

// ---------------------------------------------------------------------------
// Win_WndProc  (0x00486250)
// ---------------------------------------------------------------------------
// Main window procedure ("FAR PASCAL GIWndProc").
// Handled messages:
//   WM_PAINT        (0x000f)  blit back-buffer, ValidateRect
//   WM_DESTROY      (0x0002)  ExitProcess if debug mode
//   WM_SIZE         (0x0003)  GetWindowRect
//   WM_ACTIVATE     (0x0006)  Curs_ShowWin32 / Curs_HideWin32
//   WM_ACTIVATEAPP  (0x001c)  toggle g_nAppActive, fullscreen/windowed switch
//   WM_CLOSE        (0x0010)  Win_ConfirmExit; if fails: Win_CleanExit+ExitProcess
//   WM_USER+0x2a    (0x002a)  return 0 (callback already fired)
//   WM_SETCURSOR    (0x0020)  hide/show Win32 cursor
//   WM_KEYDOWN      (0x0100)  record keycode
//   WM_CHAR         (0x0102)  Alt+Escape = confirm exit
//   WM_SYSKEYDOWN   (0x0104)  Alt+Enter = toggle fullscreen/windowed
//   WM_COMMAND      (0x0111)  menu command dispatch
//   WM_MOUSEMOVE    (0x0200)  thunk_FUN_00452030 (mouse handler)
//   WM_LBUTTONDOWN  (0x0201)  thunk_FUN_00452030
//   WM_LBUTTONUP    (0x0202)  thunk_FUN_00452030
//   WM_LBUTTONDBLCLK(0x0203)  thunk_FUN_00452030
//   WM_RBUTTONDOWN  (0x0204)  thunk_FUN_00452030
//   WM_RBUTTONUP    (0x0205)  thunk_FUN_00452030
//   WM_RENDERALLFORMATS(0x311) set dirty palette flag
//   WM_SYSCOMMAND   (0x0112)  swallow SC_SCREENSAVE / SC_MONITORPOWER
LRESULT CALLBACK Win_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Full implementation in decompile at 0x00486250.
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Win_Main  (0x00484040)  — actual WinMain entry point
// ---------------------------------------------------------------------------
// PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
//
// Startup sequence:
//  1. CreateMutex(NULL, FALSE, "CRUX_MUTEX") -- single-instance guard
//  2. InitializeCriticalSection(&g_csEngine)
//  3. Sched_BeginHighPriority()
//  4. Win_QueryDisplayMetrics()
//  5. Files_FindWatcomDebugger()
//  6. Win_ParseCmdLine(lpCmdLine)   -> Win_GetWorkingDirs -> Err_LoadStrings
//  7. Win_ReadIni()
//  8. Err_ReadDebugLevel()
//  9. Win_CreateHiddenWindow(hInstance)
// 10. Win_CreateMainWindow(&hInstance)
// 11. DDI_InitDirectDraw(g_nHwndMain)
// 12. if (!g_bQuiet): Theme_Init(), SndMem_Reset()
// 13. ShowWindow(g_nHwndMain, SW_SHOW)
// 14. Anim_Init(), SndMem_InitLipsync(), Sleep(1000)
// 15. FrmTimer_Init(), Anim_GameInit()
// 16. InitializeCriticalSection(&g_nAreaCritSec)
// 17. g_hMsgThreadInitEvent = CreateEvent(NULL,0,0,"MessageThreadInit")
// 18. g_hExecThread = CreateThread(NULL,0,Win_ExecutionThread,NULL,0,&g_dwExecThreadId)
// 19. Win_MessageLoop()             -- blocks until PostQuitMessage
// 20. Win_CleanExit(...)
// 21. PostQuitMessage(0)
// Returns 0.
int PASCAL Win_Main(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
    // See decompile at 0x00484040 for full implementation.
    (void)hPrevInstance;
    (void)nCmdShow;

    g_hInstance = (HANDLE)hInstance;

    // Full body omitted; see decompile.
    return 0;
}
