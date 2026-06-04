// ---------------------------------------------------------------------------
// FILES.h — File I/O, save/load game, palette/bitmap loading,
//           common dialogs, and text-editor helpers
//
// Original: C:\DevStudio\Projects\Crux\FILES.cpp
// RE address range: 0x004223a0 – 0x0042a6f0
// Functions: 41
// ---------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Area cache
extern int  g_nAreaCacheCount;          // 0x007127d0
extern int  g_anAreaCacheTable[];       // 0x0070c258
extern int  g_anAreaCacheSlots[];       // 0x007114d0  15 entries, reset to -1
extern int  g_nAreaCacheActive;         // 0x007127ec

// Special-save callback registry
extern int  g_nSpecialSaveCount;        // 0x006b8234  max 10
extern int  g_nSpecialSaveFuncs[];      // 0x006b8190  [save_fn, load_fn] pairs

// Save directory (pointer stored as int)
extern int  g_nSaveDir;                 // 0x006b81e0

// Directory paths
extern char g_abSaveGameDir[];          // 0x007d6248
extern char g_abAdventDir[];            // 0x007d6468

// Screen dimensions (used to centre OFN dialogs)
extern int  g_nScreenWidth;             // 0x006b8f00
extern int  g_nScreenHeight;            // 0x006b8f08

// OFN dialog timer
extern int  g_nOFNTimerId;              // 0x004cd970  -1 = none
extern int  g_nOFNParentWnd;            // 0x006b82b4  HWND of OFN parent

// OFN cancel result strings (empty)
extern char g_abLoadGameResult[];       // 0x006b82b8
extern char g_abSaveGameResult[];       // 0x006b82bc
extern char g_abSelectFileResult[];     // 0x006b82c0

// Edit-control text buffer
extern int  g_nEditTextLocal;           // 0x006b8d4c  HLOCAL

// Find/Replace dialog
extern int  g_nSearchString;            // 0x006b8c60
extern int  g_nFindReplaceOwner;        // 0x006b8ce8
extern char g_abFindString[];           // 0x006b8cf0  81 bytes
extern char g_abReplaceString[];        // 0x006b8c68  81 bytes
extern int  g_nFindReplaceStruct;       // 0x006b8cc0  FINDREPLACEA
extern int  g_nFindDialogOpen;          // 0x006b8d44
extern int  g_nFindDialog;              // 0x006b8d48  HWND

// ---------------------------------------------------------------------------
// 1. Game state save / load
// ---------------------------------------------------------------------------

// 0x004223a0  Serialize core game variables to nBuf
void  Files_SaveState(int nBuf);

// 0x00422d00  Deserialize core game variables from nBuf
void  Files_LoadState(int nBuf);

// 0x00423710  Serialize secondary game state (room/NPC variables)
void  Files_SaveStateAlt(void);

// 0x00424490  Free all cached area entries; reset slot table to -1
void  Files_FreeAreaCache(void);

// 0x00424980  RLE decompress: 0xFF escape, value, run-length
void  Files_RleDecompress(char *pSrc, char *pDst, int nSrcLen);

// 0x00424af0  Load external palette by resource ID
void  Files_LoadPalExternal(int nResId, int nParam2);

// 0x00424ca0  Load named palette (25-entry)
void  Files_LoadPal(int nResId, int nParam2, unsigned char nParam3);

// 0x00424ec0  Dev-build raw palette load; returns -1 on failure
int   Files_DevLoadPal(int nResId, int nParam2);

// 0x00425020  Load a .SAV file from disk; returns 0=ok, -1=fail
int   Files_LoadSaveGame(int nParam1);

// 0x004253c0  Load ADVENT/INVMAIN inventory database
void  Files_LoadInvMain(void);

// 0x004258c0  Delete all *.ST files from the save directory
void  Files_EraseStates(void);

// 0x00425a20  Serialize game state and write to .SAV file
void  Files_SaveGame(int nSlot);

// 0x00426530  Register a [save_fn, load_fn] special-save pair (max 10)
void  Files_RegisterSpecialSave(int nSaveFn, int nLoadFn);

// 0x00426620  Remove all special-save pairs whose save_fn == nSaveFn
void  Files_UnregisterSpecialSave(int nSaveFn);

// 0x00426750  Full save: broadcast to special callbacks + core save
int   Files_SaveGameFull(int nSlot);

// 0x00427530  Invoke all registered save callbacks with nParam
void  Files_BroadcastSave(int nParam);

// 0x004275f0  Full load: broadcast to special callbacks + core load
int   Files_LoadGameFull(int nSlot);

// 0x004285e0  Invoke all registered load callbacks with nParam
void  Files_BroadcastLoad(int nParam);

// ---------------------------------------------------------------------------
// 2. File-path helpers
// ---------------------------------------------------------------------------

// 0x004286a0  Strip extension from pszPath then append pszNewExt
void  Files_ReplaceExtension(char *pszPath, int pszNewExt);

// 0x004287a0  Read one line from file handle, strip trailing newlines
int   Files_ReadLine(char *pszBuf, int nParam2, int nParam3);

// ---------------------------------------------------------------------------
// 3. Win32 common-dialog wrappers
// ---------------------------------------------------------------------------

// 0x004289f0  OFN hook proc: centres dialog, realizes palette, sets timer
int   APIENTRY Files_OFNHookProc(HWND hDlg, int nMsg);

// 0x00428bb0  Theme-timer callback: kill timer, post WM_ACTIVATE
void  Files_OFNTimerCallback(void);

// 0x00428c80  GetOpenFileNameA for .SAV files; returns 0=ok, -1=cancel
int   Files_SelectLoadGame(int nPathBuf, HWND hOwner);

// 0x00428e30  GetSaveFileNameA for .SAV files; returns 0=ok, -1=cancel
int   Files_SelectSaveGame(int nPathBuf, HWND hOwner);

// 0x00429010  Set the current save-directory pointer
void  Files_SetSaveDir(int nDirPtr);

// 0x004290a0  Clear the save-directory pointer
void  Files_ClearSaveDir(void);

// 0x00429130  GetSaveFileNameA for arbitrary extension; returns 0=ok, -1=cancel
int   Files_SelectFile(int nPathBuf, LPCSTR pszExt, int pszLabel);

// ---------------------------------------------------------------------------
// 4. Bitmap loading
// ---------------------------------------------------------------------------

// 0x00429320  Load bitmap resource by numeric index; calls BmpFileToHBitmap
void  Files_LoadBitmapByNum(int nResNum, int nPalHandle);

// 0x00429440  Convert in-memory 256-colour BMP to HBITMAP; remaps palette
HBITMAP Files_BmpFileToHBitmap(int nBmpData, int nPalHandle);

// ---------------------------------------------------------------------------
// 5. Edit-control text helpers
// ---------------------------------------------------------------------------

// 0x00429950  WM_GETTEXT into a LocalAlloc buffer; stores handle in g_nEditTextLocal
LPVOID Files_GetEditText(HWND hCtl);

// 0x00429a70  WM_SETTEXT from g_nEditTextLocal then free the buffer
void  Files_SetEditText(HWND hCtl);

// 0x00429b50  True if substring [pMatch..pMatch+nLen) is on a word boundary
int   Files_IsWholeWord(int pMatch, int pBufStart, int pBufEnd, int nLen);

// 0x00429ca0  Init Find/Replace state: set owner HWND, zero string buffers
void  Files_InitFindReplace(int nOwnerHwnd);

// 0x00429d50  Search hCtl for search string; select match if found. Returns 1=found
int   Files_SearchText(HWND hCtl, int pFR, int nUnused,
                       int bForward, int bMatchCase, int bWholeWord);

// 0x0042a090  EM_REPLACESEL with replace string from pFR+0x14
void  Files_ReplaceSelection(HWND hCtl, int pFR);

// 0x0042a130  Process FINDMSGSTRING message; returns 1 if dialog closed
int   Files_HandleFindMessage(int pFR, HWND hEdit);

// 0x0042a440  Zero g_nFindDialog and g_nFindDialogOpen
void  Files_CloseFindDialog(void);

// 0x0042a4e0  Open modeless Find (nMode=1) or Replace (nMode=2) dialog
HWND  Files_OpenFindDialog(int nOwner, int nMode);

// ---------------------------------------------------------------------------
// 6. WATCOM debugger detection
// ---------------------------------------------------------------------------

// 0x0042a660  EnumWindows scan for "The WATCOM Debugger"; sets g_nFullscreen
void  Files_FindWatcomDebugger(void);

// 0x0042a6f0  WNDENUMPROC: detect WATCOM debugger by window title
bool  CALLBACK Files_EnumWindowsProc(HWND hWnd, LPARAM lParam);
