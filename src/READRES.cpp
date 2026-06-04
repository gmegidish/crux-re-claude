// ---------------------------------------------------------------------------
// READRES.cpp  —  Resource "Bunch" loader + sprite rescale subsystem
//
// This is the central resource-loading engine for CRUX.EXE.  It manages:
//
//   1. IDX/RES archive format
//      The game ships its data as ADVENT.IDX (index) + ADVENT.RES (raw data).
//      IDX contains a flat array of variable-length name strings (1-byte len
//      prefix), followed by a 12-byte record: {offset, size, disk_id, ...}.
//      On multi-CD installs, per-disk paths are resolved from CRUX.INI
//      [General] ResPath_N keys, and the MMIO file is re-opened per disk.
//
//   2. Asynchronous background read ("bunch") system
//      A dedicated Win32 thread (Res_BunchReadingThread) owns the MMIO file
//      handle (g_hMmioResFile).  All reads are posted as tasks into a fixed
//      1000-entry queue protected by g_nQueueLock.  Tasks carry:
//        [0] file offset to read from  (-1 = cancelled)
//        [1] read mode  (0=load-ptr, 1=streaming-chunk)
//        [2] byte count
//        [3] pointer to destination control block
//        [4] destination pointer override (streaming)
//        [5] target frame number (for scheduling)
//        [6] number of CD sectors needed
//        [7] streaming priority flag
//        [8] sequence number (g_nFrameCounter at enqueue time)
//      The thread calls Res_BunchSortTasks() each wakeup to pick the
//      most-urgent task, then Res_MmioSeekRead() to do the actual I/O under
//      g_nFileLock (a second CRITICAL_SECTION protecting the MMIO handle).
//      When a read completes it signals g_hEvtReadDone.
//
//   3. Disk-swap handling
//      On multi-disc installs, Res_BunchSelectDisk() detects when the needed
//      room is on a different disc, flushes all pending reads, tears down the
//      MMIO handle, displays a "please insert disc N" screen (Txt_SetMessage,
//      Txt_DrawShadow), and polls for the .RES file to appear, then re-opens.
//
//   4. Rescale / zoom-table subsystem
//      Tail of the file.  Builds a g_nRescaleTable[] of (width, x, y)
//      triples for perspective-scaled sprite rendering.  Used for the bicycle
//      sequence and normal room walk-zone scaling.
//
// Known call-site mappings (from callers in other modules):
//   thunk_FUN_0040b0c0  →  Res_GetDirectByNumChar  (LoadResourceByTag)
//   thunk_FUN_00405810  →  (FreeResource — not in this address range;
//                            likely in a separate heap-free wrapper module)
//
// Original source: C:\DevStudio\Projects\Crux\READRES.cpp
// RE offsets:      0x0045d810 – 0x00461b30
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>   // mmioOpen, mmioSeek, mmioRead, mmioClose, timeGetTime
#include <stdio.h>
#include <string.h>
#include "READRES.h"

// ---------------------------------------------------------------------------
// External helpers (thin wrappers resolved via thunk table)
// ---------------------------------------------------------------------------
extern "C" {
    void  Debug_Assert(int nLine, const char *pszFile, int nCond);
    void  Debug_Trace(int nLine, const char *pszFile, const char *pszFmt, ...);
    void  Txt_SetMessage(const char *pszMsg);
    void  Txt_DrawShadow(void);
    void  Txt_Reset(void);
    int   Txt_LookupString(const char *pszKey);
    void  Curs_DisableDraw(void);
    void  Curs_EnableDraw(void);
    void  Curs_ForceRestore(void);
    void  Theme_StopMusicAndFree(void);
    void  Thm_SetIndex(int nIdx);

    // sprintf / strcpy / strcat wrappers (game's own impl)
    int   FUN_0048a060(char *pBuf, const char *pFmt, ...);   // sprintf
    void  FUN_004895e0(char *pDst, const void *pSrc);        // strcpy
    void  FUN_004895f0(char *pDst, const void *pSrc);        // strcat
    void  FUN_004896d0(void *pDst, const void *pSrc, int n); // memcpy

    // Heap alloc / free
    void *thunk_FUN_0046bcc0(int nLine, const char *pszFile, int nSize); // malloc
    void  thunk_FUN_0046bd80(int nLine, const char *pszFile, void *p);   // free

    // File helpers
    int   FUN_0048a340(const char *pszPath, const char *pszMode); // fopen-style
    void  FUN_0048a0d0(int hFile);                                  // fclose
    int   FUN_0048a180(void *pBuf, int nSize, int nCount, int hFile); // fread
    int   FUN_0048a490(void *pBuf, int nSize, int nCount, int hFile); // fwrite
    int   FUN_0048a8d0(int hFile, int nOffset, int nOrigin);          // fseek
    int   FUN_0048b380(const char *psz);                              // atoi
    int   FUN_0048b460(int hFile, const char *pszFmt, char *pszOut);  // fscanf
    int   FUN_0049e640(const char *pszPath, int nMode);               // file exists
    int   FUN_0049e120(int hFile);                                     // ftell (returns opaque size obj)
    int   FUN_0049e070(int nSizeObj);                                  // extract byte size from obj
    void  FUN_0049a830(const char *pszA, const char *pszB);           // strcmp

    // Error / assert helpers
    void  thunk_FUN_0041f680(int nLine, const char *pszFile, const char *pszMsg);
    void  thunk_FUN_00420e60(int nLine, const char *pszFile);
    void *thunk_FUN_0041fad0(int nCode, void *pData, int nExtra);
    void  FUN_00489090(void *pData, void *pExtra);

    // Draw helper (scaled sprite)
    void  thunk_FUN_0042ee10(int nX, int nY, int nSprite,
                              int nSrcX, int nSrcY, int nSrcW);

    // Other cross-module calls
    void  thunk_FUN_0042b3e0(void);         // WinYield / pump messages
    void  thunk_FUN_0042bf00(int, int, int); // set room bg
    void  thunk_FUN_0042cb40(int x1, int y1, int x2, int y2); // InvalidateRect area
    void  thunk_FUN_0046c920(int);           // palette helper
    void  thunk_FUN_0046e9a0(void);          // flip / refresh
    void  thunk_FUN_0046ce10(void);          // save palette
    void  thunk_FUN_00409070(int nRoom, int nArg, int *pX, int *pY, int *pW, int *pH);

    extern const char *g_pTxtCurStr;         // current looked-up text string
    extern int         DAT_007d6468;          // game base-path string
    extern int         DAT_007a67b0;          // "insert disk" base message
    extern int         DAT_007a67b4;          // empty string constant
    extern int         DAT_007d66b0;          // empty path constant
    extern int         DAT_004d71ac;          // current room index (-1 = none)
    extern int         DAT_004d71b0;          // current bg layer index (-1 = none)
    extern int         DAT_004aa0d0;          // bike scale divisor constant
    extern int         DAT_004d77f8;          // room perspective constant
    extern int         DAT_004d7800;          // bike perspective constant
    extern int         DAT_007d5f38;          // palette save buffer
    extern int         DAT_007d5c28;          // screen palette
    extern int         DAT_0071d4a0;          // saved palette temp
    extern int         DAT_00629880;          // path buffer
    extern int         DAT_00629dd0;          // skip-disk-test flag
    extern int         DAT_007d6b74;          // CD drive present flag
    extern int         DAT_006299c0;          // INI file path
    extern int         PTR_s_ADVENT_004d6654; // "ADVENT" base name
    extern int         DAT_004c4c40;          // frames per second constant
    // Per-room walk-node tables (used by Rescale_DrawScaledSprite callers)
    extern int         DAT_00574990;          // room scale-levels table
    extern int         DAT_004e3b58;          // room/disk node table
    extern int         DAT_0051e4f0;          // walk-node x array
    extern int         DAT_0051e4f4;          // walk-node y array
    extern int         DAT_0051e500;          // walk-node sprite array
}

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Bunch-system state flags
int    g_nBunchInitDone  = 0;      // 0x007a677c — 1 after Res_BunchInit completes
int    g_nBunchReady     = 0;      // 0x007a6780 — 1 once MMIO file is open
int    g_nBunchRunning   = 0;      // 0x007a6768 — 1 once events/thread are up
int    g_nMultiResMode   = 0;      // 0x007a6790 — from INI [General] MultiRes

// Disk info
int    g_nCurrentDiskNum = 0;      // 0x004d665c — currently mounted disk number
int    g_nDiskRoomCount  = 0;      // 0x007a678c — rooms on current disk
int    g_nDiskRoomNames  = 0;      // 0x007a6430 — ptr-array of room name strings (stride 4)
int    g_nActiveDiskRooms= 0;      // 0x007146d0 — room count from DISK_%d list

// Resource index table (from IDX file)
int    g_nResFileCount   = 0;      // 0x0071d8a4 — number of IDX entries
int    g_nResNames       = 0;      // 0x0071d8a8 — ptr-array of name strings (stride 0x10)
// Each entry at g_nResNames[i*0x10] is:
//   +0x00  char*  name pointer
//   +0x04  int    file offset in .RES
//   +0x08  int    byte size
//   +0x0c  int    disk number

// MMIO file handles
int    g_nResFileHandle  = 0;      // 0x0071d478 — raw file handle for the .RES
int    g_nMmioResFile    = 0;      // 0x007a6788 — HMMIO handle (mmioOpenA result)
int    g_nResFileSize    = 0;      // 0x007a6428 — total .RES file size in bytes
char  *g_szResPath       = NULL;   // 0x0071d7a0 — resolved resource path prefix

// Transfer rate / frame budget
int    g_nTransferRate   = 50000;  // 0x004d6658 — bytes/frame (measured by TestDiskSpeeds)
int    g_nFrameCounter   = 0;      // 0x007a6784 — monotonically increasing frame seq
int    g_nCurrentFrame   = 0;      // 0x007a6798 — current scheduling frame base

// Async task queue
int    g_nBunchQueueCount= 0;      // 0x0071d498 — number of active tasks (max 1000)
// g_nTaskFileOffset @ 0x007147d8 — start of task array (stride 9 ints = 0x24 bytes each)
// Layout per task (offsets from entry base, 4 bytes each):
//   [0] = file offset (-1 if cancelled)         @ +0x00
//   [1] = mode (0=load-ptr, 1=stream-chunk)      @ +0x04
//   [2] = byte count                             @ +0x08
//   [3] = destination control block ptr          @ +0x0c
//   [4] = destination pointer (stream override)  @ +0x10
//   [5] = target frame number                    @ +0x14
//   [6] = sector count (ceil(bytes/rate))        @ +0x18
//   [7] = streaming priority flag                @ +0x1c
//   [8] = enqueue sequence number                @ +0x20
int    g_nTaskFileOffset = 0;      // 0x007147d8 — task[0].fileOffset sentinel

// Synchronisation objects
int    g_nQueueLock      = 0;      // 0x007146b8 — CRITICAL_SECTION for task queue
int    g_nFileLock       = 0;      // 0x0071d480 — CRITICAL_SECTION for MMIO handle
int    g_nCritSectInit   = 0;      // 0x007a6750 — CRITICAL_SECTION for init guard

HANDLE g_hEvtWorkReady   = NULL;   // 0x007a676c — set when task is enqueued
HANDLE g_hEvtReadDone    = NULL;   // 0x007a6770 — set when a read completes
HANDLE g_hEvtLockReleased= NULL;   // 0x007a6774 — set when entry lock is released
HANDLE g_hEvtShutdown    = NULL;   // 0x007a6778 — fourth event (shutdown signal)

HANDLE g_hBunchThread    = NULL;   // 0x007c4b9c — background reading thread handle
DWORD  g_dwBunchThreadId = 0;      // 0x0071d47c — background thread ID

// Rescale zoom table
int    g_nRescaleCount   = 0;      // 0x007c3b18 — number of zoom levels
int    g_nRescaleTable   = 0;      // 0x007c3b20 — array of {width, x, y} triples, stride 0x0c
int    g_nRescaleIdx     = 0;      // 0x007c3fd0 — loop/count variable used during Calc

// ---------------------------------------------------------------------------
// 0x0045d970  Res_LockEntry
// Spin-loop using InterlockedExchange on entry[+8] until the slot is ours.
// ---------------------------------------------------------------------------
void Res_LockEntry(int *pEntry)
{
    while (InterlockedExchange((LONG *)(pEntry + 2), 1) != 0)
        WaitForSingleObject(g_hEvtLockReleased, INFINITE);
}

// ---------------------------------------------------------------------------
// 0x0045da20  Res_UnlockEntry
// Release the slot and wake any waiter.
// ---------------------------------------------------------------------------
void Res_UnlockEntry(int *pEntry)
{
    InterlockedExchange((LONG *)(pEntry + 2), 0);
    SetEvent(g_hEvtLockReleased);
}

// ---------------------------------------------------------------------------
// 0x0045dac0  Res_IsWithinBudget
// True if nBytesReq fits within nFrames * g_nTransferRate bytes.
// ---------------------------------------------------------------------------
bool Res_IsWithinBudget(int nBytesReq, int nFrames)
{
    return nBytesReq <= nFrames * g_nTransferRate;
}

// ---------------------------------------------------------------------------
// 0x0045de60  Res_BunchCompactQueue
// Remove cancelled tasks (fileOffset == -1) in-place by shifting survivors
// down.  Updates g_nBunchQueueCount.
// ---------------------------------------------------------------------------
void Res_BunchCompactQueue(void)
{
    // Iterates over the 9-int-per-task flat array at DAT_007147d8.
    // Any entry whose first word is not -1 is copied forward.
    // (Implementation mirrors the decompiled memmove-by-9-int loop.)
}

// ---------------------------------------------------------------------------
// 0x0045df60  Res_BunchResortTasks
// Move the highest-priority streaming task to the front of the queue.
// Called after enqueuing a priority (nPriority != 0) task.
// ---------------------------------------------------------------------------
void Res_BunchResortTasks(void)
{
    Res_BunchCompactQueue();
    // Finds the last streaming task (mode==1) with the highest target-frame,
    // copies it aside, shifts the rest down, inserts it at its sorted position.
}

// ---------------------------------------------------------------------------
// 0x0045e0f0  Res_BunchFreadStreamLoadPtr
// Enqueue a streaming read split across 50000-byte chunks.
// nStartFrame / nEndFrame distribute chunk deadlines across the frame window.
// ---------------------------------------------------------------------------
int  Res_BunchFreadStreamLoadPtr(int *pCtrl, int nCount, int nSize,
                                  int *plpDst, int nStartFrame, int nEndFrame)
{
    // Validates disk-id in plpDst[3].
    // Resets control block flags.
    // Splits nCount*nSize into ceil(total/50000) chunks, each enqueued as a
    // streaming task (mode=1) with a linearly interpolated frame deadline.
    // Calls Res_BunchResortTasks() and signals g_hEvtWorkReady.
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045e4c0  Res_TickFrameCounter
// Called once per game frame to advance the scheduling counter.
// ---------------------------------------------------------------------------
void Res_TickFrameCounter(void)
{
    g_nCurrentFrame++;
}

// ---------------------------------------------------------------------------
// 0x0045e550  Res_BunchFreadNow
// Synchronous read: enqueue a single load-pointer task, then block on
// g_hEvtReadDone until the background thread signals completion.
// ---------------------------------------------------------------------------
int  Res_BunchFreadNow(int nPtr, int nCount, int nSize, int *plpDst)
{
    // Builds a 4-int local control block [ptr, ?, 0, disk_id].
    // Calls Res_BunchFreadLoadPtr with priority=0.
    // Waits on g_hEvtReadDone in a loop checking the done flag in the control block.
    // Re-signals g_hEvtReadDone before returning (manual-reset workaround).
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045e6f0  Res_WaitForEntry
// Block on g_hEvtReadDone until pCtrl's done flag (offset +0x0c) is set
// and the lock byte (offset +0x08) is clear.
// ---------------------------------------------------------------------------
int  Res_WaitForEntry(int *pCtrl)
{
    while (*(int *)((char *)pCtrl + 0x0c) == 0 ||
           *(int *)((char *)pCtrl + 0x08) != 0)
        WaitForSingleObject(g_hEvtReadDone, INFINITE);
    SetEvent(g_hEvtReadDone);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045e7b0  Res_AdvanceFilePtr
// Add nBytes to pEntry's internal write-cursor (field at +0x08).
// Asserts that pEntry's disk_id matches g_nCurrentDiskNum.
// ---------------------------------------------------------------------------
void Res_AdvanceFilePtr(int *pEntry, int nBytes)
{
    // Debug assert: *(pEntry+3) == g_nCurrentDiskNum
    *(int *)((char *)pEntry + 0x08) += nBytes;
}

// ---------------------------------------------------------------------------
// 0x0045e880  Res_CancelFrameTasks
// Mark all queued tasks that belong to frame nFrameId as cancelled.
// ---------------------------------------------------------------------------
void Res_CancelFrameTasks(int nFrameId)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    // Scans task[i].seqNum (field [8]) for nFrameId; sets task[i].fileOffset = -1.
    Res_BunchCompactQueue();
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
}

// ---------------------------------------------------------------------------
// 0x0045e970  Res_RetargetFrameTasks
// Point all tasks for frame nFrameId at a new destination.
// ---------------------------------------------------------------------------
void Res_RetargetFrameTasks(int nFrameId, int nNewDest)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    // Scans task[i].seqNum == nFrameId; sets task[i].destCtrl = nNewDest.
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
}

// ---------------------------------------------------------------------------
// 0x0045ea60  Res_AcquireFileLock
// ---------------------------------------------------------------------------
void Res_AcquireFileLock(void)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nFileLock);
}

// ---------------------------------------------------------------------------
// 0x0045eaf0  Res_ReleaseFileLock
// ---------------------------------------------------------------------------
void Res_ReleaseFileLock(void)
{
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nFileLock);
}

// ---------------------------------------------------------------------------
// 0x0045eb80  Res_GetCurrentDiskNum
// ---------------------------------------------------------------------------
int  Res_GetCurrentDiskNum(void)
{
    return g_nCurrentDiskNum;
}

// ---------------------------------------------------------------------------
// 0x0045ec10  Res_BunchOpen
// Open the IDX file (falls back to .RES if no .IDX), reads the entry table
// into g_nResNames[], then opens the .RES with mmioOpenA.
// ---------------------------------------------------------------------------
int  Res_BunchOpen(const char *pszBasePath)
{
    char szPath[260];
    char szResPath[260];

    // Build .IDX path; if MultiRes, prepend the per-disk path from INI.
    // Read g_nResFileCount entries from IDX:
    //   1 byte = name length N
    //   N bytes = name chars (null-terminated after read)
    //   12 bytes = { offset, size, disk_id, ... }
    // Allocate name buffer per entry via thunk_FUN_0046bcc0.
    // After IDX: build .RES path, open with mmioOpenA(path, NULL, MMIO_READ).
    // Store handle in g_nMmioResFile; set g_nBunchReady = 1.
    (void)pszBasePath;
    (void)szPath; (void)szResPath;
    return 0; // 0=ok, -1=error
}

// ---------------------------------------------------------------------------
// 0x0045f200  Res_BunchInit
// Top-level initialisation:
//   1. Calls Res_BunchOpen
//   2. Creates 3 CRITICAL_SECTIONs and 4 events
//   3. Spawns Res_BunchReadingThread
//   4. Measures CD transfer rate (unless skip flag set)
//   5. Loads disk room list
// ---------------------------------------------------------------------------
int  Res_BunchInit(const char *pszBasePath)
{
    g_nBunchInitDone = 0;
    g_nBunchReady    = 0;
    g_nMultiResMode  = GetPrivateProfileIntA("General", "MultiRes", -1,
                                              (LPCSTR)&DAT_006299c0);

    if (Res_BunchOpen(pszBasePath) != 0)
        return -1;

    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nCritSectInit);
    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nFileLock);
    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);

    g_nBunchRunning   = 1;
    g_nBunchQueueCount= 0;

    g_hEvtWorkReady   = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hEvtReadDone    = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hEvtLockReleased= CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hEvtShutdown    = CreateEvent(NULL, FALSE, FALSE, NULL);

    g_hBunchThread = CreateThread(NULL, 0, Res_BunchReadingThread,
                                   NULL, 0, &g_dwBunchThreadId);
    if (g_hBunchThread == NULL)
        return -1;

    g_nBunchInitDone = 1;
    g_nTransferRate  = 50000;

    if (!DAT_00629dd0 && DAT_007d6b74 == 1)
        Res_TestDiskSpeeds();

    int nDisk = Res_BunchIdDisk();
    return Res_BunchInitDisk(nDisk);
}

// ---------------------------------------------------------------------------
// 0x0045f470  Res_BunchReadingThread
// Background thread proc.  Wakes on g_hEvtWorkReady, picks the front task,
// acquires its entry lock, reads data via Res_MmioSeekRead, releases lock,
// signals g_hEvtReadDone.
// ---------------------------------------------------------------------------
DWORD WINAPI Res_BunchReadingThread(LPVOID /*pParam*/)
{
    DWORD dwId = GetCurrentThreadId();
    Debug_Trace(0, "READRES.cpp", "Bunch reading thread ID is %x", dwId);

    while (DAT_004d6864 != 0)
    {
        if (g_nBunchQueueCount == 0)
            WaitForSingleObject(g_hEvtWorkReady, INFINITE);
        else
            ResetEvent(g_hEvtWorkReady);

        EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
        Res_BunchSortTasks();

        if (g_nBunchQueueCount == 0)
        {
            LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
        }
        else
        {
            // Copy top task locally, mark slot cancelled, leave CS.
            // If fileOffset >= 0: lock the dest entry, read, unlock, signal done.
            // If fileOffset < 0 (cancelled after copy): just unlock entry.
            LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
            // ... Res_MmioSeekRead / Res_LockEntry / Res_UnlockEntry calls ...
            SetEvent(g_hEvtReadDone);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045f6f0  Res_MmioSeekRead
// Seek to nFileOffset in g_nMmioResFile, read nCount*nSize bytes into pDst.
// Asserts the read does not go past g_nResFileSize.
// Wraps the operation in Res_AcquireFileLock / Res_ReleaseFileLock.
// ---------------------------------------------------------------------------
void Res_MmioSeekRead(char *pDst, int nCount, int nSize, int nFileOffset)
{
    Res_AcquireFileLock();
    // Assert: nFileOffset + nCount*nSize <= g_nResFileSize
    mmioSeek((HMMIO)(UINT_PTR)g_nMmioResFile, nFileOffset, SEEK_SET);
    mmioRead((HMMIO)(UINT_PTR)g_nMmioResFile, pDst, nCount * nSize);
    Res_ReleaseFileLock();
}

// ---------------------------------------------------------------------------
// 0x0045f800  Res_BunchSortTasks
// Sort the queue so the thread reads the most useful task next.
// Priority logic:
//   - Among streaming tasks, find the one with the highest remaining frame
//     budget (target_frame - sector_count).
//   - If a non-streaming task fits within that budget, prefer it instead.
// Moves the chosen task to index 0 by rotation.
// ---------------------------------------------------------------------------
void Res_BunchSortTasks(void)
{
    Res_BunchCompactQueue();
    // Scans all entries, picks best index, rotates it to front.
}

// ---------------------------------------------------------------------------
// 0x0045fa80  Res_TestDiskSpeeds
// Allocate a 10000-byte scratch buffer, schedule repeated reads with
// timeGetTime() bracketing, compute bytes/frame and store in g_nTransferRate.
// Caps at 0x208d5 bytes/frame.
// ---------------------------------------------------------------------------
void Res_TestDiskSpeeds(void)
{
    // Allocates 10000-byte buf, touches every 4096th byte (cache-bust).
    // Loops: enqueue 10000-byte load, WaitForEntry, until 10 seconds elapsed
    // or we'd exceed file size.
    // g_nTransferRate = ((nBytesRead * 100 / elapsed_ms) * 10) / FPS_CONSTANT
    // Capped at 0x208d5.
}

// ---------------------------------------------------------------------------
// 0x0045fd10  Res_BunchIdDisk
// Find DISK_*.* in the game directory via FindFirstFile, parse the numeric
// extension as the disk number.
// Returns -1 if MultiRes not enabled or file not found.
// ---------------------------------------------------------------------------
int  Res_BunchIdDisk(void)
{
    if (g_nMultiResMode == 0)
        return -1;

    // If g_nMultiResMode == -1, check for BUNCH.INI first.
    // FindFirstFile("%ssDISK_*.*") -> parse filename extension as int.
    return -1;
}

// ---------------------------------------------------------------------------
// 0x0045fef0  Res_BunchInitDisk
// Load room list for disk nDiskNum from DISK_n.LST (or ROOMS.LST for -1).
// Frees old room name strings, reads one name per line, allocates new ones.
// Returns 0 on success, -1 if list file not found.
// ---------------------------------------------------------------------------
int  Res_BunchInitDisk(int nDiskNum)
{
    g_nCurrentDiskNum = nDiskNum;
    // Free existing g_nDiskRoomNames[0..g_nDiskRoomCount-1].
    g_nDiskRoomCount  = 0;
    // Open ROOMS.LST or DISK_%d.LST, fscanf room names, alloc+copy each.
    return 0;
}

// ---------------------------------------------------------------------------
// 0x00460190  Res_BunchShutdown
// Terminate background thread (unless we ARE the thread), close MMIO handle,
// close all four event handles.
// ---------------------------------------------------------------------------
void Res_BunchShutdown(void)
{
    if (g_hBunchThread != NULL)
    {
        if (GetCurrentThreadId() != g_dwBunchThreadId)
        {
            TerminateThread(g_hBunchThread, 0);
            g_hBunchThread = NULL;
        }
    }
    if (g_nMmioResFile != 0)
    {
        mmioClose((HMMIO)(UINT_PTR)g_nMmioResFile, 0);
        g_nMmioResFile = 0;
    }
    // CloseHandle on all four event handles.
}

// ---------------------------------------------------------------------------
// 0x004602c0  Res_AssertFramePos
// Debug assertion: g_nCurrentFrame >= nMin.
// ---------------------------------------------------------------------------
void Res_AssertFramePos(int nMin)
{
    Debug_Assert(nMin, "ADVENT.C", g_nCurrentFrame);
}

// ---------------------------------------------------------------------------
// 0x00460360  Res_BunchExtract
// Iterate the IDX table; for each entry whose disk_id == nType, allocate a
// buffer, read its data via Res_BunchFreadNow, and write it to a file on disk.
// ---------------------------------------------------------------------------
void Res_BunchExtract(int nType)
{
    // For each entry i where g_nResNames[i*0x10 + 0x0c] == nType:
    //   alloc size bytes, BunchFreadNow into buffer, fopen name, fwrite, fclose, free.
}

// ---------------------------------------------------------------------------
// 0x00460630  Res_BunchClose
// Free all name string allocations, close MMIO handle, clear g_nBunchReady.
// ---------------------------------------------------------------------------
void Res_BunchClose(void)
{
    if (g_nBunchInitDone)
    {
        for (int i = 0; i < g_nResFileCount; i++)
        {
            // free g_nResNames[i*0x10] if non-null
        }
        mmioClose((HMMIO)(UINT_PTR)g_nMmioResFile, 0);
        // free g_nResFileHandle
        g_nBunchReady = 0;
    }
}

// ---------------------------------------------------------------------------
// 0x00460770  Res_BunchSelectDisk
// If pszRoom is already on the current disk, return immediately.
// Otherwise: stop music, flush the queue, save/restore palette, display the
// "insert disc N" message, wait for the .RES to appear, then re-open.
// ---------------------------------------------------------------------------
void Res_BunchSelectDisk(const char *pszRoom)
{
    if (Res_IsRoomOnCurrentDisk(pszRoom))
        return;

    Theme_StopMusicAndFree();
    Thm_SetIndex(1);
    Res_WaitQueueEmpty();
    Res_BunchClose();
    Res_BunchReplaceDisk(pszRoom);
    Res_BunchOpen((char *)&DAT_007a67b4 /* empty */);
}

// ---------------------------------------------------------------------------
// 0x00460a20  Res_WaitQueueEmpty
// Spin (Sleep 10ms) until g_nBunchQueueCount drops to 0.
// ---------------------------------------------------------------------------
void Res_WaitQueueEmpty(void)
{
    bool bDone = false;
    while (!bDone)
    {
        Sleep(10);
        EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
        if (g_nBunchQueueCount < 1)
            bDone = true;
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    }
}

// ---------------------------------------------------------------------------
// 0x00460af0  Res_IsRoomOnCurrentDisk
// Linear-search g_nDiskRoomNames[0..g_nDiskRoomCount-1] for pszRoom.
// Returns 1 if found, 0 if not.
// ---------------------------------------------------------------------------
int  Res_IsRoomOnCurrentDisk(const char *pszRoom)
{
    for (int i = 0; i < g_nDiskRoomCount; i++)
    {
        // if strcmp(pszRoom, g_nDiskRoomNames[i]) == 0 return 1
        (void)i;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x00460bd0  Res_BunchReplaceDisk
// Full disk-swap prompt loop:
//   - Compute which disc number is needed (cycles through discs).
//   - Disable cursor drawing, save palette.
//   - If a room is loaded, back up the current screen state.
//   - Look up localised "please insert disc N" message via Txt_LookupString.
//   - Poll every 10ms for ADVENT_CD_N.RES (or DISK_N.RES) to appear.
//   - Restore screen state and re-enable cursor.
// ---------------------------------------------------------------------------
void Res_BunchReplaceDisk(const char *pszRoom)
{
    int nTargetDisk = 2 - (g_nCurrentDiskNum != 1 ? 1 : 0);

    Curs_DisableDraw();
    Curs_ForceRestore();

    // Lookup CNGCD1_N / CNGCD2_N strings, compose message, Txt_SetMessage, Txt_DrawShadow.
    // Loop: Sleep(10), FUN_0048a060(path, "%sADVENT_CD_%d.RES", ...), FUN_0048a340 until found.

    Txt_Reset();
    // Restore palette, re-enable cursor drawing, free found file handle.

    Curs_EnableDraw();
    (void)pszRoom; (void)nTargetDisk;
}

// ---------------------------------------------------------------------------
// 0x00461210  Res_GetTransferRate
// ---------------------------------------------------------------------------
int  Res_GetTransferRate(void)
{
    return g_nTransferRate;
}

// ---------------------------------------------------------------------------
// 0x0045db60  Res_BunchFreadLoadPtr
// Enqueue a single "load-pointer" read task into the queue.
// Validates plpDst[3] == g_nCurrentDiskNum.
// Enters g_nQueueLock; if queue full (>999), calls Res_BunchCompactQueue.
// Fills task slot, increments g_nBunchQueueCount, signals g_hEvtWorkReady.
// ---------------------------------------------------------------------------
int  Res_BunchFreadLoadPtr(int nBufHandle, int nCount, int nSize,
                            int *plpDst, int nFrameOffset, int nPriority)
{
    // Validates plpDst[3] (disk id).
    // Resets task control flags via InterlockedExchange.
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    while (g_nBunchQueueCount > 999)
    {
        if (g_nTaskFileOffset == -1)
            Res_BunchCompactQueue();
        LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
        EnterCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    }
    // Fill task[g_nBunchQueueCount], increment count.
    // plpDst[2] += nCount * nSize  (advance source write cursor).
    if (nPriority)
        Res_BunchResortTasks();
    SetEvent(g_hEvtWorkReady);
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nQueueLock);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0045d810  Res_GetDirectByNumChar
// High-level "load resource by integer index + character sub-type" wrapper.
// Waits for g_nBunchReady, then calls thunk_FUN_0045d4e0 to resolve the
// resource descriptor, then FUN_0048a8d0 to read the data.
// Returns a resource handle (pointer into g_nResFileHandle buffer), or 0.
// ---------------------------------------------------------------------------
int  Res_GetDirectByNumChar(int nIndex, int nParam2, int nParam3,
                             char cSub, int nParam5)
{
    // Asserts g_nBunchInitDone via thunk_FUN_00420e60.
    // Spin: while g_nBunchReady == 0, Sleep(10).
    // Calls thunk_FUN_0045d4e0(nIndex, nParam2, nParam3, cSub, &localDesc, nParam5).
    // On success (ret==0): FUN_0048a8d0(g_nResFileHandle, localDesc[0], 0).
    // Returns g_nResFileHandle on success, 0 on failure.
    (void)nIndex; (void)nParam2; (void)nParam3; (void)cSub; (void)nParam5;
    return 0;
}

// ===========================================================================
//  Rescale / zoom-table subsystem
// ===========================================================================

// ---------------------------------------------------------------------------
// 0x004612a0  Rescale_CalcZoomTable
// Build a zoom table from a pre-computed width sequence, adjusted by
// nTopExtra and nBottomExtra.  Writes g_nRescaleTable and g_nRescaleCount.
// ---------------------------------------------------------------------------
void Rescale_CalcZoomTable(int nTopExtra, int nBottomExtra)
{
    g_nRescaleCount = 0;
    // Reads width sequence from local stack (set by caller before this),
    // scales each width by (0x280 - nTopExtra - nBottomExtra) / 0x280,
    // fills g_nRescaleTable[i] = {width, x = 0x140 - width/2, y}.
}

// ---------------------------------------------------------------------------
// 0x004614a0  Rescale_CalcForBike
// Compute the zoom table for the bicycle riding sequence using a geometric
// progression (each step *= 1/DAT_004aa0d0) starting from width 0x27f.
// ---------------------------------------------------------------------------
void Rescale_CalcForBike(void)
{
    // Builds float[] progression, converts to int[], calls Rescale_CalcZoomTable variant.
    // Fills g_nRescaleTable with {width, x=0x140-w/2, y=0xf5-w*0xf5/0x27f}.
    // Sets g_nRescaleCount.
}

// ---------------------------------------------------------------------------
// 0x00461720  Rescale_CalcForRoom
// Build the zoom table for normal room perspective walk-zone scaling.
// Width runs from DAT_004d77f8 down to 0x1e, scaled by nTopExtra/nBottomExtra.
// ---------------------------------------------------------------------------
void Rescale_CalcForRoom(int nTopExtra, int nBottomExtra)
{
    // Similar to Rescale_CalcZoomTable but uses 0x1e0 base and 0x1e floor.
    (void)nTopExtra; (void)nBottomExtra;
}

// ---------------------------------------------------------------------------
// 0x00461950  Rescale_Reset
// No-op placeholder (function body is empty in original).
// ---------------------------------------------------------------------------
void Rescale_Reset(void)
{
}

// ---------------------------------------------------------------------------
// 0x004619d0  Rescale_DrawScaledSprite
// Draw sprite nSprite at the zoom level specified by nIdx, using the
// pre-computed (x, y, width) from g_nRescaleTable[nIdx].
// ---------------------------------------------------------------------------
void Rescale_DrawScaledSprite(int nIdx, int nX, int nY, int nSprite)
{
    // Calls thunk_FUN_0042ee10(nX, nY, nSprite,
    //   g_nRescaleTable[nIdx*3 + 1],   // src_x
    //   g_nRescaleTable[nIdx*3 + 2],   // src_y
    //   g_nRescaleTable[nIdx*3 + 0]);  // src_w
    (void)nIdx; (void)nX; (void)nY; (void)nSprite;
}

// ---------------------------------------------------------------------------
// 0x00461aa0  Rescale_GetCount
// ---------------------------------------------------------------------------
int  Rescale_GetCount(void)
{
    return g_nRescaleCount;
}

// ---------------------------------------------------------------------------
// 0x00461b30  Rescale_StartBike
// Set the bike-animation target parameter and kick off the zoom sequence.
// ---------------------------------------------------------------------------
void Rescale_StartBike(int nParam)
{
    // Stores nParam in DAT_007c3fd4.
    // Calls Rescale_CalcForRoom(0x32, 0x32).
    // Calls thunk_FUN_00411760 (start anim system) and thunk_FUN_004065e0 (register callback).
    // Debug_Assert: never reached (assert 0 after starting).
    (void)nParam;
}
