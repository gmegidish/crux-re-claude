// TextRender.h — Hebrew text rasterizer for subtitles (clean-room substitute for
// the engine's Win32 GDI text path).
//
// The original drew text with GDI (CreateFontA HEBREW_CHARSET, TextOutA with
// TA_RTLREADING + a 1px black drop shadow). The port has no GDI, so we rasterize
// a system Hebrew TTF with stb_truetype, lay the line out right-to-left, and blit
// into the 8-bit framebuffer using the nearest-white (text) and nearest-black
// (shadow) palette indices.
#pragma once
#include "Framebuffer.h"
#include <string>

namespace TextRender {

// Load a system Hebrew TTF. Returns false if none is found.
bool init();
bool ready();

// Render a CP1255 sentence into fb: centered horizontally on centerX, with the
// text baseline at baselineY, laid out RTL with a drop shadow. No-op if not ready.
void drawSentence(Framebuffer& fb, const std::string& cp1255, int centerX, int baselineY);

}  // namespace TextRender
