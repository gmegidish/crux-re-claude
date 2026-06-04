#ifndef TEXT_H
#define TEXT_H

// ---------------------------------------------------------------------------
// TEXT.h  --  GDI-based scrolling text display + music theme manager
// Original: C:\DevStudio\Projects\Crux\TEXT.cpp
// RE range (text):  0x00475970 -- 0x00478240
// RE range (theme): 0x004782f0 -- 0x00479810
// ---------------------------------------------------------------------------
//
// Two subsystems live in this translation unit:
//
//  Txt_*  : Animated, auto-wrapped, scrolling text overlay using Win32 GDI.
//           Strings are looked up by name from an external table, or supplied
//           directly with an '@' prefix.  Text is drawn with a 1-pixel drop
//           shadow (black then coloured) via DrawTextA / TextOutA onto the
//           game's HDC.  Alignment (left/center/right), right-to-left flag,
//           per-character scroll speed, multi-page support, and a highlight
//           range (for cursor-driven selection) are all supported.
//
//  Thm_*  : Music theme manager.  A theme is a binary data file containing
//           named segments, labels, import-labels, command blocks, and event
//           blocks.  The manager loads the file, keeps string tables for each
//           category, and drives segment-to-segment transitions by calling the
//           audio playback API on a background thread synchronised via a Win32
//           event.
//
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ===========================================================================
// TEXT subsystem globals
// ===========================================================================

// --- Subsystem enable/mode flags -------------------------------------------
extern int g_nTxtEnabled;       // 0x007cd7c4  master enable (all Txt_ check this first)
extern int g_nTxtActive;        // 0x007cd7c0  1 = a string is set and ready to display
extern int g_nTxtIsDirect;      // 0x007cd7bc  1 = string set via @-prefix (literal)
extern int g_nTxtMode;          // 0x004da164  subsystem mode flag; 0 disables layout
extern int g_nTxtRTL;           // 0x007cd7c8  right-to-left draw flag (DT_RTLREADING)

// --- Font ------------------------------------------------------------------
extern int g_nTxtFont;          // 0x007cd7b4  HFONT created by Txt_CreateFont
extern int g_nTxtFontHeight;    // 0x004da16c  font height passed to CreateFontA
extern int g_nTxtCharSet;       // 0x007ca874  Windows charset for CreateFontA
extern int g_nTxtLineHeight;    // 0x007ca38c  tmHeight + tmExternalLeading (pixels)
extern int g_nTxtFontWidthUnit; // 0x007ca884  font metric denominator for scroll speed

// --- Colour ----------------------------------------------------------------
extern int g_nTxtColorGDI;      // 0x007c93b8  COLORREF (6-bit per channel, packed)
extern int g_nTxtColorHighlight;// 0x007ca880  colour for highlighted text span

// --- String state ----------------------------------------------------------
extern char *g_pTxtCurStr;      // 0x007cc800  pointer to current position in string
extern char *g_pTxtStrStart;    // 0x007ca85c  first non-whitespace char (trimmed start)
extern char *g_pTxtStrEnd;      // 0x007ca87c  last non-whitespace char  (trimmed end)
extern int   g_nTxtStringCount; // 0x007cc7d8  number of entries in lookup table
extern int   g_nTxtStringKeys;  // 0x007c93bc  base of key ID array (parallel to values)
extern int   g_nTxtStringValues;// 0x007ca78c  base of char* value array
extern char  g_abTxtDirectBuf[];// 0x007ca790  buffer for @-prefix literal strings
extern char *g_szTxtMsgBuf;     // 0x007ca398  printf-format message buffer

// --- Layout / scroll -------------------------------------------------------
extern int g_nTxtMaxLines;      // 0x004da168  maximum wrapped lines per block (dflt 1000)
extern int g_nTxtAlign;         // 0x004da160  alignment: 0=left 1=center 2=right
extern int g_nTxtAlignMode;     // 0x007ca878  alignment cached for current draw pass
extern int g_nTxtDrawFlags;     // 0x007ca780  DrawTextA flags (align | RTL)
extern int g_nTxtLinesPerPage;  // 0x007cd7d0  visible lines per page
extern int g_nTxtTotalLines;    // 0x007cd7d4  total wrapped lines in current string
extern int g_nTxtFirstLine;     // 0x007cd7d8  first visible line index (paging)
extern int g_nTxtScrollSpeed;   // 0x007ca788  characters revealed per tick
extern int g_nTxtScrollPos;     // 0x007ca858  current tick counter (Txt_Update increments)
extern int g_nTxtScrollTotal;   // 0x007ca390  accumulated baseline for paging
extern int g_nTxtDisplayDuration;// 0x007cc804 total display ticks from Txt_SetString
extern int g_nTxtDuration;      // 0x007cd7b8  duration cached inside the module

// --- Bounding rect ---------------------------------------------------------
extern int g_nTxtRectLeft;      // 0x007cc7f0  explicit rect (Txt_SetRect); -1 = auto
extern int g_nTxtRectTop;       // 0x007cc7f4
extern int g_nTxtRectRight;     // 0x007cc7f8
extern int g_nTxtRectBottom;    // 0x007cc7fc
extern int g_nTxtBoxLeft;       // 0x007ca860  computed/clamped draw box
extern int g_nTxtBoxTop;        // 0x007ca864
extern int g_nTxtBoxRight;      // 0x007ca868
extern int g_nTxtBoxBottom;     // 0x007ca86c

// --- Background save -------------------------------------------------------
extern int g_nTxtBgSurface;     // 0x007ca360  DirectDraw surface for save-behind
extern int g_nTxtSurfaceWidth;  // 0x007ca394  surface width (scroll speed normalisation)
extern int g_nTxtSavedBoxLeft;  // 0x007ca888  saved box coords for erase restore
extern int g_nTxtSavedBoxTop;   // 0x007ca88c  -1 = nothing saved
extern int g_nTxtSavedBoxRight; // 0x007ca890
extern int g_nTxtSavedBoxBottom;// 0x007ca894

// --- Highlight range -------------------------------------------------------
extern char *g_pTxtHighlightStart; // 0x007ca784  start of highlight span (NULL=none)
extern char *g_pTxtHighlightEnd;   // 0x007ca870  end   of highlight span
extern int   g_nTxtHighlightIdx;   // 0x007cc808  index into highlight range table

// --- Saved rect for @-prefix path ------------------------------------------
extern int g_nTxtSavedRectLeft; // 0x007cc7e0

// ===========================================================================
// TEXT subsystem public API
// ===========================================================================

// Set the four-coordinate explicit text rect.  Pass all -1 to use auto-layout.
void Txt_SetRect(int nLeft, int nTop, int nRight, int nBottom);

// Set maximum wrapped lines per block (-1 resets to default 1000).
void Txt_SetMaxLines(int nMax);

// Get current maximum wrapped lines per block.
int  Txt_GetMaxLines(void);

// Get current alignment value (0=left 1=center 2=right).
int  Txt_GetAlign(void);

// Set alignment (0=left 1=center 2=right).
void Txt_SetAlign(int nAlign);

// Set the text subsystem mode / enabled flag.
// thunk_FUN_00475c70 is called from the startup CRT thunk — this initialises
// the text subsystem.
void Txt_SetMode(int nMode);

// Get the text subsystem mode.
int  Txt_GetMode(void);

// Create the GDI font (CreateFontA).  nPitchFamily is ORed with 2 and
// combined with the low byte of nPitchFamily.
void Txt_CreateFont(int nPitchFamily);

// Set text colour from three 6-bit RGB components.
// Packs two separate COLORREF formats: g_nTxtColorGDI (<<2/<<10/<<18) and
// g_nTxtColorHighlight (<<1/<<9/<<17).
void Txt_SetColor(int nR, int nG, int nB);

// Look up a string by name in the external string table.
// Trims leading/trailing whitespace from the found string.
// Sets g_pTxtCurStr, g_pTxtStrStart, g_pTxtStrEnd.
// Returns 0 on success, -1 if not found or system disabled.
int  Txt_LookupString(int nNameId);

// Set the current display string.
// pszName may start with '@' to supply a literal string, otherwise it is
// looked up via Txt_LookupString.  pRect points to a 4-int bounding rect
// (or NULL for auto).  nDuration is the tick count for scrolling.
// Calls Txt_ResolveDirectString / Txt_LookupString, wraps the text with
// GetTextExtentPoint32A, computes g_nTxtScrollSpeed, and resets scroll state.
void Txt_SetString(const char *pszName, int *pRect, int nDuration);

// Internal: resolve the @-prefix direct string buffer.
// Copies the string to g_abTxtDirectBuf, trims whitespace, and sets
// g_pTxtCurStr / g_pTxtStrStart / g_pTxtStrEnd.
void Txt_ResolveDirectString(void);

// Return 1 if the text is active but has not yet finished scrolling
// (i.e. there are more characters to reveal).
int  Txt_IsScrollPending(void);

// Set a formatted message string (printf-style).
// Calls vsprintf into g_szTxtMsgBuf, auto-computes bounding rect,
// and centres the text on screen.
void Txt_SetMessage(int nFmtId);

// Draw the current string with a drop shadow (black offset +1,+1 then
// coloured at 0,0) directly onto the game HDC.
// Uses the HDC from the global surface -- called as a shadow pass only.
void Txt_DrawShadow(void);

// Reset / restart the current text block.
// If g_nTxtDuration < 1 clears g_nTxtActive, otherwise re-calls Txt_SetString
// on the empty string to restart.
void Txt_Reset(void);

// Advance text display by one page.
// Increments g_nTxtFirstLine by g_nTxtLinesPerPage.  If all lines have been
// shown, calls Txt_Reset; otherwise recalculates g_nTxtScrollSpeed for the
// remaining lines.
void Txt_PageAdvance(void);

// Per-frame text update and draw.
// Decrements g_nTxtScrollSpeed, advances the highlight range, acquires the
// game HDC, selects the font, and draws all visible lines with TextOutA.
// Lines overlapping the highlight range [g_pTxtHighlightStart, g_pTxtHighlightEnd)
// are split and drawn in g_nTxtColorHighlight.  Calls Txt_PageAdvance when
// the scroll counter reaches zero.
void Txt_Update(void);

// Blit the background rectangle to save-behind surface, output dirty rect.
// If system/active/mode checks pass, calls DDI_CreateOffscreenSurf and blits
// the text area to g_nTxtBgSurface.  Updates *pTop / *pBottom clamps.
void Txt_DrawBackground(int *pTop, int *pBottom);

// Erase the text area by restoring from the save-behind surface.
// Only acts if g_nTxtSavedBoxTop != -1.  Updates *pTop / *pBottom clamps.
void Txt_EraseBackground(int *pTop, int *pBottom);

// Return 1 if the current text display is fully done (no more scroll pending
// and either no active string or the @-prefix flag is not set while active).
int  Txt_IsDone(void);

// ===========================================================================
// THEME subsystem globals
// ===========================================================================

extern int g_nThmReady;              // 0x007d3950  theme subsystem initialised
extern int g_nThmIndex;              // 0x007d3a90  current theme index
extern int g_nThmSegmentIdx;         // 0x007d3970  currently playing segment index
extern int g_nThmSegmentCount;       // 0x007d3954  number of segment name strings
extern int g_nThmNameCount;          // 0x007d3958  number of theme name strings
extern int g_nThmLabelCount;         // 0x007d395c  number of label strings
extern int g_nThmImportLabelCount;   // 0x007d3960  number of import-label strings
extern int g_nThmEventNameCount;     // 0x007d3964  number of event name strings
extern int g_nThmCommandCount;       // 0x007d3968  number of command blocks (max 400)
extern int g_nThmEventCount;         // 0x007d396c  number of event blocks (max 2000)
extern int g_nThmCurrentSegmentData; // 0x007d347c  resource handle for current segment
extern int g_nThmCurrentSegmentLen;  // 0x007d394c  length of current segment data
extern int g_nThmCurrentSegmentFlags;// 0x007d026c  flags (loop/channel/priority)
extern int g_nThmPlayEvent;          // 0x007d3978  Win32 event signalled on theme start

// ===========================================================================
// THEME subsystem public API
// ===========================================================================

// Set the active theme index.
void Thm_SetIndex(int nIndex);

// Play the next music segment in sequence.
// Advances g_nThmSegmentIdx, loads segment resource, calls audio play API,
// registers Thm_PlayNextSegment as the completion callback, and optionally
// signals g_nThmPlayEvent (if param != -1).
void Thm_PlayNextSegment(int nSignalEvent);

// Load and start playing a named theme, optionally seeking to a label.
// pszName is the theme filename (extension stripped); pszLabel is the
// starting label or NULL.  Stops any current music, frees old data, calls
// Thm_LoadTheme, then Thm_FindLabel, then signals g_nThmPlayEvent.
void Thm_Play(char *pszName, char *pszLabel);

// Search for a label string pszLabel in the string table starting at
// pTable[0..nCount].  Returns the index on success, -1 if not found.
int  Thm_FindLabel(int pTable, int nCount, int pszLabel);

// Load a theme data file by name.  Reads all string tables (segment names,
// theme names, labels, import labels, event names), command blocks, and
// event blocks.  Returns 0 on success, -1 on read error.
int  Thm_LoadTheme(int pszName);

// Read a null-terminated string table from the binary stream.
// pszTableName is used for error messages; pTable receives the char* array;
// *pCount receives the count; nMax is the maximum allowed entries.
// Returns 0 on success, -1 on read error.
int  Thm_ReadTable(int pszTableName, int pStream, int pTable, int *pCount, int nMax);

// Free all loaded theme data: all string tables, command blocks, event blocks.
void Thm_FreeData(void);

// Free a single string pointer array (pTable[0..*pCount]).
// Calls heap free on each entry and decrements *pCount.
void Thm_FreeStringTable(int pTable, int *pCount);

#endif // TEXT_H
