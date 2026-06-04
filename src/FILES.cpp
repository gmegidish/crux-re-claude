// FILES.cpp — Game file I/O, save/load game state, palette loading, bitmap
//             loading, Win32 common-dialog wrappers, and text editor helpers
//
// This is a wide-purpose file-management layer.  Despite the name it is NOT a
// low-level file abstraction — it sits above Win32 and the bunch-resource
// system (READRES.cpp) and implements the following distinct subsystems:
//
//  1. GAME STATE SAVE / LOAD
//     Files_SaveState / Files_LoadState / Files_SaveStateAlt serialize and
//     deserialize hundreds of game variables in one shot (very large functions
//     with ~200 locals each).  Files_SaveGame / Files_LoadSaveGame manage the
//     on-disk .SAV file including a header struct and RLE-compressed payload.
//     Files_SaveGameFull / Files_LoadGameFull are higher-level wrappers that
//     enumerate all registered "special save" callbacks and the core state.
//     Files_EraseStates deletes *.ST save files from the save directory.
//     Files_LoadInvMain loads the ADVENT/INVMAIN inventory database.
//
//  2. SPECIAL SAVE CALLBACK REGISTRY
//     Up to 10 [save_fn, load_fn] pairs can be registered via
//     Files_RegisterSpecialSave / Files_UnregisterSpecialSave.
//     Files_BroadcastSave / Files_BroadcastLoad invoke all registered pairs.
//
//  3. PALETTE LOADING
//     Files_LoadPalExternal / Files_LoadPal / Files_DevLoadPal all load an
//     8-bit (256-entry) palette from the resource system into the game's
//     active palette, using SETPAL helpers.
//
//  4. BITMAP LOADING
//     Files_LoadBitmapByNum — loads a resource by index/char code and calls
//     Files_BmpFileToHBitmap.
//     Files_BmpFileToHBitmap — converts an in-memory BMP file (256-colour
//     only) to a Win32 HBITMAP, remapping palette entries via
//     SetPal_FindNearestColor.
//
//  5. WIN32 COMMON DIALOGS  (Save/Load/Select)
//     Files_SelectLoadGame — GetOpenFileNameA for .SAV files
//     Files_SelectSaveGame — GetSaveFileNameA for .SAV files
//     Files_SelectFile     — GetSaveFileNameA for arbitrary extension
//     All three use Files_OFNHookProc to centre the dialog on screen and
//     realize the game palette; Files_OFNTimerCallback fires one frame later
//     to bring the dialog window to the foreground.
//     Files_SetSaveDir / Files_ClearSaveDir manage the initial-directory
//     pointer passed to the OFN structs.
//
//  6. RLE DECOMPRESSION
//     Files_RleDecompress — simple RLE with 0xFF escape byte (run: 0xFF,
//     value, count).
//
//  7. FILE-PATH HELPERS
//     Files_ReplaceExtension — strips extension and appends a new one.
//     Files_ReadLine         — wrapper around FUN_0048a790 that strips
//                              trailing newlines.
//
//  8. TEXT EDITOR HELPERS (Find / Replace dialog)
//     Files_GetEditText / Files_SetEditText — LocalAlloc-based WM_GETTEXT /
//     WM_SETTEXT wrappers.
//     Files_IsWholeWord — tests that a substring match sits on word boundary
//     (space / tab / comma / period / CR or start/end of buffer).
//     Files_InitFindReplace — initialises the FINDREPLACEA struct and buffers.
//     Files_SearchText — performs forward/backward case-sensitive or
//     case-insensitive search+select inside an Edit control.
//     Files_ReplaceSelection — sends EM_REPLACESEL.
//     Files_HandleFindMessage — processes FINDMSGSTRING messages (Find Next,
//     Replace, Replace All, dialog closed).
//     Files_CloseFindDialog / Files_OpenFindDialog — open/close the modeless
//     FindTextA or ReplaceTextA common dialog.
//
//  9. WATCOM DEBUGGER DETECTION
//     Files_FindWatcomDebugger — EnumWindows scan; sets g_nFullscreen when
//     "The WATCOM Debugger" window title is detected.
//     Files_EnumWindowsProc   — the WNDENUMPROC callback.
//
// Original source: C:\DevStudio\Projects\Crux\FILES.cpp
// RE address range: 0x004223a0 – 0x0042a6f0
// Functions: 41

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <string.h>
#include "FILES.h"
#include "SETPAL.h"
#include "READRES.h"
#include "ERRORS.h"
#include "SAFEHEAP.h"
#include "THEMES.h"

// ============================================================
//  Globals
// ============================================================

// Area cache — count and table freed by Files_FreeAreaCache
int  g_nAreaCacheCount   = 0;         // 0x007127d0
int  g_anAreaCacheTable[1];           // 0x0070c258  (array of handles)
int  g_anAreaCacheSlots[15];          // 0x007114d0  reset to 0xffffffff
int  g_nAreaCacheActive  = 0;         // 0x007127ec

// Special-save callback registry
int  g_nSpecialSaveCount  = 0;        // 0x006b8234  (max 10)
int  g_nSpecialSaveFuncs[1];          // 0x006b8190  [save_fn, load_fn] pairs, stride 8

// Save directory override (pointer stored as int)
int  g_nSaveDir          = 0;         // 0x006b81e0

// Save-game base directory path
char g_abSaveGameDir[260];            // 0x007d6248
char g_abAdventDir[260];             // 0x007d6468  ADVENT resource directory

// OFN dialog helpers
int  g_nOFNTimerId       = -1;        // 0x004cd970  theme-timer ID; -1 = none
int  g_nOFNParentWnd     = 0;         // 0x006b82b4  HWND of OFN dialog parent
int  g_nScreenWidth      = 0;         // 0x006b8f00  used to centre dialog
int  g_nScreenHeight     = 0;         // 0x006b8f08  used to centre dialog

// OFN result strings (set empty when dialog is cancelled)
char g_abLoadGameResult[1];           // 0x006b82b8
char g_abSaveGameResult[1];           // 0x006b82bc
char g_abSelectFileResult[1];         // 0x006b82c0

// Edit-control text buffer handle
int  g_nEditTextLocal    = 0;         // 0x006b8d4c  HLOCAL

// Find/Replace dialog state
int  g_nSearchString     = 0;         // 0x006b8c60  pointer to search string
int  g_nFindReplaceOwner = 0;         // 0x006b8ce8  owner HWND
char g_abFindString[81];              // 0x006b8cf0
char g_abReplaceString[81];           // 0x006b8c68
int  g_nFindReplaceStruct = 0;        // 0x006b8cc0  FINDREPLACEA struct base
int  g_nFindDialogOpen   = 0;         // 0x006b8d44
int  g_nFindDialog       = 0;         // 0x006b8d48  HWND of active dialog

// Fullscreen flag (also written by SETPAL; shared)
// int g_nFullscreen at 0x006b8d80 — declared in SETPAL.h


// ============================================================
//  1. Game state serialization
// ============================================================

// 0x004223a0
// Serialize a large set of game state variables to param_1 (a buffer/handle).
// Hundreds of locals mirror individual game variables.  Called by
// Files_SaveGameFull.
void Files_SaveState(int nBuf)
{
    // ~200 game variables written sequentially to nBuf.
    // [body omitted — decompile is too large to reconstruct cleanly]
    (void)nBuf;
}

// 0x00422d00
// Deserialize game state from param_1 back into the live game variables.
// Mirror of Files_SaveState.  Called by Files_LoadGameFull.
void Files_LoadState(int nBuf)
{
    (void)nBuf;
}

// 0x00423710
// Alternate/secondary save of a further set of game variables (e.g. room
// state, NPC positions) not covered by Files_SaveState.
void Files_SaveStateAlt(void)
{
}

// 0x00424490
// Free all cached area entries and reset the 15-slot cache table to
// 0xffffffff.  Acquires g_nAreaCritSec and DAT_0070ae78 around the work.
void Files_FreeAreaCache(void)
{
    // EnterCriticalSection(g_nAreaCritSec);
    // for each g_anAreaCacheTable[i]  SafeHeap_Free(...)
    // for (i=0; i<15; i++) g_anAreaCacheSlots[i] = -1;
    // EnterCriticalSection(DAT_0070ae78);  g_nAreaCacheActive = 0;
    // LeaveCriticalSection ...
}

// 0x00424980
// Simple RLE decompression.  Escape byte is 0xFF.  Format: 0xFF, value, count
// expands to `count` copies of `value`; any other byte copies through.
// param_1 = source, param_2 = destination, param_3 = compressed size.
void Files_RleDecompress(char *pSrc, char *pDst, int nSrcLen)
{
    int   nPos  = 0;
    char *pS    = pSrc;
    char *pD    = pDst;

    while (nPos < nSrcLen)
    {
        if ((unsigned char)*pS == 0xFF)
        {
            char   cVal   = pS[1];
            unsigned int nCount = (unsigned char)pS[2];
            pS += 3;
            for (unsigned int i = 0; i < nCount; i++)
                *pD++ = cVal;
            nPos += 3;
        }
        else
        {
            *pD++ = *pS++;
            nPos++;
        }
    }
}

// 0x00424af0
// Load an external (named) palette file: look up param_1 in resource system,
// read 800-byte buffer, call SetPal_* helpers.
// Debug name: "load_pal_external char* name, uchar* ..."
void Files_LoadPalExternal(int nResId, int nParam2)
{
    (void)nResId; (void)nParam2;
}

// 0x00424ca0
// Load a palette by name (25-entry variant).
// Debug name: "load_pal char* name, uchar* pal, 25..."
void Files_LoadPal(int nResId, int nParam2, unsigned char nParam3)
{
    (void)nResId; (void)nParam2; (void)nParam3;
}

// 0x00424ec0
// Development-build palette load (reads raw .PAL resource directly).
// Debug name: "dev_load_pal char* name, uchar* pa..."
int Files_DevLoadPal(int nResId, int nParam2)
{
    (void)nResId; (void)nParam2;
    return -1;
}

// 0x00425020
// Load a save-game file from disk.  Reads a header (magic, version, size),
// verifies it, then reads the compressed body and calls Files_RleDecompress +
// Files_LoadState.
// Returns 0 on success, -1 on failure.
int Files_LoadSaveGame(int nParam1)
{
    (void)nParam1;
    return -1;
}

// 0x004253c0
// Load ADVENT/INVMAIN inventory database: builds path
// g_abAdventDir + "ADVENT" + ext, opens via resource system.
void Files_LoadInvMain(void)
{
}

// 0x004258c0
// Delete all *.ST state files in g_abSaveGameDir.
// Uses FindFirstFileA / FindNextFileA / DeleteFileA.
void Files_EraseStates(void)
{
}

// 0x00425a20
// Serialise the full game state to a .SAV file.  Calls Files_SaveState,
// Files_SaveStateAlt, then writes the compressed result to disk.
void Files_SaveGame(int nSlot)
{
    (void)nSlot;
}

// 0x00426530
// Register a [save_fn, load_fn] callback pair in the special-save table.
// Asserts if count would exceed 10.
void Files_RegisterSpecialSave(int nSaveFn, int nLoadFn)
{
    // if (g_nSpecialSaveCount > 9) Err_BadResEntry(...);
    // g_nSpecialSaveFuncs[g_nSpecialSaveCount*2 + 0] = nSaveFn;
    // g_nSpecialSaveFuncs[g_nSpecialSaveCount*2 + 1] = nLoadFn;
    // g_nSpecialSaveCount++;
    (void)nSaveFn; (void)nLoadFn;
}

// 0x00426620
// Remove all entries whose save-fn matches nSaveFn.  Compacts the table.
void Files_UnregisterSpecialSave(int nSaveFn)
{
    (void)nSaveFn;
}

// 0x00426750
// Full game save: calls all registered special-save callbacks via
// Files_BroadcastSave, then calls the core Files_SaveGame.
int Files_SaveGameFull(int nSlot)
{
    (void)nSlot;
    return -1;
}

// 0x00427530
// Call every registered save callback with param_1.
void Files_BroadcastSave(int nParam)
{
    // for (i=0; i<g_nSpecialSaveCount; i++)
    //   ((void(*)(int))g_nSpecialSaveFuncs[i*2])(nParam);
    (void)nParam;
}

// 0x004275f0
// Full game load: calls all registered load callbacks via Files_BroadcastLoad,
// then calls Files_LoadSaveGame.
int Files_LoadGameFull(int nSlot)
{
    (void)nSlot;
    return -1;
}

// 0x004285e0
// Call every registered load callback with param_1.
void Files_BroadcastLoad(int nParam)
{
    // for (i=0; i<g_nSpecialSaveCount; i++)
    //   ((void(*)(int))g_nSpecialSaveFuncs[i*2+1])(nParam);
    (void)nParam;
}


// ============================================================
//  2. File-path helpers
// ============================================================

// 0x004286a0
// Strip the extension from pszPath then append pszNewExt (via DAT_004cd944
// separator + pszNewExt).
void Files_ReplaceExtension(char *pszPath, int pszNewExt)
{
    size_t nLen = strlen(pszPath);
    while (--nLen != 0)
    {
        if (pszPath[nLen] == '.') break;
    }
    if (nLen != 0)
        pszPath[nLen] = '\0';
    // FUN_004895f0(pszPath, separator);
    // FUN_004895f0(pszPath, pszNewExt);
    (void)pszNewExt;
}

// 0x004287a0
// Read one line (wrapping FUN_0048a790), then strip all trailing '\n'.
int Files_ReadLine(char *pszBuf, int nParam2, int nParam3)
{
    // int nRet = FUN_0048a790(pszBuf, nParam2, nParam3);
    // while (pszBuf[strlen(pszBuf)-1] == '\n')
    //   pszBuf[strlen(pszBuf)-1] = '\0';
    // return nRet;
    (void)pszBuf; (void)nParam2; (void)nParam3;
    return 0;
}


// ============================================================
//  3. OFN common-dialog helpers
// ============================================================

// 0x004289f0
// APIENTRY OFNHookProc: on WM_INITDIALOG (0x18) centres the parent on screen
// and starts a one-shot theme timer to realize the game palette.
// On WM_ACTIVATE (0x401) brings the window to foreground.
int APIENTRY Files_OFNHookProc(HWND hDlg, int nMsg)
{
    if (nMsg == WM_INITDIALOG)
    {
        HWND hParent = GetParent(hDlg);
        RECT rc;
        GetWindowRect(hParent, &rc);
        int nW = rc.right - rc.left + 1;
        int nH = rc.bottom - rc.top + 1;
        SetWindowPos(hParent, (HWND)0xFFFFFFFF,
                     (g_nScreenWidth  - nW) / 2,
                     (g_nScreenHeight - nH) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
        SetForegroundWindow(hParent);
        HDC hdc = GetDC(hParent);
        RealizePalette(hdc);
        ReleaseDC(hParent, hdc);
        if (g_nOFNTimerId == -1)
        {
            g_nOFNParentWnd = (int)hParent;
            // g_nOFNTimerId = Theme_SetTimer(Files_OFNTimerCallback, 3);
        }
    }
    else if (nMsg == 0x401)   // WM_ACTIVATE
    {
        HWND hParent = GetParent(hDlg);
        SetWindowPos(hParent, (HWND)0xFFFFFFFF, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        SetForegroundWindow(hParent);
    }
    return 0;
}

// 0x00428bb0
// Theme-timer callback: kill timer, post WM_ACTIVATE to the OFN parent.
void Files_OFNTimerCallback(void)
{
    if (g_nOFNTimerId != -1)
    {
        // Theme_KillTimer(g_nOFNTimerId);
        g_nOFNTimerId = -1;
        if (g_nOFNParentWnd != 0)
            PostMessageA((HWND)(INT_PTR)g_nOFNParentWnd, 0x401, 0, 0);
    }
}

// 0x00428c80
// Show an Open File dialog filtered to Save Games (*.sav).
// Returns 0 and copies the path to param_1 on success; -1 on cancel.
int Files_SelectLoadGame(int nPathBuf, HWND hOwner)
{
    (void)nPathBuf; (void)hOwner;
    // OPENFILENAME ofn = {0};
    // ... filter "Save Games\0*.sav\0" ...
    // GetOpenFileNameA(&ofn);
    return -1;
}

// 0x00428e30
// Show a Save File dialog filtered to Save Games (*.sav).
// Returns 0 and copies the path to param_1 on success; -1 on cancel.
int Files_SelectSaveGame(int nPathBuf, HWND hOwner)
{
    (void)nPathBuf; (void)hOwner;
    // OPENFILENAME ofn = {0};
    // ... filter "Save Games\0*.sav\0" ...
    // GetSaveFileNameA(&ofn);
    return -1;
}

// 0x00429010
// Store param_1 as the current save directory pointer.
void Files_SetSaveDir(int nDirPtr)
{
    g_nSaveDir = nDirPtr;
}

// 0x004290a0
// Clear the save directory pointer (reset to NULL/0).
void Files_ClearSaveDir(void)
{
    g_nSaveDir = 0;
}

// 0x00429130
// Generic file-save dialog.  pszExt is both the default extension and the
// filter string; pszLabel is the filter display name.
// Returns 0 on success, -1 on cancel.
int Files_SelectFile(int nPathBuf, LPCSTR pszExt, int pszLabel)
{
    (void)nPathBuf; (void)pszExt; (void)pszLabel;
    // builds filter "%s\0*.%s\0"; calls GetSaveFileNameA
    return -1;
}


// ============================================================
//  4. Bitmap loading
// ============================================================

// 0x00429320
// Load bitmap resource by numeric index + char sub-code, read it synchronously,
// then convert to HBITMAP via Files_BmpFileToHBitmap.
void Files_LoadBitmapByNum(int nResNum, int nPalHandle)
{
    (void)nResNum; (void)nPalHandle;
    // Res_FindByNumChar(0x28, nResNum, szPath, 1, lpDst, &hEntry);
    // FUN_0048ac60(hEntry);
    // Res_BunchFreadNow(hEntry, 1, lpDst);
    // Files_BmpFileToHBitmap(hEntry, nPalHandle);
}

// 0x00429440
// Convert an in-memory BMP file (256-colour DIB only) to a Win32 HBITMAP.
// Palette entries are remapped through SetPal_FindNearestColor (6-bit R,G,B).
// Bottom-up DIBs are flipped to top-down before CreateBitmapIndirect.
HBITMAP Files_BmpFileToHBitmap(int nBmpData, int nPalHandle)
{
    (void)nBmpData; (void)nPalHandle;
    // reads BITMAPFILEHEADER at offset 0  (magic 0x4D42 = 'BM')
    // reads BITMAPINFOHEADER at offset 14
    // asserts nColors == 256
    // allocates palette + pixel buffers via SafeHeap_Alloc
    // if top-down (height > 0): flip rows
    // remaps each pixel's palette index via SetPal_FindNearestColor
    // CreateBitmapIndirect(&bm)
    return NULL;
}


// ============================================================
//  5. Edit-control text helpers
// ============================================================

// 0x00429950
// Allocate a LocalAlloc buffer, WM_GETTEXT the edit control into it.
// Stores HLOCAL in g_nEditTextLocal.  Returns pointer to text, or NULL.
LPVOID Files_GetEditText(HWND hCtl)
{
    LRESULT nLen = SendMessageA(hCtl, WM_GETTEXTLENGTH, 0, 0);
    g_nEditTextLocal = (int)LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                                       (SIZE_T)(nLen + 1));
    if (!g_nEditTextLocal)
        return NULL;
    LPVOID pBuf = LocalLock((HLOCAL)(INT_PTR)g_nEditTextLocal);
    if (!pBuf)
    {
        LocalFree((HLOCAL)(INT_PTR)g_nEditTextLocal);
        g_nEditTextLocal = 0;
        return NULL;
    }
    SendMessageA(hCtl, WM_GETTEXT, (WPARAM)(nLen + 1), (LPARAM)pBuf);
    return pBuf;
}

// 0x00429a70
// WM_SETTEXT the locked g_nEditTextLocal buffer, then unlock and free it.
void Files_SetEditText(HWND hCtl)
{
    LPVOID pBuf = LocalLock((HLOCAL)(INT_PTR)g_nEditTextLocal);
    SendMessageA(hCtl, WM_SETTEXT, 0, (LPARAM)pBuf);
    LocalUnlock((HLOCAL)(INT_PTR)g_nEditTextLocal);
    LocalFree((HLOCAL)(INT_PTR)g_nEditTextLocal);
    g_nEditTextLocal = 0;
}

// 0x00429b50
// Return non-zero if the substring at pMatch[0..nLen-1] sits on a word
// boundary (preceded and followed by space/tab/comma/period/CR or
// start/end of buffer).
int Files_IsWholeWord(int pMatch, int pBufStart, int pBufEnd, int nLen)
{
    // check char before pMatch
    int bBefore = (pMatch == pBufStart ||
                   *(char*)(pMatch-1) == ' '  ||
                   *(char*)(pMatch-1) == '\t' ||
                   *(char*)(pMatch-1) == ','  ||
                   *(char*)(pMatch-1) == '.'  ||
                   *(char*)(pMatch-1) == '\r');
    // check char after pMatch+nLen
    int bAfter  = (pMatch + nLen == pBufEnd ||
                   *(char*)(pMatch+nLen) == ' '  ||
                   *(char*)(pMatch+nLen) == '\t' ||
                   *(char*)(pMatch+nLen) == ','  ||
                   *(char*)(pMatch+nLen) == '.'  ||
                   *(char*)(pMatch+nLen) == '\r');
    return bBefore && bAfter;
}

// 0x00429ca0
// Initialise Find/Replace dialog state: store owner HWND, zero both string
// buffers.
void Files_InitFindReplace(int nOwnerHwnd)
{
    g_nFindReplaceOwner = nOwnerHwnd;
    memset(&g_abFindString,    0, 0x51);
    memset(&g_abReplaceString, 0, 0x51);
}

// 0x00429d50
// Search hCtl (an Edit control) for the string in *pFR.
//   bForward  = search direction (0 = backward)
//   bMatchCase
//   bWholeWord
// Selects the match if found.  Returns 1 if found, 0 if not.
int Files_SearchText(HWND hCtl, int pFR, int nUnused,
                     int bForward, int bMatchCase, int bWholeWord)
{
    (void)nUnused;
    // g_nSearchString = *(char**)(pFR + 0x10);
    // pBuf = Files_GetEditText(hCtl);
    // ... bidirectional scan with optional case-fold and word-boundary test ...
    // Files_SetEditText(hCtl, pBuf);
    // SendMessageA(hCtl, EM_SETSEL, matchStart, matchEnd);
    // SendMessageA(hCtl, EM_SCROLLCARET, 0, 0);
    return 0;
}

// 0x0042a090
// Send EM_REPLACESEL with the replace string from pFR (offset 0x14).
void Files_ReplaceSelection(HWND hCtl, int pFR)
{
    SendMessageA(hCtl, 0xC2 /*EM_REPLACESEL*/, 0, *(LPARAM*)(pFR + 0x14));
}

// 0x0042a130
// Dispatch a FINDMSGSTRING message from the modeless Find/Replace dialog.
// Handles: Find Next (FR_FINDNEXT), Replace (FR_REPLACE), Replace All
// (FR_REPLACEALL), and dialog-closed (FR_DIALOGTERM).
// Returns 1 if dialog was closed, 0 otherwise.
int Files_HandleFindMessage(int pFR, HWND hEdit)
{
    (void)pFR; (void)hEdit;
    // if FR_DIALOGTERM: Files_CloseFindDialog(); return 1;
    // if FR_FINDNEXT:   Files_SearchText(...);
    //                   if not found: MessageBoxA("String not found");
    // if FR_REPLACE:    Files_SearchText(...);
    //                   if found: Files_ReplaceSelection(...);
    //                   else:     MessageBoxA("String not found");
    // if FR_REPLACEALL: loop Files_SearchText until no more matches;
    //                   MessageBoxA("Done replacing all" | "String not found");
    return 0;
}

// 0x0042a440
// Close/reset the Find/Replace dialog: clear g_nFindDialog and
// g_nFindDialogOpen.
void Files_CloseFindDialog(void)
{
    g_nFindDialog     = 0;
    g_nFindDialogOpen = 0;
}

// 0x0042a4e0
// Open the modeless Find (nMode=1) or Replace (nMode=2) dialog.
// If already open, bring to foreground via thunk_FUN_00486d80 (SetForegroundWindow).
// Stores HWND in g_nFindDialog; sets g_nFindDialogOpen = 1.
HWND Files_OpenFindDialog(int nOwner, int nMode)
{
    if (!g_nFindDialogOpen)
    {
        memset((void*)(INT_PTR)g_nFindReplaceStruct, 0, 0x28);
        // populate FINDREPLACEA struct ...
        if (nMode == 1)
            g_nFindDialog = (int)FindTextA((LPFINDREPLACEA)(INT_PTR)g_nFindReplaceStruct);
        else
            g_nFindDialog = (int)ReplaceTextA((LPFINDREPLACEA)(INT_PTR)g_nFindReplaceStruct);
        if (!g_nFindDialog)
            CommDlgExtendedError();
        g_nFindDialogOpen = 1;
    }
    else
    {
        // thunk_FUN_00486d80(g_nFindDialog);  // SetForegroundWindow
    }
    (void)nOwner;
    return (HWND)(INT_PTR)g_nFindDialog;
}


// ============================================================
//  6. WATCOM debugger detection
// ============================================================

// 0x0042a660
// Enumerate all top-level windows to detect "The WATCOM Debugger".
// Sets g_nFullscreen if found (so the game adjusts its display mode).
void Files_FindWatcomDebugger(void)
{
    EnumWindows((WNDENUMPROC)Files_EnumWindowsProc, 0);
}

// 0x0042a6f0
// WNDENUMPROC: if window title == "The WATCOM Debugger" set g_nFullscreen=1.
bool CALLBACK Files_EnumWindowsProc(HWND hWnd, LPARAM /*lParam*/)
{
    char szTitle[20];
    GetWindowTextA(hWnd, szTitle, sizeof(szTitle));
    if (strcmp(szTitle, "The WATCOM Debugger") == 0)
        g_nFullscreen = 1;
    return strcmp(szTitle, "The WATCOM Debugger") != 0;
}
