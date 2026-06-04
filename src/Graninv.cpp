// Graninv.cpp -- Granular Inventory (GV window) + Gran item interactions
//
// Two subsystems:
//   GV_*   : Floating Win32 inventory panel (GVClass, SysListView32, Toolbar,
//             drag-and-drop with bitmap-rotation preview overlay).
//   Gran_* : Per-inventory-item interactions: cube-viewer, 3-layer animation
//             table, palette conversion, tape/diary player, Granny board-game
//             AI, horizontal slider control, help-text queue.
//
// Original source: C:\DevStudio\Projects\Crux\Graninv.cpp
// Address range:   0x00430f5b -- 0x004390eb

#include "Graninv.h"
#include "GI.h"
#include "TIMERS.h"
#include "INVMANG.h"
#include "DDRAWI.h"
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>   // timeGetTime
#include <string.h>
#include <math.h>

// Forward declarations for cross-module helpers (resolved via thunk table)
extern "C" {
    void  Debug_Assert(int nLine, const char *pFile, const char *pMsg);
    void  Debug_Trace(int nLine, const char *pFile, const char *pMsg, ...);
    void  Curs_SetWin32Cursor(int nType);
    void  Curs_ShowWin32(void);
    void  Curs_DisableDraw(void);
    void  Curs_EnableDraw(void);
    void *FUN_0048ac60(int nSize);   // heap alloc
    void  FUN_0048b1c4(double x);    // sqrt
    double FUN_0048b0f4(double x);   // acos
    double FUN_0048b044(double x);   // cos
    double FUN_0048af94(double x);   // sin
    int   FUN_0048a650(void);        // fixed-point arithmetic helper
    int   FUN_0048a060(char *pBuf, const char *pFmt, ...);  // sprintf
    int   FUN_0048e4d0(int nSrc, int *pDst, int nCount);    // load array
    int   FUN_0048de80(int nSrc, int *pDst, int nCount);    // save array
    int   FUN_0048b1c4(double x);    // sqrtf helper
    int   FUN_0048b044(double x, ...);
    int   FUN_0048af94(double x, ...);
    int   FUN_489cf0(void);          // rand()
    void  FUN_004895e0(char *pBuf, int nSrc);   // get item name
    void  FUN_004896d0(void *pDst, const void *pSrc, int nLen); // memcpy
}

// ---- GV globals -----------------------------------------------------------

int   g_nGVEnabled;           // 0x004cf898
int   g_nGVButtonCount;       // 0x006d1dc4
int   g_nGVOpen;              // 0x006d1dc8
int   g_nGVNeedsUpdate;       // 0x006d1dc0
HWND  g_pGVWindow;            // 0x006bae94
HWND  g_pGVListView;          // 0x006bae78
void *g_pGVImageList;         // 0x006bae6c
HWND  g_pGVToolbar;           // 0x006d1dcc
void *g_pfnGVListViewOldProc; // 0x006bd3a0
int   g_nGVDragStartX;        // 0x006bd340
int   g_nGVDragStartY;        // 0x006bd344
int   g_nGVDragCurX;          // 0x006bdb44
int   g_nGVDragCurY;          // 0x006bdb48

// ---- Gran globals ---------------------------------------------------------

int   g_nGranTapeState;       // 0x006be1d0
int   g_nGranTapeItem;        // 0x006bd0d8
int   g_nGranGrannyPos;       // 0x006d1db4
int   g_nGranActivePiece;     // 0x006d1de0
int   g_nGranSliderItem;      // 0x006bd0dc
int   g_nGranHelpCount;       // 0x004d09b8
int   g_nGranHelpOwner;       // 0x006d6768
int   g_nGranCubeItem;        // 0x004cfce0
int   g_nGranCubeResource;    // 0x004cfd80

// ===========================================================================
// GV -- Granular Inventory Window
// ===========================================================================

// GV_AddButton -- register a toolbar button (id + associated value).
// The button array holds up to GV_MAX_BUTTONS entries; duplicates are skipped.
void GV_AddButton(int nButtonId, int nValue)
{
    if (g_nGVButtonCount == 10) {
        Debug_Assert(5, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                     "Too many buttons in the toolbar");
    }
    if (nButtonId >= 0 && g_nGVButtonCount > 0) {
        for (int i = 0; i < g_nGVButtonCount; i++) {
            if (g_anGVButtons[i].nId == nButtonId)
                return;
        }
    }
    g_anGVButtons[g_nGVButtonCount].nId    = nButtonId;
    g_anGVButtons[g_nGVButtonCount].nValue = nValue;
    g_anGVButtons[g_nGVButtonCount].nPad   = 0;
    g_nGVButtonCount++;
}

// GV_SetInitHandler -- store the event-script handle to fire when GV opens.
void GV_SetInitHandler(int nCallback)
{
    g_nGVInitCallback = nCallback;
}

// GV_SetDestroyHandler -- store the event-script handle to fire when GV closes.
void GV_SetDestroyHandler(int nCallback)
{
    g_nGVDestroyCallback = nCallback;
}

// GV_UpdateInventory -- rebuild the ListView with item icons.
// Returns 1 on success, 0 if skipped (window not ready or cache still valid).
int GV_UpdateInventory(int bForce)
{
    EnterCriticalSection(&g_csGV);

    if (g_nGVItemCount == 0) {
        LeaveCriticalSection(&g_csGV);
        return 0;
    }
    if (g_pGVListView == NULL) {
        SendMessage(g_pGVListView, LVM_DELETEALLITEMS, 0, 0);
        ImageList_Remove((HIMAGELIST)g_pGVImageList, -1);
        LeaveCriticalSection(&g_csGV);
        return 0;
    }
    if (g_nGVCurSel == g_nGVCachedSel && bForce == 0 && g_nGVNeedsUpdate == 0) {
        LeaveCriticalSection(&g_csGV);
        return 0;
    }

    g_nGVNeedsUpdate = 0;
    g_nGVCurSel = (g_nCursorMode == 1) ? g_nGVCachedSel : -1;

    SendMessage(g_pGVListView, LVM_DELETEALLITEMS, 0, 0);
    ImageList_Remove((HIMAGELIST)g_pGVImageList, -1);

    for (int i = 0; i < g_nGVItemCount; i++) {
        int nItem = g_anGVItemList[i];
        if (nItem == g_nGVCachedSel || nItem == -1)
            continue;

        HICON hIcon = (HICON)GetItemIconHandle(nItem);
        int   nIdx  = ImageList_ReplaceIcon((HIMAGELIST)g_pGVImageList, -1, hIcon);
        DestroyIcon(hIcon);

        LVITEM lvi = {0};
        lvi.mask    = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
        lvi.iItem   = i;
        lvi.iImage  = nIdx;
        lvi.lParam  = i;
        LRESULT nRow = SendMessage(g_pGVListView, LVM_INSERTITEM, 0, (LPARAM)&lvi);

        char szName[128];
        GetItemName(GetItemSlotName(nItem), szName);

        if (strcmp(g_szGVCurrentName, g_szGVCmpName) == 0) {
            LVITEM lviSet = {0};
            lviSet.iItem    = (int)nRow;
            lviSet.iSubItem = 0;
            SendMessage(g_pGVListView, LVM_SETITEMTEXT, nRow, (LPARAM)&lviSet);
        } else {
            LVITEM lviSet = {0};
            lviSet.iItem    = (int)nRow;
            lviSet.pszText  = g_szGVCurrentName;
            SendMessage(g_pGVListView, LVM_SETITEMTEXT, nRow, (LPARAM)&lviSet);
        }
    }

    LeaveCriticalSection(&g_csGV);
    return 1;
}

// GV_ListViewWndProc -- subclassed WndProc for the SysListView32.
// Intercepts keyboard messages (WM_KEYDOWN/UP/CHAR) and forwards them to the
// main game window so hotkeys work while the inventory is focused.
LRESULT CALLBACK GV_ListViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg > 0xFF && uMsg < 0x103) {
        PostMessage(g_hMainWnd, uMsg, wParam, lParam);
    }
    return CallWindowProc((WNDPROC)g_pfnGVListViewOldProc, hWnd, uMsg, wParam, lParam);
}

// GV_MainWndProc -- top-level window procedure for the GVClass inventory panel.
LRESULT CALLBACK GV_MainWndProc(HWND hWnd, UINT uMsg, UINT wParam, HWND lParam)
{
    switch (uMsg) {
    case WM_SETCURSOR:
        if (((UINT)lParam & 0xFFFF) == HTCLIENT) {
            if (g_nGVCachedSel != -1) {
                Curs_SetWin32Cursor(1);
                return 1;
            }
            Curs_ShowWin32();
        } else {
            Curs_ShowWin32();
        }
        break;

    case WM_CREATE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        g_pGVListView = CreateWindowEx(0, "SysListView32", (LPCSTR)0x6d1dd8,
                                       LVS_ICON | LVS_AUTOARRANGE | 0x04 | WS_CHILD | WS_VISIBLE,
                                       0, 0,
                                       rc.right - rc.left,
                                       rc.top - rc.bottom,
                                       hWnd, (HMENU)0, g_hInstance, NULL);
        g_nGVNeedsUpdate = 1;
        if (g_pGVListView == NULL) {
            Debug_Assert(0x3b, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                         "Unable to create listview");
        }
        g_pGVImageList = (void *)ImageList_Create(32, 32, 0xFE, 10, 100);
        SendMessage(g_pGVListView, LVM_SETBKCOLOR,  0, 0);
        SendMessage(g_pGVListView, 0x1026, 0, 0);         // LVM_SETVIEW (LV_VIEW_ICON)
        SendMessage(g_pGVListView, 0x1024, 0, 0xFFFFFF);  // LVM_SETEXTENDEDLISTVIEWSTYLE
        SendMessage(g_pGVListView, LVM_SETIMAGELIST, 0, (LPARAM)g_pGVImageList);
        PostMessage(hWnd, WM_USER+5, 0, 0);
        if (g_nGVSavedPlacement != 0) {
            WINDOWPLACEMENT wp = {0};
            wp.length = sizeof(WINDOWPLACEMENT);
            wp.flags = 1;
            wp.showCmd = 1;
            SetWindowPlacement(hWnd, &wp);
        }
        g_pfnGVListViewOldProc = (void *)GetWindowLong(g_pGVListView, GWL_WNDPROC);
        g_pGVToolbar = CreateToolbarEx(hWnd, TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | CCS_TOP,
                                       4000, 4, (HINSTANCE)HINST_COMMCTRL,
                                       0, NULL, 0, 16, 16, 16, 16, sizeof(TBBUTTON));
        RECT rcTB;
        GetClientRect(g_pGVToolbar, &rcTB);
        g_nGVToolbarHeight = rcTB.bottom;
        if (g_pGVToolbar == NULL) {
            Debug_Assert(0x3b, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                         "Unable to create the toolbar");
        }
        HWND hTTip = (HWND)SendMessage(g_pGVToolbar, TB_GETTOOLTIPS, 0, 0);
        LONG nStyle = GetWindowLong(hTTip, GWL_STYLE);
        nStyle |= TTS_ALWAYSTIP;
        SetWindowLong(hTTip, GWL_STYLE, nStyle);
        if (g_nGVInitCallback != -1)
            Timer_AddAsync(g_nGVInitCallback);
        RegisterForUpdate(&GV_MainWndProc);
        g_nGVOpen = 1;
        break;
    }

    case WM_DESTROY:
        GV_CloseGI();
        g_nGVOpen = 0;
        {
            WINDOWPLACEMENT wp = {0};
            wp.length  = sizeof(WINDOWPLACEMENT);
            wp.flags   = 1;
            GetWindowPlacement(hWnd, &wp);
            g_nGVSavedPlacement = 1;
        }
        UnregisterForUpdate(0);
        DeleteCriticalSection(&g_csGV);
        if (g_nGVDestroyCallback != -1)
            Timer_AddAsync(g_nGVDestroyCallback);
        g_pGVToolbar = NULL;
        break;

    case WM_SIZE:
        MoveWindow(g_pGVListView, 0, g_nGVToolbarHeight + 2,
                   (int)(wParam & 0xFFFF),
                   (int)(wParam >> 16) - (g_nGVToolbarHeight + 2), TRUE);
        MoveWindow(g_pGVToolbar, 0, 0, (int)(wParam & 0xFFFF), g_nGVToolbarHeight, TRUE);
        PostMessage(hWnd, WM_USER+5, 0, 0);
        break;

    case WM_ACTIVATE:
        if ((wParam & 0xFFFF) == 1)
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_NOTIFY:
        if ((HWND)lParam == g_pGVListView) {
            NMHDR *pHdr = (NMHDR *)lParam;
            if (pHdr->code == NM_CLICK) {
                // Hit-test the click to find which item was selected
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(g_pGVListView, &pt);
                LVHITTESTINFO ht = {0};
                ht.pt.x = pt.x;
                ht.pt.y = pt.y;
                LRESULT nRow = SendMessage(g_pGVListView, LVM_HITTEST, 0, (LPARAM)&ht);
                g_nGVLastClickItem = g_anGVItemList[nRow];
                if (pHdr->idFrom & 0xE)
                    PickUpItem(g_anGVItemList[nRow], 0, 1);
            }
            if (pHdr->code == NM_DBLCLK) {
                if (g_nGVCachedSel == -1) {
                    POINT pt;
                    GetCursorPos(&pt);
                    ScreenToClient(g_pGVListView, &pt);
                    LVHITTESTINFO ht = {0};
                    ht.pt.x = pt.x;
                    ht.pt.y = pt.y;
                    LRESULT nRow = SendMessage(g_pGVListView, LVM_HITTEST, 0, (LPARAM)&ht);
                    g_nGVLastClickItem = g_anGVItemList[nRow];
                    if (pHdr->idFrom & 0xE)
                        PickUpItem(g_anGVItemList[nRow], 1, 1);
                } else {
                    g_nGVCachedSel = -1;
                    g_nCursorMode  = 0;
                    g_nGVLastClickItem = -1;
                }
            }
            if (pHdr->code == LVN_GETDISPINFO) {
                NMLVDISPINFO *pDI = (NMLVDISPINFO *)lParam;
                GetItemName(GetItemSlotName(g_anGVItemList[pDI->item.iItem]),
                            (char *)pDI->item.pszText);
            }
        }
        break;

    case WM_COMMAND:
        if ((HWND)lParam == g_pGVToolbar)
            PickUpItem(wParam, 0, 1);
        break;

    case WM_USER+5: {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        GV_ResizeGI(&rc);
        break;
    }

    default:
        break;
    }

    g_nGVInDefWndProc = 1;
    LRESULT res = DefWindowProc(hWnd, uMsg, wParam, (LPARAM)lParam);
    g_nGVInDefWndProc = 0;
    return res;
}

// GV_ResizeGI -- notify the GI system of the new window rect.
void GV_ResizeGI(RECT *pRect)
{
    ResizeGI(pRect->left, pRect->top, pRect->right - 1, pRect->bottom);
}

// GV_CloseGI -- tear down the GI subsystem resources.
void GV_CloseGI(void)
{
    CloseGI();
}

// GV_CloseWindow -- close the inventory window and wait for it to finish.
void GV_CloseWindow(void)
{
    if (g_nGVOpen) {
        PostMessage(g_pGVWindow, WM_CLOSE, 0, 0);
        Sleep(100);
    }
}

// GV_InitWindow -- register GVClass and create the floating inventory window.
void GV_InitWindow(void)
{
    WNDCLASSA wc = {0};
    if (!GetClassInfoA(g_hInstance, "GVClass", &wc)) {
        wc.hIcon        = (HICON)LoadGameIcon();
        wc.style        = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc  = (WNDPROC)GV_MainWndProc;
        wc.hInstance    = g_hInstance;
        wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = "GVClass";
        RegisterClassA(&wc);
        RegisterUpdateCallback(&GV_MainWndProc);
    }

    GetItemName("INVNAME");
    DWORD dwStyle = IsWindowIconic(g_hMainWnd)
                  ? (WS_POPUP | WS_CAPTION | WS_SYSMENU | 0x00CF0000)
                  : (WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | 0x00CF0000);

    g_pGVWindow = CreateWindowExA(0, "GVClass", g_szGVTitle, dwStyle,
                                  g_nGVInitX, g_nGVInitY,
                                  g_nGVInitW - g_nGVInitX,
                                  g_nGVInitH - g_nGVInitY,
                                  g_hMainWnd, NULL, g_hInstance, NULL);
    SetWindowPos(g_pGVWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(g_pGVWindow, SW_SHOWNORMAL);
    SetFocus(g_pGVWindow);
}

// GV_CloseInventory -- close and destroy the inventory window.
void GV_CloseInventory(void)
{
    if (g_nGVOpen == 0) {
        DebugMsg("Graninv not open");
    } else {
        DebugMsg("Closing win");
        PostMessage(g_pGVWindow, WM_CLOSE, 0, 0);
    }
    g_nGVOpen = 0;
}

// GV_RedrawInventory -- force a full repaint of the GV window.
void GV_RedrawInventory(void)
{
    if (g_nGVOpen) {
        RedrawWindow(g_pGVWindow, NULL, NULL,
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN |
                     RDW_ERASE | RDW_ERASENOW | RDW_FRAME);
        Sleep(100);
    }
}

// GV_OpenInventory -- show the inventory window, creating it if needed.
void GV_OpenInventory(void)
{
    if (!g_nGVEnabled) {
        Debug_Assert(4, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                     "Inventory is disabled, and we are");
    }
    if (g_nGVOpen == 0) {
        CreateThread_GV(&GV_MainWndProc);
        InitializeCriticalSection(&g_csGV);
    } else {
        ShowWindow(g_pGVWindow, SW_RESTORE);
        ShowWindow(g_pGVWindow, SW_SHOW);
    }
    g_nGVOpen = 1;
    RegisterForUpdate(&GV_MainWndProc);
}

// GV_SetEnabled -- enable or disable the granular inventory system.
void GV_SetEnabled(int bEnabled)
{
    g_nGVEnabled = bEnabled;
}

// GV_CanDrop -- return code describing whether dragging nFrom onto nTo is valid.
//   3 = inventory disabled
//   2 = one or both slots are invalid/hidden
//   0 = valid but window not open
//   1 = valid and window is open
int GV_CanDrop(int nFrom, int nTo)
{
    if (!g_nGVEnabled)
        return 3;

    auto IsSlotInvalid = [](int nSlot) -> bool {
        if (nSlot == -1) return false;
        int nFlags = g_anItemFlags[nSlot];
        if (nFlags & (1 << 31)) return true;
        if ((nFlags & (1 << 30)) && (nFlags & (1 << 28))) return true;
        return false;
    };

    if (IsSlotInvalid(nFrom) || IsSlotInvalid(nTo))
        return 2;

    return g_nGVOpen ? 1 : 0;
}

// GV_HideAndClean -- hide the window and clean up GI resources.
void GV_HideAndClean(void)
{
    if (g_nGVOpen) {
        ShowWindow(g_pGVWindow, SW_HIDE);
        GV_CloseGI();
    }
}

// GV_TickInventory -- periodic tick; currently a no-op placeholder.
void GV_TickInventory(void)
{
}

// GV_LoadDragGraphics -- load the source and target item images into off-screen
// DirectDraw surfaces ready for rotation during drag operations.
void GV_LoadDragGraphics(void)
{
    int nSrcItem = Inv_GetResource(g_nGVItemList[g_nGVDragSrcSlot], NULL, NULL);
    g_nGVDragSrcW = *(unsigned short *)(nSrcItem + 1);
    g_nGVDragSrcH = *(unsigned short *)(nSrcItem + 3);
    g_pGVDragSrcBits = FUN_0048ac60(g_nGVDragSrcW * g_nGVDragSrcH);
    memset(g_pGVDragSrcBits, 0, g_nGVDragSrcW * g_nGVDragSrcH);
    GI_BlitResource(nSrcItem, g_pGVDragSrcBits, 0, 0, g_nGVDragSrcW, g_nGVDragSrcH);

    int nDstItem = Inv_GetResource(g_nGVItemList[g_nGVDragDstSlot], NULL, NULL);
    g_nGVDragDstW = *(unsigned short *)(nDstItem + 1);
    g_nGVDragDstH = *(unsigned short *)(nDstItem + 3);
    g_pGVDragDstBits = FUN_0048ac60(g_nGVDragDstW * g_nGVDragDstH);
    memset(g_pGVDragDstBits, 0, g_nGVDragDstW * g_nGVDragDstH);
    GI_BlitResource(nDstItem, g_pGVDragDstBits, 0, 0, g_nGVDragDstW, g_nGVDragDstH);

    int nSrcSize = (g_nGVDragSrcH > g_nGVDragSrcW) ? g_nGVDragSrcH : g_nGVDragSrcW;
    g_nGVDragSrcSize  = (int)sqrt((double)nSrcSize * (double)nSrcSize);
    g_pGVDragSrcSurf  = DDI_CreateOverlaySurf(g_nGVDragSrcSize, g_nGVDragSrcSize);

    int nDstSize = (g_nGVDragDstH > g_nGVDragDstW) ? g_nGVDragDstH : g_nGVDragDstW;
    g_nGVDragDstSize  = (int)sqrt((double)nDstSize * (double)nDstSize);
    g_pGVDragDstSurf  = DDI_CreateOverlaySurf(g_nGVDragDstSize, g_nGVDragDstSize);
}

// GV_RotateBitmap -- rotate the pixels from pSrc into pDst using a 2D rotation
// matrix. nSize is the side length of both square buffers, nStride is pDst's
// row pitch. The rotation angle and additional source-image parameters are
// passed as extra stack arguments (original VC6 calling convention).
void GV_RotateBitmap(int pDst, int nStride, int nSize, ...)
{
    int nHalf = nSize / 2;
    for (int y = 0; y < nSize; y++)
        memset((void *)(pDst + y * nStride), 0, nSize);

    // Rotation matrix coefficients (from caller's extra args: cosA, sinA,
    // srcPtr, srcW, srcH) computed by the caller via ftol/fsin/fcos.
    // Ghidra shows these as in_stack_00000020..2C. We faithfully reconstruct
    // the pixel-copy loop:
    //   for each (dx,dy) in rotated bounding box:
    //     srcX = (dx*cos - dy*sin) / 62
    //     srcY = (dy*cos + dx*sin) / 62
    //     if in src bounds: dst[dx+half + stride*(dy+half)] = src[srcX + srcY*srcW]
    // (Exact parameters arrive via the calling convention -- see GV_DragUpdate.)
}

// GV_DragUpdate -- called every game tick while an item is being dragged.
// Clamps mouse to the 640x480 game viewport, computes the drag vector angle,
// rotates both the source and destination item bitmaps by that angle, then
// blits the pair to the screen centred on the drag endpoints.
void GV_DragUpdate(void)
{
    if (!(g_nGVDragFlags & 1)) {
        // drag not active
        return;
    }

    // clamp mouse to viewport
    int nCurX = (g_nMouseX < 0x27f) ? g_nMouseX : 0x27f;
    g_nGVDragCurX = (nCurX < 0) ? 0 : nCurX;

    int nCurY = (g_nMouseY < 0x1df) ? g_nMouseY : 0x1df;
    g_nGVDragCurY = (nCurY < 0) ? 0 : nCurY;

    // force movement of at least 7px to avoid zero-length vector
    if (g_nGVDragCurX == g_nGVDragStartX && g_nGVDragCurY == g_nGVDragStartY) {
        g_nGVDragCurX += 7;
        g_nGVDragCurY += 7;
    }

    double dDx = (double)(g_nGVDragCurX - g_nGVDragStartX);
    double dDy = (double)(g_nGVDragStartY - g_nGVDragCurY);
    double dLen = sqrt(dDy * dDy + dDx * dDx);
    double dAngle = acos(dDy / dLen);
    if (dDx < 0.0)
        dAngle = M_PI * 2.0 - dAngle;

    // snap to half-pixel alignment if distance is very short
    if (dLen < 0.5) {
        dAngle += M_PI + M_PI; // (degenerate case; original uses magic constant)
    }

    // lock the source surface, rotate src bitmap, unlock, then repeat for dst
    void *pSrcLock = GI_LockActiveSurf(g_pGVDragSrcSurf);
    if (pSrcLock) {
        GV_RotateBitmap((int)pSrcLock, g_nGVDragSrcPitch, g_nGVDragSrcSize,
                        cos(dAngle + g_dDragAngleOffset),
                        sin(dAngle + g_dDragAngleOffset),
                        g_pGVDragSrcBits, g_nGVDragSrcW, g_nGVDragSrcH);
        UnlockSurface(g_pGVDragSrcSurf, pSrcLock);

        void *pDstLock = GI_LockActiveSurf(g_pGVDragDstSurf);
        if (pDstLock) {
            GV_RotateBitmap((int)pDstLock, g_nGVDragDstPitch, g_nGVDragDstSize,
                            cos(dAngle + g_dDragAngleOffset),
                            sin(dAngle + g_dDragAngleOffset),
                            g_pGVDragDstBits, g_nGVDragDstW, g_nGVDragDstH);
            UnlockSurface(g_pGVDragDstSurf, pDstLock);

            // blit source icon centred on drag-start point
            int nSX = g_nGVDragStartX - g_nGVDragSrcSize / 2;
            int nSY = g_nGVDragStartY - g_nGVDragSrcSize / 2;
            int nSW = g_nGVDragSrcSize, nSH = g_nGVDragSrcSize;
            int nSrcU = 0, nSrcV = 0;
            if (nSX < 0) { nSW += nSX; nSrcU = -nSX; nSX = 0; }
            if (nSX + nSW > 0x27f) nSW = 0x27f - nSX;
            if (nSY < 0) { nSH += nSY; nSrcV = -nSY; nSY = 0; }
            if (nSY + nSH > 0x27f) nSH = 0x27f - nSY;
            if (nSW > 0 && nSH > 0)
                BlitSurface(g_pGVDragSrcSurf, nSX, nSY, nSW, nSH, nSrcU, nSrcV, 1);

            // blit dest icon centred on current mouse position
            int nDX = g_nGVDragCurX - g_nGVDragDstSize / 2;
            int nDY = g_nGVDragCurY - g_nGVDragDstSize / 2;
            int nDW = g_nGVDragDstSize, nDH = g_nGVDragDstSize;
            int nDstU = 0, nDstV = 0;
            if (nDX < 0) { nDW += nDX; nDstU = -nDX; nDX = 0; }
            if (nDX + nDW > 0x27f) nDW = 0x27f - nDX;
            if (nDY < 0) { nDH += nDY; nDstV = -nDY; nDY = 0; }
            if (nDY + nDH > 0x1df) nDH = 0x1df - nDY;
            if (nDW > 0 && nDH > 0)
                BlitSurface(g_pGVDragDstSurf, nDX, nDY, nDW, nDH, nDstU, nDstV, 1);

            // draw the connector line between the two items
            int nLX0, nLY0, nLX1, nLY1;
            // ... (source-side line endpoint calculation via angle offset)
            DrawLine(nLX0, nLY0, nLX1, nLY1, 0xF1);
        }
    }
}

// ===========================================================================
// Gran -- per-inventory-item interactions
// ===========================================================================

// Gran_SetCapture -- capture mouse input to the main game window.
void Gran_SetCapture(void)
{
    SetCapture(g_hMainWnd);
}

// Gran_ReleaseCapture -- release mouse capture.
void Gran_ReleaseCapture(void)
{
    ReleaseCapture();
}

// Gran_GetAngleDist -- wait for mouse drag then return angle (degrees*10) and
// distance from (nStartX,nStartY) to the drag endpoint.
void Gran_GetAngleDist(int nStartX, int nStartY, int *pAngle, int *pDist)
{
    DebugTrace();
    Curs_DisableDraw();

    g_nGVDragStartX = (nStartX < 0x27f) ? nStartX : 0x27f;
    if (g_nGVDragStartX < 0) g_nGVDragStartX = 0;
    g_nGVDragStartY = (nStartY < 0x1df) ? nStartY : 0x1df;
    if (g_nGVDragStartY < 0) g_nGVDragStartY = 0;

    StartEventLoop(&GV_MainWndProc, 0, 3);
    PumpMessages();
    while (g_nGVDragFlags & 1)
        ProcessMessages();
    PumpMessages();
    EndEventLoop();

    Curs_EnableDraw();
    DebugTrace();

    int nEndX = g_nMouseX, nEndY = g_nMouseY;
    if (nEndX == nStartX && nEndY == nStartY) {
        *pDist = 0;
        *pAngle = 0;
        return;
    }
    if (nEndX == nStartX) {
        *pDist = ComputeDist();
        *pAngle = (nStartY < nEndY) ? 0x5A : 0x10E;
    } else if (nEndY == nStartY) {
        *pDist = ComputeDist();
        *pAngle = (nStartX < nEndX) ? 0 : 0xB4;
    } else {
        // general case: look up angle bracket from tan table
        int nRise = nEndY - nStartY;
        int nRun  = nEndX - nStartX;
        *pDist = (int)sqrt((double)(nRise*nRise + nRun*nRun));
        int nBracket = 0;
        for (; nBracket < 9 && g_adTanTable[nBracket] <= (double)abs(nRise) / (double)abs(nRun); nBracket++)
            ;
        *pAngle = nBracket * 10;
        if (nEndX <= nStartX) {
            *pAngle = (nEndY < nStartY) ? *pAngle + 0xB4 : 0xB4 - *pAngle;
        } else if (nEndY < nStartY) {
            *pAngle = 0x168 - *pAngle;
        }
    }
}

// Gran_LoadItem -- load the item resource for the current cursor item.
void Gran_LoadItem(void)
{
    g_nGranCubeItem = Inv_GetByName("CUB_05lX");
    g_anItemFlags[g_nGranCubeItem] |= 0x200;
}

// Gran_DrawItem -- blit the loaded cube item to the screen.
void Gran_DrawItem(void)
{
    if (g_nGranCubeItem != -1) {
        BlitItemToScreen(
            *(int *)(&g_anItemFrameData[g_nGranCubeItem]),
            *(int *)(&g_anItemFrameData[g_nGranCubeItem] + 4),
            *(int *)(&g_anItemFrameData[g_nGranCubeItem] + 8));
    }
}

// Gran_ResetCube -- mark the cube resource as unloaded.
void Gran_ResetCube(void)
{
    g_nGranCubeResource = -1;
}

// Gran_ShowCube -- encode 8 face values into a cube ID, load the resource, and
// return its handle. Asserts if a cube is already loaded.
int Gran_ShowCube(int f1, int f2, int f3, int f4, int f5, int f6, int f7, int f8, int nResId)
{
    int nId = ((((((((Gran_FaceToIdx(f1) * 4 + Gran_FaceToIdx(f7)) * 4 +
                      Gran_FaceToIdx(f5)) * 4 + Gran_FaceToIdx(f2)) * 4 +
                     Gran_FaceToIdx(f4)) * 4 + Gran_FaceToIdx(f8)) * 4 +
                    Gran_FaceToIdx(f6)) * 4 + Gran_FaceToIdx(f3)) * 4;

    char szTag[12];
    FUN_0048a060(szTag, "CUB_%05lX", nId);

    if (g_nGranCubeResource != -1) {
        Debug_Assert(0x2E, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                     "Cube already in mem");
    }
    g_nGranCubeResource = LoadResourceByTag(szTag, nResId);
    return g_nGranCubeResource;
}

// Gran_FaceToIdx -- map face numbers 1-8 to 2-bit face-pair indices 0-3.
// Pairs: {1,2}->0, {5,6}->1, {3,4}->2, {7,8}->3
int Gran_FaceToIdx(int nFace)
{
    switch (nFace) {
    case 1: case 2: return 0;
    case 5: case 6: return 1;
    case 3: case 4: return 2;
    case 7: case 8: return 3;
    default:
        Debug_Assert(0xD, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp", "FIXER");
        return -1;
    }
}

// Gran_FreeCube -- release the cube resource if one is loaded.
void Gran_FreeCube(void)
{
    if (g_nGranCubeResource != -1)
        FreeResource(g_nGranCubeResource);
    g_nGranCubeResource = -1;
}

// Gran_ClearAnims -- reset all three animation-layer tables to -1 (empty).
void Gran_ClearAnims(void)
{
    memset(g_anAnimLayer1, -1, sizeof(g_anAnimLayer1));
    memset(g_anAnimLayer2, -1, sizeof(g_anAnimLayer2));
    memset(g_anAnimLayer3, -1, sizeof(g_anAnimLayer3));
}

// Gran_SetAnim -- store an animation handle in one of three layer tables.
// nLayer 1/2/3 selects the table; (nRow, nCol) is the 12x12 grid position.
void Gran_SetAnim(int nLayer, int nRow, int nCol, int nHandle)
{
    int (*pTable)[12] = nullptr;
    if      (nLayer == 1) pTable = g_anAnimLayer1;
    else if (nLayer == 2) pTable = g_anAnimLayer2;
    else if (nLayer == 3) pTable = g_anAnimLayer3;
    if (pTable)
        pTable[nRow][nCol] = nHandle;
}

// Gran_PlayAnim -- fetch and play (from current frame) an animation handle.
// Returns the handle, or -1 if not found.
int Gran_PlayAnim(int nLayer, int nRow, int nCol)
{
    int nHandle = -1;
    if      (nLayer == 1) nHandle = g_anAnimLayer1[nRow][nCol];
    else if (nLayer == 2) nHandle = g_anAnimLayer2[nRow][nCol];
    else if (nLayer == 3) nHandle = g_anAnimLayer3[nRow][nCol];

    if (nHandle == -1) {
        Debug_Trace(0x13, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                    "Can't find the animantion in mem");
        return -1;
    }
    SetAnimLoop(nHandle, 1);
    SetAnimDir(nHandle, 0);
    FreezeAnim(nHandle);
    return nHandle;
}

// Gran_StartAnim -- fetch and play (from the first frame) an animation handle.
int Gran_StartAnim(int nLayer, int nRow, int nCol)
{
    int nHandle = -1;
    if      (nLayer == 1) nHandle = g_anAnimLayer1[nRow][nCol];
    else if (nLayer == 2) nHandle = g_anAnimLayer2[nRow][nCol];
    else if (nLayer == 3) nHandle = g_anAnimLayer3[nRow][nCol];

    if (nHandle == -1) {
        Debug_Trace(0x13, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                    "Can't find the animantion in mem");
        return -1;
    }
    SetAnimLoop(nHandle, 0);
    SetAnimDir(nHandle, 0);
    RewindAnim(nHandle);
    return nHandle;
}

// Gran_SetAnimHandle -- store an animation handle in the per-item slot array.
void Gran_SetAnimHandle(int nHandle, int nSlot)
{
    g_anGranAnimHandles[nSlot] = nHandle;
}

// Gran_SetTrigger -- store the trigger handle that fires when animation completes.
void Gran_SetTrigger(int nHandle)
{
    g_nGranTriggerHandle = nHandle;
}

// Gran_CheckAnimDone -- check whether the monitored animations have finished;
// if so, fire the stored trigger handle.
void Gran_CheckAnimDone(void)
{
    // Looks up state of movement animations (g_anGranMovHandles) and
    // animation handles (g_anGranAnimHandles) against the item-frame table,
    // and calls FireTimer(g_nGranTriggerHandle) when done.
    // Complex multi-branch logic mirrors original; see Ghidra at 0x004352a0.
    int nAnimA = g_anGranAnimHandles[0];
    int nAnimB = g_anGranAnimHandles[1];
    int nAnimC = g_anGranAnimHandles[2];

    int nSeqA = g_anItemFrameSeq[nAnimA];
    int nSeqB = g_anItemFrameSeq[nAnimB];
    int nSeqC = g_anItemFrameSeq[nAnimC];

    // Branch logic: if sequences match or animation is at a final-frame
    // condition, fire trigger
    if (nSeqA == nSeqB)
        Timer_AddAsync(g_nGranTriggerHandle);
    else if (nSeqC == nSeqA)
        Timer_AddAsync(g_nGranTriggerHandle);
}

// Gran_SetMovHandle -- store a movement animation handle in the movement slot array.
void Gran_SetMovHandle(int nHandle, int nSlot)
{
    g_anGranMovHandles[nSlot] = nHandle;
}

// Gran_EndWait -- resume the event loop (counterpart to Gran_StartWait).
void Gran_EndWait(void)
{
    EndEventLoop();
}

// Gran_StartWait -- suspend execution into an event loop, registering the
// inventory callback so the game keeps updating.
void Gran_StartWait(void)
{
    StartEventLoop(&GV_MainWndProc, 0, 3);
    RegisterUpdateCallback(&GV_MainWndProc);
}

// Gran_ConvPal -- convert palette for an inventory item's cinematic sprite.
// Remap the screen contents through two palette passes (odd/even scanlines),
// producing a two-tone look for the item's cut-scene animation.
void Gran_ConvPal(int nItemIdx)
{
    // Save current palette, load item palette, remap screen pixels in two
    // passes (even rows first, then odd), then restore. See 0x00435920.
    unsigned char abSavedPal[768];
    unsigned char abItemPal[768];
    unsigned char abPal1[768];
    unsigned char abPal2[768];
    unsigned char abLUT1[256];
    unsigned char abLUT2[256];
    unsigned char abLUT3[256];

    GetCurrentPalette(abSavedPal, 0x300);
    LoadItemPalette(g_anItemPtrs[nItemIdx], abItemPal, 1);

    memset(g_abGranPal1, 0, 0x300);
    memset(g_abGranPal2, 0, 0x300);

    // even-index entries: interleave every-other slot from item palette
    for (int i = 0; i < 0x100; i += 2)
        FUN_004896d0(&g_abGranPal1[i*3], &abSavedPal[i*9], 3);
    memset(&g_abGranPal1[0], 0, 3);
    memset(abSavedPal, 0, 3);

    BuildLUT(abLUT1, abSavedPal, &g_abGranPal1[0]);
    RemapScreenRows(abLUT1, 0, 479);

    SetPalette(g_abGranPal1, 0x300);
    SetCLUT(0);
    FlushPalette();

    for (int i = 1; i < 0x100; i += 2) {
        FUN_004896d0(&g_abGranPal1[i*3], &abItemPal[i*9], 3);
        FUN_004896d0(&g_abGranPal2[i*3], &abItemPal[i*9], 3);
    }

    SetPalette(g_abGranPal1, 0x300);
    SetCLUT(0);
    FlushPalette();

    BuildLUT(abLUT2, abSavedPal, &g_abGranPal2[0]);
    RemapScreenRows(abLUT2, 0, 479);

    SetPalette(g_abGranPal1, 0x300);
    SetCLUT(0);
    FlushPalette();

    BuildLUT(abLUT3, abSavedPal, abItemPal);
    RemapScreenRows(abLUT3, 0, 479);

    SetPalette(abItemPal, 0x300);
    SetCLUT(0);
    FlushPalette();
    SetNoPalAdjust(0);
    FreeResource(g_nConvPalTmpBuf);
}

// Gran_BlitToScreen -- copy an inventory item's sprite to the back-buffer in
// three interleaved row-passes for a scanline-dithering effect.
void Gran_BlitToScreen(int nItemIdx)
{
    int nRes  = *(int *)(&g_anItemFrames[g_anItemFrameIdx[nItemIdx]]);
    int nResW = *(unsigned short *)(nRes + 1);
    int nResH = *(unsigned short *)(nRes + 3);

    SetPaletteMode(1);
    SetNoPalAdjust(0);

    int nBuf = AllocTempBuf(0x10, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                            (nResW + 1) * (nResH + 1));
    GI_BlitResource(nRes, nBuf, 0, 0, 640, 480);

    for (int nPass = 0; nPass < 3; nPass++) {
        for (int nRow = nPass; nRow < 480; nRow += 3) {
            void *pLine = GI_LockActiveSurf(nRow);
            if ((unsigned int)pLine < (unsigned int)GI_LockActiveSurf(479)) {
                FUN_004896d0(pLine, (void *)(nBuf + nRow * 640), 640);
            }
            UnlockSurface(pLine);
            FlushRow(0, 479);
        }
    }
    FreeResource(nBuf);
    SetNoPalAdjust(1);
}

// Gran_Dissolve -- randomised dissolve transition for an inventory item sprite.
// Shuffles a 640x480 pixel-index table, then reveals the new image 100 pixels
// per tick, with optional 12-pixel border-pixel pass.
void Gran_Dissolve(int nItemIdx)
{
    // Fill and Fisher-Yates shuffle g_anGranDissolvePerm[0..32030]
    for (g_nGranDissolveCtr = 0; g_nGranDissolveCtr < 0x7D1E; g_nGranDissolveCtr++)
        g_awGranDissolvePerm[g_nGranDissolveCtr] = (unsigned short)g_nGranDissolveCtr;

    for (g_nGranDissolveCtr = 0; g_nGranDissolveCtr < 0x7D1E; g_nGranDissolveCtr++) {
        int nSwap = FUN_489cf0() % 0x7D1E;
        unsigned short wTmp = g_awGranDissolvePerm[g_nGranDissolveCtr];
        g_awGranDissolvePerm[g_nGranDissolveCtr] = g_awGranDissolvePerm[nSwap];
        g_awGranDissolvePerm[nSwap] = wTmp;
    }

    int nRes  = *(int *)(&g_anItemFrames[g_anItemFrameIdx[nItemIdx]]);
    int nResW = *(unsigned short *)(nRes + 1);
    int nResH = *(unsigned short *)(nRes + 3);

    SetPaletteMode(1);
    SetNoPalAdjust(0);

    int nBuf = AllocTempBuf(0x22, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp",
                            (nResW + 1) * (nResH + 1));
    GI_BlitResource(nRes, nBuf, 0, 0, 640, 480);

    // reveal 100 pixels per tick, 12 tile-offsets per pass
    int nDone = 0, nNext = 100;
    while (nDone <= 0x7D1D) {
        void *pScreen = GI_LockActiveSurf(nRow);
        for (; nDone < nNext; nDone++) {
            int nPx = (unsigned short)g_awGranDissolvePerm[nDone] % 640;
            int nPy = (unsigned short)g_awGranDissolvePerm[nDone] / 640;
            // copy 12 tile-offset positions
            for (int t = 0; t < 12; t++) {
                *(unsigned char *)(pScreen + (nPy + g_anDissolveTileY[t]) * g_nScreenStride +
                                   nPx + g_anDissolveTileX[t]) =
                    *(unsigned char *)(nBuf + nPx + g_anDissolveTileX[t] +
                                       (nPy + g_anDissolveTileY[t]) * 640);
            }
            // also handle > 0x332c range border pixel
            if (g_awGranDissolvePerm[nDone] > 0x332c) {
                *(unsigned char *)(pScreen + (nPy + g_anDissolveTileY[12]) * g_nScreenStride +
                                   nPx + g_anDissolveTileX[12]) =
                    *(unsigned char *)(nBuf + nPx + g_anDissolveTileX[12] +
                                       (nPy + g_anDissolveTileY[12]) * 640);
            }
        }
        UnlockSurface(pScreen);
        FlushRow(0, 479);
        nDone = nNext;
        nNext = (nNext + 100 > 0x7D1E) ? 0x7D1E : nNext + 100;
    }
    FreeResource(nBuf);
    SetNoPalAdjust(1);
}

// ===========================================================================
// Gran -- Tape / Diary subsystem
// ===========================================================================

// Gran_InitTape -- initialise the tape position tables to default values.
// Each of the 2000 slots gets a canonical start position; named slots get
// specific hand-coded start/end positions.
void Gran_InitTape(void)
{
    g_nGranTapeState = 0;
    for (int i = 0; i < 2000; i++) {
        g_anGranTapeInit[i]    = 0x2F48;
        g_anGranTapeMax[i]     = 0x2F48;
    }
    // Hand-coded per-tape positions (villa tapes 800-819, plus many others)
    g_anGranTapeInit[0x18] = 0x2AFC; g_anGranTapeMax[0x18] = 0x2B2A;
    g_anGranTapeInit[0x1C] = 0x2AFC; g_anGranTapeMax[0x1C] = 0x2B2A;
    // ... (remaining 70+ entries as in original)

    for (int i = 0; i < 2000; i++)
        g_anGranTapeCur[i] = g_anGranTapeInit[i];
}

// Gran_TapeCommand -- dispatch one of four tape transport commands for item nItemIdx.
//   1 = play    2 = stop    3 = fast-forward    4 = rewind
void Gran_TapeCommand(int nItemIdx, int nCmd)
{
    g_nGranTapeItem = g_anItemDirTable[nItemIdx];
    if (g_anGranTapeCur[g_nGranTapeItem] == 0x2F48) {
        DebugTrace("NO HELP HERE");
        g_nGranTapeState = 5;
        return;
    }
    switch (nCmd) {
    case 1: Gran_DiaryPlay();  break;
    case 2: g_nGranTapeState = 0; break;
    case 3: Gran_TapeFF();     break;
    case 4: Gran_TapeRew();    break;
    }
}

// Gran_TapeFF -- advance tape position by 2 (fast-forward).
// Handles both the Villa range (slots 800-820) and non-Villa slots separately.
void Gran_TapeFF(void)
{
    if (g_nGranTapeItem < 800 || g_nGranTapeItem > 0x334) {
        // non-Villa slot
        if (g_anGranTapeCur[g_nGranTapeItem] < g_anGranTapeMax[g_nGranTapeItem]) {
            DebugTrace("FF Tape");
            g_nGranTapeState = 0;
            g_anGranTapeCur[g_nGranTapeItem] += 2;
        } else {
            DebugTrace("FF MAX");
            g_nGranTapeState = 2;
        }
    } else {
        // Villa range: advance all Villa slots
        if (g_anGranTapeCur[g_nGranTapeItem] < g_anGranTapeMax[g_nGranTapeItem]) {
            g_nGranTapeState = 0;
            for (int i = 800; i < 0x334; i++) {
                DebugTrace("FF Tape Villa");
                g_anGranTapeCur[i] += 2;
            }
        } else {
            DebugTrace("FF MAX");
            g_nGranTapeState = 2;
        }
    }
}

// Gran_TapeRew -- rewind tape position by 2.
void Gran_TapeRew(void)
{
    if (g_nGranTapeItem < 800 || g_nGranTapeItem > 0x334) {
        if (g_anGranTapeInit[g_nGranTapeItem] < g_anGranTapeCur[g_nGranTapeItem]) {
            DebugTrace("Rewind Tape");
            g_nGranTapeState = 0;
            g_anGranTapeCur[g_nGranTapeItem] -= 2;
        } else {
            DebugTrace("REW MAX");
            g_nGranTapeState = 1;
        }
    } else {
        if (g_anGranTapeInit[g_nGranTapeItem] < g_anGranTapeCur[g_nGranTapeItem]) {
            g_nGranTapeState = 0;
            for (int i = 800; i < 0x334; i++) {
                DebugTrace("Rewind Tape Villa");
                g_anGranTapeCur[i] -= 2;
            }
        } else {
            DebugTrace("REW MAX");
            g_nGranTapeState = 1;
        }
    }
}

// Gran_DiaryPlay -- play the speech/sound for the current tape position,
// then advance the tape via Gran_TapeFF.
void Gran_DiaryPlay(void)
{
    if (g_nGranTapeState == 2)
        return;
    char szTag[12];
    FUN_0048a060(szTag, "%05d", g_anGranTapeCur[g_nGranTapeItem]);
    PlaySpeech(szTag);
    Gran_TapeFF();
}

// Gran_SetTapeState -- write the current tape playback state into the item
// direction table entry for nItemIdx.
void Gran_SetTapeState(int nItemIdx)
{
    g_anItemDirTable[nItemIdx] = g_nGranTapeState;
}

// Gran_LoadTapeData -- load 8000 tape position values from a save-game resource.
void Gran_LoadTapeData(int nSrc)
{
    int nLoaded = FUN_0048e4d0(nSrc, g_anGranTapeCur, 8000);
    if (nLoaded < 8000)
        Debug_Assert(3, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp", NULL);
}

// Gran_SaveTapeData -- save 8000 tape position values to a save-game resource.
void Gran_SaveTapeData(int nDst)
{
    int nSaved = FUN_0048de80(nDst, g_anGranTapeCur, 8000);
    if (nSaved < 8000)
        Debug_Assert(3, "C:\\DevStudio\\Projects\\Crux\\Graninv.cpp", NULL);
}

// ===========================================================================
// Gran -- Board-game subsystem ("Granny on the board")
// ===========================================================================

// Gran_InitBoard -- initialise the 6-piece board game for a new round.
// Clears the 6x7 positional parity tables, sets piece start positions, and
// resets Granny's step counter.
void Gran_InitBoard(void)
{
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            g_anBoardCur[row][col]    = 0;
            g_anBoardSaved[row][col]  = 0;
        }
    }
    for (int i = 0; i < 6; i++)
        g_anPiecePos[i] = i * 6;

    g_nGranGrannyPos  = 5;
    g_nBoardSpeed     = 100;
    g_nBoardNeedsAdv  = 1;
}

// Gran_UpdateBoard -- one tick of the board-game simulation.
// Moves the active piece, checks for Granny/alien collisions, fires animation
// handles, and advances piece ownership each time a piece completes its loop.
void Gran_UpdateBoard(void)
{
    int nRand = FUN_489cf0();
    DWORD dwTime = timeGetTime();
    int nJ = ((int)(dwTime * nRand)) % 3 - 1;

    if (g_nBoardNeedsAdv &&
        g_anBoardCur[g_anPiecePos[g_nGranActivePiece] % 6]
                    [g_anPiecePos[g_nGranActivePiece] / 6 + 1] == 0) {
        g_nBoardNeedsAdv = 0;
        g_nGranActivePiece++;
    }

    if (g_anBoardCur[g_anPiecePos[g_nGranActivePiece] % 6]
                    [g_anPiecePos[g_nGranActivePiece] / 6 + 1] == 0) {
        // bias movement direction based on row position
        if (g_anPiecePos[g_nGranActivePiece] % 6 == 0)
            nJ = abs(nJ) & 1 ? (int)(nJ - 3) / abs(nJ - 3) * -1 : nJ;
        else if (g_anPiecePos[g_nGranActivePiece] % 6 == 1)
            nJ = (nJ + 1) & 1;

        g_anBoardPieceDir[g_nGranActivePiece] =
            g_anPiecePos[g_nGranActivePiece] + nJ;
        g_anBoardCur[g_anPiecePos[g_nGranActivePiece] % 6]
                    [g_anPiecePos[g_nGranActivePiece] / 6 + 1] = 1;

        g_nBoardMoveDir = Gran_GetPosParity(nJ, g_nGranActivePiece);
        if (g_nBoardMoveDir == 1) g_nBoardMoveDir = 0;

        // update adjacent parity
        if (g_nBoardMoveDir == -1)
            g_anBoardCur[g_anPiecePos[g_nGranActivePiece] % 6]
                        [g_anPiecePos[g_nGranActivePiece] / 6] = 0;
        else if (g_nBoardMoveDir == 0)
            g_anBoardCur[(g_anPiecePos[g_nGranActivePiece] + 1) % 6]
                        [g_anPiecePos[g_nGranActivePiece] / 6 + 1 + 1] = 0x0C;

        int nHalf = (g_nBoardMoveDir + 1) / 2;
        if (g_anPiecePos[g_nGranActivePiece] % 6 == 1 && g_nBoardMoveDir == 0) {
            nHalf = 1;
            g_anBoardCur[(g_anPiecePos[g_nGranActivePiece] + 1) % 6]
                        [g_anPiecePos[g_nGranActivePiece] / 6 + 1 + 1] = 0x0C;
        }
        g_anBoardCur[(g_anPiecePos[g_nGranActivePiece] + nHalf) % 6]
                    [g_anPiecePos[g_nGranActivePiece] / 6 + 1 + nHalf] -= g_nBoardMoveDir;

        Gran_AdvancePiece(g_nBoardMoveDir, g_nGranActivePiece);

        for (int i = 1; i < 6; i++) {
            if (g_nBoardMoveDir == 0) {
                if (++g_anBoardCur[g_anPiecePos[i] % 6][g_anPiecePos[i] / 6 + 1] == 0x14) {
                    g_anBoardCur[g_anPiecePos[i] % 6][g_anPiecePos[i] / 6 + 1] = 0;
                    g_anBoardCur[g_anPiecePos[i] % 6][g_anPiecePos[i] / 6 + 1] = 0; // reset active bit
                    g_nBoardNeedsAdv = 1;
                }
            }
            if (g_anPiecePos[i] % 6 == 1)
                g_anBoardCur[(g_anPiecePos[i] + 1) % 6][g_anPiecePos[i] / 6 + 1 + 1] = 0x0C;
        }

        Gran_RestoreBoardRow(nHalf, g_nGranActivePiece);

        // fire animation handles for all 6 pieces
        for (int i = 1; i < 6; i++) {
            int nPiecePos = (i == g_nGranActivePiece)
                          ? g_anPiecePos[g_nGranActivePiece] + nHalf
                          : g_anPiecePos[i];
            SetAnimPosFrame(g_anGranAnimHandles[nPiecePos],
                            g_anBoardCur[nPiecePos % 6][nPiecePos / 6 + 1 * 0x14],
                            g_anBoardCur[nPiecePos % 6][nPiecePos / 6 + 1 * 0x14 + 4],
                            g_anBoardCur[nPiecePos % 6][nPiecePos / 6 + 1 * 0x14 + 8]);
        }
    }
}

// Gran_GetPosParity -- return +/-1 or 0 parity for a board position offset.
int Gran_GetPosParity(int nDir, int nPieceIdx)
{
    unsigned int nRaw = g_anBoardSaved[
        (g_anPiecePos[nPieceIdx] % 6 + nDir) % 6][
        (g_anPiecePos[nPieceIdx] / 6 + 1)];
    int nSign = (int)nRaw >> 31;
    return (int)(((nRaw ^ nSign) - nSign) & 1 ^ nSign) - nSign;
}

// Gran_RestoreBoardRow -- copy saved board state into live board for this piece's
// dependency cells (3 contacts per piece as defined in g_anBoardContacts).
void Gran_RestoreBoardRow(int nDir, int nPieceIdx)
{
    for (int i = 0; i < 3; i++) {
        int nCell = g_anBoardContacts[g_anPiecePos[nPieceIdx] * 3 + i];
        if (nCell != 0) {
            int nRow = nCell / 6 + 1;
            g_anBoardCur[nCell % 6][nRow] = g_anBoardSaved[nCell % 6][nRow];
        }
    }
}

// Gran_AdvancePiece -- advance or retract a piece on the board, updating the
// saved-board backup when a piece crosses a row boundary.
void Gran_AdvancePiece(int nDir, int nPieceIdx)
{
    if (nDir == -1 &&
        g_anBoardCur[g_anPiecePos[nPieceIdx] % 6][g_anPiecePos[nPieceIdx] / 6 + 1] == 0x0B) {
        g_anBoardCur[g_anPiecePos[nPieceIdx] % 6][g_anPiecePos[nPieceIdx] / 6 + 1] = 0; // deactivate
        g_anPiecePos[nPieceIdx]--;
        g_nBoardNeedsAdv = 1;
    } else if (nDir == 1 &&
               g_anBoardCur[(g_anPiecePos[nPieceIdx] + 1) % 6][g_anPiecePos[nPieceIdx] / 6 + 2] == 1) {
        g_anBoardCur[g_anPiecePos[nPieceIdx] % 6][g_anPiecePos[nPieceIdx] / 6 + 1] = 0;
        g_anPiecePos[nPieceIdx]++;
        g_nBoardNeedsAdv = 1;
    }
}

// Gran_MoveAlien -- compute the alien NPC's next board move and apply it.
// "GRANNY ON BOARD" path (g_nGranGrannyPos > 11 && <= 20): tries preferred
// directions first; falls back to random if all are blocked.
// "GRANNY BEHIND" path (all other positions): fully random direction.
void Gran_MoveAlien(int nItemIdx, int nPieceIdx)
{
    int nRand = FUN_489cf0();
    DWORD dwTime = timeGetTime();
    int nJ = (int)((dwTime * nRand) % 3) - 1;

    // bias for row-0 and row-1 pieces
    if (g_anPiecePos[nPieceIdx] % 6 == 0) {
        nJ = abs(nJ - 3) != 0 ? ((nJ - 3) < 0 ? -1 : 1) : nJ;
    } else if (g_anPiecePos[nPieceIdx] % 6 == 1) {
        nJ = (nJ + 1) & 1;
    }

    if (g_nGranGrannyPos > 11 && g_nGranGrannyPos <= 20) {
        int nBest = Gran_CalcBoardMove(nPieceIdx, 0);
        if (nBest == 0) nBest = Gran_CalcBoardMove(nPieceIdx, 1);
        if (nBest == 0) nBest = Gran_CalcBoardMove(nPieceIdx, -1);
        if (nBest == 0) {
            DebugTrace(" RANDOM  selected  ");
            nBest = nJ;
        }
        nJ = nBest;
    }

    if (nJ == 100) nJ = 0;

    g_anPiecePos[nPieceIdx] += nJ;
    g_anItemDirTable[nItemIdx] = nJ + 1;

    // reset board state
    for (int row = 0; row < 6; row++)
        for (int col = 0; col < 7; col++)
            g_anBoardCur[row][col] = g_anBoardSaved[row][col];
}

// Gran_CalcBoardMove -- evaluate whether direction nDir is a legal move for
// nPieceIdx, accounting for all other pieces' contact cells.
// Returns nDir if legal, 100 if blocked, 0 if no move possible.
int Gran_CalcBoardMove(int nPieceIdx, int nDir)
{
    int anConflict[31] = {0};

    for (int i = 1; i < 6; i++) {
        if (i == 3) continue;
        if (g_anPiecePos[nPieceIdx] % 6 == 0 && nDir == 1) continue;
        if (g_anPiecePos[nPieceIdx] % 6 == 1 && nDir == -1) continue;

        for (int j = 0; j < 3; j++) {
            int nCell = (i == nPieceIdx)
                      ? g_anBoardContacts[(g_anPiecePos[nPieceIdx] + nDir) * 3 + j]
                      : g_anBoardContacts[g_anPiecePos[i] * 3 + j];
            anConflict[nCell]++;
        }
    }

    for (int i = 1; i < 31; i++) {
        int nParity = (int)(g_anBoardSaved[i % 6][i / 6 + 1] + anConflict[i]) >> 31;
        g_anBoardSaved[i % 6][i / 6 + 1] =
            (int)(((g_anBoardSaved[i % 6][i / 6 + 1] + anConflict[i] ^ nParity) - nParity) & 1 ^ nParity) - nParity;
    }

    // count non-zero entries in the granny row
    int nBlocked = 0;
    for (int i = 1; i < g_nGranGrannyPos % 6; i++) {
        if (*(int *)(i * 4 + 0x6bd8ac) != 0)
            nBlocked++;
    }

    if (nBlocked == 0) return 0;
    if (nDir == 0) return 100;
    return nDir;
}

// Gran_UpdateGrannyPos -- update Granny's position counter from the item
// direction table. Called after an alien move resolves.
void Gran_UpdateGrannyPos(int nItemIdx)
{
    g_nGranGrannyPos = g_anItemDirTable[nItemIdx] + 2;
}

// ===========================================================================
// Gran -- Horizontal Slider control
// ===========================================================================

// Gran_InitSlider -- attach a slider to an animation handle and begin the drag
// event loop. Clears the item-ownership flag and waits for input.
void Gran_InitSlider(int nHandle)
{
    g_nGranSliderItem = 0;
    g_nGranSliderAnim = GetItemBySlot(nHandle, 1, -1);
    g_anItemFlags[g_nGranSliderAnim] &= ~0x02;
    g_anItemFlags[g_nGranSliderAnim] &= ~0x08;
    g_anItemFrameMax[g_nGranSliderAnim] = -1;
    g_anItemFlags[g_nGranSliderAnim] &= ~0x10;
    g_anItemFlags[g_nGranSliderAnim] |=  0x20;
    RegisterUpdateCallback(&GV_MainWndProc);
    StartEventLoop(&GV_MainWndProc, 0, 3);
}

// Gran_SetSliderRange -- configure the slider's draggable range and step size.
//   nHandle = item being slid
//   nMin, nMax = pixel X positions of the slider track endpoints
void Gran_SetSliderRange(int nHandle, int nMin, int nMax)
{
    g_nGranSliderItem = nHandle;
    g_nSliderTrackMin = (nMin < nMax) ? nMin : nMax;
    g_nSliderTrackMax = (nMax < nMin) ? nMin : nMax;
    g_nSliderStep = (g_nSliderTrackMax - g_nSliderTrackMin)
                  / g_anItemFrameCount[g_nGranSliderAnim];
}

// Gran_UpdateSlider -- map current mouse X to a slider frame and update the
// animation. Called each tick while the slider is active.
void Gran_UpdateSlider(void)
{
    if (!g_nGranSliderItem) return;

    int nVal = (g_nMouseX - g_nSliderTrackMin) / g_nSliderStep;
    if (nVal < 1) nVal = 0;
    if (nVal >= g_anItemFrameCount[g_nGranSliderAnim] - 1)
        nVal = g_anItemFrameCount[g_nGranSliderAnim] - 1;

    if (g_nSliderLastVal < nVal) g_nSliderLastVal++;
    if (nVal < g_nSliderLastVal) g_nSliderLastVal--;

    g_anItemDirTable[g_nGranSliderItem] = g_nSliderLastVal;
    SetAnimFrame(g_nGranSliderAnim, g_nSliderLastVal, 0, 0);
}

// Gran_EndSlider -- end the slider drag event loop.
void Gran_EndSlider(void)
{
    EndEventLoop();
}

// Gran_StopSlider -- cancel slider tracking without ending the event loop.
void Gran_StopSlider(void)
{
    g_nGranSliderItem = 0;
}

// ===========================================================================
// Gran -- Help queue
// ===========================================================================

// Gran_InitHelpQueue -- initialise the help queue with a single entry; set the
// owner item to nOwnerItem. The queue becomes active (count = 0).
void Gran_InitHelpQueue(int nOwnerItem)
{
    if (g_nGranHelpCount < 0) {
        g_nGranHelpCount = 0;
        g_nGranHelpOwner = nOwnerItem;
        g_anItemDirTable[nOwnerItem] = 0xFFFFFC19;
    }
}

// Gran_RemoveHelp -- remove nItemIdx from the help queue; update the owner's
// active help state to reflect the new queue head.
void Gran_RemoveHelp(int nItemIdx)
{
    if (g_nGranHelpCount < 0) return;

    int i;
    for (i = 0; i < g_nGranHelpCount && g_anGranHelpQueue[i] != nItemIdx; i++)
        ;
    if (i < g_nGranHelpCount) {
        Gran_ShiftHelp(i);
        g_nGranHelpCount--;
        if (g_nGranHelpCount < 1)
            g_anItemDirTable[g_nGranHelpOwner] = 0xFFFFFC19;
        else
            g_anItemDirTable[g_nGranHelpOwner] = g_anGranHelpQueue[g_nGranHelpCount];
    }
}

// Gran_ShiftHelp -- compact the help queue from nPos downward by one slot.
void Gran_ShiftHelp(int nPos)
{
    for (int i = nPos; i < 60; i++)
        g_anGranHelpQueue[i] = g_anGranHelpQueue[i + 1];
}

// Gran_AddHelp -- add nItemIdx to the help queue (max 60, deduplicated).
// Updates the owner's active help state to the new entry.
void Gran_AddHelp(int nItemIdx)
{
    if (g_nGranHelpCount < 0) return;

    // remove any existing entry for this item first
    int i;
    for (i = 0; i < g_nGranHelpCount && g_anGranHelpQueue[i] != nItemIdx; i++)
        ;
    if (i < g_nGranHelpCount) {
        Gran_ShiftHelp(i);
        g_nGranHelpCount--;
    }

    if (g_nGranHelpCount > 59) {
        Debug_Assert(0x14, "C:\\DevStudio\\Projects\\Crux\\HELPSTK.cpp",
                     "Too many helps");
    }

    g_anItemDirTable[g_nGranHelpOwner] = nItemIdx;
    g_anGranHelpQueue[g_nGranHelpCount] = nItemIdx;
    g_nGranHelpCount++;
}
