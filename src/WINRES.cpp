// ---------------------------------------------------------------------------
// WINRES.cpp  —  SMA-to-Win32 GDI resource conversion (icons / cursors / bitmaps)
// Original: C:\DevStudio\Projects\Crux\WINRES.cpp
// RE offsets: 0x00486fe0 – 0x00487bb0  (WINRES portion; 0x00487cb0 = Wiz_FindVar)
// ---------------------------------------------------------------------------
// Converts the game's 8-bit palettised "SMA" sprite resources into native
// Win32 GDI objects.  An SMA buffer begins with a small header:
//     +1 : short  width
//     +3 : short  height
//     +5 : pixel data (8bpp, 0xFF == transparent)
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "WINRES.h"

// External helpers from other modules ---------------------------------------
extern void  *SafeHeap_Alloc(int nCtx, const char *pszFile, size_t nSize);
extern void   SafeHeap_Free(int nCtx, const char *pszFile, void *p);
extern void   Debug_Trace(int nCtx, const char *pszFile, const char *pszFmt, ...);
extern void   GI_BlitResource(const void *pSma, void *pDst,
                              int nX, int nY, int nW, int nH);
extern void  *Win_BuildCursorMask(const void *pSrc, int nW, int nH);
extern void   Err_Trace(int nCtx, const char *pszFile);
extern void  *Err_SetRecord3(int nCode, void *pRec, int nArg);

// ---------------------------------------------------------------------------
// Debug / source-context base indices
// ---------------------------------------------------------------------------
int g_nWinResDbgCtxBuildAndMask;  // 0x004de4d8
int g_nWinResDbgCtxSma2Icon;      // 0x004de538
int g_nWinResDbgCtxSma2Bitmap;    // 0x004de5f8
int g_nWinResDbgCtxSma2IconMono;  // 0x004de6c0

// ---------------------------------------------------------------------------
// 0x00486fe0  WinRes_BuildAndMask
// Pack an 8bpp buffer into a 1bpp plane: each source byte equal to 0xFF sets
// the matching bit (MSB-first); each row is padded to a whole byte boundary.
// Used to build the AND / colour mask plane for monochrome icons.
// ---------------------------------------------------------------------------
void *WinRes_BuildAndMask(const unsigned char *pSrc, int nHeight, int nWidth)
{
    int    nBit   = 7;
    int    nByte  = 0;
    int    x, y;
    size_t nSize  = (((nWidth + ((nWidth >> 31) & 7)) >> 3) + 1) * nHeight;

    unsigned char *pDst =
        (unsigned char *)SafeHeap_Alloc(g_nWinResDbgCtxBuildAndMask + 7,
                                        "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp",
                                        nSize);
    memset(pDst, 0, nSize);

    for (y = 0; y < nHeight; y++)
    {
        for (x = 0; x < nWidth; x++)
        {
            if (pSrc[y * nWidth + x] == 0xFF)
                pDst[nByte] |= (unsigned char)(1 << nBit);

            if (--nBit == -1)
            {
                nBit = 7;
                nByte++;
            }
        }
        // pad to byte boundary at the end of each row
        if (nBit != 7)
        {
            nBit = 7;
            nByte++;
        }
    }

    return pDst;
}

// ---------------------------------------------------------------------------
// 0x00487250  WinRes_Sma2IconCore
// Build a colour HICON (fIcon=TRUE) or HCURSOR (fIcon=FALSE) from an SMA sprite,
// centred within an nWidth x nHeight box.  Returns NULL if the sprite is larger
// than the box.
// ---------------------------------------------------------------------------
HICON WinRes_Sma2IconCore(const void *pSma, BOOL fIcon,
                          int nWidth, int nHeight, int nHotX, int nHotY)
{
    ICONINFO ii;
    HICON    hIcon = NULL;
    int      nImgW = (int)*(const short *)((const char *)pSma + 1);
    int      nImgH = (int)*(const short *)((const char *)pSma + 3);

    if (nWidth < nImgW || nHeight < nImgH)
        return NULL;

    int nOffX = (nWidth  - nImgW) / 2;
    int nOffY = (nHeight - nImgH) / 2;

    ii.fIcon    = fIcon;
    ii.xHotspot = nHotX + nOffX;
    ii.yHotspot = nHotY + nOffY;

    size_t nSize = (size_t)(nHeight + 1) * (nWidth + 1);
    void  *pBuf  = SafeHeap_Alloc(g_nWinResDbgCtxSma2Icon + 0x1d,
                                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", nSize);
    memset(pBuf, 0, nSize);
    GI_BlitResource(pSma, pBuf, nOffX, nOffY, nWidth, nHeight);

    HBITMAP hColor = CreateBitmap(nWidth, nHeight, 1, 8, pBuf);
    void   *pMask  = Win_BuildCursorMask(pBuf, nWidth, nHeight);
    ii.hbmMask  = CreateBitmap(nWidth, nHeight, 1, 1, pMask);
    ii.hbmColor = hColor;

    hIcon = CreateIconIndirect(&ii);

    SafeHeap_Free(g_nWinResDbgCtxSma2Icon + 0x2e,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pBuf);
    SafeHeap_Free(g_nWinResDbgCtxSma2Icon + 0x2f,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pMask);
    DeleteObject(hColor);
    DeleteObject(ii.hbmMask);

    return hIcon;
}

// ---------------------------------------------------------------------------
// 0x004871b0  WinRes_Sma2Icon  (SEH wrapper)
// Build a 32x32 colour HICON from an SMA sprite.
// ---------------------------------------------------------------------------
void WinRes_Sma2Icon(const void *pSma, int nHotX, int nHotY)
{
    WinRes_Sma2IconCore(pSma, TRUE, 0x20, 0x20, nHotX, nHotY);
}

// ---------------------------------------------------------------------------
// 0x004874a0  WinRes_MakeEmptyIcon  ("wr_make_empty_icon")
// Build a blank 32x32 window icon from a zeroed 9-byte SMA header.
// ---------------------------------------------------------------------------
void WinRes_MakeEmptyIcon(void)
{
    unsigned char sma[12];
    memset(sma, 0, 9);
    WinRes_Sma2IconCore(sma, TRUE, 0x20, 0x20, 0, 0);
}

// ---------------------------------------------------------------------------
// 0x004875e0  WinRes_Sma2BitmapCore
// Build an 8bpp HBITMAP from an SMA sprite (no centring; uses sprite size).
// ---------------------------------------------------------------------------
HBITMAP WinRes_Sma2BitmapCore(const void *pSma)
{
    int nWidth  = (int)*(const short *)((const char *)pSma + 1);
    int nHeight = (int)*(const short *)((const char *)pSma + 3);

    Debug_Trace(g_nWinResDbgCtxSma2Bitmap + 10,
                "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp",
                "The img x is %d and the img y is %d", nWidth, nHeight);

    size_t nSize = (size_t)(nWidth + 1) * (nHeight + 1);
    void  *pBuf  = SafeHeap_Alloc(g_nWinResDbgCtxSma2Bitmap + 0xf,
                                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", nSize);
    memset(pBuf, 0, nSize);
    GI_BlitResource(pSma, pBuf, 0, 0, nWidth, nHeight);

    HBITMAP hbm = CreateBitmap(nWidth, nHeight, 1, 8, pBuf);

    SafeHeap_Free(g_nWinResDbgCtxSma2Bitmap + 0x15,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pBuf);
    return hbm;
}

// ---------------------------------------------------------------------------
// 0x00487550  WinRes_Sma2Bitmap  (SEH wrapper)
// ---------------------------------------------------------------------------
void WinRes_Sma2Bitmap(const void *pSma)
{
    WinRes_Sma2BitmapCore(pSma);
}

// ---------------------------------------------------------------------------
// 0x00487770  WinRes_Sma2Cursor  (SEH wrapper)
// Build a 32x32 colour HCURSOR from an SMA sprite (fIcon = FALSE).
// ---------------------------------------------------------------------------
void WinRes_Sma2Cursor(const void *pSma, int nHotX, int nHotY)
{
    WinRes_Sma2IconCore(pSma, FALSE, 0x20, 0x20, nHotX, nHotY);
}

// ---------------------------------------------------------------------------
// 0x00487810  WinRes_Sma2CursorMono  (SEH wrapper)
// Build a 32x32 monochrome HCURSOR from an SMA sprite (fIcon = FALSE).
// ---------------------------------------------------------------------------
void WinRes_Sma2CursorMono(const void *pSma, int nHotX, int nHotY)
{
    WinRes_Sma2IconMonoCore(pSma, FALSE, 0x20, 0x20, nHotX, nHotY);
}

// ---------------------------------------------------------------------------
// 0x004878b0  WinRes_Sma2IconMonoCore
// Build a monochrome HICON/HCURSOR from an SMA sprite.  The colour plane is a
// 1bpp plane built by WinRes_BuildAndMask.  Reports an error and continues if
// the sprite is larger than the requested box.
// ---------------------------------------------------------------------------
HICON WinRes_Sma2IconMonoCore(const void *pSma, BOOL fIcon,
                              int nWidth, int nHeight, int nHotX, int nHotY)
{
    ICONINFO ii;
    HICON    hIcon = NULL;
    int      nImgW = (int)*(const short *)((const char *)pSma + 1);
    int      nImgH = (int)*(const short *)((const char *)pSma + 3);

    if (nWidth < nImgW || nHeight < nImgH)
    {
        // The sprite does not fit the requested box: log + raise an error record.
        Err_Trace(g_nWinResDbgCtxSma2IconMono + 0x11,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp");
        Err_SetRecord3(0x16, NULL, -1);
        // (original then forwards the record to the error dispatcher and falls
        //  through to attempt the conversion anyway)
    }

    int nOffX = (nWidth  - nImgW) / 2;
    int nOffY = (nHeight - nImgH) / 2;

    ii.fIcon    = fIcon;
    ii.xHotspot = nHotX + nOffX;
    ii.yHotspot = nHotY + nOffY;

    size_t nSize = (size_t)(nHeight + 1) * (nWidth + 1);
    void  *pBuf  = SafeHeap_Alloc(g_nWinResDbgCtxSma2IconMono + 0x1d,
                                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", nSize);
    memset(pBuf, 0, nSize);
    GI_BlitResource(pSma, pBuf, nOffX, nOffY, nWidth, nHeight);

    HBITMAP hColor8 = CreateBitmap(nWidth, nHeight, 1, 8, pBuf);   // unused colour DDB
    void   *pMask   = Win_BuildCursorMask(pBuf, nWidth, nHeight);
    ii.hbmMask = CreateBitmap(nWidth, nHeight, 1, 1, pMask);

    void *pMono = WinRes_BuildAndMask((const unsigned char *)pBuf, nWidth, nHeight);
    ii.hbmColor = CreateBitmap(nWidth, nHeight, 1, 1, pMono);

    hIcon = CreateIconIndirect(&ii);

    SafeHeap_Free(g_nWinResDbgCtxSma2IconMono + 0x30,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pBuf);
    SafeHeap_Free(g_nWinResDbgCtxSma2IconMono + 0x31,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pMono);
    SafeHeap_Free(g_nWinResDbgCtxSma2IconMono + 0x32,
                  "C:\\DevStudio\\Projects\\Crux\\WINRES.cpp", pMask);
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    return hIcon;
}

// ---------------------------------------------------------------------------
// 0x00487bb0  WinRes_RegInitKey  ("wr_reg_init_key")
// Open a parent registry key and create a destination sub-key beneath it,
// then close both handles.
// ---------------------------------------------------------------------------
void WinRes_RegInitKey(HKEY hParent, LPCSTR pszSrcSubKey, LPCSTR pszDstSubKey)
{
    HKEY    hSrc = NULL;
    HKEY    hDst = NULL;
    DWORD   dwDisposition;
    LSTATUS ls;

    ls = RegOpenKeyExA(hParent, pszSrcSubKey, 0, KEY_ALL_ACCESS, &hSrc);
    if (ls == ERROR_SUCCESS)
    {
        ls = RegCreateKeyExA(hSrc, pszDstSubKey, 0, NULL, 0, KEY_ALL_ACCESS,
                             NULL, &hDst, &dwDisposition);
        RegCloseKey(hDst);
        RegCloseKey(hSrc);
    }
}
