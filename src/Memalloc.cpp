// ---------------------------------------------------------------------------
// Memalloc.cpp  —  Sound-memory pool partitioning
// Original: C:\DevStudio\Projects\Crux\Memalloc.cpp
// RE offsets: 0x0043fa70 – 0x0043fd10  (tail of Memalloc.cpp)
// ---------------------------------------------------------------------------
// init_mems() allocates one large heap block and records:
//
//   g_nMemPoolBase / g_nMemPoolTotalSize          - primary region (bulk)
//   g_nMemPoolSecondaryBase / g_nMemPoolSecondarySize - reserved secondary region
//
// Mem_PickPoolLayout() then tries a handful of ways to lay out the three
// requested consumers (theme, music, speech) across those two regions and
// returns the index of the layout that wastes the least space.  Mem_PartitionPool()
// commits the chosen layout into the concrete base/size globals used by the
// rest of the engine.  All "base" values are byte offsets into the managed heap
// block, not raw pointers.
// ---------------------------------------------------------------------------

#include "Memalloc.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

int g_nMemPoolBase           = 0;  // 0x006dc038
int g_nMemPoolTotalSize      = 0;  // 0x006dc028
int g_nMemPoolSecondaryBase  = 0;  // 0x006dc03c
int g_nMemPoolSecondarySize  = 0;  // 0x006dc02c

int g_nThemeMemRegionBase    = 0;  // 0x006dc044
int g_nThemeMemRegionSize    = 0;  // 0x006dc020

int g_nMemBlockSecondaryBase = 0;  // 0x007d5c24
int g_nMemBlockSecondarySize = 0;  // 0x007d5f28

int g_nMemPoolLayoutOverflow = 0;  // 0x006dc048

// ---------------------------------------------------------------------------
// 0x0043fa70 — Mem_InitPools
// Boot the inventory / heap manager for the memory pools.
// ---------------------------------------------------------------------------
void Mem_InitPools(void)
{
    InitInvMang(1);
}

// ---------------------------------------------------------------------------
// 0x0043fb00 — Mem_PickPoolLayout
// Evaluate candidate split layouts for the requested theme/music/speech sizes
// across the primary pool (g_nMemPoolTotalSize) and the secondary region
// (g_nMemPoolSecondarySize).  Each candidate that fits is scored by the amount
// of primary-pool space it leaves available; the best-scoring layout index is
// returned.  Returns 0 if nothing fits.  Sets g_nMemPoolLayoutOverflow when the
// theme+music pair overflows the primary pool yet speech still fits the
// secondary region (drives the fix_sndmem error path in init_mems).
// ---------------------------------------------------------------------------
int Mem_PickPoolLayout(int nTheme, int nMusic, int nSpeech)
{
    int nBest = 0;
    int nBestScore = -1;
    int nScore;

    // Layout 1: speech in secondary, theme+music in primary.
    if (nSpeech < g_nMemPoolSecondarySize &&
        nTheme + nMusic < g_nMemPoolSecondarySize)
    {
        nBestScore = g_nMemPoolSecondarySize;
        nBest = 1;
    }

    // Layout 2: speech+music share the secondary, theme alone in primary.
    if (nSpeech + nMusic < g_nMemPoolSecondarySize &&
        nTheme < g_nMemPoolSecondarySize)
    {
        nScore = (g_nMemPoolSecondarySize + g_nMemPoolTotalSize) - nSpeech;
        if (nBestScore < nScore)
        {
            nBest = 2;
            nBestScore = nScore;
        }
    }

    // Layout 3: speech+theme share the secondary, music alone in primary.
    if (nSpeech + nTheme < g_nMemPoolSecondarySize &&
        nMusic < g_nMemPoolSecondarySize)
    {
        nScore = (g_nMemPoolSecondarySize + g_nMemPoolTotalSize) - nSpeech;
        if (nBestScore < nScore)
        {
            nBest = 3;
            nBestScore = nScore;
        }
    }

    // Layout 4: speech+theme+music all packed into the primary pool.
    if (nSpeech + nTheme + nMusic < g_nMemPoolTotalSize &&
        nBestScore < g_nMemPoolTotalSize - nSpeech)
    {
        nBest = 4;
        nBestScore = g_nMemPoolTotalSize - nSpeech;
    }

    // Layout 5: theme+music in primary, speech in secondary (last resort).
    if (nMusic + nTheme < g_nMemPoolTotalSize &&
        nSpeech < g_nMemPoolSecondarySize)
    {
        if (nBestScore < g_nMemPoolTotalSize)
            nBest = 5;
    }
    else if (nSpeech < g_nMemPoolSecondarySize)
    {
        // theme+music do not fit the primary pool, but speech fits the
        // secondary region: flag the overflow for the fix_sndmem path.
        g_nMemPoolLayoutOverflow = 1;
    }

    return nBest;
}

// ---------------------------------------------------------------------------
// 0x0043fd10 — Mem_PartitionPool
// Commit the layout chosen by Mem_PickPoolLayout into concrete base offsets and
// sizes for the theme region, the secondary block and the music (sound) pool.
// Sizes computed with /10000 scaling are proportional splits of a region
// between theme and music in proportion to their requested sizes.
// ---------------------------------------------------------------------------
void Mem_PartitionPool(int nTheme, int nMusic, int nSpeech, int nLayout)
{
    switch (nLayout)
    {
    case 1:
        g_nThemeMemRegionBase    = g_nMemPoolBase;
        g_nThemeMemRegionSize    = g_nMemPoolTotalSize;
        g_nSndMemPoolBase        = g_nMemPoolSecondaryBase;
        g_nSndMemPoolSize        = (g_nMemPoolSecondarySize / 10000) *
                                   (nMusic / ((nMusic + nTheme) / 10000));
        g_nMemBlockSecondaryBase = g_nMemPoolSecondaryBase + g_nSndMemPoolSize;
        g_nMemBlockSecondarySize = g_nMemPoolSecondarySize - g_nSndMemPoolSize;
        break;

    case 2:
        g_nThemeMemRegionBase    = g_nMemPoolBase;
        g_nThemeMemRegionSize    = nSpeech;
        g_nSndMemPoolBase        = g_nMemPoolBase + nSpeech;
        g_nSndMemPoolSize        = g_nMemPoolTotalSize - nSpeech;
        g_nMemBlockSecondaryBase = g_nMemPoolSecondaryBase;
        g_nMemBlockSecondarySize = g_nMemPoolSecondarySize;
        break;

    case 3:
        g_nThemeMemRegionBase    = g_nMemPoolBase;
        g_nThemeMemRegionSize    = nSpeech;
        g_nMemBlockSecondaryBase = g_nMemPoolBase + nSpeech;
        g_nMemBlockSecondarySize = g_nMemPoolTotalSize - nSpeech;
        g_nSndMemPoolBase        = g_nMemPoolSecondaryBase;
        g_nSndMemPoolSize        = g_nMemPoolSecondarySize;
        break;

    case 4:
        g_nThemeMemRegionBase    = g_nMemPoolBase;
        g_nThemeMemRegionSize    = nSpeech;
        g_nMemBlockSecondaryBase = g_nMemPoolBase + nSpeech;
        g_nMemBlockSecondarySize = ((g_nMemPoolTotalSize - nSpeech) / 10000) *
                                   (nTheme / ((nMusic + nTheme) / 10000));
        g_nSndMemPoolBase        = g_nMemPoolBase + nSpeech + g_nMemBlockSecondarySize;
        g_nSndMemPoolSize        = (g_nMemPoolTotalSize - nSpeech) - g_nMemBlockSecondarySize;
        break;

    case 5:
        g_nThemeMemRegionBase    = g_nMemPoolSecondaryBase;
        g_nThemeMemRegionSize    = g_nMemPoolSecondarySize;
        g_nMemBlockSecondaryBase = g_nMemPoolBase;
        g_nMemBlockSecondarySize = (g_nMemPoolTotalSize / 10000) *
                                   (nTheme / ((nTheme + nMusic) / 10000));
        g_nSndMemPoolBase        = g_nMemPoolBase + g_nMemBlockSecondarySize;
        g_nSndMemPoolSize        = g_nMemPoolTotalSize - g_nMemBlockSecondarySize;
        break;
    }
}
