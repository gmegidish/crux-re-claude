// AREAS.cpp — Room node/area management and hit-testing
//
// Provides the spatial data structures that map screen coordinates to game
// areas/nodes.  The central hit-test function (Area_FindAt) is called every
// time the cursor moves to identify what the player is pointing at.
//
// Architecture overview:
//   - Node table: g_pAreaNodeTable is an array of pointers to node records.
//     Each record is at least 0x94 bytes.  The first four ints are a bounding
//     box (x, y, x2, y2); +0x14 is a tag ID; +0x90 is a Z depth used for
//     priority comparison; +0x94 is a type flag (2 = exclude from hit-test).
//   - Y-bucket index (g_anAreaYBuckets): 120 rows (y/4), each holding a
//     -1-terminated list of node indices.  Area_AddNodeToYBuckets fills this;
//     Area_ClearYBuckets wipes it.  Area_FindAt uses row y/4 to narrow the
//     candidate set before doing full bbox tests.
//   - Sprite-area list (g_anAreaSpriteList): up to ~N records (stride 0x20),
//     each describing a rectangle and an area-ID return value.  These represent
//     on-screen sprites that have been "registered" as clickable hit zones,
//     independent of the walk-node table.
//   - Selection list (g_anAreaList): a small cursor-indexed int array used by
//     the adventure engine to iterate over a set of area IDs, managed via the
//     Area_List* family.
//
// Area_FindAt search order:
//   1.  Sprite-area list (high-priority hit zones).  If a matching sprite has
//       flags==0, return immediately.  Otherwise remember the best Z.
//   2.  Active-node bounding box (the node the character is currently on).
//   3.  Y-bucket rows (walk-node rectangles with depth comparison).
//
// The function is serialised with g_nAreaCritSec (CRITICAL_SECTION).
//
// Original source: C:\DevStudio\Projects\Crux\AREAS.cpp
// Last boundary function in this file: Area_ListAppend (0x00414ed0)
// Note: Bani_PutBlock (0x00414f90) shares this address range but belongs to
//       BANI.cpp (debug string references C:\DevStudio\Projects\Crux\BANI*).

#include "AREAS.h"
#include <windows.h>

// ============================================================
//  Globals
// ============================================================

// Cached result of the last Area_FindAt call.
int  g_nAreaLastHit      = -1;   // 004c7db4

// Total number of nodes in g_pAreaNodeTable.
int  g_nAreaNodeCount    = 0;    // 007127e8

// Currently active walk node index (-1 = none).
int  g_nAreaActiveNode   = -1;   // 0070c248

// Number of active entries in the sprite-area list.
int  g_nAreaSpriteCount  = 0;    // 005f3308

// Selection list: count, cursor, and data array.
int  g_nAreaListCount    = 0;    // 00646328
int  g_nAreaListCursor   = 0;    // 0064632c
int  g_anAreaList[64];           // 00646330

// Y-bucket spatial index: 120 rows × 200 int slots (stride 800 bytes).
// Row index = screen-y / 4.  Each row stores node indices, -1-terminated.
int  g_anAreaYBuckets[120 * 200];  // 0062a120

// Sprite-area record array.  Each record is 8 ints (stride 0x20 = 32 bytes).
// Layout: x, y, offX, offY, areaId, nodeId, flags, (pad).
int  g_anAreaSpriteList[8 * 8];    // 00575780  (field aliases below for Ghidra clarity)
int  g_anAreaSpriteList_y;         // 00575784
int  g_anAreaSpriteList_offX;      // 00575788
int  g_anAreaSpriteList_offY;      // 0057578c
int  g_anAreaSpriteList_areaId;    // 00575790
int  g_anAreaSpriteList_nodeId;    // 00575794
int  g_anAreaSpriteList_flags;     // 00575798

// Array of pointers to node records.  g_pAreaNodeTable[i] → node struct.
int* g_pAreaNodeTable;             // 0070ded8

// Extended sprite table for node indices >= 0x96.  Each entry is a pointer to
// a 5-int record: { x, y, x2, y2, nodeIdx }.
int* g_pAreaExtraSpriteTable;      // 0070c360

// Active-node bounding box (4 separate ints, not a struct in original code).
int  g_nAreaActiveBBoxX2  = 0;    // 0062a100
int  g_nAreaActiveBBoxX1  = 0;    // 0062a104
int  g_nAreaActiveBBoxY2  = 0;    // 0062a108
int  g_nAreaActiveBBoxY1  = 0;    // 0062a10c

// CRITICAL_SECTION protecting Y-bucket table and sprite-area list.
// Stored as raw int in the original (sizeof CRITICAL_SECTION = 24 bytes at 0x0070c348).
int  g_nAreaCritSec;               // 0070c348

// Character walk table base for Z-depth comparisons (0x58 bytes per char).
int  g_nCharWalkTableBase;         // 005b10bc

// ============================================================
//  External dependencies
// ============================================================

// MOVEMENT.cpp — destination node (-1 = character is idle).
extern int  g_nMovDestNode;        // 006dd5d8

// GI.cpp / Advanim.cpp — retrieves a node's bbox relative to screen origin.
// Fills *pX, *pY, *pW, *pH (returns -1 in *pX if node has no screen rect).
extern void FUN_00408c40(int nNodeIdx, int* pX, int* pY, int* pW, int* pH);

// Debug / assert (ERRORS.cpp).
extern void Debug_Assert(const char* pszExpr, const char* pszFile, int nLine);
extern void thunk_FUN_00420e60(const char* pszFile, int nLine);
extern int* thunk_FUN_0041fad0(int a, void* b, int c);
extern void FUN_00489090(void* a, void* b);

// ============================================================
//  Area_AddNodeToYBuckets  (0x00413f10)
//
//  Register node nNodeIdx in every Y-bucket row whose scanline range
//  overlaps the node's bounding box.  Rows are y/4 (120 rows for 480 lines).
//  Each row is a -1-terminated list; the node index is appended before the
//  existing sentinel.  Called once per node when a room is loaded.
// ============================================================
void Area_AddNodeToYBuckets(int nNodeIdx)
{
    int* pNode = (int*)((int*)g_pAreaNodeTable)[nNodeIdx];

    for (int nRow = 0; nRow < 0x78 /* 120 */; nRow++)
    {
        // Only add if this row's scanline band (nRow*4 .. nRow*4+3) overlaps
        // the node's vertical extent [pNode[1] .. pNode[3]].
        if (pNode[1] <= nRow * 4 + 3 && nRow * 4 <= pNode[3])
        {
            // Walk to the end of this row's list and append.
            int nSlot = 0;
            int* pRow = &g_anAreaYBuckets[nRow * 200];
            while (pRow[nSlot] != -1)
                nSlot++;

            pRow[nSlot]     = nNodeIdx;
            pRow[nSlot + 1] = -1;   // re-terminate
        }
    }
}

// ============================================================
//  Area_ClearYBuckets  (0x004140a0)
//
//  Reset the Y-bucket table to empty: write a -1 sentinel at the start of
//  every row, and clear g_nAreaLastHit.  Serialised with g_nAreaCritSec.
// ============================================================
void Area_ClearYBuckets(void)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec);

    for (int nRow = 0; nRow < 0x78 /* 120 */; nRow++)
        g_anAreaYBuckets[nRow * 200] = -1;

    g_nAreaLastHit = -1;

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec);
}

// ============================================================
//  Area_FindAt  (0x00414180)
//
//  Find the topmost area/node at screen coordinate (nX, nY).
//
//  Screen is 320×200 in game-space (640×480 display).  Valid range:
//    0 <= nX <= 0x27F (639), 0 <= nY <= 0x1DF (479).
//
//  Search order and priority:
//    1. Sprite-area list: checked without the Y-bucket; uses Z depth from the
//       character table for priority.  If a sprite's flags==0, return immediately.
//    2. Active node (g_nAreaActiveNode): if the cursor is inside its saved
//       bbox and the node is walkable, seed the best-Z search.
//    3. Y-bucket row nY/4: iterate node indices, check bbox, compare Z.
//    4. Best sprite from step 1 wins if its Z > best node Z.
//
//  Returns area ID, or -1 if nothing found.  Updates g_nAreaLastHit.
//  Debug string: "find_area_int x_int y"
// ============================================================
int Area_FindAt(int nX, int nY)
{
    int nBestSpriteIdx = -1;
    int nBestZ         = -0x7FFFFFFF;
    int nResult        = -1;
    char bHit;

    // Bounds check: game screen is 640×480.
    if (nX < 0 || nX > 0x27F || nY < 0 || nY > 0x1DF)
    {
        g_nAreaLastHit = -1;
        return -1;
    }

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec);

    // --- Step 1: sprite-area list ---
    // Iterate in reverse so higher-indexed (more recently added) sprites win
    // ties.
    int nSpriteCount = g_nAreaSpriteCount;
    while (--nSpriteCount >= 0)
    {
        int* pRec = &g_anAreaSpriteList[nSpriteCount * 8];
        // pRec[0..3] = x, y, w, h — but Area_FindAt fetches screen bbox via
        // FUN_00408c40 first and then applies the record's offsets.
        int nNodeId = pRec[5 /* nodeId at +0x14 */];
        int nSX, nSY, nSW, nSH;
        FUN_00408c40(nNodeId, &nSX, &nSY, &nSW, &nSH);

        if (nSX != -1)
        {
            // If the record has a valid override offset (offX >= 0), apply it.
            if (pRec[2 /* offX */] >= 0)
            {
                nSX += pRec[2];
                nSY += pRec[3 /* offY */];
                nSW  = pRec[0 /* x = w override */];
                nSH  = pRec[1 /* y = h override */];
            }
            if (nSX <= nX && nX <= nSW + nSX &&
                nSY <= nY && nY <= nSH + nSY)
            {
                if (pRec[6 /* flags */] == 0)
                {
                    // Immediate match — return now.
                    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec);
                    nResult = pRec[4 /* areaId */];
                    g_nAreaLastHit = nResult;
                    return nResult;
                }
                // Depth-compare: remember best.
                if (nBestSpriteIdx == -1 ||
                    g_nCharWalkTableBase + ((int*)g_pAreaNodeTable)[g_anAreaSpriteList[nBestSpriteIdx * 8 + 5 /* nodeId */]] * 0x58 <
                    g_nCharWalkTableBase + nNodeId * 0x58)
                {
                    nBestSpriteIdx = nSpriteCount;
                }
            }
        }
    }

    // --- Step 2: active node ---
    // If the character is walking somewhere and is on a transparent area,
    // seed bBestZ with the active node's Z so walk-to nodes can still be found.
    {
        int* pActiveNode = (g_nAreaActiveNode != -1)
                           ? (int*)((int*)g_pAreaNodeTable)[g_nAreaActiveNode]
                           : NULL;

        if (g_nAreaActiveNode  != -1 &&
            g_nMovDestNode     != -1 &&
            ((pActiveNode[4 /* flags */] << 16) >> 24) == 0 &&
            g_nAreaActiveBBoxX1 <= nX && nX <= g_nAreaActiveBBoxX2 &&
            g_nAreaActiveBBoxY1 <= nY && nY <= g_nAreaActiveBBoxY2)
        {
            nBestZ   = pActiveNode[0x90 / 4 /* z */];
            nResult  = g_nAreaActiveNode;
        }
    }

    // --- Step 3: Y-bucket walk-node scan ---
    int nRow   = (nY + (nY >> 31 & 3)) >> 2;   // floor(nY / 4)
    int* pRow  = &g_anAreaYBuckets[nRow * 200];
    int  nSlot = 0;

    while (true)
    {
        int nNodeIdx = pRow[nSlot++];
        if (nNodeIdx == -1)
            break;

        bHit = 0;

        if (nNodeIdx < 0x96 /* normal node */)
        {
            int* pNode = (int*)((int*)g_pAreaNodeTable)[nNodeIdx];
            if (pNode[0x94 / 4 /* type */] != 2 /* excluded */)
            {
                // AABB test.
                if (pNode[0] <= nX && nX <= pNode[2] &&
                    pNode[1] <= nY && nY <= pNode[3])
                {
                    bHit = 1;
                }
                // Z-compare.
                if (bHit &&
                    ((pNode[0x10 / 4 /* flags */] << 16) >> 24) == 0 &&
                    nBestZ < pNode[0x90 / 4 /* z */])
                {
                    nBestZ  = pNode[0x90 / 4];
                    nResult = nNodeIdx;
                }
            }
        }
        else
        {
            // Extended sprite record (nodeIdx >= 0x96).
            int* pExt = ((int**)g_pAreaExtraSpriteTable)[nNodeIdx];
            // pExt: {x, y, x2, y2, nodeIdx}
            int* pNode = (int*)((int*)g_pAreaNodeTable)[pExt[4]];

            if (pExt[0] <= nX && nX <= pExt[2] &&
                pExt[1] <= nY && nY <= pExt[3])
            {
                bHit = 1;
            }
            if (bHit &&
                ((pNode[0x10 / 4 /* flags */] << 16) >> 24) == 0 &&
                nBestZ < pNode[0x90 / 4 /* z */])
            {
                nBestZ  = pNode[0x90 / 4];
                nResult = pExt[4];
            }
        }
    }

    // --- Step 4: best sprite vs best node ---
    if (nBestSpriteIdx != -1)
    {
        int nSpriteNodeId = g_anAreaSpriteList[nBestSpriteIdx * 8 + 5];
        int nSpriteZ = *(int*)(g_nCharWalkTableBase + nSpriteNodeId * 0x58);
        if (nBestZ < nSpriteZ)
            nResult = g_anAreaSpriteList[nBestSpriteIdx * 8 + 4 /* areaId */];
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec);
    g_nAreaLastHit = nResult;
    return nResult;
}

// ============================================================
//  Area_GetLastHit  (0x004146d0)
//
//  Return the cached area ID from the most recent Area_FindAt call.
//  Thread-safe read (no lock needed — atomic 32-bit read on x86).
// ============================================================
int Area_GetLastHit(void)
{
    return g_nAreaLastHit;
}

// ============================================================
//  Area_FindNodeByTag  (0x00414760)
//
//  Linear scan of the node table for the first node whose tag field
//  (+0x14 = int[5]) equals nTag.  Returns node index, or -1 if not found.
// ============================================================
int Area_FindNodeByTag(int nTag)
{
    int nIdx = 0;
    while (nIdx < g_nAreaNodeCount &&
           ((int*)g_pAreaNodeTable)[nIdx * 1 /* ptr table, each entry is a pointer */],
           (*(int*)(((int*)g_pAreaNodeTable)[nIdx] + 0x14)) != nTag)
    {
        nIdx++;
    }
    if (nIdx == g_nAreaNodeCount)
        nIdx = -1;
    return nIdx;
}

// ============================================================
//  Area_RemoveSprite  (0x00414840)
//
//  Remove all sprite-area records whose nodeId field (+0x14) equals nNodeId.
//  Compacts the array in place (shift remaining entries down); decrements
//  g_nAreaSpriteCount for each removal.
// ============================================================
void Area_RemoveSprite(int nNodeId)
{
    int nShift = 0;   // number of removed entries so far (used as compaction offset)

    for (int i = 0; i < g_nAreaSpriteCount; i++)
    {
        if (g_anAreaSpriteList[(i + nShift) * 8 + 5 /* nodeId */] == nNodeId)
        {
            nShift++;
            g_nAreaSpriteCount--;
            // Copy the entry that was displaced into slot i.
            int* pDst = &g_anAreaSpriteList[i * 8];
            int* pSrc = &g_anAreaSpriteList[(i + nShift) * 8];
            for (int k = 0; k < 8; k++)
                pDst[k] = pSrc[k];
            i--;   // re-examine this slot
        }
        else
        {
            // Still compact even non-removed entries.
            int* pDst = &g_anAreaSpriteList[i * 8];
            int* pSrc = &g_anAreaSpriteList[(i + nShift) * 8];
            for (int k = 0; k < 8; k++)
                pDst[k] = pSrc[k];
        }
    }
}

// ============================================================
//  Area_RemoveSpriteAt  (0x00414990)
//
//  Remove the sprite-area record at position nIdx by shifting all subsequent
//  entries one slot towards the start.  Decrements g_nAreaSpriteCount.
// ============================================================
void Area_RemoveSpriteAt(int nIdx)
{
    for (int i = nIdx; i < g_nAreaSpriteCount - 1; i++)
    {
        int* pDst = &g_anAreaSpriteList[i * 8];
        int* pSrc = &g_anAreaSpriteList[(i + 1) * 8];
        for (int k = 0; k < 8; k++)
            pDst[k] = pSrc[k];
    }
    g_nAreaSpriteCount--;
}

// ============================================================
//  Area_ResetList  (0x00414a80)
//
//  Clear both the count and the cursor of the selection list.
//  After this call g_nAreaListCount == 0 and g_nAreaListCursor == 0.
// ============================================================
void Area_ResetList(void)
{
    g_nAreaListCount  = 0;
    g_nAreaListCursor = 0;
}

// ============================================================
//  Area_RewindList  (0x00414b20)
//
//  Reset the cursor to the start of the selection list without clearing it.
// ============================================================
void Area_RewindList(void)
{
    g_nAreaListCursor = 0;
}

// ============================================================
//  Area_SeekListEnd  (0x00414bb0)
//
//  Position the cursor on the last item (count - 1).
// ============================================================
void Area_SeekListEnd(void)
{
    g_nAreaListCursor = g_nAreaListCount - 1;
}

// ============================================================
//  Area_ListNext  (0x00414c40)
//
//  Advance the cursor by 1.
//  Returns 0 on success, -1 if the cursor was already at or past the last
//  item (cursor is clamped to count-1 in that case).
// ============================================================
int Area_ListNext(void)
{
    g_nAreaListCursor++;
    if (g_nAreaListCursor < g_nAreaListCount)
        return 0;

    g_nAreaListCursor = g_nAreaListCount - 1;
    return -1;
}

// ============================================================
//  Area_ListPrev  (0x00414d00)
//
//  Retreat the cursor by 1.
//  Returns 0 on success, -1 if the cursor was already at 0 (cursor clamped).
// ============================================================
int Area_ListPrev(void)
{
    g_nAreaListCursor--;
    if (g_nAreaListCursor < 0)
    {
        g_nAreaListCursor = 0;
        return -1;
    }
    return 0;
}

// ============================================================
//  Area_ListGet  (0x00414db0)
//
//  Return the item at the current cursor position.
// ============================================================
int Area_ListGet(void)
{
    return g_anAreaList[g_nAreaListCursor];
}

// ============================================================
//  Area_ListSet  (0x00414e40)
//
//  Write nValue to the item at the current cursor position.
// ============================================================
void Area_ListSet(int nValue)
{
    g_anAreaList[g_nAreaListCursor] = nValue;
}

// ============================================================
//  Area_ListAppend  (0x00414ed0)
//
//  Append a new zero-initialised slot to the selection list; the cursor is
//  advanced to point at the new slot.  The caller should then call
//  Area_ListSet() to store a value.
// ============================================================
void Area_ListAppend(void)
{
    g_nAreaListCursor = g_nAreaListCount;
    int nOffset       = g_nAreaListCount * 4;   // byte offset into g_anAreaList
    g_nAreaListCount++;
    g_anAreaList[nOffset / 4] = 0;
}
