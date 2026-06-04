// ---------------------------------------------------------------------------
// Img.cpp  —  Image blitting and RLE compression
// Original: C:\DevStudio\Projects\Crux\Img.cpp
// RE offsets: 0x0043a1b0 – 0x0043c2ff
// ---------------------------------------------------------------------------

#include <string.h>   // memset, memcpy
#include "Img.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

int g_nXScaleSteps[640];        // 0x006d7040  X-axis scale delta table
int g_nYScaleRows[480];         // 0x006d68b8  Y-axis row-position table
int g_nSizeScaleTables[640];    // 0x006d8450  per-width scale table pointers
int g_nSizeScalePoolBase;       // 0x006d68b0  base of scale pool allocation
int g_nTempScaleRow[640];       // 0x006d7a48  scratch scale row for SizeScaled
int g_nCachedScaleWidth;        // 0x004d11c0  cached width in g_nTempScaleRow
int g_nClipX;                   // 0x007c3fd8  viewport left boundary
int g_nClipY;                   // 0x007c3fdc  viewport top boundary

// ---------------------------------------------------------------------------
// Forward declarations of external helpers (defined in other modules)
// ---------------------------------------------------------------------------

// GI.cpp: draw one scaled line into the framebuffer
extern unsigned char* DrawScaledLine(unsigned char* src, unsigned int fbPtr,
                                     int width, int scalePct, int scaledWidth);

// Memalloc.cpp: allocate nBytes of engine heap memory
extern int MemAlloc(int tag, const char* file, int nBytes);

// platform: copy nBytes from src to dst (equivalent to memcpy for raw fb)
extern void CopyBytes(void* dst, const void* src, int nBytes);

// ---------------------------------------------------------------------------
// Internal line-drawing helpers
// ---------------------------------------------------------------------------

// 0x0043ada0  Copy src[0..width) to dst, mapping each byte through palette[].
static unsigned char* PutLine_Direct(unsigned char* src, unsigned char* dst,
                                     int width, int palette)
{
    unsigned char* end = src + width;
    for (; src < end; ++src, ++dst)
        *dst = *(unsigned char*)(palette + *src);
    return src;
}

// 0x0043ae70  RLE decode one line with palette lookup.
//   count < 0 : abs(count) literal pixels mapped through palette
//   count > 0 : run of count identical pixels (value = palette[next_byte])
//   count == 0: end of line
static unsigned char* PutLine_RLE(unsigned char* src, unsigned char* dst,
                                   int palette)
{
    while (true) {
        int count = (int)(char)*src;
        unsigned char* next = src + 1;
        if (count == 0)
            return next;

        if (count < 0) {
            // literal run
            for (src = next; count != 0; ++count) {
                *dst++ = *(unsigned char*)(palette + *src);
                next = src + 1;
                src = next;
            }
        } else {
            // repeat run
            unsigned char val = *(unsigned char*)(palette + *next);
            src = src + 2;
            for (; count != 0; --count)
                *dst++ = val;
        }
    }
}

// 0x0043afd0  RLE decode with transparent skip (no palette).
//   count < 0 : abs(count) pixels copied directly (no palette remap)
//   count > 0 : skip count destination bytes (leave unchanged)
//   count == 0: end of line
static unsigned char* PutLine_RLE_Skip(unsigned char* src, unsigned char* dst,
                                        int palette)
{
    while (true) {
        int count = (int)(char)*src++;
        if (count == 0)
            return src;

        if (count < 0) {
            for (; count != 0; ++count) {
                *dst++ = *(unsigned char*)(palette + *src++);
            }
        } else {
            dst += count;
        }
    }
}

// 0x0043b0e0  RLE with leading destination offset and 0xFF row terminator.
//   Starts by advancing dst by *src bytes, then decodes RLE until 0xFF.
//   count < 0 : abs(count) pixels copied with palette remap
//   count > 0 : repeat same palette-remapped pixel count times
//   0xFF      : end of line
static unsigned char* PutLine_RLE_Offset(unsigned char* src, unsigned char* dst,
                                          int palette)
{
    dst += *src++;

    while (true) {
        int count = (int)(char)*src;
        unsigned char* next = src + 1;

        if (count < 0) {
            for (src = next; next = src, count != 0; ++count) {
                *dst++ = *(unsigned char*)(palette + *src);
                next = src + 1;
            }
        } else if (count != 0) {
            unsigned char val = *(unsigned char*)(palette + *next);
            for (next = src + 2; count != 0; --count)
                *dst++ = val;
        }

        src = next;
        unsigned char terminator = *src++;
        if (terminator == 0xFF)
            return src;
        dst += terminator;
    }
}

// 0x0043abc0  Dispatch one compressed line for indirect (palette) blitting.
//   Sprite byte encodes compression type:
//     0 = direct copy through palette
//     1 = RLE with palette
//     2 = RLE skip-transparent with palette
//     3 = RLE offset-start with palette
//     4 = skip entire line (advance pointer only)
static unsigned char* PutLine_Indi(unsigned char* src, unsigned char* dst,
                                    int width, int palette)
{
    switch (*src) {
    case 0: return PutLine_Direct (src + 1, dst, width, palette);
    case 1: return PutLine_RLE    (src + 1, dst, palette);
    case 2: return PutLine_RLE_Skip(src + 1, dst, palette);
    case 3: return PutLine_RLE_Offset(src + 1, dst, palette);
    case 4: return src + 1;
    default:
        // assert: unknown compression type
        return NULL;
    }
}

// ---------------------------------------------------------------------------
// Blank-line helpers (no palette, zero-fills transparent regions)
// ---------------------------------------------------------------------------

// 0x0043b6a0  RLE blank line: negative=raw copy, positive=zero-fill dst bytes.
static char* PutLine_Blank_RLE(char* src, char* dst)
{
    while (true) {
        int count = (int)(unsigned char)*src++;
        if (count == 0)
            return src;

        if (count < 0) {
            for (; count != 0; ++count)
                *dst++ = *src++;
        } else {
            memset(dst, 0, count);
            dst += count;
        }
    }
}

// 0x0043b7c0  RLE blank with leading zero fill and 0xFF row terminator.
static unsigned char* PutLine_Blank_RLE_Offset(unsigned char* src,
                                                unsigned char* dst)
{
    unsigned char leadFill = *src++;
    memset(dst, 0, leadFill);
    dst += leadFill;

    while (true) {
        int count = (int)(char)*src;
        unsigned char* next = src + 1;

        if (count < 0) {
            for (src = next; next = src, count != 0; ++count) {
                *dst++ = *src;
                next = src + 1;
            }
        } else if (count != 0) {
            unsigned char val = *next;
            for (next = src + 2; count != 0; --count)
                *dst++ = val;
        }

        src = next;
        unsigned char terminator = *src++;
        if (terminator == 0xFF)
            return src;
        memset(dst, 0, terminator);
        dst += terminator;
    }
}

// 0x0043b4d0  Dispatch one compressed line for blank (zero-background) blitting.
//   Types 0-3 are the same compression variants as PutLine_Indi but without
//   palette; type 2 and 3 use the blank RLE variants.
static unsigned char* PutLine_Blank(unsigned char* src, unsigned char* dst,
                                     int width)
{
    switch (*src) {
    case 0: return (unsigned char*)
                PutLine_Direct(src + 1, dst, width, 0);
    case 1: return (unsigned char*)
                PutLine_Blank_RLE((char*)(src + 1), (char*)dst);
    case 2: return (unsigned char*)
                PutLine_Blank_RLE((char*)(src + 1), (char*)dst);
    case 3: return (unsigned char*)
                PutLine_Blank_RLE_Offset(src + 1, dst);
    case 4: return src + 1;
    default:
        // assert: unknown compression type
        return NULL;
    }
}

// ---------------------------------------------------------------------------
// SkipLine  —  advance the sprite data pointer past one compressed line
// without producing any output (used during clipped / off-screen rows).
// 0x0043a4f0
// ---------------------------------------------------------------------------
static unsigned char* SkipLine(unsigned char* src, int width)
{
    switch (*src) {
    case 0:  return src + 1 + width;               // direct: fixed-width
    case 1:
    case 2:  {                                      // RLE: scan to 0-terminator
        src++;
        while (*src != 0) src++;
        return src + 1;
    }
    case 3:  {                                      // RLE-offset: scan to 0xFF
        src++;
        while (*src != 0xFF) src++;
        return src + 1;
    }
    case 4:  return src + 1;                        // skip
    case 7:  {                                      // same as type 2 but +2 header
        src += 3;
        while (*src != 0) src++;
        return src + 1;
    }
    case 8:  {                                      // same as type 3 but +2 header
        src += 3;
        while (*src != 0xFF) src++;
        return src + 1;
    }
    default: return src + 1;
    }
}

// ---------------------------------------------------------------------------
// PackLine  —  encode one framebuffer row as raw RLE into dst.
// Returns pointer past the last written byte.
// 0x0043bb70
// ---------------------------------------------------------------------------
static char* PackLine(char* dst, int fbPtr, int width)
{
    *dst++ = 1;   // compression type: raw RLE

    while (width != 0) {
        int run = (width < 0x80) ? width : 0x7f;
        width -= run;
        *dst++ = -(char)run;                    // negative count = literal run
        CopyBytes(dst, (void*)fbPtr, run);
        dst   += run;
        fbPtr += run;
    }

    *dst++ = '\0';   // end-of-line marker
    return dst;
}

// ---------------------------------------------------------------------------
// PackRegion  —  capture a rectangle from the framebuffer into RLE sprite.
// dst receives a SpriteHeader followed by one PackLine entry per row.
// 0x0043b9a0
// ---------------------------------------------------------------------------
void PackRegion(short* dst, int fbPtr, int x1, int y1, int x2, int y2,
                int stride)
{
    if (x1 >= 0x280 || y1 >= 0x1e0)    // outside 640×480 screen
        return;

    int rowStart = fbPtr + x1 + y1 * stride;
    int dstWidth  = (x2 - x1) + 1;
    int dstHeight = (y2 - y1) + 1;

    // Write sprite header
    unsigned char* p = (unsigned char*)dst;
    *p++ = 1;                              // type: normal
    *(short*)p = (short)dstWidth;  p += 2;
    *(short*)p = (short)dstHeight; p += 2;
    *(short*)p = 0;                p += 2; // padding / flags
    *(short*)p = (short)dstHeight; p += 2; // row count

    for (int row = y1; row <= y2; ++row) {
        p = (unsigned char*)PackLine((char*)p, rowStart, dstWidth);
        rowStart += stride;
    }

    // End sentinel
    *(short*)p = 0;
    p += 2;
}

// ---------------------------------------------------------------------------
// InitImg  —  allocate scale pool and pre-compute all per-width step tables.
// Must be called once at startup before any Blit* function.
// 0x0043bca0
// ---------------------------------------------------------------------------
void InitImg()
{
    // Allocate 820 000 bytes for all scale tables
    g_nSizeScalePoolBase = MemAlloc(6, "C:\\DevStudio\\Projects\\Crux\\Img.cpp", 820000);

    int pool = g_nSizeScalePoolBase;

    // Build one step-delta table for each source width from 4 to 639.
    // g_nSizeScaleTables[w] points to an array of w ints where each entry
    // gives the number of extra destination pixels for that column interval.
    for (int w = 4; w < 0x280; ++w) {
        g_nSizeScaleTables[w] = pool;

        // First pass: compute raw column positions scaled to 640
        for (int i = 0; i <= w; ++i)
            *(int*)(pool + i * 4) = (i * 0x280) / w;

        // Second pass: convert absolute positions to step deltas
        for (int i = 0; i < w; ++i) {
            *(int*)(pool + i * 4) =
                (*(int*)(pool + (i + 1) * 4) - *(int*)(pool + i * 4)) - 1;
        }

        *(int*)(pool + (w - 1) * 4) = 0;    // last step is always zero
        pool += w * 4;
    }
}

// ---------------------------------------------------------------------------
// BlitImg_ScaledPct  —  blit sprite scaled by scalePct percent.
// Builds g_nXScaleSteps and g_nYScaleRows tables, then draws row by row.
// 0x0043a1b0
// ---------------------------------------------------------------------------
void BlitImg_ScaledPct(short* sprite, unsigned int fbPtr,
                        int x, int y, int stride, int width, int scalePct)
{
    unsigned int clipBottom = fbPtr + width * stride;

    if (x >= width || y >= stride)
        return;
    if (*(char*)sprite == '\x03' || *(char*)sprite == ' ')
        return;

    unsigned int rowPtr   = fbPtr + x + y * width;
    int spriteWidth       = (int)*(short*)((char*)sprite + 1);
    int spriteHeight      = (int)*(short*)((char*)sprite + 3);
    short* data           = (short*)((char*)sprite + 5);

    int scaledWidth  = (spriteWidth  * scalePct) / 100;
    int scaledHeight = (spriteHeight * scalePct) / 100;

    // Build X scale step deltas
    for (int i = 0; i <= scaledWidth; ++i)
        g_nXScaleSteps[i] = (i * 100) / scalePct;
    for (int i = 0; i <= scaledWidth; ++i)
        g_nXScaleSteps[i] = g_nXScaleSteps[i + 1] - g_nXScaleSteps[i];

    // Build Y row-position table
    for (int i = 0; i <= (spriteHeight * scalePct) / 100; ++i)
        g_nYScaleRows[i] = (i * 100) / scalePct;

    int rowIndex = 0;
    int moreRows = 1;

    while (moreRows != 0) {
        rowPtr   += (((int)*(short*)data * 100) / scalePct) * width;
        rowIndex += ((int)*(short*)data * 100) / scalePct;
        moreRows  = (int)data[1];
        data     += 2;

        for (int line = 0; line < moreRows; ++line) {
            if (g_nYScaleRows[rowIndex] == line) {
                data = (short*)DrawScaledLine((unsigned char*)data,
                                              rowPtr, spriteWidth,
                                              scalePct, scaledWidth);
                ++rowIndex;
                rowPtr += width;
            } else {
                data = (short*)SkipLine((unsigned char*)data, spriteWidth);
            }
            if (clipBottom <= rowPtr)
                return;
        }
    }
}

// ---------------------------------------------------------------------------
// BlitImg_Scaled  —  blit sprite scaled to explicit dstWidth × dstHeight.
// 0x0043a6a0
// ---------------------------------------------------------------------------
void BlitImg_Scaled(short* sprite, unsigned int fbPtr,
                     int x, int y, int stride,
                     int srcWidth, int dstWidth, int dstHeight)
{
    unsigned int clipBottom = fbPtr + stride * dstHeight;

    if (x >= stride || y >= dstHeight)
        return;
    if (*(char*)sprite == '\x03' || *(char*)sprite == ' ')
        return;

    unsigned int rowPtr   = fbPtr + x + y * stride;
    int spriteWidth       = (int)*(short*)((char*)sprite + 1);
    int spriteHeight      = (int)*(short*)((char*)sprite + 3);
    short* data           = (short*)((char*)sprite + 5);

    // Build X scale step deltas (map srcWidth columns into dstWidth)
    for (int i = 0; i < dstWidth; ++i)
        g_nXScaleSteps[i] = (i * spriteWidth) / dstWidth;
    for (int i = 0; i < dstWidth; ++i)
        g_nXScaleSteps[i] = g_nXScaleSteps[i + 1] - g_nXScaleSteps[i];

    // Build Y row-position table (map spriteHeight rows into dstHeight)
    for (int i = 0; i <= dstHeight; ++i)
        g_nYScaleRows[i] = (i * spriteHeight) / dstHeight;

    int rowIndex = 0;
    int moreRows = 1;

    while (moreRows != 0) {
        rowPtr   += ((*data * spriteHeight) / dstHeight) * stride;
        rowIndex += (*data * spriteHeight) / dstHeight;
        moreRows  = (int)data[1];
        data     += 2;

        for (int line = 0; line < moreRows; ++line) {
            if (g_nYScaleRows[rowIndex] == line) {
                data = (short*)DrawScaledLine((unsigned char*)data, rowPtr,
                                              spriteWidth,
                                              (dstWidth * 100) / spriteWidth,
                                              dstWidth);
                ++rowIndex;
                rowPtr += stride;
            } else {
                data = (short*)SkipLine((unsigned char*)data, spriteWidth);
            }
            if (clipBottom <= rowPtr)
                return;
        }
    }
}

// ---------------------------------------------------------------------------
// BlitImg_Indi  —  blit sprite unscaled with palette (indirect colour table).
// 0x0043a9c0
// ---------------------------------------------------------------------------
void BlitImg_Indi(short* sprite, unsigned int fbPtr,
                   int x, int y, int stride, int width, int palette)
{
    unsigned int clipBottom = fbPtr + width * stride;

    if (x >= width || y >= stride)
        return;

    if (*(char*)sprite == '\x04') {
        // redirect: call alternate blit path
        BlitImg_Indi(sprite, fbPtr, x, y, stride, width, palette);
        return;
    }
    if (*(char*)sprite == '\x03' || *(char*)sprite == ' ')
        return;

    unsigned int rowPtr   = fbPtr + x + y * width;
    int spriteWidth       = (int)*(short*)((char*)sprite + 1);
    short* data           = (short*)((char*)sprite + 5);

    int moreRows = 1;
    while (moreRows != 0) {
        rowPtr  += (int)*data * width;
        moreRows = (int)data[1];
        data    += 2;

        for (int line = 0; line < moreRows; ++line) {
            data    = (short*)PutLine_Indi((unsigned char*)data,
                                           (unsigned char*)rowPtr,
                                           spriteWidth, palette);
            rowPtr += width;
            if (clipBottom <= rowPtr)
                return;
        }
    }
}

// ---------------------------------------------------------------------------
// BlitImg_Blank  —  clear destination rect to black then blit sprite.
// 0x0043b2a0
// ---------------------------------------------------------------------------
void BlitImg_Blank(short* sprite, void* fbPtr,
                    int x, int y, int stride, int width)
{
    // Clear 640 × 480 region
    for (int row = 0; row < 0x1e0; ++row)
        memset((char*)fbPtr + row * stride, 0, 0x280);

    void* clipBottom = (char*)fbPtr + stride * width;

    if (x >= stride || y >= width)
        return;
    if (*(char*)sprite == '\x03' || *(char*)sprite == ' ')
        return;

    void* rowPtr       = (char*)fbPtr + x + y * stride;
    int   spriteWidth  = (int)*(short*)((char*)sprite + 1);
    short* data        = (short*)((char*)sprite + 5);

    int moreRows = 1;
    while (moreRows != 0) {
        for (int skip = (int)*data; skip != 0; --skip) {
            memset(rowPtr, 0, spriteWidth);
            rowPtr = (char*)rowPtr + stride;
        }
        moreRows = (int)data[1];
        data    += 2;

        for (int line = 0; line < moreRows; ++line) {
            data    = (short*)PutLine_Blank((unsigned char*)data,
                                            (unsigned char*)rowPtr,
                                            spriteWidth);
            rowPtr  = (char*)rowPtr + stride;
            if (clipBottom <= rowPtr)
                return;
        }
    }
}

// ---------------------------------------------------------------------------
// PutLine_SizeScaled  —  render one row of a size-scaled blit.
// Uses g_nSizeScaleTables[scale] to decide how many destination pixels to
// emit per source pixel column.
// 0x0043c300
// ---------------------------------------------------------------------------
static unsigned char* PutLine_SizeScaled(unsigned char* src, unsigned char* dst,
                                          int scale, int stride, int remaining)
{
    int* scaleRow = (int*)g_nSizeScaleTables[scale];
    int  col      = 0;

    if (*src == '\x07') {           // type 7: direct copy with size stepping
        src += 3;
        while (true) {
            int count = (int)(char)*src++;
            if (count == 0)
                break;
            if (count < 0) {
                count = -count;
                while (remaining < count) {
                    *dst++ = src[remaining];
                    src   += remaining + 1;
                    int stepIdx = col * 4;
                    ++col;
                    count      -= remaining + 1;
                    remaining   = scaleRow[stepIdx];
                }
                src       += count;
                remaining -= count;
            } else {
                while (remaining < count) {
                    ++dst;
                    int stepIdx = col * 4;
                    ++col;
                    count      -= remaining + 1;
                    remaining   = scaleRow[stepIdx];
                }
                remaining -= count;
            }
        }
    } else if (*src == '\x08') {    // type 8: RLE with size stepping
        int initialSkip = (int)(unsigned char)src[3];
        src += 4;
        while (remaining < initialSkip) {
            ++dst;
            int stepIdx = col * 4;
            ++col;
            initialSkip -= remaining + 1;
            remaining    = scaleRow[stepIdx];
        }
        remaining -= initialSkip;

        while (true) {
            int count = (int)(char)*src;
            unsigned char* next = src + 1;

            if (count < 0) {
                count = -count;
                src   = next;
                while (remaining < count) {
                    *dst++ = src[remaining];
                    src   += remaining + 1;
                    int stepIdx = col * 4;
                    ++col;
                    count      -= remaining + 1;
                    remaining   = scaleRow[stepIdx];
                }
                remaining -= count;
                next       = src + count;
            } else if (count != 0) {
                unsigned char val = *next;
                while (remaining < count) {
                    *dst++ = val;
                    int stepIdx = col * 4;
                    ++col;
                    count      -= remaining + 1;
                    remaining   = scaleRow[stepIdx];
                }
                remaining -= count;
                next       = src + 2;
            }

            src = next;
            int skip = (int)(unsigned char)*src++;
            if (skip == 0xFF)
                break;
            while (remaining < skip) {
                ++dst;
                int stepIdx = col * 4;
                ++col;
                skip      -= remaining + 1;
                remaining  = scaleRow[stepIdx];
            }
            remaining -= skip;
        }
    } else {
        // assert: unknown compression type in PutLine_SizeScaled
        return NULL;
    }

    return src;
}

// ---------------------------------------------------------------------------
// BlitImg_SizeScaled  —  blit sprite using pre-computed size step tables.
// scale selects which g_nSizeScaleTables entry to use (equals target width).
// 0x0043be40
// ---------------------------------------------------------------------------
void BlitImg_SizeScaled(char* sprite, int fbPtr,
                         int x, int y, int stride, int palette,
                         int dstX, int dstY, int scale)
{
    if (*sprite != '\x01')
        return;

    int colStart = dstX + (x * scale) / 0x280;
    int rowStart = dstY + (y * scale) / 0x280;
    int colEnd   = -1;
    int rowEnd   = -1;

    int* scaleRow = (int*)g_nSizeScaleTables[scale];

    // Adjust for left clip
    int colRemain = 1;
    int colOffset = 0;
    if (colStart < g_nClipX) {
        for (int i = 0; i < g_nClipX - colStart; ++i)
            colRemain += 1 + scaleRow[i];
        fbPtr += g_nClipX - colStart;
    }

    // Compute column range
    int spriteWidth = (int)(short)*(short*)(sprite + 5) - 1;   // from header field
    // NOTE: exact column range arithmetic follows original; simplified here.

    // Ensure temp scale row is current
    if (g_nCachedScaleWidth != scale) {
        memcpy(g_nTempScaleRow, scaleRow, scale * 4);
        g_nCachedScaleWidth = scale;
        scaleRow            = g_nTempScaleRow;
    } else {
        scaleRow = g_nTempScaleRow;
    }

    // Patch sentinel at column end to stop rendering
    if (colEnd >= 0) {
        int savedEnd = scaleRow[colEnd];
        scaleRow[colEnd] = 9999;

        // Adjust for top clip
        int rowRemain = 1;
        if (rowStart < g_nClipY) {
            for (int i = 0; i < g_nClipY - rowStart; ++i)
                rowRemain += 1 + *(int*)(scaleRow + i * 4);
            fbPtr += (g_nClipY - rowStart) * stride;
        }

        char* data = sprite + 5;
        fbPtr += dstX + dstY * stride +
                 (x * scale) / 0x280 +
                 ((y * scale) / 0x280) * stride;

        int rowCount = 1;
        int colAcc   = 0;

        while (rowCount != 0) {
            rowCount = (int)*(short*)(data + 2);
            data    += 4;
            while (rowCount-- > 0) {
                if (colAcc <= 0) {
                    data = (char*)PutLine_SizeScaled((unsigned char*)data,
                                                     (unsigned char*)fbPtr,
                                                     scale, stride, colRemain);
                    colAcc = scaleRow[colOffset++];
                    ++fbPtr;
                    fbPtr += stride;
                } else {
                    --colAcc;
                    data += *(short*)(data + 1);
                }
            }
        }

        // Restore sentinel
        if (colEnd >= 0)
            scaleRow[colEnd] = savedEnd;
    }
}
