#include "HelpBlit.h"
#include "Bani.h"
#include "Log.h"

namespace {

constexpr int kPitch = Framebuffer::W;          // 640
constexpr long kSurface = (long)Framebuffer::W * Framebuffer::H;  // 640*480

// A bounds-checked cursor over the sprite payload. Tracks the byte offset so we
// can detect a stream overrun and bail out gracefully.
struct Reader {
    const uint8_t* base;
    const uint8_t* p;
    const uint8_t* end;
    bool ok = true;
    uint8_t u8() {
        if (p >= end) { ok = false; return 0; }
        return *p++;
    }
    int8_t s8() { return (int8_t)u8(); }
    // signed 16-bit, little-endian
    int16_t s16() {
        uint16_t a = u8();
        uint16_t b = u8();
        return (int16_t)(uint16_t)(a | (b << 8));
    }
    size_t offset() const { return (size_t)(p - base); }
};

// Destination writer over the framebuffer. `pos` is a signed byte offset into
// the 640x480 index buffer (it may legitimately go negative when the clip
// origin is off-screen). Writes are clamped to [0, kSurface); off-surface
// pixels are dropped but the destination cursor still advances so the codecs
// stay byte-aligned, exactly mirroring the engine's pointer arithmetic.
struct Dst {
    uint8_t* fb;
    long pos;
    void put(uint8_t v) {
        if (pos >= 0 && pos < kSurface) { fb[pos] = v; }
        ++pos;
    }
    void advance(long n) { pos += n; }
};

// ---- The four Help_Line* codecs ------------------------------------------
// Each consumes exactly the bytes the engine consumes and writes pixels
// verbatim (no colorBank offset). Index 0 is NOT transparent for literal runs.

// Opcode 0: Help_LineCopyRun(pStream, pDst, nLen) — copy nLen literal bytes.
// nLen is the image's colorBank field (the per-line literal length).
void lineCopyRun(Reader& rd, Dst& dst, int nLen) {
    for (int i = 0; i < nLen && rd.ok; ++i) {
        dst.put(rd.u8());
    }
}

// Opcode 1: Help_LineDecodeRLE(pStream, pDst).
//   count < 0 : literal run of |count| bytes
//   count > 0 : fill run of count copies of the next byte
//   count = 0 : end of line
void lineDecodeRLE(Reader& rd, Dst& dst) {
    while (rd.ok) {
        int count = rd.s8();
        if (count == 0) {
            break;
        }
        if (count < 0) {
            for (; count != 0 && rd.ok; ++count) {
                dst.put(rd.u8());
            }
        } else {
            uint8_t c = rd.u8();
            for (; count != 0; --count) {
                dst.put(c);
            }
        }
    }
}

// Opcode 2 (and 7, payload at +3): Help_LineSkipRLE(pStream, pDst).
//   count < 0 : literal run of |count| bytes
//   count > 0 : transparent gap of count bytes (advance dst, leave pixels)
//   count = 0 : end of line
void lineSkipRLE(Reader& rd, Dst& dst) {
    while (rd.ok) {
        int count = rd.s8();
        if (count == 0) {
            break;
        }
        if (count < 0) {
            for (; count != 0 && rd.ok; ++count) {
                dst.put(rd.u8());
            }
        } else {
            dst.advance(count);
        }
    }
}

// Opcode 3 (and 8, payload at +3): Help_LineDecodeRLEOffset(pStream, pDst).
//   [u8 leadOffset] then repeated:
//     [s8 count]  (<0 literal run | >0 fill run | 0 no run this token)
//     [u8 gap]    0xFF ends the line, else transparent gap of `gap` bytes
void lineDecodeRLEOffset(Reader& rd, Dst& dst) {
    dst.advance(rd.u8());                     // initial transparent offset
    while (rd.ok) {
        int count = rd.s8();
        if (count < 0) {
            for (; count != 0 && rd.ok; ++count) {
                dst.put(rd.u8());
            }
        } else if (count != 0) {
            uint8_t c = rd.u8();
            for (; count != 0; --count) {
                dst.put(c);
            }
        }
        uint8_t gap = rd.u8();
        if (gap == 0xFF) {
            break;
        }
        dst.advance(gap);
    }
}

// Help_PutLine — decode one scanline at `dst` by leading opcode byte.
// Returns false on an unknown opcode (matches the engine's assert/throw).
bool putLine(Reader& rd, Dst dst, int colorBank) {
    uint8_t opcode = rd.u8();
    switch (opcode) {
        case 0: {
            lineCopyRun(rd, dst, colorBank);
            break;
        }
        case 1: {
            lineDecodeRLE(rd, dst);
            break;
        }
        case 2: {
            lineSkipRLE(rd, dst);
            break;
        }
        case 3: {
            lineDecodeRLEOffset(rd, dst);
            break;
        }
        case 4: {
            // skip whole line: consume only the opcode byte, no draw
            break;
        }
        case 7: {
            // like opcode 2 but payload starts at +3 (skip 2 extra bytes)
            rd.u8();
            rd.u8();
            lineSkipRLE(rd, dst);
            break;
        }
        case 8: {
            // like opcode 3 but payload starts at +3 (skip 2 extra bytes)
            rd.u8();
            rd.u8();
            lineDecodeRLEOffset(rd, dst);
            break;
        }
        default: {
            Log::warn("blitHelpImage: bad line opcode %u at stream offset %zu",
                      opcode, rd.offset() - 1);
            return false;
        }
    }
    return true;
}

}  // namespace

bool blitHelpImage(const uint8_t* blob, size_t size, Framebuffer& fb,
                   int dstX, int dstY) {
    if (blob == nullptr || size < 1) {
        Log::warn("blitHelpImage: empty blob");
        return false;
    }

    const uint8_t tag = blob[0];

    // tag 3 / 0x20 (' '): nothing to draw.
    if (tag == 0x03 || tag == 0x20) {
        return true;
    }
    // tag 4: BANI block codec — delegate to the existing decoder.
    if (tag == 0x04) {
        return decodeBaniSprite(blob, size, fb, dstX, dstY);
    }

    // Header is 5 bytes: [u8 tag][s16 colorBank][u16 unused]; strips start at 5.
    if (size < 5) {
        Log::warn("blitHelpImage: header truncated (%zu bytes)", size);
        return false;
    }
    const int colorBank = (int16_t)(blob[1] | (blob[2] << 8));

    Reader rd{ blob, blob + 5, blob + size };

    // Running destination offset, in framebuffer bytes. Origin = (dstX, dstY) —
    // Help_BlitImage's "pDst + nClipW + nClipH*nPitch".
    long pos = (long)dstY * kPitch + dstX;

    // Strip walk: [s16 yAdvance][s16 nLines] repeated until nLines == 0, or the
    // running offset reaches the end of the surface (engine's nEnd check).
    int nLines = 1;
    while (nLines != 0 && rd.ok) {
        int16_t yAdvance = rd.s16();
        nLines = rd.s16();
        pos += (long)yAdvance * kPitch;
        if (nLines == 0) {
            break;
        }
        for (int i = 0; i < nLines && rd.ok; ++i) {
            Dst dst{ fb.pixels(), pos };
            if (!putLine(rd, dst, colorBank)) {
                return false;
            }
            pos += kPitch;
            if (pos >= kSurface) {              // mirror engine's nEnd <= pDst
                return true;
            }
        }
    }

    if (!rd.ok) {
        Log::warn("blitHelpImage: stream overran (size %zu)", size);
    }
    return true;
}
