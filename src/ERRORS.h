// ---------------------------------------------------------------------------
// ERRORS.h  —  Error reporting, debug tracing, and fatal dialog subsystem
//
// Original source: C:\DevStudio\Projects\Crux\ERRORS.cpp
// RE offsets:      0x0041f120 – 0x00420bd0
// ---------------------------------------------------------------------------
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Error record type — passed by pointer to Err_SetRecord2 / Err_SetRecord3.
// Three-field structure: [code, data_ptr_or_name, extra_int].
// ---------------------------------------------------------------------------
typedef struct ERR_RECORD {
    int  nCode;     // error code (enum ErrCode)
    int  nData;     // context value 1 (filename pointer or count)
    int  nExtra;    // context value 2 (line number or size)
} ERR_RECORD;

// ---------------------------------------------------------------------------
// Error codes used in Err_Dispatch / Err_GetSeverity.
// Values match the switch cases in FUN_0041fb70 / FUN_00420a10.
// ---------------------------------------------------------------------------
enum ErrCode {
    ERR_OUT_OF_MEMORY            = 0,
    ERR_FILE_NOT_FOUND           = 1,
    ERR_OUT_OF_ANI_SLOTS         = 3,
    ERR_INVALID_FRAME_COUNT      = 4,
    ERR_NO_MORE_FRAMES           = 5,
    ERR_ANI_DUMPED               = 6,
    ERR_ILLEGAL_ANI_SLOT         = 7,
    ERR_INVALID_FRAME_NUMBER     = 8,
    ERR_FILE_TOO_SHORT           = 9,
    ERR_TOO_MANY_SOUND_FX        = 10,
    ERR_TOO_MANY_INDI_PALS       = 11,
    ERR_OUT_OF_ANI_MEMORY        = 12,
    ERR_TOO_MANY_ONSCREEN        = 13,
    ERR_BAD_FILE_HEADER          = 14,
    ERR_INVALID_IMAGE            = 15,
    ERR_TOO_MANY_CLEANUPS        = 16,
    ERR_EMPTY_CURSOR             = 17,
    ERR_CORRUPT_FILE_DATA        = 18,
    ERR_FILE_SYSTEM_CORRUPT      = 19,
    ERR_ERROR_IN_FILE            = 20,
    ERR_SCREEN_MODE_ERROR        = 21,
    ERR_IMAGE_TOO_BIG_FOR_ICON   = 22,
    ERR_TOO_MANY_AREAS           = 23,
    ERR_ERROR_WAITING_FOR_OBJECT = 24,
    ERR_ERROR_IN_DEBUG_LOG       = 25,
    ERR_DIRECTDRAW_ERROR         = 26,
    ERR_ERROR_CREATING_SURFACE   = 27,
    ERR_DIRECTDRAW_INIT_ERROR    = 28,
    ERR_TOO_MANY_OBJECTS         = 29,
    ERR_SCN_VERSION_ERROR        = 30,
    ERR_TOO_MANY_SCRIPTS         = 31,
    ERR_MOV_VERSION_ERROR        = 32,
    ERR_INVENTORY_ERROR          = 33,
    ERR_INSUFFICIENT_DISK_SPACE  = 34,
    ERR_TOO_MUCH_DATA            = 35,
    ERR_INV_MEM_ERROR            = 36,
    ERR_DIRECTSOUND_ERROR        = 37,
    ERR_DIRECTSOUND_INIT_ERROR   = 38,
    ERR_TOO_MANY_SCRIPTS_FOR_AREA= 39,
    ERR_STACK_OVER_UNDERFLOW     = 40,
    ERR_THEME_SEGMENT_TOO_BIG    = 41,
    ERR_UNINITIALIZED            = 9999,
};

// ---------------------------------------------------------------------------
// Severity classes returned by Err_GetSeverity.
// ---------------------------------------------------------------------------
#define ERR_SEV_MEMORY          0   // out-of-memory / ani-memory
#define ERR_SEV_FILE_VERSION    1   // file-not-found, bad version, inventory
#define ERR_SEV_SLOTS           2   // out of slots / too many
#define ERR_SEV_INTERNAL        4   // illegal values, stack issues, etc.
#define ERR_SEV_IMAGE           5   // invalid image
#define ERR_SEV_DISK            6   // disk/log/space errors
#define ERR_SEV_HARDWARE        7   // DirectDraw, DirectSound, screen mode
#define ERR_SEV_UNKNOWN         -1  // unrecognized code

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

// Read [General] DebugLevel from CRUX.INI; store as flags in g_nDebugFlags.
// 0x0041f120
void Err_ReadDebugLevel(void);

// Read ERRORS.TXT from the game directory into g_pErrStrings[0..23].
// 0x0041f160
void Err_LoadStrings(void);

// ---------------------------------------------------------------------------
// Error string table access
// ---------------------------------------------------------------------------

// Return g_pErrStrings[nIdx].  Used to retrieve dialog title / body strings.
// 0x0041f8f0
const char *Err_GetString(int nIdx);

// ---------------------------------------------------------------------------
// Error record constructors (C++ __thiscall wrappers)
// ---------------------------------------------------------------------------

// Initialise a 3-field error record and reset the call stack depth.
// 0x0041fad0
ERR_RECORD *Err_SetRecord3(ERR_RECORD *pRec, int nCode, int nData, int nExtra);

// Initialise a 2-field error record (nData = &g_abCallStackBuf) and reset depth.
// 0x0041fb20
ERR_RECORD *Err_SetRecord2(ERR_RECORD *pRec, int nCode, int nExtra);

// ---------------------------------------------------------------------------
// Call-stack context tracking
// ---------------------------------------------------------------------------

// Reset the call-stack depth to 0.
// 0x00420b50
void Err_ClearStack(void);

// Push a context string pointer onto g_apCallStack[g_nCallStackDepth++].
// 0x00420b70
void Err_PushStack(const char *pszContext);

// ---------------------------------------------------------------------------
// Error dispatching and fatal dialogs
// ---------------------------------------------------------------------------

// Dispatch error record *pRec: build title + detail strings, call Err_Fatal.
// 0x0041fb70
void Err_Dispatch(ERR_RECORD *pRec);

// Return severity class (ERR_SEV_*) for the error code in *pRec.
// 0x00420a10
int  Err_GetSeverity(ERR_RECORD *pRec);

// Write error to ERROR_LOG, show a fatal MessageBox, optionally offer restart.
// param_4 non-zero: include "Line N at file.cpp" prefix from g_nAssertLine.
// 0x00420bd0
void Err_Fatal(const char *pszTitle, const char *pszDetail,
               const char *pszButtons, int nShowLine);

// Unhandled SEH exception entry point: calls Err_Fatal with generic strings.
// 0x00420ba0
void Err_UnhandledException(void);

// ---------------------------------------------------------------------------
// Dialog helpers
// ---------------------------------------------------------------------------

// Show a simple information MessageBox using g_pErrStrings[0] as title.
// 0x0041f700
void Err_ShowDialog(void);

// Show a YES/NO question MessageBox; returns the MessageBox return value.
// 0x0041f910
int  Err_AskDialog(void);

// ---------------------------------------------------------------------------
// Specific error wrappers  (all call Err_FatalMsg then ExitProcess)
// ---------------------------------------------------------------------------

// ERRORS.TXT lookup-based wrappers (slot indices from ERRORS.TXT):
void Err_BadResFile       (int nLine, const char *pszFile);   // 0x0041f4e0
void Err_BadIdxFile       (int nLine, const char *pszFile);   // 0x0041f510
void Err_BadVersion       (int nLine, const char *pszFile);   // 0x0041f540
void Err_BadSP            (int nLine, const char *pszFile);   // 0x0041f570
void Err_MissingDefaultSP (int nLine, const char *pszFile);   // 0x0041f5a0
void Err_MissingRes       (int nLine, const char *pszFile, const char *pszName); // 0x0041f5c0
void Err_MissingResFile   (int nLine, const char *pszFile,   // 0x0041f620
                            const char *pszRes, int nDisk);
void Err_BadResEntry      (int nLine, const char *pszFile);   // 0x0041f680
void Err_OutOfMemory      (int nLine, const char *pszFile);   // 0x0041f6d0
void Err_BadSCN           (int nLine, const char *pszFile);   // 0x0041f740
void Err_DriveCheck       (int nLine, const char *pszFile, int nDriveState); // 0x0041f770
void Err_BadSound         (int nLine, const char *pszFile);   // 0x0041f870
void Err_BadMOV           (int nLine, const char *pszFile);   // 0x0041f8a0
void Err_BadSCNVersion    (int nLine, const char *pszFile);   // 0x0041f8d0

// Hard abort: assert-check then ExitProcess(0).
// 0x0041f480
void Err_Abort            (int nLine, const char *pszFile);

// Restart game: capture EXE path + command line, CreateProcess, ExitProcess.
// 0x0041f960
void Err_RestartGame(void);

// ---------------------------------------------------------------------------
// Debug output
// ---------------------------------------------------------------------------

// Conditional assert/trace display: shows MessageBox in debug builds only.
// Suppressed when g_nReleaseMode != 0.
// 0x0041fa60
void Debug_TraceVal(const char *pszFmt, ...);

// Public assert wrapper — called at every __FILE__/__LINE__ assert site.
// Passes (nLine, pszFile, g_szAssertMsg) to inner assert handler.
// 0x0041f4b0
void Debug_Assert(int nLine, const char *pszFile);

#ifdef __cplusplus
}
#endif
