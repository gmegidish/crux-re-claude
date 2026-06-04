// HelpBlit.h — faithful port of the engine's Help_BlitImage sprite blitter.
//
// This is the sprite path used by ALL animations (.ANI, type 7) and cursors
// (type 2) — distinct from the SCM-video sprite path handled by decodeSprite()
// in Sprite.cpp. The two formats differ in their headers and in how the leading
// per-line codec byte 0 is interpreted:
//
//   * SCM video sprite (Sprite.cpp): header [u8 type][u16 w][u16 h]; line codec
//     0 copies `width` raw bytes.
//   * Help_BlitImage (this file): header [u8 tag][s16 colorBank][u16 unused]
//     (5 bytes) then strips; line codec 0 copies `colorBank` raw bytes.
//
// Ground truth: src/HELPSTK.cpp (Help_BlitImage / Help_PutLine / the four
// Help_Line* decoders), confirmed against CRUX.EXE @ 0x004398e0/0x00439980/
// 0x00439ad0/0x00439bd0/0x00439f70.
//
// IMPORTANT: despite the "colorBank" name, the engine does NOT add colorBank to
// any pixel. colorBank is the literal-run byte count passed to Help_LineCopyRun
// (opcode 0). Pixels are written verbatim as 8-bit palette indices. There is no
// index-0 transparency; transparency is structural (strip yAdvance skips rows,
// and opcodes 2/3/7/8 advance the destination over untouched gaps).
#pragma once
#include "Framebuffer.h"
#include <cstdint>
#include <cstddef>

// Blit one Help_BlitImage stream onto fb (640x480 index buffer) with the
// image's clip origin at (dstX, dstY). Handles tag 3 / 0x20 (' ') as
// "nothing to draw" and delegates tag 4 to decodeBaniSprite (Bani.h).
// Returns false only on malformed data (e.g. a bad line opcode).
bool blitHelpImage(const uint8_t* blob, size_t size, Framebuffer& fb,
                   int dstX, int dstY);
