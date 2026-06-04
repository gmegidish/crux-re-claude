// ---------------------------------------------------------------------------
// Except.cpp — SCN (scene/room) data-file loader and its error-context helpers
//
// See Except.h for the full module note.  In short: despite the placeholder
// "Except.cpp" name, these six functions are the front of FILES.cpp
// (0x00420e50-0x00422240, just before Files_SaveState).  They are NOT C++
// exception / SEH plumbing; the FS-segment frame in the decompile is the
// MSVC __try prologue around the game's fatal-error path.
//
// Original source: C:\DevStudio\Projects\Crux\FILES.cpp
// RE address range: 0x00420e50 - 0x00422240
// Functions: 6
// ---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "Except.h"
#include "ERRORS.h"
#include "READRES.h"
#include "SAFEHEAP.h"

// ---------------------------------------------------------------------------
// External helpers (named in their own translation units)
// ---------------------------------------------------------------------------

// FILES.cpp serializer the loader hands off to once parsing succeeds.
extern void Files_SaveState(int nBuf);

// READRES.cpp: locate a resource by number/char into a bunch-file handle.
extern void Res_FindByNumChar(int nKind, char *pszName, char *pszPathOut,
                              int nFlags, int *pFileOut, int nReserved);

// READRES.cpp: read nCount*nSize bytes from the open bunch file into pDst.
// Returns 0 on success, non-zero on short read.  (0x... Res_BunchFreadNow)
extern int  Res_BunchFreadNow(int pDst, int nCount, int nSize, int *pFile);

// Forward declaration: assert-then-dispatch helper defined at end of file.
static void RaiseFatal(void *pRecOut, int nCode, char *pData);

// ERRORS.cpp helpers.
extern "C" void  FUN_00489090(void *pRec, void *pDispatchTable);  // Err_Dispatch
extern "C" void  Debug_Assert(int nLine, const char *pszFile, int nValue);

// CRT-ish string thunks used by the SCN loader.
extern "C" void  FUN_004895e0(char *pszDst, const char *pszSrc);  // strcpy
extern "C" int   FUN_0049a830(const char *psz, const char *pszPrefix); // strcmp/strncmp

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// 0x006b476c  Pointer to the current room name; NULL when no room loaded.
//             (Defined in ERRORS.cpp; used as fatal-error context.)
extern char *g_pCurrentRoom;

// 0x004caa40  Assert/error line number (consumed by Err_Fatal).
extern int   g_nAssertLine;          // DAT_004caa40

// 0x006b4770  Pointer to the basename of the source file for the current error.
extern char *g_pAssertFile;          // overlaps g_abAssertFile in ERRORS.cpp

// Compiler-emitted __LINE__ base constants (offset is added per call site).
extern int   g_nFilesLoadScnLineBase;       // 0x004cb930
extern int   g_nFilesDoReadLineBase;        // 0x004cb6bc
extern int   g_nFilesFreadStringLineBase;   // 0x004cb648

// Default dispatch table passed to Err_Dispatch (FUN_00489090).
extern int   g_ErrDispatchTable;            // 0x004ab3f8 (DAT_004ab3f8)

// Area subsystem (defined in AREAS.cpp / ONTHEFLY.cpp).
extern CRITICAL_SECTION g_AreaCritSec;      // 0x0070ae78 (DAT_0070ae78)
extern CRITICAL_SECTION g_AreaCacheCritSec; // g_nAreaCritSec
extern int   g_nAreaNodeCount;
extern int   g_nAreaCacheActive;
extern int   g_nAreaCacheCount;             // 0x007127d0
extern int   g_anAreaCacheTable[];          // 0x0070c258
extern int  *g_pAreaNodeTable;              // 0x... array of node ptrs
extern int  *g_apAreaCacheRecords;          // 0x... array of cache record ptrs
extern int   g_anAreaCacheSlots[];          // 15 slots
extern int   g_nOtfNodeListCount;           // on-the-fly node list count
extern void *g_pOtfNodeListPool;            // on-the-fly node list pool

// Per-table maxima (referenced via their own modules' globals).
extern int   g_nPaletteCount, g_nExitCount, g_nAnimCount, g_nScaScmCount,
             g_nThemeCount, g_nSoundCount;

// SCN signature compared against the file's 3-byte tag.
extern char  g_abScnSignature[];            // 0x004cb978

// Saved invalidation flag.
extern int   g_nScnReloadFlag;              // 0x007c441c

// ---------------------------------------------------------------------------
// 0x00420e50  Files_SetCurrentRoom
//
// Records the room-name pointer that the fatal-error reporter prints as the
// "current room" context line.
// ---------------------------------------------------------------------------
void Files_SetCurrentRoom(char *pszRoom)
{
    g_pCurrentRoom = pszRoom;
}

// ---------------------------------------------------------------------------
// 0x00420e60  Files_SetErrSource   (a.k.a. Err_PushFrame)
//
// Stores the source line number and a pointer to the *basename* of pszFile
// (the substring after the last backslash) so that the next fatal error
// (Err_Fatal) can report exactly where it was triggered.  Called immediately
// before every Err_SetRecord3 / Err_Dispatch call site throughout the engine.
// ---------------------------------------------------------------------------
void Files_SetErrSource(int nLine, char *pszFile)
{
    g_nAssertLine = nLine;

    // Walk back from the end of the path to the character just past the last
    // backslash, leaving g_pAssertFile pointing at the bare filename.
    size_t nLen = strlen(pszFile);
    g_pAssertFile = pszFile + (nLen - 1);
    while (pszFile < g_pAssertFile && g_pAssertFile[-1] != '\\')
        g_pAssertFile--;
}

// ---------------------------------------------------------------------------
// 0x00420ed0  Files_GetTime
//
// Returns a file's last-write FILETIME via FindFirstFileA/FindClose.
// Returns 0 and fills *pftLastWrite on success; -1 if the file was not found.
// ---------------------------------------------------------------------------
int Files_GetTime(LPCSTR pszPath, DWORD *pftLastWrite)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pszPath, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    FindClose(h);
    pftLastWrite[0] = fd.ftLastWriteTime.dwLowDateTime;
    pftLastWrite[1] = fd.ftLastWriteTime.dwHighDateTime;
    return 0;
}

// ---------------------------------------------------------------------------
// 0x004220e0  Files_DoRead
//
// Reads one length-prefixed string table from the open .SCN bunch file:
//   - a 4-byte entry count into *pnCount,
//   - then `*pnCount` strings (via Files_FreadString) into paTable[].
// If the count exceeds nMax a fatal error is raised.  Returns 0 on success,
// -1 if a string read failed.
// ---------------------------------------------------------------------------
int Files_DoRead(char *pszErrTitle, int *pnCount, int nMax,
                 int *paTable, int *pFile)
{
    char *errRec[3];

    if (Res_BunchFreadNow((int)pnCount, 1, 4, pFile) != 0)
        return -1;

    if (nMax < *pnCount) {
        Files_SetErrSource(g_nFilesDoReadLineBase + 10,
                           "C:\\DevStudio\\Projects\\Crux\\FILES.cpp");
        char **pRec = (char **)Err_SetRecord3((ERR_RECORD *)errRec, 0x1d,
                                              (int)pszErrTitle, -1);
        FUN_00489090(pRec, &g_ErrDispatchTable);
    }

    for (int i = 0; i < *pnCount; i++) {
        if (Files_FreadString(paTable + i, pFile) != 0)
            return -1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x00422240  Files_FreadString
//
// Reads a single 1-byte-length-prefixed string from the .SCN bunch file,
// allocates (len+1) bytes on the safe heap, NUL-terminates it, and applies the
// two legacy filename fix-ups: any string equal to "FLIWINS" is overwritten
// with "AATALK".  Returns 0 on success, -1 on read or alloc failure.
// ---------------------------------------------------------------------------
int Files_FreadString(int *ppszOut, int *pFile)
{
    char len[4];

    if (Res_BunchFreadNow((int)len, 1, 1, pFile) != 0)
        return -1;

    char *psz = (char *)SafeHeap_Alloc(
        (const char *)(g_nFilesFreadStringLineBase + 8),
        (int)"C:\\DevStudio\\Projects\\Crux\\FILES.cpp",
        len[0] + 1);
    *ppszOut = (int)psz;
    if (psz == NULL)
        return -1;

    if (Res_BunchFreadNow((int)psz, 1, (int)len[0], pFile) != 0)
        return -1;

    psz[(int)len[0]] = '\0';
    if (FUN_0049a830(psz, "FLIWINS") == 0)
        FUN_004895e0(psz, "AATALK");

    return 0;
}

// ---------------------------------------------------------------------------
// 0x00420fb0  Files_LoadScn
//
// Loads a compiled .SCN scene/room file into the engine's area, cache and
// on-the-fly tables.  The on-disk layout is, in order:
//   [4] format/version tag (3 chars + version byte)
//   string tables: area, palette, exit, animation, SCA/SCM, theme,
//                  sound/speech   (each: count + Files_FreadString entries)
//   [4] area-node count, then that many 0xB0-byte area-node records
//   [4] area-cache count, then that many 0x14-byte cache records
//   [0x3C] 15 x DWORD cache-slot table
//   [4] on-the-fly node-list count, then per-list: count + 0x10-byte entries
//
// Any read or allocation failure raises a fatal error through
// Err_SetRecord3 + Err_Dispatch (FUN_00489090).  On success it invalidates the
// reload flag and serialises the freshly loaded state via Files_SaveState.
// All table mutations are guarded by the area critical sections.
// ---------------------------------------------------------------------------
void Files_LoadScn(char *pszName)
{
    char  errRec[12];
    char  szPath[260];
    int   header[3];      // [0]=tag/version, [1]=read-kind, [2]=oldFormatFlag
    int  *pFile = NULL;
    int   count;
    int   nodeSize;

    static const char *const kFile =
        "C:\\DevStudio\\Projects\\Crux\\FILES.cpp";

    Files_SetErrSource(g_nFilesLoadScnLineBase, kFile);  // establish __try frame

    header[2] = 0;

    EnterCriticalSection(&g_AreaCritSec);
    g_nAreaNodeCount  = 0;
    g_nAreaCacheActive = 0;
    LeaveCriticalSection(&g_AreaCritSec);

    header[1] = 4;
    Res_FindByNumChar(4, pszName, szPath, 1, (int *)&pFile, 0);

    // -- format tag --------------------------------------------------------
    if (Res_BunchFreadNow((int)header, 1, 4, pFile) != 0)
        goto fail_read;

    if (strncmp(g_abScnSignature, (char *)header, 3) != 0 || header[0] > 2) {
        Files_SetErrSource(g_nFilesLoadScnLineBase + 0x1a, kFile);
        char **pRec = (char **)Err_SetRecord3((ERR_RECORD *)errRec, 0x1e,
                                              (int)pszName, 4);
        FUN_00489090(pRec, &g_ErrDispatchTable);
    }
    if (header[0] == 1)
        header[2] = 1;   // old-format flag (1-byte node counts)

    // -- string tables -----------------------------------------------------
    EnterCriticalSection(&g_AreaCacheCritSec);

    if (Files_DoRead("Too many strings",   &g_nAreaCacheCount, 0x3c,
                     g_anAreaCacheTable, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many palettes",  &g_nPaletteCount, 0x32,
                     (int *)0x00712190, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many exits",     &g_nExitCount, 100,
                     (int *)0x0070d560, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many animations",&g_nAnimCount, 800,
                     (int *)0x00711510, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many SCAs/SCMs", &g_nScaScmCount, 100,
                     (int *)0x0070b1b0, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many themes",    &g_nThemeCount, 100,
                     (int *)0x0070c428, pFile) != 0) goto fail_tbl;
    if (Files_DoRead("Too many sounds/speech", &g_nSoundCount, 200,
                     (int *)0x007111a8, pFile) != 0) goto fail_tbl;

    // -- area-node array ---------------------------------------------------
    if (Res_BunchFreadNow((int)&count, 1, 4, pFile) != 0) goto fail_tbl;
    if (count > 0x96) {
        Debug_Assert(g_nFilesLoadScnLineBase + 0x54, kFile, count);
        RaiseFatal(errRec, 0x17, pszName);  // too many area nodes
    }
    for (int i = 0; i < count; i++) {
        int *p = (int *)SafeHeap_Alloc((const char *)(g_nFilesLoadScnLineBase + 0x58),
                                       (int)kFile, 0xb0);
        (&g_pAreaNodeTable)[i] = p;
        if (p == NULL)
            RaiseFatal(errRec, 0, NULL);
        if (Res_BunchFreadNow((int)p, 1, 0xb0, pFile) != 0) goto fail_tbl;
    }
    EnterCriticalSection(&g_AreaCritSec);
    g_nAreaNodeCount = count;
    LeaveCriticalSection(&g_AreaCritSec);

    // -- area-cache record array ------------------------------------------
    if (Res_BunchFreadNow((int)&count, 1, 4, pFile) != 0) goto fail_tbl;
    if (count > 1000) {
        Debug_Assert(g_nFilesLoadScnLineBase + 0x6c, kFile, count);
        RaiseFatal(errRec, 0x17, NULL);
    }
    for (int i = 0; i < count; i++) {
        int p = SafeHeap_Alloc((const char *)(g_nFilesLoadScnLineBase + 0x71),
                               (int)kFile, 0x14);
        (&g_apAreaCacheRecords)[i] = (int *)p;
        if (p == 0)
            RaiseFatal(errRec, 0, NULL);
        if (Res_BunchFreadNow(p, 1, 0x14, pFile) != 0) goto fail_tbl;
    }
    EnterCriticalSection(&g_AreaCritSec);
    g_nAreaCacheActive = count;
    LeaveCriticalSection(&g_AreaCritSec);

    // -- cache slot table + on-the-fly lists -------------------------------
    if (Res_BunchFreadNow((int)g_anAreaCacheSlots, 4, 0xf, pFile) != 0) goto fail_tbl;
    if (Res_BunchFreadNow((int)&g_nOtfNodeListCount, 1, 4, pFile) != 0) goto fail_tbl;
    if (g_nOtfNodeListCount > 0x15e) {
        Debug_Assert(g_nFilesLoadScnLineBase + 0x8c, kFile, g_nOtfNodeListCount);
        RaiseFatal(errRec, 0x1f, NULL);
    }
    for (int i = 0; i < g_nOtfNodeListCount; i++) {
        if (header[2] == 0) {
            if (Res_BunchFreadNow((int)&nodeSize, 1, 4, pFile) != 0) goto fail_tbl;
        } else {
            nodeSize = 0;
            if (Res_BunchFreadNow((int)&nodeSize, 1, 1, pFile) != 0) goto fail_tbl;
        }
        void *p = (void *)SafeHeap_Alloc((const char *)(g_nFilesLoadScnLineBase + 0x9d),
                                         (int)kFile, nodeSize * 0x10 + 4);
        (&g_pOtfNodeListPool)[i] = p;
        if (p == NULL)
            RaiseFatal(errRec, 0, NULL);
        *(int *)p = nodeSize;
        if (Res_BunchFreadNow((int)((char *)p + 4), 0x10, nodeSize, pFile) != 0) goto fail_tbl;
    }

    g_nScnReloadFlag = -1;
    Files_SaveState((int)pFile);
    LeaveCriticalSection(&g_AreaCacheCritSec);
    return;

fail_read:
    Files_SetErrSource(g_nFilesLoadScnLineBase + 0x15, kFile);
    {
        char **pRec = (char **)Err_SetRecord3((ERR_RECORD *)errRec, 0x0e,
                                              (int)pszName, 4);
        FUN_00489090(pRec, &g_ErrDispatchTable);
    }
    return;

fail_tbl:
    Files_SetErrSource(g_nFilesLoadScnLineBase, kFile);
    {
        char **pRec = (char **)Err_SetRecord3((ERR_RECORD *)errRec, 0x12,
                                              (int)pszName, 4);
        FUN_00489090(pRec, &g_ErrDispatchTable);
    }
    LeaveCriticalSection(&g_AreaCacheCritSec);
}

// ---------------------------------------------------------------------------
// Local convenience used by Files_LoadScn for the assert-then-dispatch idiom.
// (Reconstructed from the repeated inline sequences in the decompile.)
// ---------------------------------------------------------------------------
static void RaiseFatal(void *pRecOut, int nCode, char *pData)
{
    char **pRec = (char **)Err_SetRecord3((ERR_RECORD *)pRecOut, nCode,
                                          (int)pData, -1);
    FUN_00489090(pRec, &g_ErrDispatchTable);
}
