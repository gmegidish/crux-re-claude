// Sprite.h — decode the engine's paletted RLE sprite (Img.cpp format).
//
// Header:  [u8 type][u16 width][u16 height]  then row-groups:
//   repeat { [u16 yskip][u16 nrows]  nrows lines }  until nrows == 0
// Each line begins with a codec byte (PutLine_Indi):
//   0 = Direct  : width raw bytes
//   1 = RLE     : tokens int8 count (<0 literal | >0 repeat next | 0 end-of-line)
//   2 = RLE_Skip: tokens (<0 literal | >0 skip dst | 0 end)   ← leaves dst pixels = delta
//   3 = RLE_Off : [u8 leadOffset] then tokens, 0xFF ends, else byte = skip
//   4 = skip whole line
//
// Pixels are written as raw 8-bit indices into the framebuffer (the RGB palette
// is applied at present time). Delta frames rely on codec 2/3 leaving untouched
// pixels, so the caller must NOT clear the framebuffer between video frames.
#pragma once
#include "Framebuffer.h"
#include <cstdint>
#include <cstddef>

// Decode one sprite blob onto fb (640x480 index buffer) with the sprite's
// top-left at (dstX, dstY) — Help_BlitImage's clip origin. SCM full-screen
// frames use (0,0); positioned anim frames pass their screen position.
// Returns false on malformed data. spriteW/spriteH receive the header dimensions.
bool decodeSprite(const uint8_t* data, size_t size, Framebuffer& fb,
                  int dstX = 0, int dstY = 0,
                  int* spriteW = nullptr, int* spriteH = nullptr);
