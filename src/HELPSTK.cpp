// HELPSTK.cpp -- Help-queue persistence + image-line (RLE sprite) primitives
// Original path: C:\DevStudio\Projects\Crux\HELPSTK.cpp
//
// See HELPSTK.h for the overview. Two groups of functions share this TU:
//   * Help_Save/LoadHelpStack -- serialize the on-screen help-bar queue.
//   * Help_PutLine* / Help_LineDecode* / Help_BlitImage -- low-level RLE sprite
//     line decoders + blitter (asserts reference the original "Crux\Img.c").

#include "HELPSTK.h"

// ---- Cross-module stubs (resolved at link time) ---------------------------

extern "C" {
// Stream I/O helpers (CRT-style buffered read/write that return byte counts).
unsigned int Stream_Write(int hStream, const void *pSrc, unsigned int nLen);  // 0x0048e4d0
unsigned int Stream_Read (int hStream, void *pDst, unsigned int nLen);        // 0x0048de80

// Raw block copy used by the literal-run line decoder.
void Mem_Copy(void *pDst, const void *pSrc, int nLen);                        // 0x004896d0

// Error / assert machinery.
void  Debug_Assert(int nLine, const char *pszFile, ...);                      // 0x...
void *Err_SetRecord3(int nCode, void *pRecord, int nArg);                     // Err_SetRecord3
void  Err_Throw(void *pRecord, void *pType);                                  // 0x00489090
void  Err_SetFileLine(int slot, const char *pszFile);                         // thunk_FUN_00420e60

// BANI block blitter (opcode 0x04 column-strip images).
void Bani_DrawBlocks(short *pStream, unsigned int pDst, int x, int y,
                     int pitch, int height);                                  // 0x00416670
}

// ---- Globals ---------------------------------------------------------------

int g_anGranHelpQueue[60];          // 0x006d6770

extern int g_nGranHelpOwner;        // 0x006d6768  (Graninv.cpp)
extern int g_nGranHelpCount;        // 0x004d09b8  (Graninv.cpp)

// Horizontal scale-step table consumed by Help_PutLineScaled.
extern unsigned char g_nXScaleSteps[];

// Error-record blobs for the throw paths.
extern void *g_pErrTypeDefault;     // 0x004ab3f8
extern void  g_ErrRecSaveQueue;     // 0x006d6860
extern void  g_ErrRecSaveOwner;     // 0x006d6864
extern void  g_ErrRecSaveCount;     // 0x006d6868
extern void  g_ErrRecLoadQueue;     // 0x006d686c
extern void  g_ErrRecLoadOwner;     // 0x006d6870
extern void  g_ErrRecLoadCount;     // 0x006d6874
extern void  g_ErrRecPutLineScaled; // 0x006d8e54
extern void  g_ErrRecPutLine;       // 0x006d8e50

// Throw an exception record built from Err_SetRecord3's 3-word return.
static void RaiseErrRecord(int nCode, void *pRecord)
{
    int *r = (int *)Err_SetRecord3(nCode, pRecord, -1);
    int rec[3] = { r[0], r[1], r[2] };
    Err_Throw(rec, &g_pErrTypeDefault);
}

// ===========================================================================
// Help-queue persistence
// ===========================================================================

// Help_SaveHelpStack -- write the help queue (blob, owner, count) to a stream.
// Each short write raises a code-0x22 (write) error record.
void Help_SaveHelpStack(int hStream)
{
    if (Stream_Write(hStream, g_anGranHelpQueue, 0xF0) < 0xF0)
        RaiseErrRecord(0x22, &g_ErrRecSaveQueue);

    if (Stream_Write(hStream, &g_nGranHelpOwner, 4) < 4)
        RaiseErrRecord(0x22, &g_ErrRecSaveOwner);

    if (Stream_Write(hStream, &g_nGranHelpCount, 4) < 4)
        RaiseErrRecord(0x22, &g_ErrRecSaveCount);
}

// Help_LoadHelpStack -- read the help queue back from a stream.
// Each short read raises a code-0x13 (read) error record.
void Help_LoadHelpStack(int hStream)
{
    if (Stream_Read(hStream, g_anGranHelpQueue, 0xF0) < 0xF0)
        RaiseErrRecord(0x13, &g_ErrRecLoadQueue);

    if (Stream_Read(hStream, &g_nGranHelpOwner, 4) < 4)
        RaiseErrRecord(0x13, &g_ErrRecLoadOwner);

    if (Stream_Read(hStream, &g_nGranHelpCount, 4) < 4)
        RaiseErrRecord(0x13, &g_ErrRecLoadCount);
}

// ===========================================================================
// RLE sprite-line primitives
// ===========================================================================

// Help_LineCopyRun (opcode 0) -- copy nLen literal bytes from stream to dst.
// Returns the stream pointer advanced past the copied run.
int Help_LineCopyRun(int pStream, void *pDst, int nLen)
{
    Mem_Copy(pDst, (const void *)pStream, nLen);
    return pStream + nLen;
}

// Help_LineDecodeRLE (opcode 1) -- signed run-length decoder.
//   count < 0 : literal run of |count| bytes
//   count > 0 : fill run of count copies of the next byte
//   count = 0 : end of line
char *Help_LineDecodeRLE(char *pStream, char *pDst)
{
    for (;;) {
        int nCount = (signed char)*pStream++;
        if (nCount == 0)
            break;

        if (nCount < 0) {
            // literal run
            while (nCount != 0) {
                *pDst++ = *pStream++;
                nCount++;
            }
        } else {
            // fill run
            char c = *pStream++;
            while (nCount != 0) {
                *pDst++ = c;
                nCount--;
            }
        }
    }
    return pStream;
}

// Help_LineSkipRLE (opcode 2) -- signed RLE that treats positive counts as
// transparent gaps (advance dst without writing).
//   count < 0 : copy |count| literal bytes
//   count > 0 : skip count destination bytes (leave them untouched)
//   count = 0 : end of line
char *Help_LineSkipRLE(char *pStream, char *pDst)
{
    for (;;) {
        int nCount = (signed char)*pStream++;
        if (nCount == 0)
            break;

        if (nCount < 0) {
            while (nCount != 0) {
                *pDst++ = *pStream++;
                nCount++;
            }
        } else {
            pDst += nCount;     // transparent gap
        }
    }
    return pStream;
}

// Help_LineDecodeRLEOffset (opcode 3) -- RLE with a leading destination skip
// and a 0xFF terminator. Layout per line:
//   [skip]            unsigned byte: initial transparent offset
//   repeated:
//     [count]         signed byte (negative=literal run, positive=fill run)
//     [gap]           unsigned byte: transparent skip after the run
//                     (0xFF ends the line)
unsigned char *Help_LineDecodeRLEOffset(unsigned char *pStream,
                                        unsigned char *pDst)
{
    pDst += *pStream++;                 // initial offset

    for (;;) {
        int nCount = (signed char)*pStream++;

        if (nCount < 0) {
            while (nCount != 0) {
                *pDst++ = *pStream++;
                nCount++;
            }
        } else if (nCount != 0) {
            unsigned char c = *pStream++;
            while (nCount != 0) {
                *pDst++ = c;
                nCount--;
            }
        }

        unsigned char nGap = *pStream++;
        if (nGap == 0xFF)
            break;
        pDst += nGap;                   // transparent gap
    }
    return pStream;
}

// Help_PutLine -- decode one scanline by leading opcode byte and write it
// directly into the destination surface. Used by Help_BlitImage.
//   0 : literal run     (Help_LineCopyRun)
//   1 : signed RLE      (Help_LineDecodeRLE)
//   2 : signed RLE skip (Help_LineSkipRLE)
//   3 : RLE w/ offset   (Help_LineDecodeRLEOffset)
//   4 : skip whole line (advance one byte, no draw)
//   7 : RLE skip,  payload at +3
//   8 : RLE offset, payload at +3
// Any other opcode asserts and returns NULL.
unsigned char *Help_PutLine(unsigned char *pStream, unsigned int pDst,
                            int nColorBank)
{
    switch (*pStream) {
    case 0:
        pStream = (unsigned char *)Help_LineCopyRun((int)(pStream + 1),
                                                    (void *)pDst, nColorBank);
        break;
    case 1:
        pStream = (unsigned char *)Help_LineDecodeRLE((char *)(pStream + 1),
                                                      (char *)pDst);
        break;
    case 2:
        pStream = (unsigned char *)Help_LineSkipRLE((char *)(pStream + 1),
                                                    (char *)pDst);
        break;
    case 3:
        pStream = Help_LineDecodeRLEOffset(pStream + 1, (unsigned char *)pDst);
        break;
    case 4:
        pStream = pStream + 1;
        break;
    case 7:
        pStream = (unsigned char *)Help_LineSkipRLE((char *)(pStream + 3),
                                                    (char *)pDst);
        break;
    case 8:
        pStream = Help_LineDecodeRLEOffset(pStream + 3, (unsigned char *)pDst);
        break;
    default:
        Debug_Assert(0xE, "C:\\DevStudio\\Projects\\Crux\\Img.c", pStream);
        Debug_Assert(0xF, "C:\\DevStudio\\Projects\\Crux\\Img.c", *pStream);
        Err_SetFileLine(0x10, "C:\\DevStudio\\Projects\\Crux\\Img.c");
        RaiseErrRecord(0xF, &g_ErrRecPutLine);
        pStream = nullptr;
        break;
    }
    return pStream;
}

// Help_PutLineScaled -- decode one opcode stream into a scratch line buffer,
// then copy it to pDst horizontally scaled via g_nXScaleSteps. Source bytes
// equal to 0 are treated as transparent (not written).
unsigned char *Help_PutLineScaled(unsigned char *pStream, char *pDst,
                                  int nColorBank, int /*nUnused*/, int nWidth)
{
    char  scratch[640];
    char *pSrc = scratch;
    char *pOut = pDst;
    unsigned char *pNext;

    memset(scratch, 0, 0x27F);

    switch (*pStream) {
    case 0:
        pNext = (unsigned char *)Help_LineCopyRun((int)(pStream + 1),
                                                  scratch, nColorBank);
        break;
    case 1:
        pNext = (unsigned char *)Help_LineDecodeRLE((char *)(pStream + 1), scratch);
        break;
    case 2:
        pNext = (unsigned char *)Help_LineSkipRLE((char *)(pStream + 1), scratch);
        break;
    case 3:
        pNext = Help_LineDecodeRLEOffset(pStream + 1, (unsigned char *)scratch);
        break;
    case 4:
        // opcode 4: skip whole line, no scaled copy (early return).
        return pStream + 1;
    case 7:
        pNext = (unsigned char *)Help_LineSkipRLE((char *)(pStream + 3), scratch);
        break;
    case 8:
        pNext = Help_LineDecodeRLEOffset(pStream + 3, (unsigned char *)scratch);
        break;
    default:
        Debug_Assert(0x15, "C:\\DevStudio\\Projects\\Crux\\Img.c", *pStream);
        Err_SetFileLine(0x16, "C:\\DevStudio\\Projects\\Crux\\Img.c");
        RaiseErrRecord(0xF, &g_ErrRecPutLineScaled);
        pNext = nullptr;
        break;
    }

    // Horizontally rescale the scratch line onto the destination.
    for (int i = 0; i <= nWidth; i++) {
        if (*pSrc != '\0')
            *pOut = *pSrc;
        pSrc += g_nXScaleSteps[i];
        pOut++;
    }
    return pNext;
}

// Help_BlitImage -- blit a column-strip image into a destination surface.
//
// Header (shorts): [+0] opcode tag, [+1] color bank, then per-strip records:
//   short  yAdvance  -- rows to skip before this strip
//   short  nLines    -- number of scanlines in the strip
//   ...    payload   -- nLines opcode lines (decoded by Help_PutLine)
// The walk stops when nLines == 0 or the running destination offset reaches
// nMaxOff. Opcode 0x04 delegates the whole image to Bani_DrawBlocks; opcodes
// 0x03 (' ' == 0x20 byte tag) and 0x03 are treated as "nothing to draw".
void Help_BlitImage(short *pStream, unsigned int pDst, int nClipW, int nClipH,
                    int nPitch, int nMaxOff)
{
    int nLines = 1;
    unsigned int nEnd = pDst + nPitch * nMaxOff;

    char tag = *(char *)pStream;
    if (nClipW >= nPitch || nClipH >= nMaxOff || tag == '\x03' || tag == ' ')
        return;

    if (tag == '\x04') {
        Bani_DrawBlocks(pStream, pDst, nClipW, nClipH, nPitch, nMaxOff);
        return;
    }

    pDst = pDst + nClipW + nClipH * nPitch;
    short nColorBank = *(short *)((char *)pStream + 1);
    pStream = (short *)((char *)pStream + 5);

    while (nLines != 0) {
        pDst += *pStream * nPitch;          // yAdvance
        nLines = pStream[1];                // strip line count
        pStream += 2;

        for (int i = 0; i < nLines; i++) {
            pStream = (short *)Help_PutLine((unsigned char *)pStream, pDst,
                                            nColorBank);
            pDst += nPitch;
            if (nEnd <= pDst)
                return;
        }
    }
}
