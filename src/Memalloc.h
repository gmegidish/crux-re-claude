#ifndef MEMALLOC_H
#define MEMALLOC_H

// ---------------------------------------------------------------------------
// Memalloc.h  —  Sound-memory pool partitioning
// Original: C:\DevStudio\Projects\Crux\Memalloc.cpp
// RE offsets: 0x0043fa70 – 0x0043fd10  (tail of Memalloc.cpp)
// ---------------------------------------------------------------------------
// init_mems() (0x0043f550) reads requested theme/music/speech sizes from the
// game INI, allocates one big managed pool via SafeHeap_Alloc, then asks this
// module to slice it.  Two cooperating helpers do the work:
//
//   Mem_PickPoolLayout()  evaluates several candidate split layouts and
//                         returns the index (1..5) of the best fit, or 0 if
//                         nothing fits.
//   Mem_PartitionPool()   given that layout index, assigns the concrete base
//                         offsets / sizes for the theme region, the secondary
//                         block and the music (sound) pool.
//
// Mem_InitPools() simply boots the inventory/heap manager (InitInvMang(1)).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Whole managed pool (allocated once in init_mems) -------------------------
extern int g_nMemPoolBase;            // 0x006dc038  base offset of primary pool
extern int g_nMemPoolTotalSize;       // 0x006dc028  total size of primary pool
extern int g_nMemPoolSecondaryBase;   // 0x006dc03c  base offset of secondary region
extern int g_nMemPoolSecondarySize;   // 0x006dc02c  size of secondary region

// Theme / primary region (set by Mem_PartitionPool) ------------------------
extern int g_nThemeMemRegionBase;     // 0x006dc044  base offset
extern int g_nThemeMemRegionSize;     // 0x006dc020  size

// Secondary memory block (set by Mem_PartitionPool) ------------------------
extern int g_nMemBlockSecondaryBase;  // 0x007d5c24  base offset
extern int g_nMemBlockSecondarySize;  // 0x007d5f28  size

// Music / sound pool (set by Mem_PartitionPool) ----------------------------
extern int g_nSndMemPoolBase;         // base offset of music pool
extern int g_nSndMemPoolSize;         // size of music pool

// Diagnostics --------------------------------------------------------------
extern int g_nMemPoolLayoutOverflow;  // 0x006dc048  set when no split fits

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

// External: heap / inventory manager bootstrap (defined elsewhere).
extern void InitInvMang(int mode);

// 0x0043fa70 — initialise the memory pools (boots the inventory manager).
void Mem_InitPools(void);

// 0x0043fb00 — choose the best pool-split layout for the requested
// theme/music/speech sizes; returns the layout index 1..5, or 0 if none fit.
int Mem_PickPoolLayout(int nTheme, int nMusic, int nSpeech);

// 0x0043fd10 — partition the managed pool into theme / secondary / music
// regions according to the previously chosen layout index.
void Mem_PartitionPool(int nTheme, int nMusic, int nSpeech, int nLayout);

#endif // MEMALLOC_H
