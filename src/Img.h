#ifndef IMG_H
#define IMG_H

// ---------------------------------------------------------------------------
// Img.h  —  Image blitting and RLE compression
// Original: C:\DevStudio\Projects\Crux\Img.cpp
// RE offset: 0x0043a1b0 – 0x0043c2ff
// ---------------------------------------------------------------------------

// Sprite header (5 bytes before row data).
// type 0x01 = normal sprite
// type 0x03 = invalid / end sentinel
// type 0x20 = empty / transparent
// type 0x04 = redirect (calls alternate blit path)
struct SpriteHeader {
    unsigned char type;
    short         width;
    short         height;
    // followed immediately by compressed row stream
};

// ---------------------------------------------------------------------------
// Globals (defined in Img.cpp, shared across modules)
// ---------------------------------------------------------------------------

// X-axis scale delta table [640].  g_nXScaleSteps[i] = step adjustment for
// scaled column i.  Rebuilt each call by BlitImg_ScaledPct / BlitImg_Scaled.
extern int g_nXScaleSteps[640];

// Y-axis row-position table [480].  g_nYScaleRows[i] = source row for
// destination row i.  Rebuilt each call by BlitImg_ScaledPct / BlitImg_Scaled.
extern int g_nYScaleRows[480];

// Per-width scale tables [640].  g_nSizeScaleTables[w] is a pointer (stored
// as int) into the pool; it gives the column step-delta row for width w.
// Populated once by InitImg().
extern int g_nSizeScaleTables[640];

// Base address of the 820 000-byte scale pool allocated by InitImg().
extern int g_nSizeScalePoolBase;

// Scratch copy of one scale row used by BlitImg_SizeScaled so sentinel values
// can be patched without corrupting the permanent table.
extern int g_nTempScaleRow[640];

// Width of the scale row currently loaded into g_nTempScaleRow.  Avoids a
// redundant memcpy when consecutive blits share the same scale width.
extern int g_nCachedScaleWidth;

// Clip/viewport left boundary (x).  Blits skip columns left of this value.
extern int g_nClipX;

// Clip/viewport top boundary (y).  Blits skip rows above this value.
extern int g_nClipY;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Allocate the scale pool and pre-compute all per-width step tables (4..639).
// Must be called once at startup before any Blit* function is used.
void InitImg();

// Blit sprite at (x, y) into the flat framebuffer, scaling by scalePct/100.
// stride = framebuffer row stride in bytes.
void BlitImg_ScaledPct(short* sprite, unsigned int fbPtr,
                       int x, int y, int stride, int width, int scalePct);

// Blit sprite scaled from its natural size to (dstWidth × dstHeight).
void BlitImg_Scaled(short* sprite, unsigned int fbPtr,
                    int x, int y, int stride,
                    int srcWidth, int dstWidth, int dstHeight);

// Blit sprite unscaled using an indirect colour-table (palette).
void BlitImg_Indi(short* sprite, unsigned int fbPtr,
                  int x, int y, int stride, int width, int palette);

// Blit sprite with a full black clear of the destination rect first.
void BlitImg_Blank(short* sprite, void* fbPtr,
                   int x, int y, int stride, int width);

// Blit sprite scaled using the pre-computed per-width size tables.
// dstX, dstY = destination tile offsets; scale = table index (width).
void BlitImg_SizeScaled(char* sprite, int fbPtr,
                        int x, int y, int stride, int palette,
                        int dstX, int dstY, int scale);

// Capture the rectangle (x1,y1)-(x2,y2) from framebuffer into dst as RLE.
// stride = framebuffer row stride.  Returns a fully encoded sprite header.
void PackRegion(short* dst, int fbPtr,
                int x1, int y1, int x2, int y2, int stride);

#endif // IMG_H
