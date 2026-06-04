// BANI.cpp — Background Animation renderer
// Original path: C:\DevStudio\Projects\Crux\BANI.cpp
//
// Decodes and blits non-character looping room animations (flickering lights,
// rippling water, etc.). Distinct from Advanim.cpp character animation.
//
// Data model
// ----------
// A BANI image begins with a 9-byte header:
//   +0  byte   tag/version
//   +1  int16  total image width  (pixels)
//   +3  int16  total image height (pixels)
//   +5  int16  block width        (pixels)
//   +7  int16  block height       (pixels)
//   +9  ...    grid of compressed blocks, row-major
// The image is divided into (W/bw) x (H/bh) blocks; each block is decoded by
// the bytecode dispatcher (Bani_PutBlock / Bani_PutIndi) and placed at the
// running destination offset, advancing by block width and wrapping a full
// block-row at a time.
//
// Each block is a stream of run-length tokens decoded in "boustrophedon"
// (serpentine) order: scanline 0 left-to-right, scanline 1 right-to-left, etc.
// The decoder keeps a signed step (dir = +1 / -1) and flips it at each row
// boundary; Bani_Abs() yields the remaining count to the current row edge so a
// run can be split across the turn.
//
// Decoders are specialised by color-index bit-width. Token high bits encode a
// short run length; low bits index the per-block color table (g_pBaniPalette,
// g_nBaniPaletteCount). When a block uses more colors than the primary width
// can address, the extra colors are emitted via "continuation" sub-streams
// (Bani_DecodeRleCont). The "Remap" variants additionally pass every output
// color through a caller-supplied remap table (remapTable[colorIndex]).

#include "BANI.h"
#include "ERRORS.h"
#include <windows.h>
#include <string.h>

// -------------------------------------------------------------------------
// Cross-module helpers (defined elsewhere; stubbed here as externs)
// -------------------------------------------------------------------------

// 0x0048a650 — integer abs() (CRT). Returns the unsigned magnitude of x.
extern "C" int Bani_Abs(int x);                 // FUN_0048a650

// MSVC SEH unwind funclet entry points referenced by the original frames.
// (Address-of labels in the decompiler output; no portable equivalent.)

// =========================================================================
// Globals
// =========================================================================

unsigned char *g_pBaniPalette;       // 0x00646514
int            g_nBaniPaletteCount;  // 0x00646518

int            g_anBaniNoLoopIds[100]; // 0x00646528
int            g_nBaniNoLoopCount;     // 0x006466b8

// =========================================================================
// RLE decoders — plain (palette-indexed) family
// =========================================================================
//
// Common token layout (per decoder width N):
//   high bits = run length (split-able across rows)
//   low  bits = color index into g_pBaniPalette
//   token 0x00 = "skip" run of the *raw byte value* pixels (transparent)
//   stream 0   = end of block; extra colors beyond N then follow as
//                continuation sub-streams.

// 0x004154a0 — 6-bit index (mask 0x3F), 2-bit run; palette up to 0x40.
int Bani_DecodeRle6(unsigned char *pStream, unsigned char *pDst, int width, int pitch)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 0x40) nInline = 0x40;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xC0) == 0) {
            // transparent skip run
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x3F];
            int run = (*pStream & 0xC0) >> 6;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            while (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
                toEdge = Bani_Abs((int)(pRowEnd - pDst));
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    // Extra colors beyond the 0x40 primary range arrive as continuation streams.
    for (int i = 0x40; i < (int)nColors; i++)
        pStream = Bani_DecodeRleCont(pStream, pStart, width, pitch);

    return (int)(pStream - pStart);
}

// 0x004157f0 — single-color "continuation" sub-stream.
// 0xFF terminates; bytes < 0xEF are skip runs, >= 0xEF are fill runs of the
// initial color (*pStream at entry). Returns ptr past the 0xFF terminator.
unsigned char *Bani_DecodeRleCont(unsigned char *pStream, unsigned char *pDst, int width, int pitch)
{
    unsigned char *pPrev;
    unsigned char *pRowEnd;
    int dir = 1;
    unsigned char *p = pDst;

    pRowEnd = pDst + width;
    unsigned char color = *pStream;
    pPrev = pStream;
    while (pStream = pPrev + 1, *pStream != 0xFF) {
        pPrev = pStream;
        if (*pStream < 0xEF) {
            int toEdge = Bani_Abs((int)(pRowEnd - p));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                p = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            p += run * dir;
        } else {
            int run = *pStream - 0xEE;
            int toEdge = Bani_Abs((int)(pRowEnd - p));
            while (toEdge <= run) {
                for (; p != pRowEnd; p += dir) *p = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                p = p + (pitch - dir);
                dir = -dir;
                run -= toEdge;
                toEdge = width;
            }
            unsigned char *pEnd = p + run * dir;
            for (; p != pEnd; p += dir) *p = color;
        }
    }
    return pPrev + 2;
}

// 0x00415a60 — 4-bit index (mask 0x0F); palette up to 0x10.
int Bani_DecodeRle4(unsigned char *pStream, unsigned char *pDst, int width, int pitch)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 0x10) nInline = 0x10;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xF0) == 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x0F];
            int run = (*pStream & 0xF0) >> 4;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    for (int i = 0x10; i < (int)nColors; i++)
        pStream = Bani_DecodeRleCont(pStream, pStart, width, pitch);

    return (int)(pStream - pStart);
}

// 0x00415d90 — 3-bit index (mask 0x07); palette up to 0x08.
int Bani_DecodeRle3(unsigned char *pStream, unsigned char *pDst, int width, int pitch)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 8) nInline = 8;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xF8) == 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x07];
            int run = (*pStream & 0xF8) >> 3;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    for (int i = 8; i < (int)nColors; i++)
        pStream = Bani_DecodeRleCont(pStream, pStart, width, pitch);

    return (int)(pStream - pStart);
}

// 0x004160c0 — nibble-stream codec: tokens come from alternating high/low
// nibbles of the byte stream. nib toggles which nibble is current; each step
// may emit a skip run (high nibble) and a fill run (low nibble).
int Bani_DecodeRleNibble(unsigned char *pStream, unsigned char *pDst, int width, int pitch)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 0x10) nInline = 0x10;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }

    unsigned int nib = 0;
    pRowEnd = pDst + width;
    while (true) {
        unsigned int skipRun = ((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F;
        unsigned int fillRun = ((unsigned int)pStream[nib] >> (nib << 2)) & 0x0F;
        pStream += nib + (nib ^ 1);
        if (skipRun == 0 && fillRun == 0) break;

        unsigned char fillColor = 0;
        if (fillRun != 0) {
            fillColor = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
            pStream += nib;
            nib ^= 1;
        }

        if (skipRun != 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= (int)skipRun) {
                for (; pDst != pRowEnd; pDst += dir) {
                    *pDst = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
                    pStream += nib;
                    nib ^= 1;
                }
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                skipRun -= toEdge;
            }
        }
        unsigned char *pEnd = pDst + skipRun * dir;
        for (; pDst != pEnd; pDst += dir) {
            *pDst = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
            pStream += nib;
            nib ^= 1;
        }

        if (fillRun != 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if ((int)fillRun < toEdge) {
                while ((int)fillRun > 0) { *pDst = fillColor; pDst += dir; fillRun--; }
            } else {
                for (; pDst != pRowEnd; pDst += dir) *pDst = fillColor;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                fillRun -= toEdge;
                while ((int)fillRun > 0) { *pDst = fillColor; pDst += dir; fillRun--; }
            }
        }
    }
    if (nib != 0) pStream++;

    return (int)(pStream - pStart);
}

// =========================================================================
// RLE decoders — Remap family
// =========================================================================
// Identical to the plain decoders, except the per-block color table is first
// rewritten in place: g_pBaniPalette[i] = remapTable[g_pBaniPalette[i]].
// This lets the same compressed block render under a substituted palette.

// 0x00416bd0 — skip a continuation sub-stream (advance only; no draw).
unsigned char *Bani_SkipRleCont(unsigned char *pStream)
{
    unsigned char *pPrev;
    do {
        pPrev = pStream;
        pStream = pPrev + 1;
    } while (*pStream != 0xFF);
    return pPrev + 2;
}

// 0x00416830 — 6-bit indexed, remapped.
int Bani_DecodeRle6Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 0x40) nInline = 0x40;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;
    for (int i = 0; i < (int)nColors; i++)
        g_pBaniPalette[i] = *(unsigned char *)(remapTable + g_pBaniPalette[i]);

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xC0) == 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x3F];
            int run = (*pStream & 0xC0) >> 6;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            while (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
                toEdge = Bani_Abs((int)(pRowEnd - pDst));
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    for (int i = 0x40; i < (int)nColors; i++)
        pStream = Bani_SkipRleCont(pStream);

    return (int)(pStream - pStart);
}

// 0x00416cf0 — 4-bit indexed, remapped.
int Bani_DecodeRle4Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 0x10) nInline = 0x10;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;
    for (int i = 0; i < (int)nColors; i++)
        g_pBaniPalette[i] = *(unsigned char *)(remapTable + g_pBaniPalette[i]);

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xF0) == 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x0F];
            int run = (*pStream & 0xF0) >> 4;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    for (int i = 0x10; i < (int)nColors; i++)
        pStream = Bani_SkipRleCont(pStream);

    return (int)(pStream - pStart);
}

// 0x00417070 — 3-bit indexed, remapped.
int Bani_DecodeRle3Remap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    unsigned int nColors = *pStream++;
    if (nColors != 0xFF) {
        unsigned int nInline = nColors;
        if (nInline > 8) nInline = 8;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = (unsigned int)g_nBaniPaletteCount;
    for (int i = 0; i < (int)nColors; i++)
        g_pBaniPalette[i] = *(unsigned char *)(remapTable + g_pBaniPalette[i]);

    pRowEnd = pDst + width;
    for (; *pStream != 0; pStream++) {
        if ((*pStream & 0xF8) == 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            unsigned int run = *pStream;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = pRowEnd + (pitch - dir);
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += run * dir;
        } else {
            unsigned char color = g_pBaniPalette[*pStream & 0x07];
            int run = (*pStream & 0xF8) >> 3;
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= run) {
                for (; pDst != pRowEnd; pDst += dir) *pDst = color;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                run -= toEdge;
            }
            unsigned char *pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) *pDst = color;
        }
    }
    pStream++;
    for (int i = 8; i < (int)nColors; i++)
        pStream = Bani_SkipRleCont(pStream);

    return (int)(pStream - pStart);
}

// 0x004173f0 — nibble-stream codec, remapped.
int Bani_DecodeRleNibbleRemap(unsigned char *pStream, unsigned char *pDst, int width, int pitch, int remapTable)
{
    unsigned char *pStart = pStream;
    unsigned char *pRowEnd;
    int dir = 1;

    int nColors = *(int *)pStream; // header reads a full int in the original
    pStream = (unsigned char *)((int)pStream + 1);
    if (nColors != 0xFF) {
        int nInline = nColors;
        if (nInline > 0x10) nInline = 0x10;
        g_pBaniPalette = pStream;
        g_nBaniPaletteCount = nColors;
        pStream += nInline;
    }
    nColors = g_nBaniPaletteCount;
    for (int i = 0; i < nColors; i++)
        g_pBaniPalette[i] = *(unsigned char *)(remapTable + g_pBaniPalette[i]);

    unsigned int nib = 0;
    pRowEnd = pDst + width;
    while (true) {
        unsigned int skipRun = ((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F;
        unsigned int fillRun = ((unsigned int)pStream[nib] >> (nib << 2)) & 0x0F;
        pStream += nib + (nib ^ 1);
        if (skipRun == 0 && fillRun == 0) break;

        unsigned char fillColor = 0;
        if (fillRun != 0) {
            fillColor = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
            pStream += nib;
            nib ^= 1;
        }

        if (skipRun != 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if (toEdge <= (int)skipRun) {
                for (; pDst != pRowEnd; pDst += dir) {
                    *pDst = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
                    pStream += nib;
                    nib ^= 1;
                }
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                skipRun -= toEdge;
            }
        }
        unsigned char *pEnd = pDst + skipRun * dir;
        for (; pDst != pEnd; pDst += dir) {
            *pDst = g_pBaniPalette[((unsigned int)*pStream >> ((nib ^ 1) << 2)) & 0x0F];
            pStream += nib;
            nib ^= 1;
        }

        if (fillRun != 0) {
            int toEdge = Bani_Abs((int)(pRowEnd - pDst));
            if ((int)fillRun < toEdge) {
                while ((int)fillRun > 0) { *pDst = fillColor; pDst += dir; fillRun--; }
            } else {
                for (; pDst != pRowEnd; pDst += dir) *pDst = fillColor;
                pRowEnd = pRowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                fillRun -= toEdge;
                while ((int)fillRun > 0) { *pDst = fillColor; pDst += dir; fillRun--; }
            }
        }
    }
    if (nib != 0) pStream++;

    return (int)(pStream - pStart);
}

// =========================================================================
// Raw (uncompressed) blits
// =========================================================================

// 0x00416520 — copy width*height raw bytes in serpentine order.
int Bani_BlitRaw(unsigned char *pStream, unsigned char *pDst, int width, int height, int pitch)
{
    unsigned char *pStart = pStream;
    for (; height > 0; height -= 2) {
        unsigned char *pRowEnd = pDst + width;
        for (; pDst < pRowEnd; pDst++) *pDst = *pStream++;       // L -> R
        pDst = pDst + pitch - 1;
        unsigned char *pRowStart = pDst - width;
        for (; pRowStart < pDst; pDst--) *pDst = *pStream++;      // R -> L
        pDst = pDst + pitch + 1;
    }
    return (int)(pStream - pStart);
}

// 0x00417890 — Bani_BlitRaw with a per-byte palette remap.
int Bani_BlitRawRemap(unsigned char *pStream, unsigned char *pDst, int width,
                      int height, int pitch, int remapTable)
{
    unsigned char *pStart = pStream;
    for (; height > 0; height -= 2) {
        unsigned char *pRowEnd = pDst + width;
        for (; pDst < pRowEnd; pDst++)
            *pDst = *(unsigned char *)(remapTable + *pStream++);
        pDst = pDst + pitch - 1;
        unsigned char *pRowStart = pDst - width;
        for (; pRowStart < pDst; pDst--)
            *pDst = *(unsigned char *)(remapTable + *pStream++);
        pDst = pDst + pitch + 1;
    }
    return (int)(pStream - pStart);
}

// =========================================================================
// Indexed dispatch + whole-image drawing
// =========================================================================

// 0x00415210 — dispatch a single "indi" (indexed) draw op by its leading byte.
// op 0   : empty block (nothing to draw)
// op 1..4: width-specific RLE decoders (remap variants)
// op 8   : raw remapped blit
// default: assertion failure / error record (corrupt stream)
unsigned char *Bani_PutIndi(unsigned char *pStream, void *pDst, int width,
                            int height, int pitch, int remapTable)
{
    unsigned char *p = pStream + 1;
    switch (*pStream) {
    case 0:
        pStream = p;
        break;
    case 1: {
        int n = Bani_DecodeRle6Remap(p, (unsigned char *)pDst, width, pitch, remapTable);
        pStream = p + n;
        break;
    }
    case 2: {
        int n = Bani_DecodeRle4Remap(p, (unsigned char *)pDst, width, pitch /*,remapTable*/);
        pStream = p + n;
        break;
    }
    case 3: {
        int n = Bani_DecodeRle3Remap(p, (unsigned char *)pDst, width, pitch /*,remapTable*/);
        pStream = p + n;
        break;
    }
    case 4: {
        int n = Bani_DecodeRleNibbleRemap(p, (unsigned char *)pDst, width, pitch /*,remapTable*/);
        pStream = p + n;
        break;
    }
    case 8: {
        int n = Bani_BlitRawRemap(p, (unsigned char *)pDst, width, height, pitch, remapTable);
        pStream = p + n;
        break;
    }
    default:
        // Corrupt/unknown op: assert and raise an error record.
        Err_BadStreamOp(*pStream);  // Debug_Assert + Err_SetRecord3 in original
        pStream = p;
        break;
    }
    return pStream;
}

// 0x00417bc0 — populate the static no-loop resource-ID table.
// memset clears all 100 ints, then a fixed list of resource IDs is written.
// The trailing 0 acts as a sentinel; g_nBaniNoLoopCount is set by scanning.
void Bani_InitNoLoopList(void)
{
    memset(g_anBaniNoLoopIds, 0, sizeof(g_anBaniNoLoopIds));
    g_anBaniNoLoopIds[0]  = 9;
    g_anBaniNoLoopIds[1]  = 10;
    g_anBaniNoLoopIds[2]  = 0x0B;
    g_anBaniNoLoopIds[3]  = 0x0E;
    g_anBaniNoLoopIds[4]  = 0x11;
    g_anBaniNoLoopIds[5]  = 0x6F;
    g_anBaniNoLoopIds[6]  = 0x14C;
    g_anBaniNoLoopIds[7]  = 0x154;
    g_anBaniNoLoopIds[8]  = 0x17D;
    g_anBaniNoLoopIds[9]  = 0x1F9;
    g_anBaniNoLoopIds[10] = 0x200;
    g_anBaniNoLoopIds[11] = 0x2C4;

    g_nBaniNoLoopCount = 0;
    while (g_anBaniNoLoopIds[g_nBaniNoLoopCount] != 0)
        g_nBaniNoLoopCount++;
}

// 0x00417d20 — return 1 if nResId is registered as no-loop, else 0.
int Bani_IsNoLoopId(int nResId)
{
    for (int i = 0; i < g_nBaniNoLoopCount; i++)
        if (g_anBaniNoLoopIds[i] == nResId)
            return 1;
    return 0;
}

// 0x00416670 — draw a whole BANI image as a grid of blocks via Bani_PutBlock.
// Header: +1 int16 width, +3 int16 height, +5 int16 blockW, +7 int16 blockH.
void Bani_DrawBlocks(int pStream, int pDst, int x, int y, int pitch)
{
    short *pW = (short *)(pStream + 1);
    g_nBaniPaletteCount = -1;
    short imgH   = *(short *)(pStream + 3);
    int   blockW = *(short *)(pStream + 5);
    int   blockH = *(short *)(pStream + 7);
    pStream += 9;

    unsigned char *pOut = (unsigned char *)(pDst + x + y * pitch);
    int colsPerRow = (int)*pW / blockW;
    int totalBlocks = colsPerRow * ((int)imgH / blockH);

    for (int i = 0; i < totalBlocks; i++) {
        pStream = (int)Bani_PutBlock((unsigned char *)pStream, pOut, blockW, blockH, pitch);
        pOut += blockW;
        if ((i + 1) % colsPerRow == 0)
            pOut += (pitch * blockH - blockW * colsPerRow); // wrap to next block row
    }
}

// 0x004179f0 — draw a whole indexed BANI image via Bani_PutIndi.
// Returns 0 on success, -1 if any block fails to decode (Bani_PutIndi == 0).
int Bani_DrawBlocksIndi(int pStream, int pDst, int x, int y, int pitch,
                        int p6, int remapTable)
{
    short *pW = (short *)(pStream + 1);
    g_nBaniPaletteCount = -1;
    short imgH   = *(short *)(pStream + 3);
    int   blockW = *(short *)(pStream + 5);
    int   blockH = *(short *)(pStream + 7);
    pStream += 9;

    unsigned char *pOut = (unsigned char *)(pDst + x + y * pitch);
    int colsPerRow = (int)*pW / blockW;
    int totalBlocks = colsPerRow * ((int)imgH / blockH);

    for (int i = 0; i < totalBlocks; i++) {
        pStream = (int)Bani_PutIndi((unsigned char *)pStream, pOut, blockW, blockH, pitch, remapTable);
        if (pStream == 0)
            return -1;
        pOut += blockW;
        if ((i + 1) % colsPerRow == 0)
            pOut += (pitch * blockH - blockW * colsPerRow);
    }
    return 0;
}

// 0x00417df0 — empty stub (SEH frame only in the original; no body).
void Bani_Noop(void)
{
}
