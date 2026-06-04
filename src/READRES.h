// ---------------------------------------------------------------------------
// READRES.h  —  Resource "Bunch" loader public interface
// Original: C:\DevStudio\Projects\Crux\READRES.cpp
// RE offsets: 0x0045d810 – 0x00461b30
// ---------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Resource bunch system — init / shutdown
// ---------------------------------------------------------------------------

// 0x0045f200  Initialise the whole bunch system:
//   opens the IDX+RES archive, measures CD transfer rate, spawns reading thread.
//   Returns 0 on success, -1 on failure.
int  Res_BunchInit(const char *pszBasePath);

// 0x0045ec10  Open (or re-open after disk change) the IDX/RES file pair for
//   the given path stem.  Reads the IDX into the g_nResNames[] table.
//   Returns 0 on success, -1 on failure.
int  Res_BunchOpen(const char *pszBasePath);

// 0x00460630  Close the MMIO RES handle, free name table entries.
void Res_BunchClose(void);

// 0x00460190  Kill background thread, close all event handles + MMIO handle.
void Res_BunchShutdown(void);

// ---------------------------------------------------------------------------
// Disk management
// ---------------------------------------------------------------------------

// 0x0045fd10  Identify the disc currently in the drive by reading DISK_*.
//   Returns disk number, or -1 if not detectable.
int  Res_BunchIdDisk(void);

// 0x0045fef0  Load the room list for disk <nDiskNum> (DISK_n.LST / ROOMS.LST).
//   Returns 0 on success, -1 if file not found.
int  Res_BunchInitDisk(int nDiskNum);

// 0x00460af0  Return non-zero if the room named <pszRoom> lives on the
//   currently mounted disk's room list.
int  Res_IsRoomOnCurrentDisk(const char *pszRoom);

// 0x00460770  Handle a room-transition that requires a disk swap.
//   Stops music, prompts user to insert the correct disc, then re-opens RES.
void Res_BunchSelectDisk(const char *pszRoom);

// 0x00460bd0  Blocking "please insert disk N" prompt loop — waits until the
//   correct .RES file is accessible.
void Res_BunchReplaceDisk(const char *pszRoom);

// 0x0045eb80  Return g_nCurrentDiskNum.
int  Res_GetCurrentDiskNum(void);

// ---------------------------------------------------------------------------
// Synchronous / asynchronous read interface
// ---------------------------------------------------------------------------

// 0x0045d810  Read a resource directly by integer index + character sub-code;
//   returns an allocated handle (or 0 on failure).
//   Original debug name: "bunch_get_direct_int_num_char"
int  Res_GetDirectByNumChar(int nIndex, int nParam2, int nParam3,
                             char cSub, int nParam5);

// 0x0045db60  Schedule a "load-pointer" async read task.
//   plpDst  = [ptr_to_buf, ?, write_offset, disk_id]
//   Returns 0.
int  Res_BunchFreadLoadPtr(int nBufHandle, int nCount, int nSize,
                            int *plpDst, int nFrameOffset, int nPriority);

// 0x0045e0f0  Schedule a streaming async read task (splits into 50000-byte chunks).
int  Res_BunchFreadStreamLoadPtr(int *pCtrl, int nCount, int nSize,
                                  int *plpDst, int nStartFrame, int nEndFrame);

// 0x0045e550  Schedule a read and block until it completes (synchronous wrapper).
//   Original debug name: "bunch_fread_now"
int  Res_BunchFreadNow(int nPtr, int nCount, int nSize, int *plpDst);

// 0x0045e6f0  Block until the entry pointed to by pCtrl is marked done.
int  Res_WaitForEntry(int *pCtrl);

// 0x0045f6f0  Perform a seek+read directly on g_hMmioResFile under the file lock.
void Res_MmioSeekRead(char *pDst, int nCount, int nSize, int nFileOffset);

// ---------------------------------------------------------------------------
// Task queue management
// ---------------------------------------------------------------------------

// 0x0045de60  Compact the task queue: remove entries with offset == -1.
void Res_BunchCompactQueue(void);

// 0x0045df60  Re-sort the task queue: move the most-urgent streaming task to
//   the front for the next read.
void Res_BunchResortTasks(void);

// 0x0045f800  Sort tasks: pick the best next task (streaming-aware priority).
void Res_BunchSortTasks(void);

// 0x0045e880  Mark all tasks belonging to frame <nFrameId> as cancelled (-1).
void Res_CancelFrameTasks(int nFrameId);

// 0x0045e970  Re-target all tasks for frame <nFrameId> to a new destination.
void Res_RetargetFrameTasks(int nFrameId, int nNewDest);

// 0x00460a20  Spin (Sleep 10ms) until the task queue is empty.
void Res_WaitQueueEmpty(void);

// ---------------------------------------------------------------------------
// File-level lock helpers (thin CRITICAL_SECTION wrappers)
// ---------------------------------------------------------------------------

// 0x0045ea60  EnterCriticalSection(g_csFileLock)
void Res_AcquireFileLock(void);

// 0x0045eaf0  LeaveCriticalSection(g_csFileLock)
void Res_ReleaseFileLock(void);

// ---------------------------------------------------------------------------
// Entry spin-lock helpers
// ---------------------------------------------------------------------------

// 0x0045d970  Spin until entry[+8] lock byte == 0, then acquire it.
void Res_LockEntry(int *pEntry);

// 0x0045da20  Release entry[+8] lock byte and signal g_hEvtLockReleased.
void Res_UnlockEntry(int *pEntry);

// ---------------------------------------------------------------------------
// Budget / rate helpers
// ---------------------------------------------------------------------------

// 0x0045dac0  Return non-zero if (nBytesReq <= nFrames * g_nTransferRate).
bool Res_IsWithinBudget(int nBytesReq, int nFrames);

// 0x0045fa80  Measure CD-ROM transfer rate and store in g_nTransferRate.
void Res_TestDiskSpeeds(void);

// 0x00461210  Return g_nTransferRate (bytes per frame).
int  Res_GetTransferRate(void);

// ---------------------------------------------------------------------------
// Frame counter
// ---------------------------------------------------------------------------

// 0x0045e4c0  Increment g_nCurrentFrame (called once per game frame).
void Res_TickFrameCounter(void);

// 0x004602c0  Assert that g_nCurrentFrame >= nMin (debug check).
void Res_AssertFramePos(int nMin);

// ---------------------------------------------------------------------------
// Bulk-extract helper
// ---------------------------------------------------------------------------

// 0x00460360  Extract all resources of type <nType> from the IDX table to
//   individual files on disk.
void Res_BunchExtract(int nType);

// ---------------------------------------------------------------------------
// Background reading thread
// ---------------------------------------------------------------------------

// 0x0045f470  Thread proc for the async bunch-reading thread.
//   Wakes on g_hEvtWorkReady, sorts/reads one task, signals g_hEvtReadDone.
DWORD WINAPI Res_BunchReadingThread(LPVOID pParam);

// ---------------------------------------------------------------------------
// File-pointer advance helper
// ---------------------------------------------------------------------------

// 0x0045e7b0  Add nBytes to pEntry[+8] (advance the internal write cursor).
void Res_AdvanceFilePtr(int *pEntry, int nBytes);

// ---------------------------------------------------------------------------
// Rescale / zoom-table subsystem
//   (tail of READRES.cpp, but thematically separate from the bunch loader)
// ---------------------------------------------------------------------------

// 0x004612a0  Build a linear zoom table for a generic rectangular scene.
void Rescale_CalcZoomTable(int nTopExtra, int nBottomExtra);

// 0x004614a0  Build the zoom table for the bicycle riding sequence.
void Rescale_CalcForBike(void);

// 0x00461720  Build the zoom table for normal room perspective scaling.
void Rescale_CalcForRoom(int nTopExtra, int nBottomExtra);

// 0x00461950  Reset / no-op placeholder.
void Rescale_Reset(void);

// 0x004619d0  Draw sprite <nIdx> using the pre-computed rescale table entry.
void Rescale_DrawScaledSprite(int nIdx, int nX, int nY, int nSprite);

// 0x00461aa0  Return the number of zoom levels in the current rescale table.
int  Rescale_GetCount(void);

// 0x00461b30  Start the bicycle riding rescale animation sequence.
void Rescale_StartBike(int nParam);
