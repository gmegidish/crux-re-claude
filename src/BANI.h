#pragma once

// BANI.cpp — Background Animation renderer
// Original path: C:\DevStudio\Projects\Crux\BANI.cpp
//
// BANI draws non-character, looping room animations (flickering lights,
// rippling water, etc.). It is distinct from Advanim.cpp, which animates
// the player/NPC sprites.
//
// A BANI animation is a stream of compressed "blocks". Each block is decoded
// by a small bytecode that is dispatched per draw-op. There are two families
// of decoders:
//   - plain decoders        : write palette-indexed colors straight out
//   - "Remap" decoders       : pass every color through a remap table first
//     (param_5 = remap table base; out = remap[palette[idx]])
//
// Pixels are written in a boustrophedon (serpentine) scan: the decoder walks
// one scanline left-to-right, then the next right-to-left, flipping the step
// sign (dir = -dir) each time it crosses a row boundary. Bani_Abs() returns
// the remaining pixel count to the row edge so a run can be split across rows.
//
// The packed-color decoders come in fixed bit-widths, chosen by how many
// distinct colors the block uses:
//   Rle6     : 6-bit color index (0x3F), 2-bit run length, palette up to 0x40
//   Rle4     : 4-bit color index (0x0F), palette up to 0x10
//   Rle3     : 3-bit color index (0x07), palette up to 0x08
//   RleNibble: nibble-stream codec (alternating high/low nibbles)
// Each has a *Remap variant. Bani_DecodeRleCont / Bani_SkipRleCont handle the
// per-extra-color "continuation" sub-streams when a block declares more colors
// than its primary index width can address.

// -------------------------------------------------------------------------
// Globals (shared decoder scratch state)
// -------------------------------------------------------------------------

// Pointer to the current block's embedded color table (set from block header).
extern unsigned char *g_pBaniPalette;        // 0x00646514

// Number of color entries in g_pBaniPalette for the current block
// (0xFF marks "no palette" / continuation). Drives the continuation loops.
extern int            g_nBaniPaletteCount;   // 0x00646518

// -------------------------------------------------------------------------
// No-loop list — background-animation resource IDs that must play once
// (instead of looping forever).
// -------------------------------------------------------------------------

extern int            g_anBaniNoLoopIds[100]; // 0x00646528 (400 bytes; 0-terminated)
extern int            g_nBaniNoLoopCount;     // 0x006466b8

// -------------------------------------------------------------------------
// Bytecode dispatcher (already named in the project)
// -------------------------------------------------------------------------

// 0x00414f90 — dispatch one background-animation draw op; returns advanced stream ptr.
unsigned char *Bani_PutBlock(unsigned char *pStream, unsigned char *pDst,
                             int width, int height, int pitch);

// -------------------------------------------------------------------------
// Indexed (palette-carrying) dispatch + draw
// -------------------------------------------------------------------------

// 0x00415210 — dispatch a single "indi" (indexed) draw op; selects the right
// decoder by op byte (0=skip, 1..4 helpers, 8 RLE, default=error).
unsigned char *Bani_PutIndi(unsigned char *pStream, void *pDst, int width,
                            int height, int pitch, int remapTable);

// 0x004179f0 — draw an entire indexed BANI image (header + grid of blocks),
// each block via Bani_PutIndi. Returns 0 on success, -1 on decode failure.
int Bani_DrawBlocksIndi(int pStream, int pDst, int x, int y, int pitch,
                        int p6, int remapTable);

// 0x00416670 — draw an entire BANI image (header + grid of blocks) via
// Bani_PutBlock. Reads block grid geometry from the 9-byte header.
void Bani_DrawBlocks(int pStream, int pDst, int x, int y, int pitch);

// -------------------------------------------------------------------------
// RLE decoders (plain) — boustrophedon scan, palette-indexed
// -------------------------------------------------------------------------

int Bani_DecodeRle6(unsigned char *pStream, unsigned char *pDst, int width, int pitch);
int Bani_DecodeRle4(unsigned char *pStream, unsigned char *pDst, int width, int pitch);
int Bani_DecodeRle3(unsigned char *pStream, unsigned char *pDst, int width, int pitch);
int Bani_DecodeRleNibble(unsigned char *pStream, unsigned char *pDst, int width, int pitch);

// 0x004157f0 — decode one "continuation" color sub-stream (single fill color,
// 0xFF terminator, 0xEF run threshold). Returns advanced stream ptr.
unsigned char *Bani_DecodeRleCont(unsigned char *pStream, unsigned char *pDst,
                                  int width, int pitch);

// 0x00416bd0 — skip one continuation sub-stream without drawing (advance only).
unsigned char *Bani_SkipRleCont(unsigned char *pStream);

// -------------------------------------------------------------------------
// RLE decoders (Remap) — identical but pass colors through a remap table
// (out = remapTable[paletteIndex]); param remapTable is the table base.
// -------------------------------------------------------------------------

int Bani_DecodeRle6Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable);
int Bani_DecodeRle4Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable);
int Bani_DecodeRle3Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable);
int Bani_DecodeRleNibbleRemap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable);

// -------------------------------------------------------------------------
// Raw (uncompressed) blits — boustrophedon copy of width*height bytes
// -------------------------------------------------------------------------

// 0x00416520 — copy raw bytes in serpentine order. Returns bytes consumed.
int Bani_BlitRaw(unsigned char *pStream, unsigned char *pDst, int width, int height, int pitch);

// 0x00417890 — same as Bani_BlitRaw but each byte is remapped through a table.
int Bani_BlitRawRemap(unsigned char *pStream, unsigned char *pDst, int width,
                      int height, int pitch, int remapTable);

// -------------------------------------------------------------------------
// No-loop list management
// -------------------------------------------------------------------------

// 0x00417bc0 — build the static no-loop resource-ID table, then compute count.
void Bani_InitNoLoopList(void);

// 0x00417d20 — return 1 if the given resource ID is in the no-loop list.
int Bani_IsNoLoopId(int nResId);

// 0x00417df0 — empty stub (SEH frame only; no body).
void Bani_Noop(void);
