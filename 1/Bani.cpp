#include "Bani.h"
#include "Log.h"
#include <cstdlib>
#include <cstdio>

// BANI block codec — clean-room port of src/BANI.cpp (Bani_DrawBlocks +
// Bani_PutBlock + Bani_DecodeRle6/4/3/Nibble + Bani_DecodeRleCont + Bani_BlitRaw).
//
// A BANI image is a grid of (W/blockW) x (H/blockH) blocks, row-major. Each block
// is decoded in BOUSTROPHEDON (serpentine) order: row 0 left->right, row 1
// right->left, etc. A signed step (dir) flips at every row edge, and runs that
// cross the edge are split using the absolute distance to the edge.
//
// Tokens (per bit-width N): high bits = run length, low bits = color index into
// the block's inline palette. A token of 0x00 is a transparent skip run of the
// raw byte value (leaves prior framebuffer pixels — what makes this a delta).
// Colors beyond the primary width arrive as continuation sub-streams.
//
// This is a delta codec: untouched pixels are never cleared.

namespace {

// The original engine kept the per-block color table in two globals that PERSIST
// across blocks AND across frames:
//   g_pBaniPalette      — points at the current inline color table
//   g_nBaniPaletteCount — number of colors (reset to -1 at the start of each frame
//                         by Bani_DrawBlocks, but the *pointer* is not cleared)
// A block whose header byte is 0xFF reuses the previously-set table, so the table
// must outlive a single block — and a frame's first block can legally be 0xFF and
// reuse the previous frame's table. We mirror that with file-scope state plus a
// persistent copy of the inline colors (the original points into the stream; we
// copy so the table stays valid after the frame's stream buffer goes away).
uint8_t        g_baniPalette[256];   // persistent copy of the active color table
int            g_baniPaletteCount = -1;
bool           g_baniPaletteValid  = false;

// Decoder state for a single frame: just the framebuffer bounds for clamped writes.
struct BlockState {
    uint8_t* fbBase = nullptr;
    uint8_t* fbEnd  = nullptr;
};

inline void putPixel(BlockState& st, uint8_t* p, uint8_t color) {
    if (p >= st.fbBase && p < st.fbEnd) *p = color;
}

inline uint8_t palColor(int idx) {
    if (!g_baniPaletteValid || idx < 0 || idx >= 256) return 0;
    return g_baniPalette[idx];
}

// ---------------------------------------------------------------------------
// Continuation sub-stream (single extra color). 0xFF terminates; bytes < 0xEF
// are transparent skip runs, >= 0xEF are fill runs (run = byte - 0xEE) of the
// initial color (the byte at entry). Returns the offset past the 0xFF.
// dst is the block's top-left. width = block width, pitch = framebuffer pitch.
// ---------------------------------------------------------------------------
size_t decodeRleCont(const uint8_t* s, size_t avail, BlockState& st,
                     uint8_t* dst, int width, int pitch) {
    if (avail == 0) return 0;
    int dir = 1;
    uint8_t* p = dst;
    uint8_t* rowEnd = dst + width;
    uint8_t color = s[0];

    size_t i = 0;  // index of the "color" byte; tokens follow
    for (;;) {
        if (i + 1 >= avail) return avail;       // truncated
        uint8_t tok = s[i + 1];
        if (tok == 0xFF) return i + 2;
        i += 1;
        if (tok < 0xEF) {
            int toEdge = std::abs((int)(rowEnd - p));
            unsigned run = tok;
            while (toEdge <= (int)run) {
                run -= toEdge;
                p = rowEnd + (pitch - dir);
                rowEnd = rowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            p += (int)run * dir;
        } else {
            int run = tok - 0xEE;
            int toEdge = std::abs((int)(rowEnd - p));
            while (toEdge <= run) {
                for (; p != rowEnd; p += dir) putPixel(st, p, color);
                rowEnd = rowEnd + (pitch - (width + 1) * dir);
                p = p + (pitch - dir);
                dir = -dir;
                run -= toEdge;
                toEdge = width;
            }
            uint8_t* pEnd = p + run * dir;
            for (; p != pEnd; p += dir) putPixel(st, p, color);
        }
    }
}

// Read the block's inline palette header: [u8 nColors] then (unless 0xFF) the
// inline color bytes (clamped to maxInline). When nColors==0xFF the previously
// set table is reused (shared across blocks/frames). Returns header bytes used.
size_t readPalette(const uint8_t* s, size_t avail, int maxInline) {
    if (avail == 0) return 0;
    unsigned nColors = s[0];
    size_t used = 1;
    if (nColors != 0xFF) {
        unsigned nInline = nColors;
        if ((int)nInline > maxInline) nInline = maxInline;
        // The original sets g_pBaniPalette to point directly INTO the stream, so a
        // color index beyond nInline (when nColors < maxInline) reads whatever bytes
        // follow the palette (i.e. token data). Mirror that by copying maxInline
        // bytes into our persistent table, but only ADVANCE past nInline of them.
        unsigned copy = (unsigned)maxInline;
        if (used + copy > avail) copy = (unsigned)(avail - used);
        for (unsigned k = 0; k < copy && k < 256; k++) g_baniPalette[k] = s[used + k];
        g_baniPaletteCount = (int)nColors;
        g_baniPaletteValid = true;
        used += nInline;
    }
    return used;
}

// ---------------------------------------------------------------------------
// Width-specialised RLE decoders. mask = low-bit color mask, runShift = bits to
// shift the run length down by, maxInline = primary palette size for this width.
// `whileLoop` selects the edge-handling form: the 6-bit decoder uses a while()
// that re-evaluates toEdge each turn; the 4/3-bit decoders use a single if().
// Returns the number of bytes consumed from `s`.
// ---------------------------------------------------------------------------
size_t decodeRleN(const uint8_t* s, size_t avail, BlockState& st,
                  uint8_t* dst, int width, int pitch,
                  unsigned runMask, int runShift, int idxMask,
                  int maxInline, bool whileLoop) {
    size_t i = readPalette(s, avail, maxInline);
    int nColors = g_baniPaletteCount;
    int dir = 1;
    uint8_t* pDst = dst;
    uint8_t* rowEnd = dst + width;

    for (; i < avail && s[i] != 0; i++) {
        uint8_t tok = s[i];
        if ((tok & runMask) == 0) {
            // transparent skip run
            int toEdge = std::abs((int)(rowEnd - pDst));
            unsigned run = tok;
            while (toEdge <= (int)run) {
                run -= toEdge;
                pDst = rowEnd + (pitch - dir);
                rowEnd = rowEnd + (pitch - (width + 1) * dir);
                dir = -dir;
                toEdge = width;
            }
            pDst += (int)run * dir;
        } else {
            uint8_t color = palColor(tok & idxMask);
            int run = (tok & runMask) >> runShift;
            int toEdge = std::abs((int)(rowEnd - pDst));
            if (whileLoop) {
                while (toEdge <= run) {
                    for (; pDst != rowEnd; pDst += dir) putPixel(st, pDst, color);
                    rowEnd = rowEnd + (pitch - (width + 1) * dir);
                    pDst = pDst + (pitch - dir);
                    dir = -dir;
                    run -= toEdge;
                    toEdge = std::abs((int)(rowEnd - pDst));
                }
            } else {
                if (toEdge <= run) {
                    for (; pDst != rowEnd; pDst += dir) putPixel(st, pDst, color);
                    rowEnd = rowEnd + (pitch - (width + 1) * dir);
                    pDst = pDst + (pitch - dir);
                    dir = -dir;
                    run -= toEdge;
                }
            }
            uint8_t* pEnd = pDst + run * dir;
            for (; pDst != pEnd; pDst += dir) putPixel(st, pDst, color);
        }
    }
    if (i < avail) i++;  // step past the 0 terminator

    // Extra colors beyond the primary range arrive as continuation streams.
    for (int c = maxInline; c < nColors; c++) {
        if (i >= avail) break;
        i += decodeRleCont(s + i, avail - i, st, dst, width, pitch);
    }
    return i;
}

// nibble-stream codec: tokens come from alternating high/low nibbles. nib
// toggles which nibble is current. Each step may emit a skip run (which itself
// pulls per-pixel color nibbles) and a fill run.
size_t decodeRleNibble(const uint8_t* s, size_t avail, BlockState& st,
                       uint8_t* dst, int width, int pitch) {
    size_t i = readPalette(s, avail, 0x10);
    int dir = 1;
    unsigned nib = 0;
    uint8_t* pDst = dst;
    uint8_t* rowEnd = dst + width;

    auto rdNib = [&](unsigned which) -> unsigned {
        if (i >= avail) return 0;
        return ((unsigned)s[i] >> (which << 2)) & 0x0F;
    };

    for (;;) {
        if (i >= avail) break;
        unsigned skipRun = rdNib(nib ^ 1);
        unsigned fillRun;
        {
            size_t fi = i + nib;
            fillRun = (fi < avail) ? (((unsigned)s[fi] >> (nib << 2)) & 0x0F) : 0;
        }
        i += nib + (nib ^ 1);
        if (skipRun == 0 && fillRun == 0) break;

        uint8_t fillColor = 0;
        if (fillRun != 0) {
            fillColor = palColor((int)rdNib(nib ^ 1));
            i += nib;
            nib ^= 1;
        }

        if (skipRun != 0) {
            int toEdge = std::abs((int)(rowEnd - pDst));
            if (toEdge <= (int)skipRun) {
                for (; pDst != rowEnd; pDst += dir) {
                    putPixel(st, pDst, palColor((int)rdNib(nib ^ 1)));
                    i += nib;
                    nib ^= 1;
                }
                rowEnd = rowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                skipRun -= toEdge;
            }
        }
        uint8_t* pEnd = pDst + (int)skipRun * dir;
        for (; pDst != pEnd; pDst += dir) {
            putPixel(st, pDst, palColor((int)rdNib(nib ^ 1)));
            i += nib;
            nib ^= 1;
        }

        if (fillRun != 0) {
            int toEdge = std::abs((int)(rowEnd - pDst));
            if ((int)fillRun < toEdge) {
                while ((int)fillRun > 0) { putPixel(st, pDst, fillColor); pDst += dir; fillRun--; }
            } else {
                for (; pDst != rowEnd; pDst += dir) putPixel(st, pDst, fillColor);
                rowEnd = rowEnd + (pitch - (width + 1) * dir);
                pDst = pDst + (pitch - dir);
                dir = -dir;
                fillRun -= toEdge;
                while ((int)fillRun > 0) { putPixel(st, pDst, fillColor); pDst += dir; fillRun--; }
            }
        }
    }
    if (nib != 0) i++;
    return i;
}

// raw (uncompressed) blit of width*height bytes in serpentine order.
size_t blitRaw(const uint8_t* s, size_t avail, BlockState& st,
               uint8_t* dst, int width, int height, int pitch) {
    size_t i = 0;
    uint8_t* pDst = dst;
    for (int h = height; h > 0; h -= 2) {
        uint8_t* rowEnd = pDst + width;
        for (; pDst < rowEnd; pDst++) { if (i < avail) putPixel(st, pDst, s[i++]); }
        pDst = pDst + pitch - 1;
        uint8_t* rowStart = pDst - width;
        for (; rowStart < pDst; pDst--) { if (i < avail) putPixel(st, pDst, s[i++]); }
        pDst = pDst + pitch + 1;
    }
    return i;
}

// Dispatch one block by its leading op byte (Bani_PutBlock).
// Returns bytes consumed (including the op byte), or 0 on a corrupt op.
size_t putBlock(const uint8_t* s, size_t avail, BlockState& st,
                uint8_t* dst, int width, int height, int pitch) {
    if (avail == 0) return 0;
    uint8_t op = s[0];
    const uint8_t* body = s + 1;
    size_t bodyAvail = avail - 1;
    size_t n = 0;
    // Op -> decoder mapping matches the engine's decodePicture4 block dispatcher:
    //   0 = empty (skip, leave delta)        1 = put_block_copy   (raw serpentine)
    //   2 = put_block_brun16 (4-bit nibble)  3 = put_block_skip64 (6-bit RLE)
    //   4 = dput_block_skip16 (4-bit RLE)    8 = dput_block_skip8 (3-bit RLE, 5-bit run)
    switch (op) {
    case 0:  return 1;  // empty block
    case 1:  n = blitRaw(body, bodyAvail, st, dst, width, height, pitch); break;
    case 2:  n = decodeRleNibble(body, bodyAvail, st, dst, width, pitch); break;
    case 3:  n = decodeRleN(body, bodyAvail, st, dst, width, pitch, 0xC0, 6, 0x3F, 0x40, true);  break;
    case 4:  n = decodeRleN(body, bodyAvail, st, dst, width, pitch, 0xF0, 4, 0x0F, 0x10, false); break;
    case 8:  n = decodeRleN(body, bodyAvail, st, dst, width, pitch, 0xF8, 3, 0x07, 0x08, false); break;
    default:
        // decodePicture4 treats an unknown op as fatal; here a stray op means the
        // stream desynced. Warn (rate-limited) and skip the op byte to recover.
        Log::warn("BANI: unknown block op 0x%02x (stream desync?)", op);
        return 1;
    }
    return n + 1;
}

inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

} // namespace

bool decodeBaniSprite(const uint8_t* data, size_t size, Framebuffer& fb, int dstX, int dstY) {
    if (size < 9) { Log::warn("decodeBaniSprite: header too small (%zu bytes)", size); return false; }

    // Header: [u8 tag][u16 W][u16 H][u16 blockW][u16 blockH]
    int imgW   = rd16(data + 1);
    int imgH   = rd16(data + 3);
    int blockW = rd16(data + 5);
    int blockH = rd16(data + 7);

    if (blockW <= 0 || blockH <= 0 || imgW <= 0 || imgH <= 0) {
        Log::warn("decodeBaniSprite: bad dims img=%dx%d block=%dx%d", imgW, imgH, blockW, blockH);
        return false;
    }

    const int pitch = fb.width();           // framebuffer is 640 wide
    BlockState st;
    st.fbBase = fb.pixels();
    st.fbEnd  = fb.pixels() + (size_t)fb.width() * fb.height();
    // Bani_DrawBlocks resets the shared color count to -1 each frame, but leaves
    // the color table itself intact (so a frame's first 0xFF block reuses it).
    g_baniPaletteCount = -1;

    const int colsPerRow = imgW / blockW;
    const int blockRows   = imgH / blockH;
    if (colsPerRow <= 0 || blockRows <= 0) return false;
    const int totalBlocks = colsPerRow * blockRows;

    const uint8_t* s   = data + 9;
    const uint8_t* end = data + size;

    // top-left of the blit; putPixel clamps every write to the framebuffer.
    uint8_t* pOut = fb.pixels() + (size_t)dstY * pitch + dstX;

    for (int b = 0; b < totalBlocks; b++) {
        size_t avail = (size_t)(end - s);
        if (avail == 0) break;          // stream consumed; leave remaining blocks as delta
        size_t consumed = putBlock(s, avail, st, pOut, blockW, blockH, pitch);
        s += consumed;
        if (s >= end) break;            // stop cleanly at end of stream

        pOut += blockW;
        if ((b + 1) % colsPerRow == 0)
            pOut += (pitch * blockH - blockW * colsPerRow);  // wrap to next block row
    }
    return true;
}
