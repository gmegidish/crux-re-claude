// ---------------------------------------------------------------------------
// ERRORS.cpp  —  Error reporting, debug tracing, and fatal dialog subsystem
//
// This module owns every user-visible error path in CRUX.EXE:
//
//   1. String table
//      ERRORS.TXT is a plain-text file of up to 24 lines.  Err_LoadStrings
//      reads it at startup into g_pErrStrings[].  These strings are the
//      localised titles and body copy for all error dialogs, e.g.
//        [0]  "Error"                  — dialog title
//        [1]  "To get technical support..." — user instructions
//        [2]  "Technical info:"         — detail-box prefix
//        [5]  "Out of memory"
//        [7]  "File not found"
//        etc.
//
//   2. Fatal error path
//      Every non-recoverable error calls Err_Fatal(title, detail, buttons, nShowLine).
//      Err_Fatal:
//        - formats a "Line N at file.cpp" header if nShowLine != 0
//        - prepends the current room name if g_pCurrentRoom is set
//        - appends every string on g_apCallStack[0..g_nCallStackDepth-1]
//        - writes the full report to %GAMEDIR%\ERROR_LOG
//        - shows a MessageBox with title from g_pErrStrings[1]
//        - if the game is running (thunk_FUN_00410190 != 0), offers to
//          restart via a second YES/NO MessageBox
//
//   3. Error records
//      Higher-level code creates ERR_RECORD structs with Err_SetRecord2 /
//      Err_SetRecord3, then passes the pointer to Err_Dispatch.
//      Err_Dispatch switches on the error code to compose human-readable
//      title and detail strings, then calls Err_Fatal.
//
//   4. Call-stack context
//      Modules push context strings with Err_PushStack; Err_ClearStack /
//      Err_SetRecord reset the depth.  Err_Fatal appends them to the report.
//
//   5. Debug helpers
//      Debug_Assert  — called at every internal assert site.  Passes
//                       (nLine, szFile, assertMsg) to the inner handler which
//                       shows a fatal dialog and exits.
//      Debug_TraceVal — conditional MessageBox; suppressed when g_nReleaseMode != 0.
//
// Original source: C:\DevStudio\Projects\Crux\ERRORS.cpp
// RE offsets:      0x0041f120 – 0x00420bd0
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ERRORS.h"

// ---------------------------------------------------------------------------
// External helpers
// ---------------------------------------------------------------------------
extern "C" {
    // Game-internal string helpers
    int  FUN_0048a060(char *pBuf, const char *pFmt, ...);     // sprintf
    void FUN_004895e0(char *pDst, const void *pSrc);          // strcpy
    void FUN_004895f0(char *pDst, const void *pSrc);          // strcat
    int  FUN_0048a6a0(char *pBuf, const char *pFmt, void *pVa); // vsprintf

    // File helpers
    void *FUN_0048a340(const char *pszPath, const char *pszMode); // fopen
    void  FUN_0048a0d0(void *hFile);                               // fclose
    int   FUN_0048a790(char *pBuf, int nSize, void *hFile);        // fgets
    int   FUN_0048a820(void *pBuf);                                // _strtime or date

    // Heap
    void *thunk_FUN_0046bcc0(int nLine, const char *pszFile, int nSize); // malloc

    // CRT-style assert/abort used internally for corrupted ERRORS.TXT
    void  thunk_FUN_00483490(int nLine, const char *pszFile);   // abort-assert

    // Game-state query — non-zero if game world is active
    int   thunk_FUN_00410190(void);

    // Formatting
    int   FUN_0048a360(void *pBuf);  // _strdate or timestamp
}

// ---------------------------------------------------------------------------
// Forward declarations (internal)
// ---------------------------------------------------------------------------
static void Err_FatalMsg(int nLine, const char *pszFile, const char *pszMsg);

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// 0x006b0e70  Debug level flags (read from [General] DebugLevel in CRUX.INI,
//             shifted left 20).  OR'd into MessageBox uType on all error boxes.
int  g_nDebugFlags = 0;

// 0x006b0e10  Array of 24 localised string pointers loaded from ERRORS.TXT.
//             Layout (by index):
//               [0]  "To get technical support, please..."
//               [1]  "Error"             — dialog title
//               [2]  "Technical info:"   — detail prefix
//               [3]  "<first ERRORS.TXT data line>"
//               ... up to index 23
// (The array lives at 0x006b0e10 as 24 consecutive char* words.)
char *g_pErrStrings[24];

// 0x006b0e2c  Resource base name string (e.g. "ADVENT") used in error messages.
char *g_pResBaseName;

// 0x006b0e30  Resource path prefix used in error messages.
char *g_pResPath;

// 0x006b0e34  "Bad resource entry" error string (ERRORS.TXT slot).
char *g_pErrBadResEntry;

// 0x006b0e38  Fallback error string used by Err_OutOfMemory.
char *g_pErrOutOfMem;

// 0x006b0e3c  Text for Err_ShowDialog body.
char *g_pErrDialogBody;

// 0x006b0e40  Error string for Err_BadSCN.
char *g_pErrBadSCN;

// 0x006b0e44  Drive-error default string (used when drive state == -1).
char *g_pErrDriveDefault;

// 0x006b0e48  Error string for Err_BadMOV.
char *g_pErrBadMOV;

// 0x006b0e4c  YES/NO dialog body text (Err_AskDialog).
char *g_pAskDialogBody;

// 0x006b0e50  YES/NO dialog title (Err_AskDialog).
char *g_pAskDialogTitle;

// 0x006b0e54  Drive-state-1 string A.
char *g_pErrDrive1A;

// 0x006b0e58  Drive-state-1 string B.
char *g_pErrDrive1B;

// 0x006b0e5c  Drive-state-2 string A.
char *g_pErrDrive2A;

// 0x006b0e60  Drive-state-2 string B.
char *g_pErrDrive2B;

// 0x006b0e64  Error string for Err_BadSound.
char *g_pErrBadSound;

// 0x006b0e74  Assert-failure message string ("Assertion failed" or similar).
char *g_pAssertMsg;

// 0x006b0e1c  Error string for Err_BadResFile.
char *g_pErrBadResFile;

// 0x006b0e20  Error string for Err_BadIdxFile.
char *g_pErrBadIdxFile;

// 0x006b0e24  Error string for Err_BadVersion.
char *g_pErrBadVersion;

// 0x006b0e28  Error string for Err_BadSP.
char *g_pErrBadSP;

// 0x006b4768  Current depth of the Err_PushStack call-context stack.
int  g_nCallStackDepth = 0;

// 0x006b476c  Pointer to the current room name; NULL when no room loaded.
char *g_pCurrentRoom = NULL;

// 0x006b4770  Source filename buffer used in assert call sites.
char g_abAssertFile[260];

// 0x006b4778  Buffer head for call-stack context strings.
char g_abCallStackBuf[200];

// 0x006b10b8  Array of context string pointers (up to 50 entries).
char *g_apCallStack[50];

// 0x006b0f88  "Line N at file.cpp" formatted prefix for error reports.
char g_nErrLineAtFile[256];

// 0x006b2058  Full formatted error detail buffer written to ERROR_LOG / MessageBox.
char g_nErrDetail[4096];

// 0x006b8f10  Main window HWND; parent for all error MessageBoxes.
HWND g_nHwndMain = NULL;

// 0x004ca984  Non-zero in release builds: suppresses Debug_TraceVal output.
extern int g_nReleaseMode;

// 0x006299c0  Path to CRUX.INI (set by main startup code).
extern char DAT_006299c0[];

// 0x007d66b0  Game installation directory (base path).
extern char DAT_007d66b0[];

// 0x007d6468  Game base path string (used for restart CreateProcess).
extern char DAT_007d6468[];

// 0x006b0d08  Module filename buffer (for restart).
extern char DAT_006b0d08[];

// 0x006b0920  Command line buffer (for restart).
extern char DAT_006b0920[];

// 0x007d6af0  Game HINSTANCE (used to get module filename).
extern HINSTANCE DAT_007d6af0;

// 0x004caa40  Assert line number (-1 if not in an assert context).
extern int DAT_004caa40;

// 0x007d6248  Game installation directory (base path, variant).
extern char DAT_007d6248[];

// ---------------------------------------------------------------------------
// 0x0041f120  Err_ReadDebugLevel
// Read [General] DebugLevel from CRUX.INI. The value is shifted left 20 bits
// and stored in g_nDebugFlags, which is OR'd into every MessageBox uType call.
// ---------------------------------------------------------------------------
void Err_ReadDebugLevel(void)
{
    UINT nLevel = GetPrivateProfileIntA("General", "DebugLevel", 0,
                                        (LPCSTR)DAT_006299c0);
    g_nDebugFlags = (int)(nLevel << 20);
}

// ---------------------------------------------------------------------------
// 0x0041f160  Err_LoadStrings
// Open ERRORS.TXT from the game directory, read up to 24 lines (stripping
// trailing newlines), and store pointers in g_pErrStrings[].  Each string is
// heap-allocated via thunk_FUN_0046bcc0.  Pre-seeds the first three slots from
// hardcoded game strings before reading the file.
// ---------------------------------------------------------------------------
void Err_LoadStrings(void)
{
    char szLine[200];
    char szPath[260];
    int  i;

    // Clear the whole table first
    for (i = 0; i < 24; i++)
        g_pErrStrings[i] = 0;

    // Seed the first three slots from embedded localised strings
    g_pErrStrings[0] = (char *)thunk_FUN_0046bcc0(10,
                         "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP", 200);
    FUN_004895e0(g_pErrStrings[0], "To get technical support, please");

    g_pErrStrings[1] = (char *)thunk_FUN_0046bcc0(13,
                         "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP", 200);
    FUN_004895e0(g_pErrStrings[1], "Error:");

    g_pErrStrings[2] = (char *)thunk_FUN_0046bcc0(15,
                         "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP", 200);
    FUN_004895e0(g_pErrStrings[2], "Technical info:");

    // Build path to ERRORS.TXT in game directory
    FUN_004895e0(szPath, DAT_007d66b0);
    FUN_004895f0(szPath, "ERRORS.TXT");

    void *hFile = FUN_0048a340(szPath, "r");
    if (!hFile)
    {
        // ERRORS.TXT missing or corrupt — hard abort
        thunk_FUN_00483490(20, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP");
    }

    i = 0;
    while (i < 24 && FUN_0048a790(szLine, 200, hFile) != 0)
    {
        // Strip trailing newlines
        size_t nLen;
        while ((nLen = strlen(szLine)) > 0 && szLine[nLen - 1] == '\n')
            szLine[nLen - 1] = '\0';

        nLen = strlen(szLine);

        // Allocate slot if not already seeded (first 3 are pre-allocated above)
        if (!g_pErrStrings[i])
        {
            g_pErrStrings[i] = (char *)thunk_FUN_0046bcc0(34,
                                  "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP",
                                  (int)(nLen + 1));
        }

        if (!g_pErrStrings[i])
        {
            // Alloc failure — hard abort
            thunk_FUN_00483490(38, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP");
        }

        FUN_004895e0(g_pErrStrings[i], szLine);
        i++;
    }

    FUN_0048a0d0(hFile);
}

// ---------------------------------------------------------------------------
// 0x0041f480  Err_Abort
// Hard abort used by internal assertion failures: calls the CRT assert then
// terminates the process unconditionally.
// ---------------------------------------------------------------------------
void Err_Abort(int nLine, const char *pszFile)
{
    thunk_FUN_00483490(nLine + 1, pszFile);
    ExitProcess(0);
}

// ---------------------------------------------------------------------------
// 0x0041f4b0  Debug_Assert
// Public assert entry called at every __FILE__/__LINE__ assert site.
// Forwards (nLine, pszFile) plus the fixed assert-message string to the
// inner thunk_FUN_0041f680 handler (Err_BadResEntry variant).
// ---------------------------------------------------------------------------
void Debug_Assert(int nLine, const char *pszFile)
{
    // thunk_FUN_0041f680 is the thunk wrapper for the fatal-msg handler.
    // It takes (nLine, pszFile, pszMsg) and calls Err_FatalMsg.
    extern void thunk_FUN_0041f680(int, const char *, const char *);
    thunk_FUN_0041f680(nLine, pszFile, g_pAssertMsg);
}

// ---------------------------------------------------------------------------
// Static helper: Err_FatalMsg
// Core fatal error display used by all specific error wrappers.
// Logs to the debug output, shows a MessageBox with the formatted message,
// prints a "Technical info:" detail box, then calls ExitProcess(0).
// (Corresponds to FUN_0041f010 at 0x0041f010 — not in our 31 but the
//  canonical implementation called by every wrapper.)
// ---------------------------------------------------------------------------
static void Err_FatalMsg(int nLine, const char *pszFile, const char *pszMsg)
{
    CHAR szDetail[1000];

    // Log the call location
    extern void Debug_Trace(int, const char *, const char *, ...);
    Debug_Trace(5, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP",
                "Error in %s %d", pszFile, nLine);

    // Abort the process state
    thunk_FUN_00483490(7, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP");

    // Format and display the message
    FUN_0048a6a0(szDetail, pszMsg, (void *)(&pszMsg + 1));
    MessageBoxA(NULL, szDetail, g_pErrStrings[1],
                (UINT)(g_nDebugFlags | 0x10010));

    // Show technical detail (Technical info: <file> <line>)
    FUN_0048a060(szDetail, "%s %s > %d", g_pErrStrings[2], pszFile, nLine);
    MessageBoxA(NULL, szDetail, "Technical info",
                (UINT)(g_nDebugFlags | 0x10010));

    ExitProcess(0);
}

// ---------------------------------------------------------------------------
// 0x0041f4e0  Err_BadResFile
// ---------------------------------------------------------------------------
void Err_BadResFile(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadResFile);
}

// ---------------------------------------------------------------------------
// 0x0041f510  Err_BadIdxFile
// ---------------------------------------------------------------------------
void Err_BadIdxFile(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadIdxFile);
}

// ---------------------------------------------------------------------------
// 0x0041f540  Err_BadVersion
// ---------------------------------------------------------------------------
void Err_BadVersion(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadVersion);
}

// ---------------------------------------------------------------------------
// 0x0041f570  Err_BadSP
// ---------------------------------------------------------------------------
void Err_BadSP(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadSP);
}

// ---------------------------------------------------------------------------
// 0x0041f5a0  Err_MissingDefaultSP
// Hardcoded error — the default save-point file is missing.
// ---------------------------------------------------------------------------
void Err_MissingDefaultSP(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, "Missing DEFAULT SP");
}

// ---------------------------------------------------------------------------
// 0x0041f5c0  Err_MissingRes
// A named resource is missing from the archive.
// pszName is the missing resource name.
// ---------------------------------------------------------------------------
void Err_MissingRes(int nLine, const char *pszFile, const char *pszName)
{
    char szMsg[1000];
    FUN_0048a060(szMsg, "%s %s", g_pResBaseName, pszName);
    Err_FatalMsg(nLine, pszFile, szMsg);
}

// ---------------------------------------------------------------------------
// 0x0041f620  Err_MissingResFile
// The ADVENT.RES file for disk nDisk is missing.
// pszRes is the expected resource filename; nDisk is the disk number.
// ---------------------------------------------------------------------------
void Err_MissingResFile(int nLine, const char *pszFile,
                         const char *pszRes, int nDisk)
{
    char szMsg[1000];
    FUN_0048a060(szMsg, "%s ADVENT.RES %s %s %d",
                 g_pResBaseName, g_pResPath, pszRes, nDisk);
    Err_FatalMsg(nLine, pszFile, szMsg);
}

// ---------------------------------------------------------------------------
// 0x0041f680  Err_BadResEntry
// A resource archive entry is corrupt or invalid.
// ---------------------------------------------------------------------------
void Err_BadResEntry(int nLine, const char *pszFile)
{
    char szMsg[1000];
    FUN_004895e0(szMsg, g_pErrBadResEntry);
    Err_FatalMsg(nLine, pszFile, szMsg);
}

// ---------------------------------------------------------------------------
// 0x0041f6d0  Err_OutOfMemory
// ---------------------------------------------------------------------------
void Err_OutOfMemory(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrOutOfMem);
}

// ---------------------------------------------------------------------------
// 0x0041f700  Err_ShowDialog
// Show a simple informational MessageBox using strings from the error table.
// ---------------------------------------------------------------------------
void Err_ShowDialog(void)
{
    MessageBoxA(NULL, g_pErrDialogBody, g_pErrStrings[1],
                (UINT)(g_nDebugFlags | 0x10001));
}

// ---------------------------------------------------------------------------
// 0x0041f740  Err_BadSCN
// ---------------------------------------------------------------------------
void Err_BadSCN(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadSCN);
}

// ---------------------------------------------------------------------------
// 0x0041f770  Err_DriveCheck
// Drive availability check failure.
//   nDriveState == -1  default drive error
//   nDriveState ==  1  drive type A with specific path
//   nDriveState ==  2  drive type B with specific path
// ---------------------------------------------------------------------------
void Err_DriveCheck(int nLine, const char *pszFile, int nDriveState)
{
    char szMsg[1000];

    if (nDriveState == -1)
    {
        Err_FatalMsg(nLine, pszFile, g_pErrDriveDefault);
    }
    else if (nDriveState == 1)
    {
        FUN_0048a060(szMsg, "%s %s", g_pErrDrive1A, g_pErrDrive1B);
        Err_FatalMsg(nLine, pszFile, szMsg);
    }
    else if (nDriveState == 2)
    {
        FUN_0048a060(szMsg, "%s %s", g_pErrDrive2A, g_pErrDrive2B);
        Err_FatalMsg(nLine, pszFile, szMsg);
    }
}

// ---------------------------------------------------------------------------
// 0x0041f870  Err_BadSound
// ---------------------------------------------------------------------------
void Err_BadSound(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadSound);
}

// ---------------------------------------------------------------------------
// 0x0041f8a0  Err_BadMOV
// ---------------------------------------------------------------------------
void Err_BadMOV(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, g_pErrBadMOV);
}

// ---------------------------------------------------------------------------
// 0x0041f8d0  Err_BadSCNVersion
// Hardcoded error string (not from ERRORS.TXT).
// ---------------------------------------------------------------------------
void Err_BadSCNVersion(int nLine, const char *pszFile)
{
    Err_FatalMsg(nLine, pszFile, "Bad SCN version");
}

// ---------------------------------------------------------------------------
// 0x0041f8f0  Err_GetString
// Return g_pErrStrings[nIdx].  Used internally to retrieve dialog strings.
// ---------------------------------------------------------------------------
const char *Err_GetString(int nIdx)
{
    return g_pErrStrings[nIdx];
}

// ---------------------------------------------------------------------------
// 0x0041f910  Err_AskDialog
// Show a YES/NO question dialog using strings from the error table.
// Returns the raw MessageBoxA return value (IDYES / IDNO).
// ---------------------------------------------------------------------------
int Err_AskDialog(void)
{
    return MessageBoxA(g_nHwndMain, g_pAskDialogBody, g_pAskDialogTitle,
                       (UINT)(g_nDebugFlags | 0x10004));
}

// ---------------------------------------------------------------------------
// 0x0041f960  Err_RestartGame
// Relaunch the game EXE with the same command line, then exit this instance.
// Used from the fatal-error dialog when the user chooses "Yes, restart".
// ---------------------------------------------------------------------------
void Err_RestartGame(void)
{
    STARTUPINFOA    si;
    PROCESS_INFORMATION pi;
    LPSTR           pszCmdLine;
    size_t          nLen;

    thunk_FUN_00483490(6, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP");

    // Capture this executable's full path
    GetModuleFileNameA(DAT_007d6af0, DAT_006b0d08, 0x104);

    // Copy the original command line
    pszCmdLine = GetCommandLineA();
    FUN_004895e0(DAT_006b0920, pszCmdLine);

    // Strip trailing backslash from working directory if present
    nLen = strlen(DAT_007d6468);
    if (DAT_007d6468[nLen] == '\\')
        DAT_007d6468[nLen] = '\0';

    // Launch new instance
    memset(&si, 0, sizeof(si));
    si.cb           = sizeof(si);
    si.wShowWindow  = 10;   // SW_SHOWDEFAULT

    CreateProcessA(
        DAT_006b0d08,           // EXE path
        DAT_006b0920,           // command line
        NULL, NULL,
        TRUE,
        0x40,                   // CREATE_NEW_CONSOLE
        NULL,
        DAT_007d6468,           // working directory
        &si, &pi);

    ExitProcess(0);
}

// ---------------------------------------------------------------------------
// 0x0041fa60  Debug_TraceVal
// Conditional trace: only active in debug builds (g_nReleaseMode == 0).
// Formats pszFmt with the following varargs and shows a MessageBox.
// ---------------------------------------------------------------------------
void Debug_TraceVal(const char *pszFmt, ...)
{
    if (g_nReleaseMode == 0)
    {
        CHAR szBuf[200];
        va_list va;
        va_start(va, pszFmt);
        FUN_0048a6a0(szBuf, pszFmt, va);
        va_end(va);
        MessageBoxA(g_nHwndMain, szBuf, NULL, 0x10010);
    }
}

// ---------------------------------------------------------------------------
// 0x0041fad0  Err_SetRecord3
// __thiscall constructor for a 3-field error record.
// Also resets g_nCallStackDepth to 0.
// ---------------------------------------------------------------------------
ERR_RECORD *Err_SetRecord3(ERR_RECORD *pRec, int nCode, int nData, int nExtra)
{
    g_nCallStackDepth = 0;
    pRec->nCode  = nCode;
    pRec->nData  = nData;
    pRec->nExtra = nExtra;
    return pRec;
}

// ---------------------------------------------------------------------------
// 0x0041fb20  Err_SetRecord2
// __thiscall constructor for a 2-field error record.
// nData is set to the address of g_abCallStackBuf (shared context buffer).
// Also resets g_nCallStackDepth to 0.
// ---------------------------------------------------------------------------
ERR_RECORD *Err_SetRecord2(ERR_RECORD *pRec, int nCode, int nExtra)
{
    g_nCallStackDepth = 0;
    pRec->nCode  = nCode;
    pRec->nData  = (int)g_abCallStackBuf;
    pRec->nExtra = nExtra;
    return pRec;
}

// ---------------------------------------------------------------------------
// 0x0041fb70  Err_Dispatch
// Dispatch a structured error record to Err_Fatal.
// Builds a short title string and a longer detail string for each known error
// code, optionally appending a supplementary line from Err_GetString.
// Falls back to a generic "uninitialized error" for code 9999.
// ---------------------------------------------------------------------------
void Err_Dispatch(ERR_RECORD *pRec)
{
    char szTitle[100];
    char szDetail[200];
    char szExtra[1000];
    const char *pszContext;

    // Initialise context buffer from the shared call-stack buf
    szExtra[0] = g_abCallStackBuf[0];
    memset(&szExtra[1], 0, sizeof(szExtra) - 1);

    FUN_004895e0(szTitle, "Exception:");

    if (pRec->nCode == 9999)
    {
        FUN_004895f0(szTitle, "Uninitialized error:");
        FUN_004895e0(szDetail, "Unitialized error.");
    }
    else if (pRec->nCode < 10000)
    {
        switch (pRec->nCode)
        {
        case ERR_OUT_OF_MEMORY:
            FUN_004895f0(szTitle, "Out of memory.");
            FUN_0048a060(szDetail, "Out of memory.");
            pszContext = Err_GetString(5);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_FILE_NOT_FOUND:
            FUN_004895f0(szTitle, "File not found.");
            FUN_0048a060(szDetail, "File %s %d not found.", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_OUT_OF_ANI_SLOTS:
            FUN_004895f0(szTitle, "Out of ani slots.");
            FUN_0048a060(szDetail, "Out of ani slots.");
            break;

        case ERR_INVALID_FRAME_COUNT:
            FUN_004895f0(szTitle, "Invalid number of frames.");
            FUN_0048a060(szDetail, "Invalid number of frames in %s %d.", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_NO_MORE_FRAMES:
            FUN_004895f0(szTitle, "No more frames.");
            FUN_0048a060(szDetail, "Out of frames in %s %d.", pRec->nData, pRec->nExtra);
            break;

        case ERR_ANI_DUMPED:
            FUN_004895f0(szTitle, "Ani was dumped.");
            FUN_0048a060(szDetail, "Ani is not found in slot.");
            break;

        case ERR_ILLEGAL_ANI_SLOT:
            FUN_004895f0(szTitle, "Illegal ani slot.");
            FUN_0048a060(szDetail, "Illegal value for ani slot: %d.", pRec->nExtra);
            break;

        case ERR_INVALID_FRAME_NUMBER:
            FUN_004895f0(szTitle, "Invalid frame number.");
            FUN_0048a060(szDetail, "Frame number %d not found in ani.", pRec->nExtra, pRec->nData);
            break;

        case ERR_FILE_TOO_SHORT:
            FUN_004895f0(szTitle, "File too short.");
            FUN_0048a060(szDetail, "File %s %d is too short.", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_TOO_MANY_SOUND_FX:
            FUN_004895f0(szTitle, "Too many sound FX.");
            FUN_0048a060(szDetail, "Too many sound FX.");
            break;

        case ERR_TOO_MANY_INDI_PALS:
            FUN_004895f0(szTitle, "Too many indi pals.");
            FUN_0048a060(szDetail, "Too many indi pals.");
            break;

        case ERR_OUT_OF_ANI_MEMORY:
            FUN_004895f0(szTitle, "Out of ani memory.");
            FUN_0048a060(szDetail, "Out of animem in %s %d.", pRec->nData, pRec->nExtra);
            break;

        case ERR_TOO_MANY_ONSCREEN:
            FUN_004895f0(szTitle, "Too many onscreen animations.");
            FUN_0048a060(szDetail, "Too many onscreen animations.");
            break;

        case ERR_BAD_FILE_HEADER:
            FUN_004895f0(szTitle, "Bad file header.");
            FUN_0048a060(szDetail, "Bad file header in %s %d.", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_INVALID_IMAGE:
            FUN_004895f0(szTitle, "Invalid image.");
            FUN_0048a060(szDetail, "Invalid image — probable memory leak.");
            break;

        case ERR_TOO_MANY_CLEANUPS:
            FUN_004895f0(szTitle, "Too many cleanups.");
            FUN_0048a060(szDetail, "Too many cleanup functions.");
            break;

        case ERR_EMPTY_CURSOR:
            FUN_004895f0(szTitle, "Empty cursor.");
            FUN_0048a060(szDetail, "Empty cursor: %s", pRec->nData);
            break;

        case ERR_CORRUPT_FILE_DATA:
            FUN_004895f0(szTitle, "Corrupt file data.");
            FUN_0048a060(szDetail, "Invalid file data in %s %d.", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_FILE_SYSTEM_CORRUPT:
            FUN_004895f0(szTitle, "File system corrupt.");
            FUN_0048a060(szDetail, "File system corrupt.");
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_ERROR_IN_FILE:
            FUN_004895f0(szTitle, "Error in file.");
            FUN_0048a060(szDetail, "Error accessing or reading %s.", pRec->nData);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_SCREEN_MODE_ERROR:
            FUN_004895f0(szTitle, "Screen mode error.");
            FUN_0048a060(szDetail, "Can't run in current screen resolution.");
            pszContext = Err_GetString(0x15);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_IMAGE_TOO_BIG_FOR_ICON:
            FUN_004895f0(szTitle, "Image too big for icon.");
            FUN_0048a060(szDetail, "Image too big to create icon.");
            break;

        case ERR_TOO_MANY_AREAS:
            FUN_004895f0(szTitle, "Too many areas.");
            FUN_0048a060(szDetail, "Too many areas.");
            break;

        case ERR_ERROR_WAITING_FOR_OBJECT:
            FUN_004895f0(szTitle, "Error waiting for object.");
            FUN_0048a060(szDetail, "Sync object not triggered.");
            break;

        case ERR_ERROR_IN_DEBUG_LOG:
            FUN_004895f0(szTitle, "Error in debug log.");
            FUN_0048a060(szDetail, "Error writing debug log.");
            break;

        case ERR_DIRECTDRAW_ERROR:
            FUN_004895f0(szTitle, "Error in directDraw.");
            FUN_0048a060(szDetail, "Error in directDraw: %s", pRec->nData);
            break;

        case ERR_ERROR_CREATING_SURFACE:
            FUN_004895f0(szTitle, "Error creating surface.");
            FUN_0048a060(szDetail, "New surface couldn't be accessed.");
            break;

        case ERR_DIRECTDRAW_INIT_ERROR:
            FUN_004895f0(szTitle, "Error in directDraw initialization.");
            FUN_0048a060(szDetail, "DirectDraw error: %s", pRec->nData);
            break;

        case ERR_TOO_MANY_OBJECTS:
            FUN_004895f0(szTitle, "Too many objects.");
            FUN_0048a060(szDetail, "Number of objects exceeds maximum.", pRec->nData);
            break;

        case ERR_SCN_VERSION_ERROR:
            FUN_004895f0(szTitle, "SCN version error.");
            FUN_0048a060(szDetail, "Bad file version in %s %d", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_TOO_MANY_SCRIPTS:
            FUN_004895f0(szTitle, "Too many scripts.");
            FUN_0048a060(szDetail, "Too many scripts.");
            break;

        case ERR_MOV_VERSION_ERROR:
            FUN_004895f0(szTitle, "MOV version error.");
            FUN_0048a060(szDetail, "Bad file version in %s %d", pRec->nData, pRec->nExtra);
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_INVENTORY_ERROR:
            FUN_004895f0(szTitle, "Inventory error.");
            FUN_0048a060(szDetail, "Inventory files aren't synchronized.");
            pszContext = Err_GetString(7);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_INSUFFICIENT_DISK_SPACE:
            FUN_004895f0(szTitle, "Insufficient disk space.");
            FUN_0048a060(szDetail, "Insufficient disk space.");
            pszContext = Err_GetString(3);
            FUN_004895e0(szExtra, pszContext);
            break;

        case ERR_TOO_MUCH_DATA:
            FUN_004895f0(szTitle, "Too much data.");
            FUN_0048a060(szDetail, "Too much data for buffer: %s", pRec->nData);
            break;

        case ERR_INV_MEM_ERROR:
            FUN_004895f0(szTitle, "Inv mem error.");
            FUN_0048a060(szDetail, "Inventory mem is corrupt.");
            break;

        case ERR_DIRECTSOUND_ERROR:
            FUN_004895f0(szTitle, "DirectSound error.");
            FUN_0048a060(szDetail, "DirectSound error: %s", pRec->nData);
            break;

        case ERR_DIRECTSOUND_INIT_ERROR:
            FUN_004895f0(szTitle, "DirectSound init error.");
            FUN_0048a060(szDetail, "Error initializing directSound: %s", pRec->nData);
            if (pRec->nExtra == 1)
            {
                const char *pA = Err_GetString(0x12);
                pszContext = Err_GetString(0x11, (const char *)pA);
                FUN_0048a060(szExtra, "%s", pszContext);
            }
            else if (pRec->nExtra == 2)
            {
                const char *pA = Err_GetString(0x14);
                pszContext = Err_GetString(0x13, (const char *)pA);
                FUN_0048a060(szExtra, "%s", pszContext);
            }
            break;

        case ERR_TOO_MANY_SCRIPTS_FOR_AREA:
            FUN_004895f0(szTitle, "Too many scripts for area.");
            FUN_0048a060(szDetail, "Too many scripts while modifying area.");
            break;

        case ERR_STACK_OVER_UNDERFLOW:
            FUN_004895f0(szTitle, "Stack over/underflow.");
            FUN_0048a060(szDetail, "Stack over/underflow.");
            break;

        case ERR_THEME_SEGMENT_TOO_BIG:
            FUN_004895f0(szTitle, "Theme segment too big.");
            FUN_0048a060(szDetail, "Musicmem should be 3x largest theme segment.");
            break;

        default:
            break;
        }
    }

    // If no extra context string was set, fall back to generic contact string
    if (szExtra[0] == '\0')
    {
        pszContext = Err_GetString(9);
        FUN_004895e0(szExtra, pszContext);
    }

    Err_Fatal(szTitle, szDetail, szExtra, 1);
}

// ---------------------------------------------------------------------------
// 0x00420a10  Err_GetSeverity
// Return a severity class for error dispatching / UI colouring.
// ---------------------------------------------------------------------------
int Err_GetSeverity(ERR_RECORD *pRec)
{
    switch (pRec->nCode)
    {
    case ERR_OUT_OF_MEMORY:
    case ERR_OUT_OF_ANI_MEMORY:
        return ERR_SEV_MEMORY;

    case ERR_FILE_NOT_FOUND:
    case ERR_INVALID_FRAME_COUNT:
    case ERR_FILE_TOO_SHORT:
    case ERR_BAD_FILE_HEADER:
    case ERR_EMPTY_CURSOR:
    case ERR_CORRUPT_FILE_DATA:
    case ERR_SCN_VERSION_ERROR:
    case ERR_MOV_VERSION_ERROR:
    case ERR_INVENTORY_ERROR:
        return ERR_SEV_FILE_VERSION;

    case ERR_OUT_OF_ANI_SLOTS:
    case ERR_NO_MORE_FRAMES:
    case ERR_TOO_MANY_SOUND_FX:
    case ERR_TOO_MANY_ONSCREEN:
        return ERR_SEV_SLOTS;

    case ERR_ANI_DUMPED:
    case ERR_ILLEGAL_ANI_SLOT:
    case ERR_INVALID_FRAME_NUMBER:
    case ERR_TOO_MANY_INDI_PALS:
    case ERR_TOO_MANY_CLEANUPS:
    case ERR_IMAGE_TOO_BIG_FOR_ICON:
    case ERR_TOO_MANY_AREAS:
    case ERR_TOO_MANY_OBJECTS:
    case ERR_TOO_MANY_SCRIPTS:
    case ERR_TOO_MUCH_DATA:
    case ERR_INV_MEM_ERROR:
    case ERR_TOO_MANY_SCRIPTS_FOR_AREA:
    case ERR_STACK_OVER_UNDERFLOW:
    case ERR_THEME_SEGMENT_TOO_BIG:
        return ERR_SEV_INTERNAL;

    case ERR_INVALID_IMAGE:
        return ERR_SEV_IMAGE;

    case ERR_FILE_SYSTEM_CORRUPT:
    case ERR_ERROR_IN_FILE:
    case ERR_ERROR_IN_DEBUG_LOG:
    case ERR_INSUFFICIENT_DISK_SPACE:
        return ERR_SEV_DISK;

    case ERR_SCREEN_MODE_ERROR:
    case ERR_ERROR_WAITING_FOR_OBJECT:
    case ERR_DIRECTDRAW_ERROR:
    case ERR_ERROR_CREATING_SURFACE:
    case ERR_DIRECTDRAW_INIT_ERROR:
    case ERR_DIRECTSOUND_ERROR:
    case ERR_DIRECTSOUND_INIT_ERROR:
        return ERR_SEV_HARDWARE;

    default:
        return ERR_SEV_UNKNOWN;
    }
}

// ---------------------------------------------------------------------------
// 0x00420b50  Err_ClearStack
// Reset the call-context stack depth to 0.
// ---------------------------------------------------------------------------
void Err_ClearStack(void)
{
    g_nCallStackDepth = 0;
}

// ---------------------------------------------------------------------------
// 0x00420b70  Err_PushStack
// Append a context string pointer to g_apCallStack[g_nCallStackDepth++].
// These strings are appended to the error report in Err_Fatal.
// ---------------------------------------------------------------------------
void Err_PushStack(const char *pszContext)
{
    g_apCallStack[g_nCallStackDepth] = (char *)pszContext;
    g_nCallStackDepth++;
}

// ---------------------------------------------------------------------------
// 0x00420ba0  Err_UnhandledException
// Called from the Win32 SEH unhandled-exception filter.
// Displays a generic "An unhandled exception has occurred" fatal dialog.
// ---------------------------------------------------------------------------
void Err_UnhandledException(void)
{
    const char *pszButtons = Err_GetString(9);
    Err_Fatal("Unhandled Exception:",
              "An unhandled exception has occurred.",
              pszButtons,
              0);
}

// ---------------------------------------------------------------------------
// 0x00420bd0  Err_Fatal
// Write a full error report to ERROR_LOG, show a fatal MessageBox, and
// optionally offer to restart the game.
//
// Parameters:
//   pszTitle   — short title shown in the first MessageBox
//   pszDetail  — one-line detail description
//   pszButtons — supplementary contact/action string shown below detail
//   nShowLine  — if non-zero and DAT_004caa40 != -1, prepend
//                "Line N at file.cpp" from g_abAssertFile
// ---------------------------------------------------------------------------
void Err_Fatal(const char *pszTitle, const char *pszDetail,
               const char *pszButtons, int nShowLine)
{
    char      szDate[4];
    char      szLogPath[260];
    FILE     *pLogFile;
    int       i;
    UINT      uType;
    const char *pszMsgTitle;
    const char *pszMsgText;

    // Capture current date/time for the log header
    FUN_0048a360(szDate);

    // Build "Line N at file.cpp" prefix if requested
    if (nShowLine && DAT_004caa40 != -1)
        FUN_0048a060(g_nErrLineAtFile, "Line %d at %s", DAT_004caa40, g_abAssertFile);

    // Compose full detail: optional room context + detail + call stack
    if (!g_pCurrentRoom)
    {
        FUN_0048a060(g_nErrDetail, "%s%s\nCall Stack:\n",
                     g_nErrLineAtFile, pszDetail);
    }
    else
    {
        FUN_0048a060(g_nErrDetail, "%s%s\nCurrent room is: %s\nCall Stack:\n",
                     g_nErrLineAtFile, pszDetail, g_pCurrentRoom);
    }

    // Append each call-stack context string
    for (i = 0; i < g_nCallStackDepth; i++)
    {
        FUN_004895f0(g_nErrDetail, g_apCallStack[i]);
        FUN_004895f0(g_nErrDetail, "\n");
    }

    // Write to ERROR_LOG in the game directory
    FUN_0048a060(szLogPath, "%sERROR_LOG", DAT_007d6248);
    pLogFile = (FILE *)FUN_0048a340(szLogPath, "at");   // append+text
    if (pLogFile)
    {
        FUN_0048a060(szLogPath, "============================");  // reuse szLogPath as temp
        extern void FID_conflict__fwprintf(FILE *, const wchar_t *, ...);
        FID_conflict__fwprintf(pLogFile, (wchar_t *)"================================\n");
        int nTime = FUN_0048a820(szDate);
        FID_conflict__fwprintf(pLogFile, (wchar_t *)"Error report for %s\n", nTime);
        FID_conflict__fwprintf(pLogFile, (wchar_t *)"================================\n");
        FID_conflict__fwprintf(pLogFile, (wchar_t *)g_nErrDetail);
        FUN_0048a0d0(pLogFile);
    }

    // Assert that we've entered fatal territory
    thunk_FUN_00483490(0x24 + 0, "C:\\DevStudio\\Projects\\Crux\\ERRORS.CPP");

    // Show the fatal message box
    uType      = 0x10010;   // MB_ICONERROR | MB_OK
    pszMsgTitle = Err_GetString(1);
    MessageBoxA(NULL, pszButtons, pszMsgTitle, uType);

    // If game is running, offer to restart
    if (thunk_FUN_00410190())
    {
        const char *pszRestartText  = Err_GetString(0x17);
        const char *pszRestartTitle = Err_GetString(0x16);
        MessageBoxA(NULL, pszRestartText, pszRestartTitle, 0x10000);
    }
}
