// ANI32.cpp — "Animation 32-bit" helpers
//
// See ANI32.h for the high-level description.  Two functions:
//   Ani32_DrawScaledRLE   (0x00413530) — scaled RLE sprite blitter + bbox track
//   Ani32_BuildAreaLookup (0x00413bd0) — rebuild AREAS Y-bucket spatial index

#include "ANI32.h"
#include <windows.h>

// --- Globals (defined here, declared in ANI32.h) ---
int  g_nAni32HalfHero    = -1;        // 0x004c7ca4  (0xffffffff = not yet read)
int  g_nAni32ClipTop     = 0;         // 0x006e86cc
int  g_nAni32ClipBottom  = 0;         // 0x004d5270
int* g_apAreaCacheRecords = nullptr;  // 0x0070c5b8

// --- AREAS globals consumed here (owned by AREAS.cpp) ---
extern int  g_anAreaYBuckets[];       // 0062a120  120 rows x 200 int slots
extern int* g_pAreaNodeTable;         // 0070ded8  array of pointers to node records
extern int  g_nAreaNodeCount;         // 007127e8
extern int  g_nAreaActiveBBoxX1;      // 0062a104
extern int  g_nAreaActiveBBoxY1;      // 0062a10c
extern int  g_nAreaActiveBBoxX2;      // 0062a100
extern int  g_nAreaActiveBBoxY2;      // 0062a108
extern CRITICAL_SECTION g_nAreaCritSec;

// --- FILES.cpp ---
extern int  g_nAreaCacheActive;       // 007127ec  active entries in cache list

// --- ADVENT.cpp: animation column-extent sentinels ---
extern int  g_nAdvAnimSentinelMax;    // 004c7ca8  reset to 0x7fffffff
extern int  g_nAdvAnimSentinelMin;    // 0062a110  reset to 0

// --- CRUX.INI path (CURSORS.cpp / SETPAL.cpp) ---
extern char g_abIniPath[];            // 006299c0

// --- Helpers from other modules ---
extern UINT __stdcall Err_PushFrame(void);          // FUN_0048a620 (SEH frame setup helper)
extern void Err_SetRecord3(void);                   // Err_SetRecord3
extern void thunk_FUN_00420e60(void);
extern void FUN_00489090(void* a, void* b);
extern void Debug_Trace(int nId, const char* pszFile, const char* pszFmt, ...);

// Cache-record / extra-sprite area-record table base used by the lookup
// rebuild (the same "sprite/cache" list scanned by Area_FindAt).
extern int g_nAreaCacheTraceBase;     // 0x004c7db8  trace-id base for cache asserts

// ============================================================
//  Ani32_DrawScaledRLE  (0x00413530)
//
//  Decode an RLE-compressed paletted sprite, scale it to nScalePct percent of
//  its original size, and blit it into pDest column-by-column.  The sprite is
//  centred horizontally on nDestX and bottom-anchored at nDestY.
//
//  RLE byte format (one byte per token):
//    (b & 0xE0) == 0   -> transparent run of length b (skip b output pixels;
//                          colour cleared to 0 = transparent)
//    else              -> opaque run; length = b >> 5 (1..7), colour index =
//                          pPalMap[b & 0x1F]
//  A 0x00 byte terminates a source-column stream.
//
//  While drawing, the leftmost/rightmost destination columns that produced an
//  opaque pixel are tracked and merged into g_nAdvAnimSentinelMin/Max; the
//  resulting on-screen bounding box is published to the AREAS active-bbox
//  globals so the freshly-drawn sprite is hit-testable by Area_FindAt.
//  If [Mouse]/HalfHero is set, only the upper half of the sprite contributes
//  to the active-area Y extent.
// ============================================================
void Ani32_DrawScaledRLE(const unsigned char* pSprite, unsigned char* pDest,
                         int nDestX, int nDestY, int nPitch, int nMaxW,
                         int nMaxH, const unsigned char* pPalMap, int nScalePct)
{
    Err_PushFrame();   // SEH frame for the bounds-check throw below

    // Lazily read the HalfHero flag from CRUX.INI once.
    if (g_nAni32HalfHero == -1)
        g_nAni32HalfHero = GetPrivateProfileIntA("Mouse", "HalfHero", 0, g_abIniPath);

    // Sprite header: int16 width, int16 height, then the RLE stream.
    int nSrcW = *(const short*)pSprite;
    int nSrcH = *(const short*)(pSprite + 2);
    pSprite += 4;

    // Validate the source dimensions against the destination surface.
    if (nSrcW > nMaxW || nSrcW < 0 || nSrcH > nMaxH || nSrcH < 0)
    {
        thunk_FUN_00420e60();
        Err_SetRecord3();      // raises a Crux error record (does not return normally)
        FUN_00489090(nullptr, nullptr);
    }

    // Scaled destination dimensions.
    int nDstW = (nScalePct * nSrcW) / 100;
    int nDstH = (nScalePct * nSrcH) / 100;

    // Centre horizontally on nDestX, anchor bottom at nDestY.
    nDestX -= nDstW / 2;
    g_nAreaActiveBBoxY1 = nDestY - nDstH;
    // HalfHero: shrink the active-area Y extent to the upper half.
    g_nAreaActiveBBoxY2 = g_nAreaActiveBBoxY1 + nDstH / ((g_nAni32HalfHero != 0) + 1);

    // Destination cursor: top-left of the scaled sprite, minus one (pre-inc).
    unsigned char* pDst = pDest + nDestX + g_nAreaActiveBBoxY1 * nPitch - 1;

    // Precompute the source column index for each destination column...
    int aSrcCol[640];
    int nColStep = (nSrcW * 100) / nDstW;
    for (int x = 0; x < nDstW + 1; x++)
        aSrcCol[x] = (x * nColStep) / 100;

    // ...and the source row index for each destination row.
    int aSrcRow[480];
    int nRowStep = (nSrcH * 100) / nDstH;
    for (int y = 0; y < nDstH + 1; y++)
        aSrcRow[y] = (y * nRowStep) / 100;

    int  nSrcColCur  = 0;   // current decoded source column
    int  nLastOpaque = 0;   // rightmost dest column with an opaque pixel
    int  nFirstOpaque = 0;  // leftmost  dest column with an opaque pixel
    unsigned int uRunLen;
    unsigned int uPixel;    // packed: low byte = current colour

    for (int x = 0; x < nDstW; x++)
    {
        pDst += 1;
        unsigned char* pCol = pDst;

        // Read the first token of this column.
        unsigned char b = *pSprite++;
        if ((b & 0xE0) == 0)
        {
            uRunLen = b;
            uPixel  = 0;              // transparent
        }
        else
        {
            if (nFirstOpaque == 0)
                nFirstOpaque = x;
            nLastOpaque = x;
            uRunLen = b >> 5;
            uPixel  = pPalMap[b & 0x1F];
        }

        bool bColumnVisible =
            !(nDestX + x < g_nAni32ClipTop || g_nAni32ClipBottom < nDestX + x);

        if (!bColumnVisible)
        {
            // Off-screen column: decode the run lengths to keep the stream in
            // sync, but write nothing.
            unsigned int uStep;
            for (int y = 0; y < nDstH; y++)
            {
                for (; uRunLen <= (unsigned int)aSrcRow[y]; uRunLen += uStep)
                {
                    unsigned char bb = *pSprite++;
                    if ((bb & 0xE0) == 0)
                    {
                        uStep  = bb;
                        uPixel = 0;
                    }
                    else
                    {
                        uStep  = bb >> 5;
                        uPixel = pPalMap[bb & 0x1F];
                    }
                }
            }
        }
        else
        {
            // Visible column: emit one pixel per destination row.
            unsigned int uStep;
            for (int y = 0; y < nDstH; y++)
            {
                for (; uRunLen <= (unsigned int)aSrcRow[y]; uRunLen += uStep)
                {
                    unsigned char bb = *pSprite++;
                    if ((bb & 0xE0) == 0)
                    {
                        uStep  = bb;
                        uPixel = 0;
                    }
                    else
                    {
                        if (nFirstOpaque == 0)
                            nFirstOpaque = x;
                        nLastOpaque = x;
                        uStep  = bb >> 5;
                        uPixel = pPalMap[bb & 0x1F];
                    }
                }

                // Write the pixel if opaque and not before the surface start.
                if ((uPixel & 0xFF) != 0 && pCol >= pDest)
                    *pCol = (unsigned char)uPixel;

                pCol += nPitch;
            }
        }

        // Advance through any source columns skipped by the horizontal scale,
        // consuming their RLE streams (terminated by a 0x00 byte).
        for (; nSrcColCur < aSrcCol[x + 1]; nSrcColCur++)
        {
            while (*pSprite != 0)
                pSprite++;
            pSprite++;   // skip the 0x00 column terminator
        }
    }

    // Merge this sprite's opaque column extent into the running animation
    // sentinels and publish the active-area horizontal bbox.
    g_nAdvAnimSentinelMin = (g_nAdvAnimSentinelMin < nLastOpaque)
                          ? nLastOpaque : g_nAdvAnimSentinelMin;
    g_nAdvAnimSentinelMax = (nFirstOpaque < g_nAdvAnimSentinelMax)
                          ? nFirstOpaque : g_nAdvAnimSentinelMax;

    g_nAreaActiveBBoxX2 = nDestX + g_nAdvAnimSentinelMin;
    g_nAreaActiveBBoxX1 = nDestX + g_nAdvAnimSentinelMax;
}

// ============================================================
//  Ani32_BuildAreaLookup  (0x00413bd0)   debug: "area_lookup_init(void)"
//
//  Rebuild the entire Y-bucket spatial index in one pass.  For each of the 120
//  scanline rows (each covering 4 display lines), append:
//    1. every walkable node whose bbox overlaps the row, and
//    2. every active cache record (g_apAreaCacheRecords) that overlaps it,
//       stored with index offset +0x96 (150),
//  then terminate the row's slot list with -1.  A row may hold at most 200
//  entries; overflow raises a Crux error.  Serialised with g_nAreaCritSec.
// ============================================================
void Ani32_BuildAreaLookup(void)
{
    EnterCriticalSection(&g_nAreaCritSec);

    for (int nRow = 0; nRow < 0x78 /* 120 */; nRow++)
    {
        int nSlot = 0;
        int nY0 = nRow * 4;          // first display scanline of this row
        int nY3 = nRow * 4 + 3;      // last  display scanline of this row

        // --- Walkable nodes from the primary node table ---
        for (int i = 0; i < g_nAreaNodeCount; i++)
        {
            int* pNode = (int*)((int*)g_pAreaNodeTable)[i];
            if (pNode[1] <= nY3 && nY0 <= pNode[3])
            {
                if (nSlot > 199)
                {
                    Debug_Trace(g_nAreaCacheTraceBase + 0x10,
                                "C:\\DevStudio\\Projects\\Crux\\AREAS.cpp",
                                "Too many areas in row : %d", nRow);
                    thunk_FUN_00420e60();
                    Err_SetRecord3();
                    FUN_00489090(nullptr, nullptr);
                }
                g_anAreaYBuckets[nRow * 200 + nSlot] = i;
                nSlot++;
            }
        }

        // --- Active cache records (stored at bucket index i + 0x96) ---
        for (int i = 0; i < g_nAreaCacheActive; i++)
        {
            int* pRec = (int*)g_apAreaCacheRecords[i];
            if (pRec[1] <= nY3 && nY0 <= pRec[3])
            {
                if (nSlot > 199)
                {
                    Debug_Trace(g_nAreaCacheTraceBase + 0x1C,
                                "C:\\DevStudio\\Projects\\Crux\\AREAS.cpp",
                                "Too many areas in row : %d", nRow);
                    thunk_FUN_00420e60();
                    Err_SetRecord3();
                    FUN_00489090(nullptr, nullptr);
                }
                g_anAreaYBuckets[nRow * 200 + nSlot] = i + 0x96 /* 150 */;
                nSlot++;
            }
        }

        // Terminate the row.
        g_anAreaYBuckets[nRow * 200 + nSlot] = -1;
    }

    LeaveCriticalSection(&g_nAreaCritSec);
}
