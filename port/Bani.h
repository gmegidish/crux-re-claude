// Bani.h — BANI block-codec decoder (delta video frames, sprite tag 4).
//
// Help_BlitImage delegates tag-4 sprites to Bani_DrawBlocks. A BANI image is a
// grid of blocks decoded in boustrophedon (serpentine) order with bit-width-
// specialised RLE. Used by the SCM video deltas (and room background anims).
//
// Header: [u8 tag][u16 W][u16 H][u16 blockW][u16 blockH] then row-major blocks.
//
// decodeBaniSprite writes 8-bit indices into the framebuffer (delta: leaves
// untouched pixels), matching Bani_DrawBlocks. Returns false on malformed data.
#pragma once
#include "Framebuffer.h"
#include <cstdint>
#include <cstddef>

bool decodeBaniSprite(const uint8_t* data, size_t size, Framebuffer& fb,
                      int dstX = 0, int dstY = 0);
