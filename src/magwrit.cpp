// ---------------------------------------------------------------------------
// magwrit.cpp  —  Wacom tablet / PenNet handwriting-input driver wrapper
// Original: C:\DevStudio\Projects\Crux\magwrit.cpp
// RE offsets: 0x0043d760 – 0x0043f280
//
// Thin wrapper around kwt_lcso.dll ("PenNet 1.0"), a Wacom tablet +
// handwriting-recognition library. DEV / authoring-tool input path. See
// magwrit.h for the full TASK / BUTTON / MESSAGE model description.
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "magwrit.h"

// ---------------------------------------------------------------------------
// Task record layout (stride 0x70 = 28 ints). g_anMagwritTasks is an int[] so
// record n begins at int index n*0x1c. Field offsets (int index within record):
//   +0x00  taskId         (-1 = free slot)
//   +0x01  managerHandle  (handle from _AddTaskInPageToManager, -1 = none)
//   +0x02  callbackId     (sync-prog fired on matching message, -1 = none)
//   +0x03  rangeLow       (inclusive)
//   +0x04  rangeHigh      (inclusive)
//   +0x05  ink point ring buffer, 20 entries (bytes 0x14..0x63 of the record)
//   +0x19  collected point count
//   +0x1a  event mask
//   +0x1b  state (1=inited, 2=batch-pending, 4=done)
// ---------------------------------------------------------------------------
#define TASK_STRIDE       0x1c   // ints per task record
#define MAX_TASKS         0x1e   // 30 task slots
#define MAX_BUTTONS       10
#define MAX_BUTTON_RECTS  100
#define MAX_INK_POINTS    0x14   // 20 collected points per task
#define MAX_MSG_QUEUE     200

#define T_ID(i)      g_anMagwritTasks[(i) * TASK_STRIDE + 0]
#define T_HANDLE(i)  g_anMagwritTasks[(i) * TASK_STRIDE + 1]
#define T_CB(i)      g_anMagwritTasks[(i) * TASK_STRIDE + 2]
#define T_LOW(i)     g_anMagwritTasks[(i) * TASK_STRIDE + 3]
#define T_HIGH(i)    g_anMagwritTasks[(i) * TASK_STRIDE + 4]
#define T_COUNT(i)   g_anMagwritTasks[(i) * TASK_STRIDE + 0x19]
#define T_MASK(i)    g_anMagwritTasks[(i) * TASK_STRIDE + 0x1a]
#define T_STATE(i)   g_anMagwritTasks[(i) * TASK_STRIDE + 0x1b]
// ink buffer: int index 5 within the record (byte offset 0x14)
#define T_INK(i)     (&g_anMagwritTasks[(i) * TASK_STRIDE + 5])

// ---------------------------------------------------------------------------
// State globals
// ---------------------------------------------------------------------------
int   g_nMagwritEnabled         = 0;     // 0x006dbae8
void* g_pKwtLcsoDll             = 0;     // 0x006da178

int   g_nMagwritButtonRectCount = 0;     // 0x006da194
int   g_anMagwritButtonRects[MAX_BUTTON_RECTS * 5]; // 0x006da230  {id,l,t,r,b}
int   g_anMagwritButtons[MAX_BUTTONS * 2];          // 0x006da1a0  {buttonId,callbackId}
int   g_nMagwritCurrentPage     = 0;     // 0x006da1f0

int   g_anMagwritTasks[MAX_TASKS * TASK_STRIDE];    // 0x006daa08

int   g_anMagwritMsgQueue[MAX_MSG_QUEUE]; // 0x006db7c8
int   g_nMagwritMsgQueueCount   = 0;      // 0x006dbaec

int   g_nMagwritLastInkX        = -1;    // 0x004d2fc4
int   g_nMagwritLastInkY        = -1;    // 0x004d2fc8

// ---------------------------------------------------------------------------
// kwt_lcso.dll exports (resolved by Magwrit_LoadKwtDll)
// ---------------------------------------------------------------------------
void* g_pfnLoadTasksBook                  = 0; // 0x006da18c
void* g_pfnFreeTasksBook                  = 0; // 0x006da1f4
void* g_pfnPaintPage                      = 0; // 0x006da180
void* g_pfnTbltOpen                       = 0; // 0x006da19c
void* g_pfnTbltClose                      = 0; // 0x006da220
void* g_pfnTbltEnable                     = 0; // 0x006db7b4
void* g_pfnTbltSetNotificationWindow      = 0; // 0x006da198
void* g_pfnTbltSetActiveArea              = 0; // 0x006da204
void* g_pfnTbltAddButton                  = 0; // 0x006da210
void* g_pfnTbltDetachButton               = 0; // 0x006da1fc
void* g_pfnTbltDetachAllButtons           = 0; // 0x006db7ac
void* g_pfnTbltSetButtonFunc              = 0; // 0x006da170
void* g_pfnTbltSetButtonRect              = 0; // 0x006da20c
void* g_pfnTbltPointsBufferFlush          = 0; // 0x006da200
void* g_pfnTbltEnableCollect              = 0; // 0x006da218
void* g_pfnTbltDPtoNTP                    = 0; // 0x006da184
void* g_pfnInitTaskManager                = 0; // 0x006da228
void* g_pfnRestartManager                 = 0; // 0x006db7c4
void* g_pfnAddTaskInPageToManager         = 0; // 0x006da174
void* g_pfnAddAllPageTasksToManager       = 0; // 0x006da188
void* g_pfnPerformBatchProcess            = 0; // 0x006db7b8
void* g_pfnDetachTaskFromManager          = 0; // 0x006da17c
void* g_pfnActivateManagerTasks           = 0; // 0x006da214
void* g_pfnDeactivateManagerTasks         = 0; // 0x006da208
void* g_pfnLoadRecognizerParams           = 0; // 0x006db7bc
void* g_pfnGetNumberOfCollectedTaskPoints = 0; // 0x006da21c
void* g_pfnGetCollectedTaskPoints         = 0; // 0x006daa00
void* g_pfnGetRectOfCollectedTaskPoints   = 0; // 0x006da1f8

// ---------------------------------------------------------------------------
// Tablet-API function-pointer call signatures
// ---------------------------------------------------------------------------
typedef int  (*PFN_LoadTasksBook)(const char* path);
typedef void (*PFN_TbltClose)(void);
typedef void (*PFN_TbltEnable)(int enable);
typedef void (*PFN_LoadRecognizerParams)(const char* path);
typedef void (*PFN_InitTaskManager)(int arg);
typedef void (*PFN_RestartManager)(void);
typedef void (*PFN_DeactivateManagerTasks)(void);
typedef void (*PFN_ActivateManagerTasks)(int timeoutMs);
typedef int  (*PFN_AddTaskInPageToManager)(int page, int taskId, void* hwnd, int a, int b);
typedef void (*PFN_DetachTaskFromManager)(int handle);
typedef void (*PFN_PerformBatchProcess)(int handle);
typedef void (*PFN_TbltAddButton)(int buttonId, RECT* rect, int flags);
typedef void (*PFN_TbltDetachButton)(int buttonId);
typedef void (*PFN_TbltDetachAllButtons)(void);
typedef void (*PFN_TbltPointsBufferFlush)(void);
typedef int  (*PFN_GetNumPoints)(int handle);
typedef void (*PFN_GetPoints)(int handle, int buffer, int count);
typedef void (*PFN_GetRect)(int handle, int* outRect);

// ---------------------------------------------------------------------------
// Cross-module stubs
// ---------------------------------------------------------------------------
extern void  Debug_Trace(int line, const char* file, const char* fmt, ...);  // 0x004d2904 etc.
extern void  Err_BadResEntry(int line, const char* file, const char* msg);
extern void  Bani_Noop(const char* fmt, ...);
extern int   Magwrit_InitPenNet(void* hwnd);                 // 0x0043d6cc
extern void  Win_RegisterExitHandler(void* fn);
extern void  Win_PostCallback(void* fn);
extern void  Timer_AddSyncProg(int callbackId);
extern void  GI_SetDrawMode(int mode);
extern void  GI_LockActiveSurf_v5(int x0, int y0, int x1, int y1, unsigned char color);
extern void  Txt_SetString(void* text, void* rect, int duration);
extern int   SafeHeap_Alloc(int line, const char* file, int size);

extern void* g_nHwndMain;       // main window handle
extern char  g_abSaveGameDir[]; // save-game directory
extern char  g_abAdventDir[];   // adventure (content) directory

// Static name tables: each entry is 0x36 bytes = { int id; char name[0x32]; }.
// Used only to log a human-readable string for known PENNET message ids.
extern char  g_aTaskMgrMsgNames[];   // 0x004d1668  8 entries (task-manager msgs)
extern char  g_aTaskProcMsgNames[];  // 0x004d1818  0x4a entries (task-proc msgs)

// formatting / ini helpers (CRT-style + game ini section reader)
extern void  FUN_004895e0(void* dst, const char* src);                   // string copy
extern int   FUN_0048a060(void* dst, const char* fmt, ...);              // sprintf
extern void  FUN_0048b270(const char* src, const char* fmt, ...);        // sscanf
extern int   FUN_0048a340(void* dst, void* section);                     // open ini section
extern int   FUN_0048a790(void* dst, int max, int handle);              // read next ini line
extern void  FUN_0048a0d0(int handle);                                   // close ini section

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// 0x0043d760
void Magwrit_CleanExit(void)
{
    if (g_nMagwritEnabled != 0) {
        Debug_Trace(__LINE__, __FILE__, "tablet clean exit");
        ((PFN_TbltClose)g_pfnTbltClose)();
        g_nMagwritEnabled = 0;
        FreeLibrary((HMODULE)g_pKwtLcsoDll);
    }
}

// 0x0043d7c0  full bring-up: load DLL, tasks book, recognizer, buttons-map ini
void Magwrit_TabletInit(void)
{
    char szFmt[252];
    char szLine[256];
    int  iniHandle;
    int  rectIdx;
    int  rc;

    Debug_Trace(__LINE__, __FILE__, "tablet init");

    if (Magwrit_LoadKwtDll() == 0) {
        Err_BadResEntry(__LINE__, __FILE__, "Error loading kwt_lcso.dll");
    }

    FUN_004895e0(szLine, "c:\\pennet10\\bookdemo.kwt");
    rc = ((PFN_LoadTasksBook)g_pfnLoadTasksBook)(szLine);
    if (rc < 0) {
        FUN_0048a060(szFmt, "Error code = %d", rc);
        MessageBoxA(NULL, szFmt, "Book load error", 0);
        Debug_Trace(__LINE__, __FILE__, "tablet book load error");
    }
    Debug_Trace(__LINE__, __FILE__, "Book loaded %d pages", rc);

    if (Magwrit_InitPenNet(g_nHwndMain) == 0) {
        MessageBoxA(NULL, "Tablet not attached", "Init fail", 0x30);
        Debug_Trace(__LINE__, __FILE__, "tablet not attached");
    }

    ((PFN_LoadRecognizerParams)g_pfnLoadRecognizerParams)("c:\\pennet10\\");
    ((PFN_InitTaskManager)g_pfnInitTaskManager)(0);
    ((PFN_TbltEnable)g_pfnTbltEnable)(1);
    Win_RegisterExitHandler((void*)&Magwrit_CleanExit);

    // Load the [BUTTONS_MAP] ini section: button rect map. Try the save-game
    // dir first, then fall back to the adventure (content) dir.
    FUN_0048a060(szFmt, "%s%s", g_abSaveGameDir, "BUTTONS_MAP");
    iniHandle = FUN_0048a340(szFmt, NULL);
    if (iniHandle == 0) {
        FUN_0048a060(szFmt, "%s%s", g_abAdventDir, "BUTTONS_MAP");
        iniHandle = FUN_0048a340(szFmt, NULL);
    }

    rectIdx = 0;
    while ((*(unsigned int*)(iniHandle + 0xc) & 0x10) == 0) {
        FUN_0048a790(szFmt, 0xfa, iniHandle);
        FUN_0048b270(szFmt, "%d %d %d %d %d",
                     &g_anMagwritButtonRects[rectIdx * 5 + 0],
                     &g_anMagwritButtonRects[rectIdx * 5 + 1],
                     &g_anMagwritButtonRects[rectIdx * 5 + 2],
                     &g_anMagwritButtonRects[rectIdx * 5 + 3],
                     &g_anMagwritButtonRects[rectIdx * 5 + 4]);
        rectIdx++;
        if (rectIdx == MAX_BUTTON_RECTS) {
            Err_BadResEntry(__LINE__, __FILE__, "Too many button rects");
        }
    }
    g_nMagwritButtonRectCount = rectIdx;
    FUN_0048a0d0(iniHandle);

    g_nMagwritEnabled = 1;
    Debug_Trace(__LINE__, __FILE__, "tablet init end");
}

// 0x0043db90  LoadLibrary + GetProcAddress for every export. Aborts (returns 0)
// on the first missing symbol, otherwise returns 1.
int Magwrit_LoadKwtDll(void)
{
    g_pKwtLcsoDll = LoadLibraryA("kwt_lcso");
    if (g_pKwtLcsoDll == NULL)
        return 0;

    HMODULE h = (HMODULE)g_pKwtLcsoDll;

    if ((g_pfnLoadTasksBook                  = GetProcAddress(h, "_LoadTasksBook"))                  == 0) return 0;
    if ((g_pfnFreeTasksBook                  = GetProcAddress(h, "_FreeTasksBook"))                  == 0) return 0;
    if ((g_pfnPaintPage                      = GetProcAddress(h, "_PaintPage"))                      == 0) return 0;
    if ((g_pfnTbltOpen                       = GetProcAddress(h, "_TbltOpen"))                       == 0) return 0;
    if ((g_pfnTbltClose                      = GetProcAddress(h, "_TbltClose"))                      == 0) return 0;
    if ((g_pfnTbltEnable                     = GetProcAddress(h, "_TbltEnable"))                     == 0) return 0;
    if ((g_pfnTbltSetNotificationWindow      = GetProcAddress(h, "_TbltSetNotificationWindow"))      == 0) return 0;
    if ((g_pfnTbltSetActiveArea              = GetProcAddress(h, "_TbltSetActiveArea"))              == 0) return 0;
    if ((g_pfnTbltAddButton                  = GetProcAddress(h, "_TbltAddButton"))                  == 0) return 0;
    if ((g_pfnTbltDetachButton               = GetProcAddress(h, "_TbltDetachButton"))               == 0) return 0;
    if ((g_pfnTbltDetachAllButtons           = GetProcAddress(h, "_TbltDetachAllButtons"))           == 0) return 0;
    if ((g_pfnTbltSetButtonFunc              = GetProcAddress(h, "_TbltSetButtonFunc"))              == 0) return 0;
    if ((g_pfnTbltSetButtonRect              = GetProcAddress(h, "_TbltSetButtonRect"))              == 0) return 0;
    if ((g_pfnTbltPointsBufferFlush          = GetProcAddress(h, "_TbltPointsBufferFlush"))          == 0) return 0;
    if ((g_pfnTbltEnableCollect              = GetProcAddress(h, "_TbltEnableCollect"))              == 0) return 0;
    if ((g_pfnTbltDPtoNTP                    = GetProcAddress(h, "_TbltDPtoNTP"))                    == 0) return 0;
    if ((g_pfnInitTaskManager                = GetProcAddress(h, "_InitTaskManager"))                == 0) return 0;
    if ((g_pfnRestartManager                 = GetProcAddress(h, "_RestartManager"))                 == 0) return 0;
    if ((g_pfnAddTaskInPageToManager         = GetProcAddress(h, "_AddTaskInPageToManager"))         == 0) return 0;
    if ((g_pfnAddAllPageTasksToManager       = GetProcAddress(h, "_AddAllPageTasksToManager"))       == 0) return 0;
    if ((g_pfnPerformBatchProcess            = GetProcAddress(h, "_PerformBatchProcess"))            == 0) return 0;
    if ((g_pfnDetachTaskFromManager          = GetProcAddress(h, "_DetachTaskFromManager"))          == 0) return 0;
    if ((g_pfnActivateManagerTasks           = GetProcAddress(h, "_ActivateManagerTasks"))           == 0) return 0;
    if ((g_pfnDeactivateManagerTasks         = GetProcAddress(h, "_DeactivateManagerTasks"))         == 0) return 0;
    if ((g_pfnLoadRecognizerParams           = GetProcAddress(h, "_LoadRecognizerParams"))           == 0) return 0;
    if ((g_pfnGetNumberOfCollectedTaskPoints = GetProcAddress(h, "_GetNumberOfCollectedTaskPoints")) == 0) return 0;
    if ((g_pfnGetCollectedTaskPoints         = GetProcAddress(h, "_GetCollectedTaskPoints"))         == 0) return 0;
    if ((g_pfnGetRectOfCollectedTaskPoints   = GetProcAddress(h, "_GetRectOfCollectedTaskPoints"))   == 0) return 0;

    return 1;
}

// 0x0043e100
void Magwrit_PostInitCallback(void)
{
    Win_PostCallback((void*)&Magwrit_TabletInit);  // LAB_00401df2 deferred-init thunk
}

// ---------------------------------------------------------------------------
// Pages & tasks
// ---------------------------------------------------------------------------

// 0x0043e120  Reset button/task tables for a new tablet page and restart the
// recognition manager.
void Magwrit_BeginPage(int nPageId)
{
    if (g_nMagwritEnabled == 0)
        return;

    for (int i = 0; i < MAX_BUTTONS; i++) {
        g_anMagwritButtons[i * 2 + 1] = -1;
        g_anMagwritButtons[i * 2 + 0] = -1;
    }
    for (int i = 0; i < MAX_TASKS; i++) {
        T_ID(i) = -1;
    }
    g_nMagwritCurrentPage = nPageId;
    ((PFN_RestartManager)g_pfnRestartManager)();
}

// 0x0043e1d0  Allocate a free task slot, register it with the manager and arm it.
void Magwrit_AddTask(int nTaskId, int nCallback, int nMask)
{
    if (g_nMagwritEnabled == 0)
        return;

    int i;
    for (i = 0; T_ID(i) != -1 && i < MAX_TASKS; i++) {
    }
    if (i == MAX_TASKS) {
        Err_BadResEntry(__LINE__, __FILE__, "Too many tasks");
    }

    T_ID(i)    = nTaskId;
    T_CB(i)    = nCallback;
    T_LOW(i)   = (int)0x80000000; // INT_MIN: range fully open until set
    T_HIGH(i)  = 0x7fffffff;      // INT_MAX
    T_MASK(i)  = nMask;
    T_STATE(i) = 1;               // inited
    T_COUNT(i) = 0;

    ((PFN_DeactivateManagerTasks)g_pfnDeactivateManagerTasks)();
    Debug_Trace(__LINE__, __FILE__, "adding task %d in page %d", nTaskId, g_nMagwritCurrentPage);
    T_HANDLE(i) = ((PFN_AddTaskInPageToManager)g_pfnAddTaskInPageToManager)(
                      g_nMagwritCurrentPage, nTaskId, g_nHwndMain, 0, 0);
    if (T_HANDLE(i) == -1) {
        Err_BadResEntry(__LINE__, __FILE__, "Could not add task to manager");
    }
    Debug_Trace(__LINE__, __FILE__, "Task inited, id %d", T_HANDLE(i));
    ((PFN_ActivateManagerTasks)g_pfnActivateManagerTasks)(10000);
}

// 0x0043e3c0  Finalize a task: flush its ink through batch recognition.
void Magwrit_EndTask(int nTaskId)
{
    Debug_Trace(__LINE__, __FILE__, "End task %d", nTaskId);
    if (g_nMagwritEnabled == 0)
        return;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (T_ID(i) == nTaskId) {
            Debug_Trace(__LINE__, __FILE__, "Doing Batch fot task end %d", nTaskId);
            ((PFN_DeactivateManagerTasks)g_pfnDeactivateManagerTasks)();
            T_STATE(i) = 2; // batch-pending
            ((PFN_PerformBatchProcess)g_pfnPerformBatchProcess)(T_HANDLE(i));
            T_STATE(i) = 4; // done
            ((PFN_ActivateManagerTasks)g_pfnActivateManagerTasks)(10000);
        }
    }
}

// 0x0043e4d0  Detach a task from the manager and free its slot.
void Magwrit_DetachTask(int nTaskId)
{
    if (g_nMagwritEnabled == 0)
        return;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (T_ID(i) == nTaskId) {
            if (T_HANDLE(i) != -1) {
                ((PFN_DeactivateManagerTasks)g_pfnDeactivateManagerTasks)();
                ((PFN_DetachTaskFromManager)g_pfnDetachTaskFromManager)(T_HANDLE(i));
                ((PFN_ActivateManagerTasks)g_pfnActivateManagerTasks)(10000);
                T_HANDLE(i) = -1;
            }
            T_ID(i) = -1;
        }
    }
}

// 0x0043f0b0
void Magwrit_SetTaskRangeLow(int nTaskId, int nLow)
{
    int i;
    for (i = 0; T_ID(i) != nTaskId && i < MAX_TASKS; i++) {
    }
    if (i != MAX_TASKS) {
        T_LOW(i) = nLow;
    }
}

// 0x0043f110
void Magwrit_SetTaskRangeHigh(int nTaskId, int nHigh)
{
    int i;
    for (i = 0; T_ID(i) != nTaskId && i < MAX_TASKS; i++) {
    }
    if (i != MAX_TASKS) {
        T_HIGH(i) = nHigh;
    }
}

// 0x0043f170
int Magwrit_GetTaskPointCount(int nTaskId)
{
    int i;
    for (i = 0; T_ID(i) != nTaskId && i < MAX_TASKS; i++) {
    }
    if (i == MAX_TASKS)
        return 0;
    return T_COUNT(i);
}

// 0x0043f1d0  Pop the most recently collected ink-point id (LIFO), -1 if empty.
int Magwrit_PopTaskPoint(int nTaskId)
{
    int i;
    for (i = 0; T_ID(i) != nTaskId && i < MAX_TASKS; i++) {
    }
    if (i == MAX_TASKS)
        return -1;
    if (T_COUNT(i) < 1)
        return -1;

    T_COUNT(i) = T_COUNT(i) - 1;
    return T_INK(i)[T_COUNT(i)];
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

// 0x0043e610  Bind a hardware button to a script callback and tell the tablet
// the button's screen rectangle (from the [BUTTONS_MAP] table).
void Magwrit_AddButton(int nButtonId, int nCallback)
{
    if (g_nMagwritEnabled == 0)
        return;

    int i;
    RECT rc;

    for (i = 0; i < MAX_BUTTONS && g_anMagwritButtons[i * 2] != -1; i++) {
    }
    if (i == MAX_BUTTONS) {
        Err_BadResEntry(__LINE__, __FILE__, "Too many buttons");
    }
    g_anMagwritButtons[i * 2 + 0] = nButtonId;
    g_anMagwritButtons[i * 2 + 1] = nCallback;

    // Verify the button id exists somewhere in the full rect map.
    for (i = 0; i < MAX_BUTTON_RECTS && g_anMagwritButtonRects[i * 5] != nButtonId; i++) {
    }
    if (i == MAX_BUTTON_RECTS) {
        Err_BadResEntry(__LINE__, __FILE__, "Button id not in map");
    }

    // Find it within the actually-loaded entries and register its rect.
    for (i = 0; i < g_nMagwritButtonRectCount && g_anMagwritButtonRects[i * 5] != nButtonId; i++) {
    }
    if (i < g_nMagwritButtonRectCount) {
        SetRect(&rc,
                g_anMagwritButtonRects[i * 5 + 1],
                g_anMagwritButtonRects[i * 5 + 2],
                g_anMagwritButtonRects[i * 5 + 3],
                g_anMagwritButtonRects[i * 5 + 4]);
        ((PFN_TbltAddButton)g_pfnTbltAddButton)(nButtonId, &rc, 0);
    } else {
        Err_BadResEntry(__LINE__, __FILE__, "Button rect not loaded");
    }
}

// 0x0043e7f0
void Magwrit_DetachButton(int nButtonId)
{
    if (g_nMagwritEnabled == 0)
        return;

    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (g_anMagwritButtons[i * 2] == nButtonId) {
            ((PFN_TbltDetachButton)g_pfnTbltDetachButton)(nButtonId);
            g_anMagwritButtons[i * 2] = -1;
        }
    }
}

// 0x0043e860
void Magwrit_DetachAllButtons(void)
{
    if (g_nMagwritEnabled != 0) {
        ((PFN_TbltDetachAllButtons)g_pfnTbltDetachAllButtons)();
    }
}

// 0x0043e880  DLL notification: a hardware button was pressed. Fire its bound
// callback once and detach it.
void Magwrit_OnButtonClicked(int nButtonId)
{
    if (g_nMagwritEnabled == 0)
        return;

    Debug_Trace(__LINE__, __FILE__, "tablet button %d clicked", nButtonId);
    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (g_anMagwritButtons[i * 2] == nButtonId && g_anMagwritButtons[i * 2 + 1] != -1) {
            Timer_AddSyncProg(g_anMagwritButtons[i * 2 + 1]);
            Magwrit_DetachButton(nButtonId);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Message pump
// ---------------------------------------------------------------------------

// 0x0043e590  Pop the head of the PENNET message FIFO, -1 if empty.
int Magwrit_DequeueMessage(void)
{
    int head = g_anMagwritMsgQueue[0];
    if (g_nMagwritMsgQueueCount < 1)
        return -1;

    for (int i = 0; i < g_nMagwritMsgQueueCount - 1; i++) {
        g_anMagwritMsgQueue[i] = g_anMagwritMsgQueue[i + 1];
    }
    g_nMagwritMsgQueueCount--;
    return head;
}

// 0x0043ea40  A recognized point id arrived for a task handle. If it falls in
// the task's range and the event mask/state agree, queue it and fire the cb.
void Magwrit_EnqueueTaskMessage(int nTaskHandle, int nPointId)
{
    int i = 0;
    while (i < MAX_TASKS && T_HANDLE(i) != nTaskHandle) {
        i++;
    }
    if (i == MAX_TASKS)
        return;

    if (nPointId >= T_LOW(i) && nPointId <= T_HIGH(i) && (T_MASK(i) & T_STATE(i)) != 0) {
        g_anMagwritMsgQueue[g_nMagwritMsgQueueCount] = nPointId;
        g_nMagwritMsgQueueCount++;
        if (g_nMagwritMsgQueueCount > MAX_MSG_QUEUE) {
            Err_BadResEntry(__LINE__, __FILE__, "Too many messages in PENNET queue");
        }
        if (T_CB(i) != -1) {
            Timer_AddSyncProg(T_CB(i));
        }
    }
}

// 0x0043eb70  Handle a "task proc" message: enqueue it, log known message
// strings, and collect the point id into the task's ink ring buffer.
void Magwrit_OnTaskProcMessage(int* pMsg)
{
    int msgId = pMsg[1];

    Debug_Trace(__LINE__, __FILE__, "Task Proc msg %d", msgId);
    if (msgId == 0)
        return;

    int handle = *(unsigned short*)(pMsg + 3);   // pMsg+0xc
    Magwrit_EnqueueTaskMessage(handle, msgId);

    // Log a human-readable name for known task-proc message ids.
    for (int j = 0; j < 0x4a; j++) {
        if (*(int*)(g_aTaskProcMsgNames + j * 0x36) == msgId) {
            const char* name = g_aTaskProcMsgNames + 4 + j * 0x36;
            Bani_Noop("Return %s", name);
            Debug_Trace(__LINE__, __FILE__, "Task proc msg is %s", name);
        }
    }

    int i = 0;
    while (i < MAX_TASKS && T_HANDLE(i) != (int)*(unsigned short*)(pMsg + 3)) {
        i++;
    }
    if (i != MAX_TASKS && T_COUNT(i) < MAX_INK_POINTS &&
        msgId > T_LOW(i) && msgId < T_HIGH(i)) {
        if ((T_MASK(i) & T_STATE(i)) == 0) {
            // Collect only if the "8" event bit is set and the id is not a dup.
            if ((T_MASK(i) & 8u) != 0) {
                int* ink = T_INK(i);
                for (int k = 0; k < T_COUNT(i) && ink[k] != msgId; k++) {
                    if (k == T_COUNT(i)) {
                        ink[T_COUNT(i)] = msgId;
                        T_COUNT(i) = T_COUNT(i) + 1;
                    }
                }
            }
        } else {
            T_INK(i)[T_COUNT(i)] = msgId;
            T_COUNT(i) = T_COUNT(i) + 1;
        }
    }
}

// 0x0043ee50  Handle a "task manager" message: log known names, then enqueue.
void Magwrit_OnTaskManagerMessage(int* pMsg)
{
    for (int j = 0; j < 8; j++) {
        if (*(int*)(g_aTaskMgrMsgNames + j * 0x36) == pMsg[1]) {
            const char* name = g_aTaskMgrMsgNames + 4 + j * 0x36;
            Bani_Noop("Return %s", name);
            Debug_Trace(__LINE__, __FILE__, "Task Manager msg is %s", name);
        }
    }
    Magwrit_EnqueueTaskMessage(pMsg[0], pMsg[1]);
}

// 0x0043ef10  Top-level PENNET message dispatcher (called from the DLL's
// notification window). msg type 1=pen point, 2=task-mgr, 3=task-proc.
void Magwrit_DispatchPenNetMessage(int* pMsg)
{
    if (g_nMagwritEnabled == 0)
        return;

    Debug_Trace(__LINE__, __FILE__, "MESSAGE %d", pMsg[0]);
    int type = pMsg[0];
    if (type == 1) {
        Debug_Trace(__LINE__, __FILE__, "pen point");
        int* pt = (int*)pMsg[2];
        Bani_Noop("point %d %d", pt[0], pt[1]);
        ((PFN_TbltPointsBufferFlush)g_pfnTbltPointsBufferFlush)();
    } else if (type == 2) {
        Bani_Noop("Task Manager message");
        Magwrit_OnTaskManagerMessage((int*)pMsg[2]);
    } else if (type == 3) {
        Magwrit_OnTaskProcMessage((int*)pMsg[2]);
    }
}

// ---------------------------------------------------------------------------
// Ink rendering / debug
// ---------------------------------------------------------------------------

// 0x0043e930  Draw one ink segment, mapping a tablet NTP point (param 5/6,
// normalized in 0x15e0 x 0x1130) into screen-space and line-ing from the
// previous point. A coord of -1 means pen-up (break the stroke).
void Magwrit_DrawInkSegment(int x0, int y0, int x1, int y1,
                            int ntpX, int ntpY, unsigned char color)
{
    int sx = x0 + (ntpX * (x1 - x0)) / 0x15e0;
    int sy = y0 + ((y1 - y0) * ntpY) / 0x1130;

    if (ntpX != -1 && ntpY != -1 && g_nMagwritLastInkX != -1 && g_nMagwritLastInkY != -1) {
        GI_SetDrawMode(0);
        GI_LockActiveSurf_v5(g_nMagwritLastInkX, g_nMagwritLastInkY, sx, sy, color);
    }

    if (ntpX == -1)
        sx = ntpX;   // == -1
    g_nMagwritLastInkX = sx;

    if (ntpY == -1)
        sy = ntpY;   // == -1
    g_nMagwritLastInkY = sy;
}

// 0x0043f030  Show the recognition status text overlay.
void Magwrit_ShowStatusText(void)
{
    struct { unsigned char tag; char text[255]; } str;
    int rect[4];

    str.tag = 0x40;
    FUN_004895e0(str.text, "");          // status string

    rect[0] = 0;       // top
    rect[1] = 0x1cc;   // left
    rect[2] = 0xf0;    // bottom
    rect[3] = 0x1df;   // right
    Txt_SetString(&str, rect, 999999);
}

// 0x0043f280  Fetch a task's collected ink points from the DLL and draw the
// stroke into the screen rect (param 2/3..4/5), mapping by the points' bounds.
void Magwrit_DrawTaskInk(int nTaskId, int x0, int y0, int x1, int y1, unsigned char color)
{
    int prevX = -1;
    int prevY = -1;
    int slot  = -1;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (T_ID(i) == nTaskId) {
            slot = i;
        }
    }
    if (slot == -1)
        return;

    int count = ((PFN_GetNumPoints)g_pfnGetNumberOfCollectedTaskPoints)(T_HANDLE(slot));
    Bani_Noop("Num of ink points %d", count);

    int* points = (int*)SafeHeap_Alloc(__LINE__, __FILE__, count << 3); // count * {x,y}
    ((PFN_GetPoints)g_pfnGetCollectedTaskPoints)(T_HANDLE(slot), (int)points, count);

    int bounds[4]; // {left, top, right, bottom}
    ((PFN_GetRect)g_pfnGetRectOfCollectedTaskPoints)(T_HANDLE(slot), bounds);

    for (int i = 0; i < count; i++) {
        int px = points[i * 2 + 0];
        int py = points[i * 2 + 1];

        int sx = x0 + ((px - bounds[0]) * (x1 - x0)) / (bounds[2] - bounds[0]);
        int sy = y0 + ((py - bounds[1]) * (y1 - y0)) / (bounds[3] - bounds[1]);

        if (px != -1 && py != -1 && prevX != -1 && prevY != -1) {
            GI_SetDrawMode(0);
            GI_LockActiveSurf_v5(prevX,     prevY,     sx,     sy,     color);
            GI_LockActiveSurf_v5(prevX + 1, prevY + 1, sx + 1, sy + 1, color);
            GI_LockActiveSurf_v5(prevX + 1, prevY - 1, sx + 1, sy - 1, color);
        }

        prevX = (px == -1) ? -1 : sx;
        prevY = (py == -1) ? -1 : sy;
    }
}
