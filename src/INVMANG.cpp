// INVMANG.cpp -- Inventory texture-atlas manager + item-list lookup
//
// Two responsibilities:
//
//   1. Texture atlas management for item icons.
//      Item icons are packed into a single flat pixel pool (g_pTexPool,
//      capped at INV_POOL_MAX = 29000 bytes).  Up to INV_MAX_SLOTS (50) slots
//      describe where each icon lives inside the pool.  The 9-byte slot record
//      stores an in_use flag, the icon's byte-size, its offset within the pool,
//      and its pixel width/height.  g_anItemSlot[i] holds the atlas slot index
//      for item i, or -1 when the icon is not currently loaded.
//
//   2. Linear-search helpers for the item list.
//      Inv_GetByTag and Inv_GetByName scan the g_apItems[] array (length
//      g_nItemCount) and return the index of the first matching item, or -1.
//
// Original source: C:\DevStudio\Projects\Crux\INVMANG.cpp
// Address range:   0x0043c7fd -- 0x0043d6cb

#include "INVMANG.h"
#include <windows.h>
#include <string.h>

// External helpers
extern "C" {
    // Resource loader: open a named resource pack file, used by Inv_GetResource.
    // Returns 0 on success.
    int  thunk_FUN_0045d4e0(int nMode, const char *pszName, char *pszBuf,
                            char bFlags, void *pFile, int nUnk);

    // FreeResource via thunk
    void thunk_FUN_00420e60(int nLine, const char *pFile);

    // Res_BunchFreadNow -- read nSize*nCount bytes from a resource file handle.
    int  Res_BunchFreadNow(void *pDst, int nSize, int nCount, void *pFile);

    // FUN_00489d20 -- memmove-like copy within g_pTexPool.
    void FUN_00489d20(void *pDst, const void *pSrc, unsigned short nSize);

    // FUN_00489090 -- assert/error helper.
    void FUN_00489090(void *pMsg, const void *pContext);

    // FUN_0041fad0 -- format an error message (returns pointer to message struct).
    void *thunk_FUN_0041fad0(int nCode, const void *pArg, int nExtra);

    // FUN_004895e0 -- copy item name string from object into buffer.
    void FUN_004895e0(char *pBuf, int nSrc);

    // FUN_0049a830 -- string compare (strcmp wrapper), returns 0 on match.
    int  FUN_0049a830(const char *pA, const char *pB);
}

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// 0x006d95f0 -- per-item atlas slot mapping; -1 = not loaded
// (Ghidra: g_anItemSlot — stride 4, one int per item)
int   g_anItemSlot[1];          // array length is g_nItemCount at runtime

// 0x006d9dc0 -- raw atlas slot table, 9 bytes × INV_MAX_SLOTS
// Field layout within each 9-byte record:
//   [0]   char  in_use
//   [1-2] short size         (bytes of pixel data)
//   [3-4] short pool_offset  (offset from g_pTexPool)
//   [5-6] short width
//   [7-8] short height
char  g_abInvSlots[INV_MAX_SLOTS * 9];

// 0x0070b5c0 -- total number of items in the item list
int   g_nItemCount;

// 0x0070d6f0 -- array of item object pointers
//   item_obj + 0x00 = int  tag
//   item_obj + 0xd0 = char* name
void *g_apItems[1];             // array length is g_nItemCount at runtime

// 0x007114cc -- base of the texture pixel data pool (max INV_POOL_MAX bytes)
void *g_pTexPool;

// ---------------------------------------------------------------------------
// Helper macros -- field accessors into the 9-byte slot record
// ---------------------------------------------------------------------------

#define SLOT_INUSE(s)        (g_abInvSlots[(s) * 9 + 0])
#define SLOT_SIZE(s)         (*(short *)(&g_abInvSlots[(s) * 9 + 1]))
#define SLOT_OFFSET(s)       (*(short *)(&g_abInvSlots[(s) * 9 + 3]))
#define SLOT_WIDTH(s)        (*(short *)(&g_abInvSlots[(s) * 9 + 5]))
#define SLOT_HEIGHT(s)       (*(short *)(&g_abInvSlots[(s) * 9 + 7]))

// ---------------------------------------------------------------------------
// Inv_FreeSlot -- 0x0043c910
//
// If g_anItemSlot[nItemIdx] is a valid slot (>= 0), clears that slot's in_use
// byte and resets the mapping to -1 (0xFFFFFFFF).
// ---------------------------------------------------------------------------
void Inv_FreeSlot(int nItemIdx)
{
    int nSlot = g_anItemSlot[nItemIdx];
    if (nSlot >= 0) {
        SLOT_INUSE(nSlot) = 0;
        g_anItemSlot[nItemIdx] = -1;
    }
}

// ---------------------------------------------------------------------------
// Inv_AllocSlot -- 0x0043d040
//
// Find a free atlas slot that can hold nSize bytes beginning at the pool
// position that follows the previous slot's data.  Strategy:
//
//   Pass 1 (greedy): walk the slot table in order.  If the next contiguous
//     free slot fits nSize below the INV_POOL_MAX watermark, return it.  If a
//     free slot whose *successor* slot has spare capacity is found first,
//     return that.
//
//   Compaction: when no slot fits without exceeding INV_POOL_MAX, repeatedly
//     evict the last occupied slot (clearing its in_use, size, and all
//     g_anItemSlot back-references), then re-try.
//
//   Slide: occupied runs of slots are moved left into gaps left by freed
//     entries.  All pixel data is copied within the pool via FUN_00489d20, and
//     all g_anItemSlot entries pointing into the moved range are decremented by
//     the gap width.
//
// Returns the allocated slot index in the low byte of the return value.
// ---------------------------------------------------------------------------
int Inv_AllocSlot(int nSize)
{
    unsigned int nSlot = 0;

    do {
        // ---- look for a fully-empty slot at the current watermark ----
        while (nSlot <= 0x31 && SLOT_SIZE(nSlot) != 0) {
            // skip in-use slots; stop if we find a free one
            if (SLOT_INUSE(nSlot) == 0)
                break;
            nSlot++;
        }

        if (nSlot > 0x31 || SLOT_SIZE(nSlot) == 0) {
            // ---- no partially-used slot found: look for any free slot ----
            nSlot = 0;
            while (nSlot < 0x32 && SLOT_INUSE(nSlot) != 0)
                nSlot++;

            if (nSlot < 0x32 &&
                (unsigned int)SLOT_OFFSET(nSlot) + nSize < INV_POOL_MAX) {
                // free slot found and fits in pool
                return (int)nSlot;
            }

            // ---- need to compact: evict tail slots until we fit ----
            while (nSlot == 0x32 ||
                   (unsigned int)SLOT_OFFSET(nSlot) + nSize > 28999) {
                unsigned int nTail = (nSlot - 1) * 9;
                g_abInvSlots[nTail] = 0;          // in_use = 0
                nSlot--;
                *(short *)(&g_abInvSlots[nSlot * 9 + 1]) = 0;  // size = 0

                // clear g_anItemSlot entries pointing at the evicted slot
                for (int i = 0; i < g_nItemCount; i++) {
                    if (g_anItemSlot[i] == (int)nSlot)
                        g_anItemSlot[i] = -1;
                }
            }
            return (int)nSlot;
        }

        // ---- look for a run of free slots to coalesce into ----
        unsigned int nRunStart = nSlot;
        unsigned int nFreeEnd  = nSlot;
        do {
            nFreeEnd++;
            if (nFreeEnd > 0x31 || SLOT_SIZE(nFreeEnd) == 0)
                break;
        } while (SLOT_INUSE(nFreeEnd) == 0);

        // find end of the occupied run following the free gap
        unsigned int nOccEnd = nFreeEnd;
        if (nFreeEnd < 0x32) {
            nOccEnd = nRunStart;
        }
        for (; nOccEnd < 0x32 && SLOT_INUSE(nOccEnd) != 0; nOccEnd++)
            ;

        int nOccLast = (int)nOccEnd - 1;

        // ---- slide occupied slots left into the free gap ----
        nSlot = nFreeEnd;
        unsigned int nDst = nRunStart;
        while ((int)nSlot <= nOccLast) {
            unsigned int nGapDst = (nRunStart + nSlot) - nFreeEnd;
            SLOT_SIZE(nGapDst)   = SLOT_SIZE(nSlot);
            SLOT_WIDTH(nGapDst)  = SLOT_WIDTH(nSlot);
            SLOT_HEIGHT(nGapDst) = SLOT_HEIGHT(nSlot);
            SLOT_INUSE(nGapDst)  = SLOT_INUSE(nSlot);

            FUN_00489d20(
                (char *)g_pTexPool + SLOT_OFFSET(nGapDst),
                (char *)g_pTexPool + SLOT_OFFSET(nSlot),
                (unsigned short)SLOT_SIZE(nSlot));

            SLOT_INUSE(nSlot) = 0;
            SLOT_SIZE(nSlot)  = 0;

            // advance the pool-offset for the slot after the moved one
            SLOT_OFFSET(nGapDst + 1) = SLOT_OFFSET(nGapDst) + SLOT_SIZE(nSlot);

            nSlot++;
            nDst = nSlot;
        }

        // ---- fix up g_anItemSlot back-references ----
        nSlot = 0;
        while (nSlot < (unsigned int)g_nItemCount) {
            if (g_anItemSlot[nSlot] <= nOccLast &&
                (int)nFreeEnd <= g_anItemSlot[nSlot]) {
                g_anItemSlot[nSlot] -= (int)(nFreeEnd - nRunStart);
            }
            nSlot++;
        }

        nSlot = 0;
    } while (1);
}

// ---------------------------------------------------------------------------
// Inv_GetResource -- 0x0043c9d0
//
// Returns a pointer into g_pTexPool for item nItemIdx.  If the item is already
// loaded (g_anItemSlot[nItemIdx] >= 0) the function just reads the W/H from
// the slot and returns the pool pointer.
//
// On first call:
//   1. Opens the item's resource file by name (from item object offset 0xd0),
//      falling back to "DEFAULT" then emitting an assert.
//   2. Reads a 12-byte resource header; verifies magic (0x10) and version (1).
//   3. Reads a further 8 bytes to get size and the slot-offset hint.
//   4. Calls Inv_AllocSlot(size) to obtain an atlas slot.
//   5. Reads the pixel data into g_pTexPool + slot.pool_offset.
//   6. Updates width/height in the slot and marks it in_use.
//
// *pW and *pH receive the pixel dimensions on both paths.
// Returns (int)((char*)g_pTexPool + slot.pool_offset).
// ---------------------------------------------------------------------------
int Inv_GetResource(int nItemIdx, unsigned int *pW, unsigned int *pH, char bFlags)
{
    char szName[260];
    char szPath[260];
    char abHeader[16];
    char abInfo[4];
    unsigned int nSize;
    unsigned int nSlot;
    int  nData;
    void *pFile = NULL;
    int  nRc    = 0;

    int nCurSlot = g_anItemSlot[nItemIdx];

    if (nCurSlot < 0) {
        // item not yet loaded -- open resource file
        FUN_004895e0(szName, *(int *)((char *)g_apItems[nItemIdx] + 0xd0));

        nRc = thunk_FUN_0045d4e0(2, szName, szPath, 0, &pFile, 0);
        if (nRc != 0) {
            nRc = thunk_FUN_0045d4e0(2, "DEFAULT", szPath, bFlags, &pFile, 0);
        }
        if (nRc != 0) {
            // resource not found -- assert
            thunk_FUN_00420e60(0x19,
                "C:\\DevStudio\\Projects\\Crux\\INVMANG.cpp");
            void *pMsg = thunk_FUN_0041fad0(1, szName, 2);
            FUN_00489090(pMsg, (void *)0x004ab3f8);
        }

        // read 12-byte header
        Res_BunchFreadNow(abHeader, 0xc, 1, pFile);

        if (abHeader[0] == (char)0x10 && *(short *)(&abHeader[1]) == 1) {
            // read 8-byte info block: [0..3] = size, [4..7] = unused/offset
            Res_BunchFreadNow(abInfo, 8, 1, pFile);
            nSize = *(unsigned int *)abInfo;

            nSlot = 0xffffffff;

            // scan slots for a free or partial fit
            for (unsigned int i = 0; i < 0x32; i++) {
                if (SLOT_SIZE(i) == 0) {
                    if ((unsigned int)SLOT_OFFSET(i) + nSize < INV_POOL_MAX) {
                        nSlot = i;
                    }
                    break;
                }
                if (SLOT_INUSE(i) == 0 && nSize < (unsigned int)SLOT_SIZE(i)) {
                    nSlot = i;
                    break;
                }
            }

            if (nSlot == 0xffffffff)
                nSlot = (unsigned int)(char)Inv_AllocSlot(nSize);

            if (nSlot == 0xffffffff) {
                // still no slot -- fatal
                thunk_FUN_00420e60(0x35,
                    "C:\\DevStudio\\Projects\\Crux\\INVMANG.cpp");
                void *pMsg = thunk_FUN_0041fad0(0x24, (void *)0x006d9f84, 0xffffffff);
                FUN_00489090(pMsg, (void *)0x004ab3f8);
            }

            // read pixel data into pool
            nData = (int)g_pTexPool + (unsigned int)SLOT_OFFSET(nSlot);
            Res_BunchFreadNow((void *)nData, nSize, 1, pFile);

            SLOT_SIZE(nSlot) = (short)nSize;

            // update pool_offset of next slot if not the last and not reused
            if ((int)nSlot < 0x31) {
                SLOT_OFFSET(nSlot + 1) = SLOT_OFFSET(nSlot) + (short)nSize;
            }
        } else {
            // bad header magic
            thunk_FUN_00420e60(0x1f,
                "C:\\DevStudio\\Projects\\Crux\\INVMANG.cpp");
            void *pMsg = thunk_FUN_0041fad0(0xe, szName, 2);
            FUN_00489090(pMsg, (void *)0x004ab3f8);
        }

        // store W/H from pixel data header bytes [1..4]
        SLOT_WIDTH(nSlot)  = *(short *)((char *)g_pTexPool + SLOT_OFFSET(nSlot) + 1);
        SLOT_HEIGHT(nSlot) = *(short *)((char *)g_pTexPool + SLOT_OFFSET(nSlot) + 3);
        *pW = (unsigned int)SLOT_WIDTH(nSlot);
        *pH = (unsigned int)SLOT_HEIGHT(nSlot);

        SLOT_INUSE(nSlot) = 1;
        g_anItemSlot[nItemIdx] = (int)nSlot;

        nData = (int)g_pTexPool + (unsigned int)SLOT_OFFSET(nSlot);
    } else {
        // already loaded: fast path
        *pW = (unsigned int)SLOT_WIDTH(nCurSlot);
        *pH = (unsigned int)SLOT_HEIGHT(nCurSlot);
        nData = (int)g_pTexPool + (unsigned int)SLOT_OFFSET(nCurSlot);
    }

    return nData;
}

// ---------------------------------------------------------------------------
// Inv_GetByTag -- 0x0043d4e0
//
// Linear search through g_apItems[0..g_nItemCount-1].  For each entry,
// dereferences the pointer and compares the first int field (tag) against
// nTag.  Returns the index on match, or -1.
//
// Corresponds to the speculative call GetItemByTag(tag) in Graninv.cpp.
// ---------------------------------------------------------------------------
int Inv_GetByTag(int nTag)
{
    int i;
    for (i = 0; i < g_nItemCount; i++) {
        if (**(int **)((char *)&g_apItems + i * 4) == nTag)
            goto found;
    }
    i = -1;
found:
    return i;
}

// ---------------------------------------------------------------------------
// Inv_GetByName -- 0x0043d5b0
//
// Linear search through g_apItems[0..g_nItemCount-1].  For each entry,
// reads the string pointer at offset 0xd0 of the item object and calls
// FUN_0049a830 (strcmp).  Returns the index when the comparison returns 0,
// or -1 if no match.
// ---------------------------------------------------------------------------
int Inv_GetByName(const char *pszName)
{
    int i;
    for (i = 0; i < g_nItemCount; i++) {
        const char *pszItemName = *(const char **)
            ((char *)g_apItems[i] + 0xd0);
        if (FUN_0049a830(pszItemName, pszName) == 0)
            goto found;
    }
    i = -1;
found:
    return i;
}
