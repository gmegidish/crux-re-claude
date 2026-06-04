#ifndef WINRES_H
#define WINRES_H

// ---------------------------------------------------------------------------
// WINRES.h  —  SMA-to-Win32 GDI resource conversion (icons / cursors / bitmaps)
// Original: C:\DevStudio\Projects\Crux\WINRES.cpp
// RE offsets: 0x00486fe0 – 0x00487cb0 (WINRES portion ends before 0x00487cb0)
// ---------------------------------------------------------------------------
// This module turns the game's in-memory "SMA" sprite resources (an 8-bit
// palettised bitmap prefixed by a small header: width at +1, height at +3,
// both stored as 16-bit shorts) into native Win32 GDI objects: HICONs,
// HCURSORs and HBITMAPs.  These are used for the window icon, mouse cursors
// shown over dialog boxes, and toolbar/UI bitmaps.
//
// Conversion pipeline (sma2icon / sma2icon_mono):
//   1. Allocate an 8bpp scratch buffer sized to the requested icon dimensions.
//   2. GI_BlitResource() centres the SMA sprite into that buffer.
//   3. CreateBitmap() builds the colour DDB; Win_BuildCursorMask() +
//      WinRes_BuildAndMask() build the 1bpp AND mask (0xFF = transparent).
//   4. CreateIconIndirect() assembles the final HICON/HCURSOR.
//
// The "mono" variant reports an error (Err_SetRecord3) when the sprite is
// larger than the requested icon box, and produces a 1bpp colour plane.
//
// WinRes_RegInitKey is an unrelated helper that copies/creates a registry
// branch (RegOpenKeyEx + RegCreateKeyEx) under the given parent key.
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---------------------------------------------------------------------------
// Debug / source-context base indices (WINRES.cpp __LINE__-style tags)
// ---------------------------------------------------------------------------
extern int g_nWinResDbgCtxBuildAndMask;  // 0x004de4d8
extern int g_nWinResDbgCtxSma2Icon;      // 0x004de538
extern int g_nWinResDbgCtxSma2Bitmap;    // 0x004de5f8
extern int g_nWinResDbgCtxSma2IconMono;  // 0x004de6c0

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// 0x00486fe0  Build a packed 1bpp colour/AND plane from an 8bpp buffer.
//             A source byte of 0xFF sets the corresponding bit; rows are
//             byte-padded.  Returns a heap buffer (caller frees).
void *WinRes_BuildAndMask(const unsigned char *pSrc, int nHeight, int nWidth);

// 0x004871b0  SEH wrapper: build a 32x32 colour HICON from an SMA sprite.
void WinRes_Sma2Icon(const void *pSma, int nHotX, int nHotY);

// 0x00487250  Core: build a colour HICON/HCURSOR from an SMA sprite.
//             fIcon selects icon (TRUE) vs cursor (FALSE).
HICON WinRes_Sma2IconCore(const void *pSma, BOOL fIcon,
                          int nWidth, int nHeight, int nHotX, int nHotY);

// 0x004874a0  Build a blank 32x32 window icon ("wr_make_empty_icon").
void WinRes_MakeEmptyIcon(void);

// 0x00487550  SEH wrapper: build an HBITMAP from an SMA sprite.
void WinRes_Sma2Bitmap(const void *pSma);

// 0x004875e0  Core: build an 8bpp HBITMAP from an SMA sprite.
HBITMAP WinRes_Sma2BitmapCore(const void *pSma);

// 0x00487770  SEH wrapper: build a 32x32 colour HCURSOR from an SMA sprite.
void WinRes_Sma2Cursor(const void *pSma, int nHotX, int nHotY);

// 0x00487810  SEH wrapper: build a 32x32 monochrome HCURSOR from an SMA sprite.
void WinRes_Sma2CursorMono(const void *pSma, int nHotX, int nHotY);

// 0x004878b0  Core: build a monochrome HICON/HCURSOR from an SMA sprite.
HICON WinRes_Sma2IconMonoCore(const void *pSma, BOOL fIcon,
                              int nWidth, int nHeight, int nHotX, int nHotY);

// 0x00487bb0  Copy/create a registry branch ("wr_reg_init_key").
void WinRes_RegInitKey(HKEY hParent, LPCSTR pszSrcSubKey, LPCSTR pszDstSubKey);

#endif // WINRES_H
