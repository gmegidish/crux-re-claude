#pragma once

// HELPSTK.cpp -- Help-queue persistence + image-line (RLE sprite) primitives
// Original path: C:\DevStudio\Projects\Crux\HELPSTK.cpp
//
// This translation unit contains two unrelated groups of functions that the
// original compiler emitted into the same object file:
//
//   1. Help_SaveHelpStack / Help_LoadHelpStack
//      Serialize / deserialize the context-sensitive help queue (the array
//      that feeds the on-screen help bar) to/from a save-game stream.
//
//   2. Help_PutLine* / Help_LineDecode* / Help_BlitImage
//      Run-length (RLE) sprite line decoders + a tile/image blitter. The
//      original source path for the asserts in these is "Crux\Img.c"; they are
//      the low-level put_line primitives (see GI.cpp's thunk_FUN_00439d80).
//
// Help queue data model
// ---------------------
//   g_anGranHelpQueue[60]  (0x006d6770, 0xF0 bytes)  -- queued help item IDs
//   g_nGranHelpOwner       (0x006d6768)              -- owning item index
//   g_nGranHelpCount       (0x004d09b8)              -- live entries (-1=uninit)
// The save/load routines write/read these three records (queue blob, owner,
// count) in order, raising Err_SetRecord3 on a short read/write.

// ---- Help queue persistence ----------------------------------------------

void Help_SaveHelpStack(int hStream);   // 0x004391b0
void Help_LoadHelpStack(int hStream);   // 0x004393a0

// ---- RLE sprite-line primitives -------------------------------------------

// Decode one image opcode stream into a temporary line buffer, then copy it to
// the destination applying the horizontal X-scale step table (g_nXScaleSteps).
unsigned char *Help_PutLineScaled(unsigned char *pStream, char *pDst,
                                  int nColorBank, int nUnused, int nWidth); // 0x00439590

// opcode 0: copy a literal run of nLen bytes straight from the stream.
int  Help_LineCopyRun(int pStream, void *pDst, int nLen);                   // 0x004398e0

// opcode 1: signed RLE -- negative count = literal run, positive = fill run.
char *Help_LineDecodeRLE(char *pStream, char *pDst);                        // 0x00439980

// opcode 2: signed RLE skip -- negative count copies, positive count advances
//           the destination (transparent gap) without writing.
char *Help_LineSkipRLE(char *pStream, char *pDst);                          // 0x00439ad0

// opcode 3: RLE with a leading destination offset, terminated by a 0xFF byte;
//           gaps between runs are encoded as unsigned skip counts.
unsigned char *Help_LineDecodeRLEOffset(unsigned char *pStream,
                                        unsigned char *pDst);               // 0x00439bd0

// Blit a column-strip image (opcode 0x04 -> Bani_DrawBlocks, else per-line
// dispatch) into the destination surface, clipping to (nW, nH).
void Help_BlitImage(short *pStream, unsigned int pDst, int nClipW, int nClipH,
                    int nPitch, int nMaxOff);                               // 0x00439d80

// Per-line opcode dispatcher used by Help_BlitImage (one scanline).
unsigned char *Help_PutLine(unsigned char *pStream, unsigned int pDst,
                            int nColorBank);                                // 0x00439f70

// ---- Globals ---------------------------------------------------------------

extern int g_anGranHelpQueue[60];   // 0x006d6770  help-ID queue blob (0xF0 bytes)
// g_nGranHelpOwner / g_nGranHelpCount are declared in Graninv.h.
