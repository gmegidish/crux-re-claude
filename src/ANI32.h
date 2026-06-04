#pragma once
// ANI32.cpp — "Animation 32-bit" helpers (2 functions)
//
// Two routines that bridge the sprite/animation system and the AREAS hit-test
// index:
//
//   Ani32_DrawScaledRLE   (0x00413530)
//       Decode an RLE-compressed paletted sprite, scale it by a percentage,
//       and blit it column-by-column into a destination 8bpp surface.  While
//       drawing it also tracks the leftmost/rightmost columns that contained
//       opaque pixels (g_nAdvAnimSentinelMin/Max) and publishes the resulting
//       on-screen bounding box into the AREAS "active bbox" globals so the
//       cursor hit-test can find the freshly-drawn character.
//
//   Ani32_BuildAreaLookup (0x00413bd0)
//       Rebuild the entire Y-bucket spatial index (g_anAreaYBuckets) from
//       scratch in a single pass: for each of the 120 scanline rows, append
//       every walkable node and every active cache record whose bbox overlaps
//       that row, then terminate the row with -1.  Serialised with the AREAS
//       critical section.  Debug tag: "area_lookup_init(void)".

#include <windows.h>

// --- Globals owned / introduced by ANI32 ---

// [Mouse]/HalfHero flag cached from CRUX.INI (0xffffffff until first read).
// When nonzero, Ani32_DrawScaledRLE halves the height used for the active-area
// bounding box (the lower half of a tall sprite is excluded from hit-testing).
extern int  g_nAni32HalfHero;       // 0x004c7ca4

// Vertical clip range for Ani32_DrawScaledRLE.  A destination column whose
// X coordinate is below g_nAni32ClipTop or above g_nAni32ClipBottom is decoded
// (to keep the RLE stream in sync) but not written to the framebuffer.
extern int  g_nAni32ClipTop;        // 0x006e86cc
extern int  g_nAni32ClipBottom;     // 0x004d5270

// Array of pointers to cached area records (the secondary "sprite/cache" area
// list).  Each record stores its bbox top at +0x04 and bottom at +0x0C.
// Indices registered from this table are offset by +0x96 (150) in the buckets.
extern int* g_apAreaCacheRecords;   // 0x0070c5b8

// --- Functions ---

// 0x00413530 — Decode + scale + blit an RLE sprite; publish its on-screen bbox.
//   pSprite   RLE stream; first 4 bytes are int16 srcW, int16 srcH header.
//   pDest     destination 8bpp surface base pointer.
//   nDestX    destination X (sprite is centred horizontally around this).
//   nDestY    destination Y (sprite baseline / bottom anchor).
//   nPitch    destination surface pitch (bytes per scanline).
//   nMaxW     surface clip width  (validates srcW).
//   nMaxH     surface clip height (validates srcH).
//   pPalMap   256-entry colour remap table (RLE colour index -> dest pixel).
//   nScalePct scale factor as a percentage (100 = original size).
void Ani32_DrawScaledRLE(const unsigned char* pSprite, unsigned char* pDest,
                         int nDestX, int nDestY, int nPitch, int nMaxW,
                         int nMaxH, const unsigned char* pPalMap, int nScalePct);

// 0x00413bd0 — Rebuild the AREAS Y-bucket index from the node + cache tables.
void Ani32_BuildAreaLookup(void);
