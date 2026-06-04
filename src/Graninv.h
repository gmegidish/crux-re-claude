#pragma once
#include <windows.h>
#include <commctrl.h>

// ---------------------------------------------------------------------------
// Graninv.cpp -- Granular Inventory (GV window) + Gran item interactions
//
// Two distinct subsystems live here:
//   GV_*    -- the floating Win32 inventory panel (GVClass window, ListView,
//               Toolbar, drag-and-drop with rotation preview)
//   Gran_*  -- per-item logic driven from inventory items: cube viewer,
//               animation layers, palette conversion, tape/diary player,
//               Granny board-game AI, slider control, help queue
// ---------------------------------------------------------------------------

// ---- GV window subsystem --------------------------------------------------

void     GV_AddButton(int nButtonId, int nValue);
void     GV_SetInitHandler(int nCallback);
void     GV_SetDestroyHandler(int nCallback);
int      GV_UpdateInventory(int bForce);
LRESULT  CALLBACK GV_ListViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT  CALLBACK GV_MainWndProc(HWND hWnd, UINT uMsg, UINT wParam, HWND lParam);
void     GV_ResizeGI(RECT *pRect);
void     GV_CloseGI(void);
void     GV_CloseWindow(void);
void     GV_InitWindow(void);
void     GV_CloseInventory(void);
void     GV_RedrawInventory(void);
void     GV_OpenInventory(void);
void     GV_SetEnabled(int bEnabled);
int      GV_CanDrop(int nFrom, int nTo);
void     GV_HideAndClean(void);
void     GV_TickInventory(void);
void     GV_LoadDragGraphics(void);
void     GV_RotateBitmap(int pDst, int nStride, int nSize, ...);
void     GV_DragUpdate(void);

// ---- Gran item subsystem --------------------------------------------------

void     Gran_SetCapture(void);
void     Gran_ReleaseCapture(void);
void     Gran_GetAngleDist(int nStartX, int nStartY, int *pAngle, int *pDist);
void     Gran_LoadItem(void);
void     Gran_DrawItem(void);
void     Gran_ResetCube(void);
int      Gran_ShowCube(int f1, int f2, int f3, int f4, int f5, int f6, int f7, int f8, int nResId);
int      Gran_FaceToIdx(int nFace);
void     Gran_FreeCube(void);
void     Gran_ClearAnims(void);
void     Gran_SetAnim(int nLayer, int nRow, int nCol, int nHandle);
int      Gran_PlayAnim(int nLayer, int nRow, int nCol);
int      Gran_StartAnim(int nLayer, int nRow, int nCol);
void     Gran_SetAnimHandle(int nHandle, int nSlot);
void     Gran_SetTrigger(int nHandle);
void     Gran_CheckAnimDone(void);
void     Gran_SetMovHandle(int nHandle, int nSlot);
void     Gran_EndWait(void);
void     Gran_StartWait(void);
void     Gran_ConvPal(int nItemIdx);
void     Gran_BlitToScreen(int nItemIdx);
void     Gran_Dissolve(int nItemIdx);
void     Gran_InitTape(void);
void     Gran_TapeCommand(int nItemIdx, int nCmd);
void     Gran_TapeFF(void);
void     Gran_TapeRew(void);
void     Gran_DiaryPlay(void);
void     Gran_SetTapeState(int nItemIdx);
void     Gran_LoadTapeData(int nSrc);
void     Gran_SaveTapeData(int nDst);
void     Gran_InitBoard(void);
void     Gran_UpdateBoard(void);
int      Gran_GetPosParity(int nDir, int nPieceIdx);
void     Gran_RestoreBoardRow(int nDir, int nPieceIdx);
void     Gran_AdvancePiece(int nDir, int nPieceIdx);
void     Gran_MoveAlien(int nItemIdx, int nPieceIdx);
int      Gran_CalcBoardMove(int nPieceIdx, int nDir);
void     Gran_UpdateGrannyPos(int nItemIdx);
void     Gran_InitSlider(int nHandle);
void     Gran_SetSliderRange(int nHandle, int nMin, int nMax);
void     Gran_UpdateSlider(void);
void     Gran_EndSlider(void);
void     Gran_StopSlider(void);
void     Gran_InitHelpQueue(int nOwnerItem);
void     Gran_RemoveHelp(int nItemIdx);
void     Gran_ShiftHelp(int nPos);
void     Gran_AddHelp(int nItemIdx);

// ---- Globals (defined in Graninv.cpp) -------------------------------------

extern int   g_nGVEnabled;        // 0x004cf898  GV system enabled flag
extern int   g_nGVButtonCount;    // 0x006d1dc4  toolbar button count (max 10)
extern int   g_nGVOpen;           // 0x006d1dc8  GV window is open
extern int   g_nGVNeedsUpdate;    // 0x006d1dc0  inventory list needs refresh
extern HWND  g_pGVWindow;         // 0x006bae94  top-level GV window HWND
extern HWND  g_pGVListView;       // 0x006bae78  SysListView32 child HWND
extern void *g_pGVImageList;      // 0x006bae6c  ImageList for item icons
extern HWND  g_pGVToolbar;        // 0x006d1dcc  Toolbar HWND
extern void *g_pfnGVListViewOldProc; // 0x006bd3a0  saved ListView WndProc
extern int   g_nGVDragStartX;     // 0x006bd340
extern int   g_nGVDragStartY;     // 0x006bd344
extern int   g_nGVDragCurX;       // 0x006bdb44
extern int   g_nGVDragCurY;       // 0x006bdb48
extern int   g_nGranTapeState;    // 0x006be1d0  0=playing 1=start 2=end 5=no help
extern int   g_nGranTapeItem;     // 0x006bd0d8  current tape item index
extern int   g_nGranGrannyPos;    // 0x006d1db4  Granny board position
extern int   g_nGranActivePiece;  // 0x006d1de0  active board piece (0-5)
extern int   g_nGranSliderItem;   // 0x006bd0dc  slider-controlled item (0=none)
extern int   g_nGranHelpCount;    // 0x004d09b8  help queue count (-1=uninit)
extern int   g_nGranHelpOwner;    // 0x006d6768  help queue owner item
extern int   g_nGranCubeItem;     // 0x004cfce0  loaded cube item index (-1=none)
extern int   g_nGranCubeResource; // 0x004cfd80  cube resource handle
