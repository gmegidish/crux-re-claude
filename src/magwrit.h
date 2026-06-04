#pragma once
// magwrit.cpp — Wacom tablet / PenNet handwriting-input driver wrapper
//
// This module is a thin wrapper around kwt_lcso.dll ("PenNet 1.0"), a Wacom
// tablet + handwriting-recognition library. It is a DEV / AUTHORING-TOOL input
// path (the retail game does not require a tablet) and has low gameplay
// relevance, but is reversed here for completeness.
//
// Original source: C:\DevStudio\Projects\Crux\magwrit.cpp
//
// ============================================================================
//  MODEL
// ============================================================================
//  * The DLL is loaded lazily (Magwrit_LoadKwtDll) and every export is resolved
//    into a g_pfn* function pointer. g_nMagwritEnabled gates the whole module:
//    until init succeeds it is 0 and every entry point is a no-op.
//
//  * BUTTONS — the tablet has hardware buttons. The game maps a buttonId to a
//    screen rectangle (read from the [BUTTONS_MAP] ini section into
//    g_anMagwritButtonRects) and to a script "sync prog" callback. A live
//    binding table g_anMagwritButtons (10 slots, {buttonId, callbackId}) tracks
//    attached buttons. The DLL calls Magwrit_OnButtonClicked(buttonId); we look
//    up the slot and fire the callback via Timer_AddSyncProg, then detach.
//
//  * TASKS — a "task" is a recognition region/exercise on a tablet "page".
//    g_anMagwritTasks (30 slots, stride 0x70) tracks each: taskId, the manager
//    handle returned by _AddTaskInPageToManager, a callback, a value range
//    [low,high], a collected ink-point ring buffer (+count), an event mask and
//    a state machine (1=inited, 2=batch-pending, 4=done).
//
//  * MESSAGES — the DLL posts events (pen point / task-manager / task-proc)
//    that are queued in g_anMagwritMsgQueue (FIFO, max 200) and dispatched by
//    Magwrit_DispatchPenNetMessage. Recognized "ink point" ids matching a
//    task's range are collected so the game can replay/redraw the stroke.

#include <windows.h>

// ---------------------------------------------------------------------------
//  State globals
// ---------------------------------------------------------------------------
extern int   g_nMagwritEnabled;            // 0x006dbae8  1 once tablet initialized
extern void* g_pKwtLcsoDll;                // 0x006da178  HMODULE for kwt_lcso.dll

extern int   g_nMagwritButtonRectCount;    // 0x006da194  valid entries in rect map
extern int   g_anMagwritButtonRects[];     // 0x006da230  100 x {id,l,t,r,b} (stride 0x14)
extern int   g_anMagwritButtons[];         // 0x006da1a0  10 x {buttonId,callbackId}
extern int   g_nMagwritCurrentPage;        // 0x006da1f0  current tablet page id

extern int   g_anMagwritTasks[];           // 0x006daa08  30 x task record (stride 0x70)

extern int   g_anMagwritMsgQueue[];        // 0x006db7c8  PENNET message FIFO (max 200)
extern int   g_nMagwritMsgQueueCount;      // 0x006dbaec

extern int   g_nMagwritLastInkX;           // 0x004d2fc4  prev ink point (-1 = pen up)
extern int   g_nMagwritLastInkY;           // 0x004d2fc8

// ---------------------------------------------------------------------------
//  kwt_lcso.dll exports (resolved by Magwrit_LoadKwtDll)
// ---------------------------------------------------------------------------
extern void* g_pfnLoadTasksBook;               // 0x006da18c
extern void* g_pfnFreeTasksBook;               // 0x006da1f4
extern void* g_pfnPaintPage;                   // 0x006da180
extern void* g_pfnTbltOpen;                    // 0x006da19c
extern void* g_pfnTbltClose;                   // 0x006da220
extern void* g_pfnTbltEnable;                  // 0x006db7b4
extern void* g_pfnTbltSetNotificationWindow;   // 0x006da198
extern void* g_pfnTbltSetActiveArea;           // 0x006da204
extern void* g_pfnTbltAddButton;               // 0x006da210
extern void* g_pfnTbltDetachButton;            // 0x006da1fc
extern void* g_pfnTbltDetachAllButtons;        // 0x006db7ac
extern void* g_pfnTbltSetButtonFunc;           // 0x006da170
extern void* g_pfnTbltSetButtonRect;           // 0x006da20c
extern void* g_pfnTbltPointsBufferFlush;       // 0x006da200
extern void* g_pfnTbltEnableCollect;           // 0x006da218
extern void* g_pfnTbltDPtoNTP;                 // 0x006da184
extern void* g_pfnInitTaskManager;             // 0x006da228
extern void* g_pfnRestartManager;              // 0x006db7c4
extern void* g_pfnAddTaskInPageToManager;      // 0x006da174
extern void* g_pfnAddAllPageTasksToManager;    // 0x006da188
extern void* g_pfnPerformBatchProcess;         // 0x006db7b8
extern void* g_pfnDetachTaskFromManager;       // 0x006da17c
extern void* g_pfnActivateManagerTasks;        // 0x006da214
extern void* g_pfnDeactivateManagerTasks;      // 0x006da208
extern void* g_pfnLoadRecognizerParams;        // 0x006db7bc
extern void* g_pfnGetNumberOfCollectedTaskPoints;  // 0x006da21c
extern void* g_pfnGetCollectedTaskPoints;          // 0x006daa00
extern void* g_pfnGetRectOfCollectedTaskPoints;    // 0x006da1f8

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------
int  Magwrit_InitPenNet(void* hwnd);  // 0x0043d6cc (reversed earlier)
int  Magwrit_LoadKwtDll(void);        // 0x0043db90  LoadLibrary + GetProcAddress all exports
void Magwrit_TabletInit(void);        // 0x0043d7c0  full bring-up: load DLL, book, buttons map
void Magwrit_CleanExit(void);         // 0x0043d760  TbltClose + FreeLibrary
void Magwrit_PostInitCallback(void);  // 0x0043e100  posts deferred init callback

// ---------------------------------------------------------------------------
//  Pages & tasks
// ---------------------------------------------------------------------------
void Magwrit_BeginPage(int nPageId);                          // 0x0043e120
void Magwrit_AddTask(int nTaskId, int nCallback, int nMask);  // 0x0043e1d0
void Magwrit_EndTask(int nTaskId);                            // 0x0043e3c0
void Magwrit_DetachTask(int nTaskId);                         // 0x0043e4d0
void Magwrit_SetTaskRangeLow(int nTaskId, int nLow);          // 0x0043f0b0
void Magwrit_SetTaskRangeHigh(int nTaskId, int nHigh);        // 0x0043f110
int  Magwrit_GetTaskPointCount(int nTaskId);                  // 0x0043f170
int  Magwrit_PopTaskPoint(int nTaskId);                       // 0x0043f1d0

// ---------------------------------------------------------------------------
//  Buttons
// ---------------------------------------------------------------------------
void Magwrit_AddButton(int nButtonId, int nCallback);  // 0x0043e610
void Magwrit_DetachButton(int nButtonId);              // 0x0043e7f0
void Magwrit_DetachAllButtons(void);                   // 0x0043e860
void Magwrit_OnButtonClicked(int nButtonId);           // 0x0043e880 (DLL notification cb)

// ---------------------------------------------------------------------------
//  Message pump
// ---------------------------------------------------------------------------
int  Magwrit_DequeueMessage(void);                       // 0x0043e590
void Magwrit_EnqueueTaskMessage(int nTaskHandle, int nPointId);  // 0x0043ea40
void Magwrit_OnTaskProcMessage(int* pMsg);               // 0x0043eb70
void Magwrit_OnTaskManagerMessage(int* pMsg);            // 0x0043ee50
void Magwrit_DispatchPenNetMessage(int* pMsg);           // 0x0043ef10

// ---------------------------------------------------------------------------
//  Ink rendering / debug
// ---------------------------------------------------------------------------
void Magwrit_DrawInkSegment(int x0, int y0, int x1, int y1,
                            int ntpX, int ntpY, unsigned char color);  // 0x0043e930
void Magwrit_DrawTaskInk(int nTaskId, int x0, int y0, int x1, int y1,
                         unsigned char color);                          // 0x0043f280
void Magwrit_ShowStatusText(void);                                      // 0x0043f030
