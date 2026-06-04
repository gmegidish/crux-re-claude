#include "Sprite.h"
#include "Bani.h"
#include "Log.h"

namespace {

// A bounds-checked cursor over the sprite payload.
struct Reader {
    const uint8_t* p;
    const uint8_t* end;
    bool ok = true;
    uint8_t  u8()  { if (p >= end) { ok = false; return 0; } return *p++; }
    uint16_t u16() { uint16_t a = u8(); uint16_t b = u8(); return (uint16_t)(a | (b << 8)); }
    int8_t   s8()  { return (int8_t)u8(); }
};

// Per-line decoders. dst points at the start of the destination row; dstEnd is
// the row's exclusive end (for clamping). Advances rd past the line.
// Returns the dst write cursor (unused by caller but keeps intent clear).

void lineDirect(Reader& rd, uint8_t* dst, uint8_t* dstEnd, int width) {
    for (int i = 0; i < width && rd.ok; ++i) {
        uint8_t v = rd.u8();
        if (dst < dstEnd) *dst = v;
        ++dst;
    }
}

void lineRLE(Reader& rd, uint8_t* dst, uint8_t* dstEnd) {
    while (rd.ok) {
        int count = rd.s8();
        if (count == 0) return;
        if (count < 0) {                          // literal run of -count
            for (; count != 0 && rd.ok; ++count) { uint8_t v = rd.u8(); if (dst < dstEnd) *dst = v; ++dst; }
        } else {                                  // repeat next byte count times
            uint8_t v = rd.u8();
            for (; count != 0; --count) { if (dst < dstEnd) *dst = v; ++dst; }
        }
    }
}

void lineRLESkip(Reader& rd, uint8_t* dst, uint8_t* dstEnd) {
    while (rd.ok) {
        int count = rd.s8();
        if (count == 0) return;
        if (count < 0) {                          // literal run of -count
            for (; count != 0 && rd.ok; ++count) { uint8_t v = rd.u8(); if (dst < dstEnd) *dst = v; ++dst; }
        } else {                                  // skip count dst bytes (leave prior pixels)
            dst += count;
        }
    }
}

void lineRLEOffset(Reader& rd, uint8_t* dst, uint8_t* dstEnd) {
    dst += rd.u8();                               // leading destination offset
    while (rd.ok) {
        int count = rd.s8();
        if (count < 0) {                          // literal run
            for (; count != 0 && rd.ok; ++count) { uint8_t v = rd.u8(); if (dst < dstEnd) *dst = v; ++dst; }
        } else if (count != 0) {                  // repeat next byte
            uint8_t v = rd.u8();
            for (; count != 0; --count) { if (dst < dstEnd) *dst = v; ++dst; }
        }
        uint8_t term = rd.u8();
        if (term == 0xFF) return;                 // end of line
        dst += term;                              // else skip term bytes
    }
}

} // namespace

bool decodeSprite(const uint8_t* data, size_t size, Framebuffer& fb,
                  int dstX, int dstY, int* spriteW, int* spriteH) {
    Reader rd{ data, data + size };
    uint8_t type = rd.u8();
    int width  = rd.u16();
    int height = rd.u16();
    if (spriteW) { *spriteW = width; }
    if (spriteH) { *spriteH = height; }

    // Tag dispatch (matches Help_BlitImage):
    //   2      = per-scanline RLE strips (keyframe) — handled below
    //   4      = BANI block codec (delta frames) — see decodeBaniSprite
    //   3,0x20 = nothing to draw (sentinel)
    if (type == 0x03 || type == 0x20) { return true; }
    if (type == 0x04) { return decodeBaniSprite(data, size, fb, dstX, dstY); }
    // (type 2 and any per-line variant fall through to the strip decoder)

    if (width <= 0 || width > Framebuffer::W || height <= 0 || height > Framebuffer::H) {
        Log::warn("decodeSprite: odd dimensions %dx%d (type=0x%02x)", width, height, type);
        // continue anyway, clamped — some frames are sub-rect
    }

    uint8_t* fbp = fb.pixels();
    const int fbW = fb.width();
    int row = 0;

    // Row-group loop: [u16 yskip][u16 nrows], nrows lines, repeat until nrows==0.
    while (rd.ok) {
        int yskip = rd.u16();
        int nrows = rd.u16();
        row += yskip;
        if (nrows == 0) { break; }

        for (int i = 0; i < nrows && rd.ok; ++i) {
            const int actualRow = dstY + row;
            // Drawable only when the row is on-screen and the left edge is a valid
            // column; otherwise consume the line bytes via scratch to stay aligned.
            if (actualRow >= 0 && actualRow < Framebuffer::H && dstX >= 0 && dstX < fbW) {
                uint8_t* rowStart = fbp + actualRow * fbW;
                uint8_t* dst    = rowStart + dstX;
                uint8_t* dstEnd = rowStart + fbW;            // clamp at the row's right edge
                uint8_t codec   = rd.u8();
                switch (codec) {
                    case 0: lineDirect(rd, dst, dstEnd, width); break;
                    case 1: lineRLE(rd, dst, dstEnd);           break;
                    case 2: lineRLESkip(rd, dst, dstEnd);       break;
                    case 3: lineRLEOffset(rd, dst, dstEnd);     break;
                    case 4: /* skip whole line */               break;
                    default:
                        Log::warn("decodeSprite: bad line codec %u at row %d", codec, actualRow);
                        return false;
                }
            } else {
                // Row off-screen: still must consume the line bytes to stay aligned.
                uint8_t codec = rd.u8();
                uint8_t scratch[Framebuffer::W];
                switch (codec) {
                    case 0: lineDirect(rd, scratch, scratch + width, width); break;
                    case 1: lineRLE(rd, scratch, scratch + Framebuffer::W); break;
                    case 2: lineRLESkip(rd, scratch, scratch + Framebuffer::W); break;
                    case 3: lineRLEOffset(rd, scratch, scratch + Framebuffer::W); break;
                    case 4: break;
                    default: return false;
                }
            }
            ++row;
        }
    }
    return true;
}
