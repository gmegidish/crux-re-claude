// Palette.h — load a type-3 .PAL resource and apply it to a Framebuffer.
//
// Format (Files_LoadPal, 0x00424ca0): [18-byte header][RLE-compressed payload].
// RLE escape byte 0xFF: (0xFF, value, count) emits `count` copies of `value`;
// any other byte is literal. Decompresses to 768 bytes = 256 x 3 (6-bit RGB,
// 0..63), expanded to 8-bit via (v<<2)|(v>>4). Used for the UI "GENERAL" palette
// and the per-screen palettes (e.g. "OPTIONS").
#pragma once
#include "ResArchive.h"
#include "Framebuffer.h"

namespace Palette {

// Load type-3 resource `name`, decompress, and apply it to fb. When nonBlackOnly
// is true, only entries whose RGB is non-(0,0,0) are copied (the rest of the
// palette is preserved) — this is how the engine overlays the sparse "GENERAL"
// UI palette on top of a scene's palette (GI_ApplyGeneralPalToTarget). When
// false, all 256 entries are applied. Returns false (and logs) on failure.
bool load(ResArchive& arc, Framebuffer& fb, const char* name, bool nonBlackOnly = false);

// Set the gamma-correction value (Sched_SetGamma / g_nPalGamma; 0 = off) and
// re-realize the current target palette through it (SetPal_WaitOrRealizeIfNeeded).
void setGamma(Framebuffer& fb, int gamma);

}  // namespace Palette
