// Text.h — text subsystem state (clean-room port of TEXT.cpp).
//
// The engine keeps text layout state in globals (g_nTxtAlign, g_nTxtMode, ...).
// We mirror the pieces the script VM touches as they're needed. Glyph rendering
// isn't built yet; this holds the state the drawing path will consume.
#pragma once

namespace Text {

// Horizontal alignment for drawn text: 0 = left, 1 = center, 2 = right
// (mirrors g_nTxtAlign / Txt_SetAlign in TEXT.cpp). Default is left.
void setAlign(int align);
int  align();

// Text subsystem mode / enabled flag (mirrors g_nTxtMode / Txt_SetMode). When 0
// the GDI layout path is skipped entirely.
void setMode(int mode);
int  mode();

}  // namespace Text
