// ---------------------------------------------------------------------------
// Except.h — SCN (scene/room) data-file loader and its error-context helpers
//
// NOTE ON MODULE NAME
// -------------------
// These six functions were originally bucketed under the placeholder name
// "Except.cpp" in the project's function map.  They are NOT C++ exception or
// Win95 SEH plumbing — the apparent `unaff_FS_OFFSET` "SEH frame" seen in the
// decompiles is just the standard MSVC __try/__except prologue the compiler
// emits around any function that can raise the game's fatal-error path
// (Err_Dispatch).  The embedded source strings prove the true origin:
//
//     "int files_do_read(char* table_na..."   -> Files_DoRead
//     "int files_fread_string(char* st..."    -> Files_FreadString
//     "files_load_scn(char* name)"            -> Files_LoadScn
//     "file_get_time(char* fname, FILETI..."  -> Files_GetTime
//     source path: C:\DevStudio\Projects\Crux\FILES.cpp
//
// So this is the *front* of FILES.cpp (address range 0x00420e50-0x00422240,
// immediately preceding Files_SaveState at 0x004223a0).  It is kept in a
// separate translation unit here only to match the requested file layout.
//
// WHAT THESE FUNCTIONS DO
// -----------------------
//  - Files_SetCurrentRoom : stores the current room-name pointer (used by the
//                           error reporter as context).
//  - Files_SetErrSource   : records the source file (basename) + line that the
//                           fatal-error path (Err_Fatal) will report.  This is
//                           the "Err_PushFrame"/error-location helper referenced
//                           by many modules (thunk at 0x004013a2).
//  - Files_GetTime        : FindFirstFileA wrapper returning a file's
//                           last-write FILETIME.
//  - Files_LoadScn        : loads a compiled .SCN scene file: header, the six
//                           string tables (area/palette/exit/anim/sca/theme/
//                           sound), the area-node array, the area-cache record
//                           array, the cache slot table, and the on-the-fly
//                           node lists.  On any read failure it raises a fatal
//                           error via Err_SetRecord3 + Err_Dispatch.
//  - Files_DoRead         : reads one length-prefixed string table from the
//                           .SCN file (count, then `count` strings), bounded by
//                           a caller-supplied maximum.
//  - Files_FreadString    : reads one length-prefixed string, allocates it on
//                           the safe heap, and applies two legacy filename
//                           fix-ups ("FLIWINS" / "AATALK").
//
// Original source: C:\DevStudio\Projects\Crux\FILES.cpp
// RE address range: 0x00420e50 - 0x00422240
// Functions: 6
// ---------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 0x00420e50  Store the current room-name pointer used as error context.
void Files_SetCurrentRoom(char *pszRoom);

// 0x00420e60  Record the source location (line + basename of pszFile) that the
//             fatal-error reporter will print.  Called immediately before every
//             Err_SetRecord3/Err_Dispatch site.  (a.k.a. Err_PushFrame.)
void Files_SetErrSource(int nLine, char *pszFile);

// 0x00420ed0  Return a file's last-write FILETIME via FindFirstFileA.
//             Returns 0 on success (fills pftLastWrite), -1 if not found.
int  Files_GetTime(LPCSTR pszPath, DWORD *pftLastWrite);

// 0x00420fb0  Load a compiled .SCN scene file into the area/cache/on-the-fly
//             tables.  Raises a fatal error on any read or alloc failure.
void Files_LoadScn(char *pszName);

// 0x004220e0  Read one length-prefixed string table (count + entries) from an
//             open .SCN file, bounded by nMax.  Returns 0 on success, -1 on
//             read failure.
int  Files_DoRead(char *pszErrTitle, int *pnCount, int nMax,
                  int *paTable, int *pFile);

// 0x00422240  Read one length-prefixed string, heap-allocate it, and apply the
//             FLIWINS/AATALK legacy fix-ups.  Returns 0 on success, -1 on fail.
int  Files_FreadString(int *ppszOut, int *pFile);
