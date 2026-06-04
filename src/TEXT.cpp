// TEXT.cpp -- GDI-based scrolling text display + music theme manager
//
// Two subsystems:
//   Txt_*  : Animated, auto-wrapped, multi-page, scrolling text overlay
//            drawn via Win32 GDI (DrawTextA / TextOutA) onto the game's
//            shared HDC.  Strings are looked up by name from an external
//            table or supplied inline with an '@' prefix.  A highlight
//            range (for cursor-driven word selection) is supported.
//
//   Thm_*  : Music theme manager.  Binary theme files hold segment names,
//            labels, commands, and event blocks.  A background Win32 thread
//            advances segments and signals a Win32 event when each one ends.
//
// Original source: C:\DevStudio\Projects\Crux\TEXT.cpp
// Address range (text):  0x00475970 -- 0x00478240
// Address range (theme): 0x004782f0 -- 0x00479810

#include "TEXT.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>

// Forward declarations for cross-module helpers (resolved via thunk table)
extern "C" {
    // GDI surface acquisition / release
    HDC  DDI_GetHDC(void);                     // thunk_FUN_0042e380
    void DDI_ReleaseHDC(HDC hdc);              // thunk_FUN_0042e420

    // Surface blit helpers (Img.cpp)
    int  DDI_CreateOffscreenSurf(int nSrcX, int nSrcY, int nW, int nH,
                                  int nDstX, int nDstY, int bMirror);  // thunk_FUN_0042e600
    void DDI_BlitSurface(int nSurf, int nArg);                         // thunk_FUN_0041c7e0

    // Debug helpers
    void Debug_Assert(int nLine, const char *pFile, int nVal);
    void Debug_Trace(int nLine, const char *pFile, const char *pFmt, ...);

    // String utilities
    int  Str_IsSpace(int c);                   // FUN_0048b6d0 -- isspace-like
    int  Str_Compare(int a, int b);            // FUN_0049a830 -- case-insensitive strcmp-like
    int  Str_Format(char *pBuf, int nFmtId, ...); // FUN_0048a6a0 -- vsprintf wrapper

    // String copy into buffer
    void Str_CopyBuf(char *pDst, const char *pSrc); // FUN_004895e0

    // Audio playback
    void Audio_PlayMidi(int nChannel, int nData, int nLen,
                        int nLoop, int nVol, int nFreq,
                        int nArg6, int nArg7);  // thunk_FUN_004427e0
    void Audio_SetCallback(int nChannel, void *pfnCallback); // thunk_FUN_00442f40
    int  Audio_IsPlaying(int nChannel);         // thunk_FUN_00443d50
    void Audio_Stop(int nChannel);              // thunk_FUN_00443df0
    void Audio_SetVolume(int nChannel, int nData, int nVol); // thunk_FUN_00443060

    // Resource / INI helpers
    int  INI_ReadInt(int nSection, void *pDst, int nSize,
                     void *pKeys, int nReserved);  // thunk_FUN_0045e550
    int  INI_Open(int nMode, const char *pszName, int nSection); // thunk_FUN_0045d4e0

    // Heap allocation / free
    void *Heap_Alloc(int nLine, const char *pFile, int nSize); // thunk_FUN_0046bcc0
    void  Heap_Free(int nLine, const char *pFile, void *pMem); // thunk_FUN_0046bd80

    // Error helpers
    void Fatal_Error(int nLine, const char *pFile);            // thunk_FUN_00420e60
    void *Fatal_AllocError(int nType, const void *pArg, int nSize); // thunk_FUN_0041fad0
    void Fatal_AllocHandler(void *pBlk, const void *pTable);   // FUN_00489090
}

// ===========================================================================
// TEXT globals
// ===========================================================================

int  g_nTxtEnabled;          // 0x007cd7c4
int  g_nTxtActive;           // 0x007cd7c0
int  g_nTxtIsDirect;         // 0x007cd7bc
int  g_nTxtMode;             // 0x004da164
int  g_nTxtRTL;              // 0x007cd7c8

int  g_nTxtFont;             // 0x007cd7b4
int  g_nTxtFontHeight;       // 0x004da16c
int  g_nTxtCharSet;          // 0x007ca874
int  g_nTxtLineHeight;       // 0x007ca38c
int  g_nTxtFontWidthUnit;    // 0x007ca884

int  g_nTxtColorGDI;         // 0x007c93b8
int  g_nTxtColorHighlight;   // 0x007ca880

char *g_pTxtCurStr;          // 0x007cc800
char *g_pTxtStrStart;        // 0x007ca85c
char *g_pTxtStrEnd;          // 0x007ca87c
int   g_nTxtStringCount;     // 0x007cc7d8
int   g_nTxtStringKeys;      // 0x007c93bc
int   g_nTxtStringValues;    // 0x007ca78c
char  g_abTxtDirectBuf[1024];// 0x007ca790  (size approximate)
char *g_szTxtMsgBuf;         // 0x007ca398

int  g_nTxtMaxLines;         // 0x004da168
int  g_nTxtAlign;            // 0x004da160
int  g_nTxtAlignMode;        // 0x007ca878
int  g_nTxtDrawFlags;        // 0x007ca780
int  g_nTxtLinesPerPage;     // 0x007cd7d0
int  g_nTxtTotalLines;       // 0x007cd7d4
int  g_nTxtFirstLine;        // 0x007cd7d8
int  g_nTxtScrollSpeed;      // 0x007ca788
int  g_nTxtScrollPos;        // 0x007ca858
int  g_nTxtScrollTotal;      // 0x007ca390
int  g_nTxtDisplayDuration;  // 0x007cc804
int  g_nTxtDuration;         // 0x007cd7b8

int  g_nTxtRectLeft;         // 0x007cc7f0
int  g_nTxtRectTop;          // 0x007cc7f4
int  g_nTxtRectRight;        // 0x007cc7f8
int  g_nTxtRectBottom;       // 0x007cc7fc
int  g_nTxtBoxLeft;          // 0x007ca860
int  g_nTxtBoxTop;           // 0x007ca864
int  g_nTxtBoxRight;         // 0x007ca868
int  g_nTxtBoxBottom;        // 0x007ca86c

int  g_nTxtBgSurface;        // 0x007ca360
int  g_nTxtSurfaceWidth;     // 0x007ca394
int  g_nTxtSavedBoxLeft;     // 0x007ca888
int  g_nTxtSavedBoxTop;      // 0x007ca88c
int  g_nTxtSavedBoxRight;    // 0x007ca890
int  g_nTxtSavedBoxBottom;   // 0x007ca894

char *g_pTxtHighlightStart;  // 0x007ca784
char *g_pTxtHighlightEnd;    // 0x007ca870
int   g_nTxtHighlightIdx;    // 0x007cc808

int   g_nTxtSavedRectLeft;   // 0x007cc7e0
// (7cc7e4, 7cc7e8, 7cc7ec are g_nTxtSavedRectTop/Right/Bottom — omitted from
//  public header for brevity, addressed directly by Txt_SetString)

// ===========================================================================
// TEXT implementation
// ===========================================================================

// Txt_SetRect -- store an explicit four-coordinate rect for text display.
// Passing -1 for all four values causes Txt_SetString to auto-compute the
// layout rectangle from the string extents.
// Address: 0x00475970
void Txt_SetRect(int nLeft, int nTop, int nRight, int nBottom)
{
    g_nTxtRectLeft   = nLeft;
    g_nTxtRectTop    = nTop;
    g_nTxtRectRight  = nRight;
    g_nTxtRectBottom = nBottom;
}

// Txt_SetMaxLines -- set the maximum number of wrapped lines per text block.
// A value of -1 resets to the default of 1000.
// Address: 0x00475a20
void Txt_SetMaxLines(int nMax)
{
    if (nMax == -1)
        nMax = 1000;
    g_nTxtMaxLines = nMax;
}

// Txt_GetMaxLines -- return the current maximum lines per block.
// Address: 0x00475ac0
int Txt_GetMaxLines(void)
{
    return g_nTxtMaxLines;
}

// Txt_GetAlign -- return the current text alignment (0=left 1=center 2=right).
// Address: 0x00475b50
int Txt_GetAlign(void)
{
    return g_nTxtAlign;
}

// Txt_SetAlign -- set text alignment (0=left 1=center 2=right).
// Address: 0x00475be0
void Txt_SetAlign(int nAlign)
{
    g_nTxtAlign = nAlign;
}

// Txt_SetMode -- set the text subsystem mode / enabled flag.
// Called from the startup CRT thunk; when nMode==0 the GDI layout path in
// Txt_SetString / Txt_Update is entirely skipped.
// Address: 0x00475c70
void Txt_SetMode(int nMode)
{
    g_nTxtMode = nMode;
}

// Txt_GetMode -- return the current subsystem mode.
// Address: 0x00475d00
int Txt_GetMode(void)
{
    return g_nTxtMode;
}

// Txt_CreateFont -- create the GDI HFONT used for all text rendering.
// Font weight is hard-coded at 700 (bold).  nPitchFamily is combined with
// the FIXED_PITCH bit (0x02) and masked to a byte.  If the subsystem is
// disabled (g_nTxtEnabled == 0) the call is a no-op.
// Address: 0x00475d90
void Txt_CreateFont(int nPitchFamily)
{
    if (!g_nTxtEnabled)
        return;
    g_nTxtFont = (int)CreateFontA(
        g_nTxtFontHeight,
        0, 0, 0,
        700,          // FW_BOLD
        0, 0, 0,
        g_nTxtCharSet,
        0, 0,
        2,            // ANTIALIASED_QUALITY
        (nPitchFamily & 0xff) | 2,
        (LPCSTR)&g_nTxtFontWidthUnit /* points to font name buffer */);
}

// Txt_SetColor -- set the text colour from three 6-bit RGB components.
// Two packed formats are maintained:
//   g_nTxtColorGDI       -- (r<<2)|(g<<10)|(b<<18)  used with SetTextColor
//   g_nTxtColorHighlight -- (r<<1)|(g<<9)|(b<<17)   used for highlighted spans
// Address: 0x00475e80
void Txt_SetColor(int nR, int nG, int nB)
{
    if (!g_nTxtEnabled)
        return;
    g_nTxtColorGDI       = ((nR & 0x3f) << 2)  | ((nG & 0x3f) << 10) | ((nB & 0x3f) << 18);
    g_nTxtColorHighlight = ((nR & 0x7f) << 1)  | ((nG & 0x7f) << 9)  | ((nB & 0x7f) << 17);
}

// Txt_LookupString -- look up a string by integer name ID in the external
// string table (g_nTxtStringKeys / g_nTxtStringValues, g_nTxtStringCount entries).
//
// On success:
//   - g_pTxtCurStr    = pointer just past leading whitespace of the raw string
//   - g_pTxtStrStart  = same (will be used as the trimmed start)
//   - g_pTxtStrEnd    = pointer to last non-whitespace char (trailing trim)
//
// Returns 0 on success, -1 if not found, disabled, or the trimmed string is
// empty.
//
// NOTE: Graninv.cpp calls thunk_FUN_00475f90 as "GetItemName" -- that thunk
// lands here.  The function does not return a name; it resolves the current
// display-string pointer for the text subsystem.  Graninv's usage is setting
// up an item description string by name-ID before calling Txt_SetString.
// Address: 0x00475f90
int Txt_LookupString(int nNameId)
{
    int i;
    char *pStr;
    size_t nLen;

    if (!g_nTxtEnabled)
        return -1;

    // Linear search through the key table
    for (i = 0; i < g_nTxtStringCount; i++) {
        if (Str_Compare(nNameId, *(int *)(g_nTxtStringKeys + i * 4)) == 0)
            break;
    }

    if (i >= g_nTxtStringCount) {
        // Not found -- point at an empty sentinel
        g_pTxtCurStr = (char *)0; /* DAT_007cd7f0 sentinel */
        return -1;
    }

    // Found -- walk past leading whitespace
    pStr = *(char **)(g_nTxtStringValues + i * 4);
    while (Str_IsSpace((int)*pStr))
        pStr++;

    g_pTxtCurStr  = pStr;
    nLen          = strlen(pStr);
    g_pTxtStrEnd  = pStr + nLen;     // points one past end initially

    // Trim leading whitespace (start)
    g_pTxtStrStart = pStr;
    while (g_pTxtStrStart <= g_pTxtStrEnd && Str_IsSpace((int)*g_pTxtStrStart))
        g_pTxtStrStart++;

    // Trim trailing whitespace (end)
    while (g_pTxtStrStart <= g_pTxtStrEnd && Str_IsSpace((int)*g_pTxtStrEnd))
        g_pTxtStrEnd--;

    // Collapse to the trimmed start
    g_pTxtCurStr = g_pTxtStrStart;

    if (g_pTxtStrEnd < g_pTxtStrStart) {
        // Entirely whitespace
        g_pTxtCurStr = (char *)0; /* DAT_007cd7ec sentinel */
        return -1;
    }

    return 0;
}

// Txt_ResolveDirectString -- resolve a string stored in g_abTxtDirectBuf.
// Copies the buffer into g_pTxtCurStr, trims whitespace from both ends, and
// stores g_pTxtStrStart / g_pTxtStrEnd.  Called when the input starts with '@'.
// Address: 0x00476c20
void Txt_ResolveDirectString(void)
{
    size_t nLen;

    g_pTxtCurStr   = g_abTxtDirectBuf;
    g_pTxtStrStart = g_abTxtDirectBuf;
    nLen           = strlen(g_abTxtDirectBuf);
    g_pTxtStrEnd   = g_pTxtCurStr + nLen;

    // Trim leading whitespace
    while (g_pTxtStrStart <= g_pTxtStrEnd && Str_IsSpace((int)*g_pTxtStrStart))
        g_pTxtStrStart++;

    if (g_pTxtStrStart > g_pTxtStrEnd) {
        // Empty after trim
        g_pTxtCurStr = g_abTxtDirectBuf; /* fallback sentinel DAT_007cd7f4 */
        return;
    }

    // Trim trailing whitespace
    while (g_pTxtStrStart <= g_pTxtStrEnd && Str_IsSpace((int)*g_pTxtStrEnd))
        g_pTxtStrEnd--;

    g_pTxtCurStr = g_pTxtStrStart;
}

// Txt_SetString -- main entry point for displaying a text string.
//
// pszName: name string or '@'-prefixed literal
// pRect:   optional 4-int [left,top,right,bottom]; pass rect with left==-1
//          for auto-layout centred on screen
// nDuration: display duration in ticks; 0xffffffff = auto (proportional to
//            string length)
//
// Steps:
//  1. If pszName starts with '@': copy to g_abTxtDirectBuf, save the rect
//     and duration, call Txt_ResolveDirectString.
//  2. Otherwise: call Txt_LookupString.  On failure, clear g_nTxtActive.
//  3. Acquire GDI HDC, select g_nTxtFont, call GetTextMetricsA for line height.
//  4. If pRect[0]==-1: auto-compute bounding rect from GetTextExtentPoint32A.
//     Iteratively widen the column divisor until the text fits within
//     320x200 margins.  Clamp to screen bounds.
//  5. Compute g_nTxtLinesPerPage from box height and line height.
//  6. Word-wrap into the line table (g_pTxtStrStart / line-end pointer array
//     at &g_nTxtSavedBoxBottom).
//  7. Compute g_nTxtScrollSpeed proportional to the string length.
//  8. Initialise scroll counters and highlight range to zero / -1.
//
// Address: 0x004761d0
void Txt_SetString(const char *pszName, int *pRect, int nDuration)
{
    HDC       hdc;
    HGDIOBJ   hOldFont;
    TEXTMETRICA tm;
    SIZE      sz;
    int       nColDiv;
    int       nColWidth;
    int       nRows;
    int       nBoxW, nBoxH;
    int       nCenterX, nCenterY;
    char      *pCur, *pEnd, *pLineStart;
    int       nLine;
    int       nFits;
    int       nBoxLeft, nBoxTop, nBoxRight, nBoxBottom;

    if (!g_nTxtEnabled || !g_nTxtMode)
        return;

    if (pszName[0] == '@') {
        // Literal string -- copy past the '@', save rect/duration
        if ((int)nDuration < 1) {
            g_nTxtActive = 0;
            return;
        }
        g_nTxtDuration       = nDuration;
        g_nTxtSavedRectLeft  = pRect[0];
        /* pRect[1..3] stored in adjacent globals 7cc7e4, 7cc7e8, 7cc7ec */

        Str_CopyBuf(g_abTxtDirectBuf, pszName + 1);
        Txt_ResolveDirectString();
        g_nTxtIsDirect = 1;
    } else {
        int rc = Txt_LookupString((int)pszName);
        if (rc == 0) {
            g_nTxtIsDirect = 0;
        } else {
            if ((int)g_nTxtDuration < 1) {
                g_nTxtActive = 0;
                return;
            }
            Txt_ResolveDirectString();
            g_nTxtIsDirect = 1;
        }
    }

    // nDuration == 0xffffffff means auto-compute from string length
    if (nDuration == 0xffffffff) {
        size_t nLen = strlen(g_pTxtCurStr);
        nDuration = (unsigned int)(
            ((long long)(int)(nLen * g_nTxtAlign) * (long long)g_nTxtSurfaceWidth)
            / (unsigned long long)g_nTxtFontWidthUnit);
    }

    g_nTxtScrollSpeed    = nDuration;
    g_nTxtDisplayDuration= nDuration;
    g_nTxtScrollTotal    = 0;

    Debug_Assert(/* line 0x3c */ 0x3c, "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp", nDuration);

    hdc = DDI_GetHDC();
    if (!hdc)
        return;

    hOldFont = SelectObject(hdc, (HGDIOBJ)g_nTxtFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_nTxtColorGDI);
    GetTextMetricsA(hdc, &tm);
    g_nTxtLineHeight = tm.tmHeight + tm.tmExternalLeading;

    // --- Compute bounding rect ---
    g_nTxtLinesPerPage = 4;   // initial column divisor seed

    if (pRect[0] == -1) {
        // Auto-layout: widen columns until text fits within 619 (0x26b) chars wide
        size_t nStrLen = strlen(g_pTxtCurStr);
        GetTextExtentPoint32A(hdc, g_pTxtCurStr, (int)nStrLen, &sz);

        nColDiv = 4;
        do {
            for (nColWidth = 213 /* 0xd5 */;
                 nColWidth < sz.cx / nColDiv;
                 nColWidth += 30 /* 0x1e */)
                ;
            nColDiv++;
        } while (nColWidth > 0x26b);

        nColDiv--;
        nBoxH = nColDiv;
        if (g_nTxtMaxLines < nColDiv)
            nBoxH = g_nTxtMaxLines;

        int nMinBoxH = nColDiv;
        if (g_nTxtMaxLines <= nColDiv)
            nMinBoxH = g_nTxtMaxLines;

        nRows    = nBoxH * tm.tmHeight + tm.tmExternalLeading * (nMinBoxH - 1);
        nBoxW    = (nColWidth - 1 < 0x27f) ? nColWidth - 1 : 0x27f;

        if (pRect[0] == -1) {
            // Centre on screen (320x200 = 0x280 x 0x1c8 visible area)
            nBoxTop    = 0x1cb - nRows;
            nBoxBottom = 0x1cb;
            nBoxLeft   = (0x280 - nBoxW) / 2;
            nBoxRight  = nBoxLeft + nBoxW;
        } else {
            // Centre within supplied rect
            nCenterX   = (pRect[0] + pRect[2]) / 2;
            nBoxLeft   = nCenterX - (nBoxW + 1) / 2;
            nBoxRight  = nCenterX + (nBoxW + 1) / 2;
            nBoxTop    = pRect[1] - (nRows + 1) - 10;
            nBoxBottom = pRect[1] - 10;

            // Clamp to safe margins
            if (nBoxLeft < 10) {
                nBoxRight -= (nBoxLeft - 10);
                nBoxLeft   = 10;
            }
            if (nBoxRight > 0x275) {
                nBoxLeft  -= (nBoxRight - 0x275);
                nBoxRight  = 0x275;
            }
            if (nBoxTop < 10) {
                nBoxBottom -= (nBoxTop - 20);
                nBoxTop     = 10;
            }
            if (nBoxBottom > 0x1d5) {
                nBoxTop    -= (nBoxBottom - 0x1d5);
                nBoxBottom  = 0x1d5;
            }
        }

        g_nTxtDrawFlags = g_nTxtAlignMode | g_nTxtRTL;
    } else {
        // Explicit rect supplied
        g_nTxtDrawFlags = g_nTxtAlignMode | g_nTxtRTL;
        nBoxLeft    = g_nTxtRectLeft;
        nBoxTop     = g_nTxtRectTop;
        nBoxRight   = g_nTxtRectRight;
        nBoxBottom  = g_nTxtRectBottom;
    }

    g_nTxtBoxLeft   = nBoxLeft;
    g_nTxtBoxTop    = nBoxTop;
    g_nTxtBoxRight  = nBoxRight;
    g_nTxtBoxBottom = nBoxBottom;

    g_nTxtLinesPerPage = ((nBoxBottom - nBoxTop) - tm.tmHeight) /
                          (tm.tmHeight + tm.tmExternalLeading) + 1;

    // --- Word-wrap into line table ---
    // The line table is a parallel array of (start, end) char* pairs stored
    // starting at &g_nTxtSavedBoxBottom (address 0x007ca894).
    // g_pTxtStrStart / g_pTxtStrEnd bound the trimmed string.
    nLine    = 0;
    pCur     = g_pTxtStrStart;
    pLineStart = g_pTxtStrStart;

    while (pLineStart < g_pTxtStrEnd) {
        char *pWord = pLineStart;

        // Skip leading spaces
        while (pWord < g_pTxtStrEnd && Str_IsSpace((int)*pWord) == 0)
            pWord++;

        GetTextExtentPoint32A(hdc, pLineStart,
                              (int)(pWord - pLineStart), &sz);

        nFits = 0;
        if (g_nTxtBoxRight - g_nTxtBoxLeft < sz.cx) {
            nFits = 1;  // line too wide -- must break
        } else {
            // Store end of visible portion, advance over more words
            /* line-end pointer stored in table */
            pCur = pWord;
            while (pCur < g_pTxtStrEnd && Str_IsSpace((int)*pCur) == 0) {
                if (*pCur == '\n')
                    nFits = 1;
                pCur++;
            }
            if (pCur >= g_pTxtStrEnd)
                nFits = 1;
        }

        if (nFits) {
            // Advance to next line
            nLine++;
            pLineStart = pCur;

            // Skip whitespace at start of next line
            do {
                pCur++;
                if (pCur >= g_pTxtStrEnd) break;
            } while (Str_IsSpace((int)*pCur) != 0);

            pLineStart = pCur;
        }
    }

    g_nTxtTotalLines = nLine;

    SelectObject(hdc, hOldFont);
    DDI_ReleaseHDC(hdc);
    g_nTxtActive = 1;

    // Compute scroll speed
    if (strlen(g_pTxtStrStart) == 0) {
        g_nTxtDisplayDuration = 0;
        g_nTxtScrollSpeed     = 0;
    } else {
        if (g_nTxtLinesPerPage < g_nTxtTotalLines) {
            // Speed proportional to first page's character count
            /* see Ghidra decompile for exact formula */
        } else {
            g_nTxtScrollSpeed = g_nTxtDisplayDuration;
        }
        g_nTxtFirstLine = 0;
    }

    g_nTxtScrollTotal = g_nTxtScrollSpeed;

    Debug_Assert(/* line 0xd2 */ 0xd2, "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp",
                 g_nTxtScrollSpeed);

    // Initialise per-frame state
    g_nTxtAlignMode      = 0;
    g_nTxtScrollPos      = 0;
    g_pTxtHighlightStart = NULL;
    g_nTxtHighlightIdx   = 0;
    g_nTxtColorGDI       = -1; /* sentinel 0x007c93c0 */
    g_nTxtHighlightIdx   = 0;  /* 0x007cc810 */
}

// Txt_IsScrollPending -- return 1 if more text remains to be scrolled in.
// Returns 0 if the subsystem is disabled, no string is active, the string is
// empty, or the @-prefix path has already finished (g_nTxtIsDirect==1).
// Address: 0x00476d90
int Txt_IsScrollPending(void)
{
    if (!g_nTxtEnabled)
        return 0;
    if (!g_nTxtActive)
        return 0;
    if (strlen(g_pTxtCurStr) == 0)
        return 0;
    if (g_nTxtIsDirect == 0)
        return 1;
    return 0;
}

// Txt_SetMessage -- display a formatted, auto-centred message.
// Calls vsprintf into g_szTxtMsgBuf, measures with GetTextExtentPoint32A,
// auto-wraps, draws via DrawTextA with DT_CALCRECT first to get actual rect,
// then centres that rect on the 320×480 (0x280×0x1e0) screen.
// Address: 0x00476e60
void Txt_SetMessage(int nFmtId)
{
    HDC     hdc;
    HGDIOBJ hOldFont;
    RECT    rcText;
    SIZE    sz;
    int     nColDiv;
    int     nColWidth;
    int     nW;

    if (!g_nTxtEnabled)
        return;

    Str_Format(g_szTxtMsgBuf, nFmtId, /* va_list from stack */ 0);
    g_pTxtCurStr          = g_szTxtMsgBuf;
    g_nTxtScrollSpeed     = 99999;

    hdc = DDI_GetHDC();
    if (!hdc)
        return;

    hOldFont = SelectObject(hdc, (HGDIOBJ)g_nTxtFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_nTxtColorGDI);

    // Compute column width
    nColDiv = 6;
    {
        size_t nLen = strlen(g_pTxtCurStr);
        GetTextExtentPoint32A(hdc, g_pTxtCurStr, (int)nLen, &sz);
        do {
            for (nColWidth = 213;
                 nColWidth < sz.cx / nColDiv;
                 nColWidth += 30)
                ;
            nColDiv++;
        } while (nColWidth > 0x26b);
    }

    // Box top=0, bottom=0x1df (479), left=0, right=nColWidth-1 (capped at 639)
    g_nTxtBoxTop    = 0;
    g_nTxtBoxBottom = 0x1df;
    g_nTxtBoxLeft   = 0;
    nW = (nColWidth - 1 < 0x27f) ? nColWidth - 1 : 0x27f;
    g_nTxtBoxRight  = nW;

    // DrawTextA with DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX to get actual rect
    rcText.left   = g_nTxtBoxLeft;
    rcText.top    = g_nTxtBoxTop;
    rcText.right  = g_nTxtBoxRight;
    rcText.bottom = g_nTxtBoxBottom;
    DrawTextA(hdc, g_pTxtCurStr, -1, &rcText, g_nTxtDrawFlags | 0xc10);

    SelectObject(hdc, hOldFont);
    DDI_ReleaseHDC(hdc);

    // Update draw flags
    g_nTxtDrawFlags = g_nTxtAlignMode | g_nTxtRTL;

    // Centre the computed rect on screen
    g_nTxtBoxTop    = (0x1e0 - g_nTxtBoxBottom) / 2;
    g_nTxtBoxBottom = g_nTxtBoxBottom + g_nTxtBoxTop;
    g_nTxtBoxLeft   = (0x280 - g_nTxtBoxRight) / 2;
    g_nTxtBoxRight  = g_nTxtBoxRight + g_nTxtBoxLeft;

    Debug_Trace(/* line */ 0x31, "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp",
                "txt %d %d %d %d",
                g_nTxtBoxTop, g_nTxtBoxBottom,
                g_nTxtBoxLeft, g_nTxtBoxRight);

    g_nTxtActive = 1;
}

// Txt_DrawShadow -- draw the current string with a 1-pixel drop shadow.
// Draws black offset at (+1,+1) then white at (0,0), using the cached
// g_nTxtDrawFlags | DT_WORDBREAK (0x810).
// Address: 0x00477160
void Txt_DrawShadow(void)
{
    HDC     hdc;
    HGDIOBJ hOldFont;
    RECT    rc;

    if (!g_nTxtEnabled || !g_nTxtActive)
        return;
    // Uses HDC from global surface handle (DAT_006b8f20)
    hdc = (HDC)(*(int *)0x006b8f20);  // g_nTxtHDC -- global shared HDC
    if (!hdc)
        return;

    SetBkMode(hdc, TRANSPARENT);
    hOldFont = SelectObject(hdc, (HGDIOBJ)g_nTxtFont);

    // Shadow (black, offset by -1,-1)
    SetTextColor(hdc, 0x000000);
    g_nTxtBoxTop--;  g_nTxtBoxBottom--;
    g_nTxtBoxLeft--; g_nTxtBoxRight--;

    Debug_Trace(/* line */ 0x18, "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp",
                "txt %d %d %d %d",
                g_nTxtBoxTop, g_nTxtBoxBottom,
                g_nTxtBoxLeft, g_nTxtBoxRight);

    rc.left   = g_nTxtBoxLeft;
    rc.top    = g_nTxtBoxTop;
    rc.right  = g_nTxtBoxRight;
    rc.bottom = g_nTxtBoxBottom;
    DrawTextA(hdc, g_pTxtCurStr, -1, &rc, g_nTxtDrawFlags | 0x810);

    // White text (original position)
    SetTextColor(hdc, 0xffffff);
    g_nTxtBoxTop++;  g_nTxtBoxBottom++;
    g_nTxtBoxLeft++; g_nTxtBoxRight++;

    rc.left   = g_nTxtBoxLeft;
    rc.top    = g_nTxtBoxTop;
    rc.right  = g_nTxtBoxRight;
    rc.bottom = g_nTxtBoxBottom;
    DrawTextA(hdc, g_pTxtCurStr, -1, &rc, g_nTxtDrawFlags | 0x810);

    SelectObject(hdc, hOldFont);
}

// Txt_Reset -- restart or clear the current text block.
// If g_nTxtDuration < 1, clears g_nTxtActive; otherwise re-calls
// Txt_SetString on the empty string DAT_007cd7f8 to restart the block.
// Address: 0x004773b0
void Txt_Reset(void)
{
    if (!g_nTxtEnabled)
        return;
    if ((int)g_nTxtDuration < 1) {
        g_nTxtActive = 0;
    } else {
        // thunk_FUN_004761d0(&DAT_007cd7f8, 0, 0)
        Txt_SetString((const char *)0 /* empty sentinel */, 0, 0);
    }
}

// Txt_PageAdvance -- scroll to the next page of text.
// Increments g_nTxtFirstLine by g_nTxtLinesPerPage.  If all lines have been
// shown, calls Txt_Reset.  Otherwise recalculates g_nTxtScrollSpeed for the
// remaining lines and updates g_nTxtScrollTotal.
// Address: 0x00477470
void Txt_PageAdvance(void)
{
    if (!g_nTxtEnabled)
        return;

    g_nTxtFirstLine += g_nTxtLinesPerPage;

    if (g_nTxtFirstLine < g_nTxtTotalLines) {
        int nLastLine = (g_nTxtLinesPerPage + g_nTxtFirstLine < g_nTxtTotalLines)
                        ? g_nTxtLinesPerPage + g_nTxtFirstLine
                        : g_nTxtTotalLines;

        /* scrollSpeed = duration * (lineEndPtr[nLastLine] - strStart) / strlen */
        size_t nStrLen = strlen(g_pTxtStrStart);
        /* g_nTxtScrollSpeed = g_nTxtDisplayDuration * lineEnd[nLastLine] / nStrLen - g_nTxtScrollTotal; */
        g_nTxtScrollTotal += g_nTxtScrollSpeed;

        Debug_Assert(/* line 0x13 */ 0x13,
                     "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp",
                     g_nTxtScrollSpeed);
        Debug_Assert(/* line 0x14 */ 0x14,
                     "C:\\DevStudio\\Projects\\Crux\\TEXT.cpp",
                     g_nTxtScrollTotal);
    } else {
        Txt_Reset();
    }
}

// Txt_Update -- per-frame text update and draw.
//
// If g_nTxtMode == 0 calls Txt_Reset and returns.
// Decrements g_nTxtScrollSpeed each frame; when it hits zero calls
// Txt_PageAdvance.
//
// Acquires the game HDC, selects g_nTxtFont, sets alignment via SetTextAlign,
// then iterates over visible lines [g_nTxtFirstLine, g_nTxtFirstLine +
// g_nTxtLinesPerPage).  Each line's (start, end) pointers come from the wrap
// table built by Txt_SetString.
//
// Lines that overlap [g_pTxtHighlightStart, g_pTxtHighlightEnd) are rendered
// in three colour-coded segments:
//   before highlight  : g_nTxtColorGDI
//   within highlight  : g_nTxtColorHighlight
//   after  highlight  : g_nTxtColorGDI
//
// Lines outside the highlight range are drawn with a 1-pixel black shadow
// (TextOutA at +1,+1) then coloured text at the base position.
//
// Address: 0x00477620
void Txt_Update(void)
{
    HDC     hdc;
    HGDIOBJ hOldFont;
    int     nLine, nEndLine;
    int     nX, nY;
    UINT    nAlign;
    char    *pStart, *pEnd;
    SIZE    sz;

    if (!g_nTxtEnabled || !g_nTxtActive)
        return;

    if (g_nTxtMode == 0) {
        Txt_Reset();
        return;
    }

    // Decrement scroll counter; advance page if expired
    if (g_nTxtScrollSpeed < 1)
        Txt_PageAdvance();
    g_nTxtScrollSpeed--;

    // Advance highlight range cursor based on scroll position
    if (*(int *)((char *)&g_nTxtColorGDI + g_nTxtHighlightIdx * 4) <= g_nTxtScrollPos) {
        /* advance g_pTxtHighlightStart / g_pTxtHighlightEnd / g_nTxtHighlightIdx */
    }
    g_nTxtScrollPos++;

    hdc = DDI_GetHDC();
    if (!hdc)
        return;

    SetBkMode(hdc, TRANSPARENT);
    hOldFont = SelectObject(hdc, (HGDIOBJ)g_nTxtFont);
    SetTextColor(hdc, g_nTxtColorGDI);

    // Compute X anchor and SetTextAlign flags
    if (g_nTxtAlignMode == 0) {
        nX     = g_nTxtBoxLeft;
        nAlign = 0;             // TA_LEFT
    } else if (g_nTxtAlignMode == 1) {
        nX     = (g_nTxtBoxLeft + g_nTxtBoxRight) / 2;
        nAlign = 6;             // TA_CENTER
    } else {
        nX     = g_nTxtBoxRight;
        nAlign = 2;             // TA_RIGHT
    }

    if (g_nTxtRTL)
        nAlign |= 0x100;        // TA_RTLREADING

    SetTextAlign(hdc, nAlign);

    // Draw each visible line
    nLine    = g_nTxtFirstLine;
    nEndLine = (g_nTxtFirstLine + g_nTxtLinesPerPage < g_nTxtTotalLines)
               ? g_nTxtFirstLine + g_nTxtLinesPerPage
               : g_nTxtTotalLines;

    nY = g_nTxtBoxTop;

    while (nLine < nEndLine) {
        // pStart / pEnd from wrap table (parallel (start,end) char* array)
        pStart = (char *)(*(int *)((char *)&g_nTxtSavedBoxBottom + 8 + nLine * 8));
        pEnd   = (char *)(*(int *)((char *)&g_nTxtSavedBoxBottom + 4 + nLine * 8));

        int nLineY = nY + (nLine - g_nTxtFirstLine) * g_nTxtLineHeight;

        if (!g_pTxtHighlightStart ||
            g_pTxtHighlightEnd   < pStart ||
            pEnd                <= g_pTxtHighlightStart)
        {
            // No highlight on this line -- shadow + colour
            SetTextColor(hdc, 0x000000);
            TextOutA(hdc, nX + 1, nLineY + 1, pStart, (int)(pEnd - pStart));
            SetTextColor(hdc, g_nTxtColorGDI);
            TextOutA(hdc, nX,     nLineY,     pStart, (int)(pEnd - pStart));
        } else {
            // Partial or full highlight
            GetTextExtentPoint32A(hdc, pStart, (int)(pEnd - pStart), &sz);

            int nDrawX = nX;
            int nDir   = 1;
            if (g_nTxtRTL) {
                nDrawX = nX;
                nDir   = -1;
                SetTextAlign(hdc, 0x102);
            } else {
                SetTextAlign(hdc, 0);
            }

            if (pStart < g_pTxtHighlightStart) {
                // Draw pre-highlight segment
                SetTextColor(hdc, g_nTxtColorGDI);
                GetTextExtentPoint32A(hdc, pStart,
                    (int)(g_pTxtHighlightStart - pStart), &sz);
                TextOutA(hdc, nDrawX, nLineY, pStart,
                         (int)(g_pTxtHighlightStart - pStart));
                nDrawX += sz.cx * nDir;

                // Draw highlight segment
                SetTextColor(hdc, g_nTxtColorHighlight);
                char *pHEnd = (pEnd <= g_pTxtHighlightEnd) ? pEnd : g_pTxtHighlightEnd;
                TextOutA(hdc, nDrawX, nLineY, g_pTxtHighlightStart,
                         (int)(pHEnd - g_pTxtHighlightStart));

                if (pEnd > g_pTxtHighlightEnd) {
                    GetTextExtentPoint32A(hdc, g_pTxtHighlightStart,
                        (int)(g_pTxtHighlightEnd - g_pTxtHighlightStart), &sz);
                    nDrawX += sz.cx * nDir;
                    SetTextColor(hdc, g_nTxtColorGDI);
                    TextOutA(hdc, nDrawX, nLineY, g_pTxtHighlightEnd,
                             (int)(pEnd - g_pTxtHighlightEnd));
                }
            } else if (pEnd > g_pTxtHighlightEnd) {
                // Highlight extends past start, some normal text at end
                SetTextColor(hdc, g_nTxtColorHighlight);
                TextOutA(hdc, nDrawX, nLineY, pStart,
                         (int)(g_pTxtHighlightEnd - pStart));
                GetTextExtentPoint32A(hdc, pStart,
                    (int)(g_pTxtHighlightEnd - pStart), &sz);
                nDrawX += sz.cx * nDir;
                SetTextColor(hdc, g_nTxtColorGDI);
                TextOutA(hdc, nDrawX, nLineY, g_pTxtHighlightEnd,
                         (int)(pEnd - g_pTxtHighlightEnd));
            } else {
                // Entire line is highlighted
                SetTextColor(hdc, g_nTxtColorHighlight);
                TextOutA(hdc, nDrawX, nLineY, pStart, (int)(pEnd - pStart));
            }

            SetTextAlign(hdc, nAlign);
        }

        nLine++;
    }

    SelectObject(hdc, hOldFont);
    DDI_ReleaseHDC(hdc);
}

// Txt_DrawBackground -- save the text area to the background surface.
// Creates an off-screen DirectDraw surface capturing the text bounding rect,
// stores the saved rect in g_nTxtSavedBox*, and updates the dirty rect
// outputs (*pTop = min(pTop, boxTop), *pBottom = max(pBottom, boxBottom)).
// Returns without action if the subsystem is not active, mode is off, or
// g_nTxtSavedBoxTop != -1 (already saved).
// Address: 0x00477f00
void Txt_DrawBackground(int *pTop, int *pBottom)
{
    if (!g_nTxtEnabled || !g_nTxtActive || !g_nTxtMode) {
        g_nTxtSavedBoxTop = -1;
        return;
    }

    g_nTxtSavedBoxLeft   = g_nTxtBoxLeft;
    g_nTxtSavedBoxTop    = g_nTxtBoxTop;
    g_nTxtSavedBoxRight  = g_nTxtBoxRight;
    g_nTxtSavedBoxBottom = g_nTxtBoxBottom;

    int nSurf = DDI_CreateOffscreenSurf(
        g_nTxtBoxLeft, g_nTxtBoxTop,
        (g_nTxtBoxRight  - g_nTxtBoxLeft) + 1,
        (g_nTxtBoxBottom - g_nTxtBoxTop)  + 1,
        g_nTxtBoxLeft, g_nTxtBoxTop, 0);
    DDI_BlitSurface(g_nTxtBgSurface, nSurf);

    int nNewTop    = (*pTop    < g_nTxtSavedBoxTop)    ? *pTop    : g_nTxtSavedBoxTop;
    *pTop    = nNewTop;
    int nNewBottom = (*pBottom > g_nTxtSavedBoxBottom) ? *pBottom : g_nTxtSavedBoxBottom;
    *pBottom = nNewBottom;
}

// Txt_EraseBackground -- restore the background from the save surface.
// Only acts if g_nTxtSavedBoxTop != -1.  Blits back and updates dirty rect.
// Address: 0x004780c0
void Txt_EraseBackground(int *pTop, int *pBottom)
{
    if (!g_nTxtEnabled || !g_nTxtActive ||
        g_nTxtSavedBoxTop == -1        || !g_nTxtMode)
        return;

    int nSurf = DDI_CreateOffscreenSurf(
        g_nTxtBgSurface,
        g_nTxtSavedBoxLeft, g_nTxtSavedBoxTop,
        (g_nTxtSavedBoxRight  - g_nTxtSavedBoxLeft) + 1,
        (g_nTxtSavedBoxBottom - g_nTxtSavedBoxTop)  + 1,
        g_nTxtSavedBoxLeft, g_nTxtSavedBoxTop, 0);
    DDI_BlitSurface(nSurf);

    int nNewTop    = (*pTop    < g_nTxtSavedBoxTop)    ? *pTop    : g_nTxtSavedBoxTop;
    *pTop    = nNewTop;
    int nNewBottom = (*pBottom > g_nTxtSavedBoxBottom) ? *pBottom : g_nTxtSavedBoxBottom;
    *pBottom = nNewBottom;
}

// Txt_IsDone -- return 1 if text display is fully complete.
// Combines two conditions: no active text (g_nTxtActive==0) OR the @-prefix
// flag is clear while active (g_nTxtIsDirect==0 and g_nTxtActive!=0 → still
// animating → NOT done).  Effectively: done when inactive OR when direct+active.
// Address: 0x00478240
int Txt_IsDone(void)
{
    if (g_nTxtIsDirect == 0 && g_nTxtActive != 0)
        return 0;
    return 1;
}

// ===========================================================================
// THEME globals
// ===========================================================================

int g_nThmReady;               // 0x007d3950
int g_nThmIndex;               // 0x007d3a90
int g_nThmSegmentIdx;          // 0x007d3970
int g_nThmSegmentCount;        // 0x007d3954
int g_nThmNameCount;           // 0x007d3958
int g_nThmLabelCount;          // 0x007d395c
int g_nThmImportLabelCount;    // 0x007d3960
int g_nThmEventNameCount;      // 0x007d3964
int g_nThmCommandCount;        // 0x007d3968
int g_nThmEventCount;          // 0x007d396c
int g_nThmCurrentSegmentData;  // 0x007d347c
int g_nThmCurrentSegmentLen;   // 0x007d394c
int g_nThmCurrentSegmentFlags; // 0x007d026c
int g_nThmPlayEvent;           // 0x007d3978

// ===========================================================================
// THEME implementation
// ===========================================================================

// Thm_SetIndex -- set the current theme index.
// Address: 0x004782f0
void Thm_SetIndex(int nIndex)
{
    g_nThmIndex = nIndex;
}

// Thm_PlayNextSegment -- advance to and play the next music segment.
// Increments g_nThmSegmentIdx, loads the segment resource data and flags
// from the per-segment table, calls Audio_PlayMidi, registers itself as the
// completion callback via Audio_SetCallback, and signals g_nThmPlayEvent
// (if nSignalEvent != -1).
// Address: 0x00478380
void Thm_PlayNextSegment(int nSignalEvent)
{
    // Early-out if music system is paused (DAT_00629f58)
    /* if (g_nMusicPaused) return; */

    /* g_nThmSegmentIdx advanced from next-segment table */
    g_nThmSegmentIdx++;

    Debug_Trace(/* line */ 0x13,
                "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                "playing %s",
                /* segment name */ 0);

    // Compute Audio_PlayMidi flags from g_nThmCurrentSegmentFlags bits:
    //   bit 1 → loop flag
    //   bits 0 → volume shift (+8 if set, else 8)
    //   bits 2 → frequency multiplier (*2 if set)
    Audio_PlayMidi(
        0,                      // channel
        g_nThmCurrentSegmentData,
        g_nThmCurrentSegmentLen,
        (int)(g_nThmCurrentSegmentFlags & 2) >> 1,
        (g_nThmCurrentSegmentFlags & 1) * 8 + 8,
        (((int)(g_nThmCurrentSegmentFlags & 4) >> 2) + 1) * 0x5622,
        0xffffffff,
        0x32);

    Audio_SetCallback(0, (void *)Thm_PlayNextSegment);

    if (nSignalEvent != -1)
        SetEvent((HANDLE)g_nThmPlayEvent);
}

// Thm_FindLabel -- search for pszLabel in a string table.
// Returns the index of the matching entry (0-based) or -1 if not found or
// if pszLabel is NULL or matches the sentinel string DAT_007d3a98.
// Address: 0x00478a70
int Thm_FindLabel(int pTable, int nCount, int pszLabel)
{
    int i;

    if (pszLabel == 0 ||
        Str_Compare(pszLabel, /* sentinel DAT_007d3a98 */ 0) == 0)
        return -1;

    for (i = 0; i < nCount; i++) {
        if (Str_Compare(*(int *)(pTable + i * 4), pszLabel) == 0)
            break;
    }

    if (i >= nCount)
        return -1;

    return i;
}

// Thm_FreeStringTable -- free all strings in a pointer array.
// Walks pTable[0..*pCount] and calls Heap_Free on each entry.
// Decrements *pCount to zero.
// Address: 0x00479810
void Thm_FreeStringTable(int pTable, int *pCount)
{
    while (*pCount > 0) {
        (*pCount)--;
        Heap_Free(/* line */ 5,
                  "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                  (void *)*(int *)(pTable + *pCount * 4));
    }
}

// Thm_FreeData -- free all loaded theme data tables.
// Calls Thm_FreeStringTable for each of the 5 string tables, then frees
// all command and event block arrays.
// Address: 0x00479680
void Thm_FreeData(void)
{
    Thm_FreeStringTable(/* DAT_007cfa80 */ 0, &g_nThmSegmentCount);
    Thm_FreeStringTable(/* DAT_007d37a0 */ 0, &g_nThmNameCount);
    Thm_FreeStringTable(/* DAT_007cf440 */ 0, &g_nThmLabelCount);
    Thm_FreeStringTable(/* DAT_007cebe8 */ 0, &g_nThmImportLabelCount);
    Thm_FreeStringTable(/* DAT_007cff40 */ 0, &g_nThmEventNameCount);

    while (g_nThmCommandCount > 0) {
        g_nThmCommandCount--;
        Heap_Free(/* line */ 0xb,
                  "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                  /* DAT_007d0ef8[g_nThmCommandCount] */ 0);
    }

    while (g_nThmEventCount > 0) {
        g_nThmEventCount--;
        Heap_Free(/* line */ 0x11,
                  "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                  /* DAT_007d1538[g_nThmEventCount] */ 0);
    }
}

// Thm_ReadTable -- read a string table from a binary stream.
// Reads a 4-byte count, then for each entry: a 4-byte length followed by
// that many bytes of string data (null-terminated by appending 0).
// Allocates each string via Heap_Alloc.
// Returns 0 on success, -1 on stream read error.
// Address: 0x00479430
int Thm_ReadTable(int pszTableName, int pStream, int pTable, int *pCount, int nMax)
{
    int nLen;

    // Read count (4 bytes)
    if (INI_ReadInt(pCount, /* stream */ 0, 4, (void *)pStream) != 0)
        return -1;

    if (*pCount >= nMax) {
        Fatal_Error(/* line */ 0xb, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
        /* allocate error object */
    }

    for (int i = 0; i < *pCount; i++) {
        // Read string length
        if (INI_ReadInt(&nLen, /* stream */ 0, 4, (void *)pStream) != 0)
            return -1;

        // Allocate and read string data
        void *pStr = Heap_Alloc(/* line */ 0x13,
                                 "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                                 nLen + 1);
        *(int *)(pTable + i * 4) = (int)pStr;

        if (!pStr) {
            Fatal_Error(/* line */ 0x16, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
        }

        if (INI_ReadInt(pStr, /* stream */ 0, nLen, (void *)pStream) != 0)
            return -1;

        // Null-terminate
        *(char *)((char *)pStr + nLen) = '\0';
    }

    return 0;
}

// Thm_LoadTheme -- load a theme data file by name.
// Calls Thm_FreeData first, then INI_Open on the .thm file.
// Reads: segment names, theme names, labels, label offsets (int array),
// import labels, event names, command blocks, event blocks.
// Returns 0 on success, -1 on any read failure.
// Address: 0x00478b70
int Thm_LoadTheme(int pszName)
{
    int rc;
    /* local INI context and key buffers on stack */
    char szSection[16];
    char szBuf[256];

    Thm_FreeData();

    // Open the theme file
    if (INI_Open(0xc, (const char *)pszName, /* keys */ 0) != 0)
        return -1;

    // Read all string tables
    rc  = Thm_ReadTable(/* "Theme segment names" */ 0, /* stream */ 0,
                        /* DAT_007cfa80 */ 0, &g_nThmSegmentCount, 300);
    rc |= Thm_ReadTable(/* "Theme names" */ 0, /* stream */ 0,
                        /* DAT_007d37a0 */ 0, &g_nThmNameCount, 100);
    rc |= Thm_ReadTable(/* "Theme labels" */ 0, /* stream */ 0,
                        /* DAT_007cf440 */ 0, &g_nThmLabelCount, 400);

    // Read label offset int array (4 bytes * count)
    INI_ReadInt(/* DAT_007ce5a0 */ 0, 4, g_nThmLabelCount, /* stream */ 0);

    rc |= Thm_ReadTable(/* "Theme import labels" */ 0, /* stream */ 0,
                        /* DAT_007cebe8 */ 0, &g_nThmImportLabelCount, 400);
    rc |= Thm_ReadTable(/* "Theme event names" */ 0, /* stream */ 0,
                        /* DAT_007cff40 */ 0, &g_nThmEventNameCount, 200);

    // Read command count and command blocks (max 400)
    INI_ReadInt(&g_nThmCommandCount, 4, 1, /* stream */ 0);
    if (g_nThmCommandCount > 400) {
        Fatal_Error(/* line */ 0x32, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
    }
    for (int i = 0; i < g_nThmCommandCount; i++) {
        void *pCmd = Heap_Alloc(/* line */ 0x36,
                                 "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                                 0x14);
        /* DAT_007d0ef8[i] = pCmd; */
        if (!pCmd) {
            Fatal_Error(/* line */ 0x39, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
        }
        INI_ReadInt(pCmd, 1, 0x14, /* stream */ 0);
    }

    // Read event count and event blocks (max 2000)
    INI_ReadInt(&g_nThmEventCount, 4, 1, /* stream */ 0);
    if (g_nThmEventCount > 2000) {
        Fatal_Error(/* line */ 0x45, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
    }
    for (int i = 0; i < g_nThmEventCount; i++) {
        void *pEvt = Heap_Alloc(/* line */ 0x49,
                                 "C:\\DevStudio\\Projects\\Crux\\THEME.cpp",
                                 0x10);
        /* DAT_007d1538[i] = pEvt; */
        if (!pEvt) {
            Fatal_Error(/* line */ 0x4c, "C:\\DevStudio\\Projects\\Crux\\THEME.cpp");
        }
        INI_ReadInt(pEvt, 1, 0x10, /* stream */ 0);
    }

    return rc;
}

// Thm_Play -- load and start playing a named theme.
// 1. If pszLabel is non-NULL and matches the current label, strip the
//    extension from pszName and find the '.' separator.
// 2. Lock two critical sections (g_nThmCS, g_nThmCFCS).
// 3. Stop any playing music and free active audio resources.
// 4. Call Thm_LoadTheme on the stripped name.
// 5. If pszLabel given, call Thm_FindLabel to get starting offset.
// 6. Call Thm_SetIndex(0) and signal g_nThmPlayEvent if DAT_004da788 set.
// Address: 0x00478580
void Thm_Play(char *pszName, char *pszLabel)
{
    int nLabelIdx;
    char *pExt;

    if (!g_nThmReady)
        return;
    /* if (g_nMusicPaused) return; */

    // Strip extension if pszLabel is null or matches current
    if (pszLabel == NULL ||
        Str_Compare((int)pszLabel, /* DAT_007d3aac */ 0) == 0) {
        pExt = strchr(pszName, '.');
        if (pExt) {
            *pExt = '\0';
            pszLabel = pExt + 1;
        }
    }

    // Acquire critical sections
    EnterCriticalSection((LPCRITICAL_SECTION)0);  // DAT_007d3930
    EnterCriticalSection((LPCRITICAL_SECTION)0);  // DAT_007cf428

    // Stop current music
    if (Audio_IsPlaying(0))
        Audio_Stop(0);

    /* free active audio handles */
    Audio_SetVolume(0, /* DAT_004da780 */ 0, -1);

    // Reset theme state
    g_nThmSegmentIdx = 0;
    g_nThmIndex      = -1;
    /* reset all segment tables */

    // Load new theme
    nLabelIdx = Thm_LoadTheme((int)pszName);
    if (nLabelIdx == -1) {
        LeaveCriticalSection((LPCRITICAL_SECTION)0);
        LeaveCriticalSection((LPCRITICAL_SECTION)0);
        return;
    }

    // Find starting label offset
    if (pszLabel == NULL) {
        /* DAT_004da76c = -1; */
    } else {
        nLabelIdx = Thm_FindLabel(/* DAT_007cf440 */ 0,
                                   g_nThmLabelCount,
                                   (int)pszLabel);
        if (nLabelIdx == -1) {
            /* DAT_004da76c = -1; */
        } else {
            /* DAT_004da76c = label_offsets[nLabelIdx] - 1; */
        }
    }

    Thm_SetIndex(0);

    /* if (DAT_004da788) SetEvent(g_nThmPlayEvent); */
    SetEvent((HANDLE)g_nThmPlayEvent);

    LeaveCriticalSection((LPCRITICAL_SECTION)0);
    LeaveCriticalSection((LPCRITICAL_SECTION)0);
}
