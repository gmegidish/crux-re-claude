#pragma once
// AREAS.cpp — Room node/area management and hit-testing
//
// Owns the walkable-node pointer table, the Y-bucket spatial index for fast
// hit-testing, the sprite-area overlay list, and a small cursor-indexed area
// selection list.  Area_FindAt() is the primary entry point: given a screen
// (x,y) it returns the area/node index under the cursor.

// --- Node record layout (each entry pointed to by g_pAreaNodeTable[i]) ---
//   +0x00  int  x       bounding-box left
//   +0x04  int  y       bounding-box top       (also stored as "scanline limit" for Y-bucket)
//   +0x08  int  w       bounding-box right (x2)
//   +0x0C  int  h       bounding-box bottom (y2)
//   +0x10  int  flags   bit-field; (flags << 16) >> 24 == 0 → walkable
//   +0x14  int  tag     tag ID used by Area_FindNodeByTag
//   +0x90  int  z       depth/priority value (higher wins in hit-test)
//   +0x94  int  type    2 = excluded from hit-test

// --- Sprite-area record layout (g_anAreaSpriteList[], stride 0x20) ---
//   +0x00  int  x       bbox left
//   +0x04  int  y       bbox top
//   +0x08  int  offX    x offset applied when node override is active
//   +0x0C  int  offY    y offset applied when node override is active
//   +0x10  int  areaId  area ID returned on hit
//   +0x14  int  nodeId  node/character ID key (for remove-by-id)
//   +0x18  int  flags   0 = immediate return; non-0 = depth-compare
//   +0x1C  int  (pad)

// --- Globals ---
extern int  g_nAreaLastHit;        // 004c7db4  last result of Area_FindAt
extern int  g_nAreaNodeCount;      // 007127e8  total nodes in g_pAreaNodeTable
extern int  g_nAreaActiveNode;     // 0070c248  currently active walk node (-1=none)
extern int  g_nAreaSpriteCount;    // 005f3308  active entries in sprite-area list
extern int  g_nAreaListCount;      // 00646328  entries in selection list
extern int  g_nAreaListCursor;     // 0064632c  cursor into selection list
extern int  g_anAreaList[];        // 00646330  selection list (cursor-indexed ints)
extern int  g_anAreaYBuckets[];    // 0062a120  Y-bucket spatial index (120 rows × 200 int slots)
extern int  g_anAreaSpriteList[];  // 00575780  sprite-area records (stride 0x20)
extern int* g_pAreaNodeTable;      // 0070ded8  array of pointers to node records
extern int* g_pAreaExtraSpriteTable; // 0070c360 extended sprite record pointers (nodeIdx >= 0x96)

// Active node bounding box (set externally; tested in Area_FindAt)
extern int  g_nAreaActiveBBoxX1;   // 0062a104
extern int  g_nAreaActiveBBoxY1;   // 0062a10c
extern int  g_nAreaActiveBBoxX2;   // 0062a100
extern int  g_nAreaActiveBBoxY2;   // 0062a108

// --- Functions ---

// Register node nNodeIdx into Y-bucket rows spanned by its bbox.
// Called when a node is loaded to prime the spatial index.
void Area_AddNodeToYBuckets(int nNodeIdx);

// Clear all Y-bucket rows (fill -1 sentinel) and reset g_nAreaLastHit.
// Called before re-loading a room.
void Area_ClearYBuckets(void);

// Find the topmost area/node at screen coordinate (nX, nY).
// Checks the sprite-area list first, then the Y-bucket table.
// Returns area ID, or -1 if nothing found.  Updates g_nAreaLastHit.
// Debug name: "find_area_int x_int y"
int  Area_FindAt(int nX, int nY);

// Return the cached result of the most recent Area_FindAt call.
int  Area_GetLastHit(void);

// Search node table for the first node whose tag field (+0x14) equals nTag.
// Returns node index, or -1 if not found.
int  Area_FindNodeByTag(int nTag);

// Remove all sprite-area records whose nodeId field equals nNodeId.
// Compacts the list in place; decrements g_nAreaSpriteCount.
void Area_RemoveSprite(int nNodeId);

// Remove the sprite-area record at index nIdx by compacting the list.
void Area_RemoveSpriteAt(int nIdx);

// Reset the selection list: set count and cursor both to 0.
void Area_ResetList(void);

// Reset the selection-list cursor to 0 (rewind to start).
void Area_RewindList(void);

// Seek the selection-list cursor to the last item (count - 1).
void Area_SeekListEnd(void);

// Advance selection-list cursor by 1.  Returns 0, or -1 if already at end.
int  Area_ListNext(void);

// Retreat selection-list cursor by 1.  Returns 0, or -1 if already at start.
int  Area_ListPrev(void);

// Return the item at the current cursor position.
int  Area_ListGet(void);

// Write nValue to the item at the current cursor position.
void Area_ListSet(int nValue);

// Append a new zero-initialised slot; cursor advances to the new slot.
void Area_ListAppend(void);
