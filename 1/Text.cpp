#include "Text.h"

namespace {

// Mirrors g_nTxtAlign (0x004da160). The engine's default is left (0); set by
// Txt_SetAlign and consumed when laying out a text block's horizontal position.
int g_align = 0;

// Mirrors g_nTxtMode — the text subsystem enable/mode flag (Txt_SetMode).
int g_mode = 0;

}  // namespace

namespace Text {

void setAlign(int align) { g_align = align; }
int  align() { return g_align; }

void setMode(int mode) { g_mode = mode; }
int  mode() { return g_mode; }

}  // namespace Text
