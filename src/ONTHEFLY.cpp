// ONTHEFLY.cpp — Dynamic on-the-fly area node creation
//
// This module creates area nodes and node-list descriptors at runtime ("on the
// fly"), as opposed to the static nodes loaded from the room resource files.
// Despite the name suggesting audio decompression, ONTHEFLY is purely a spatial
// subsystem: it allocates new entries in g_pAreaNodeTable (the same table used
// by AREAS.cpp) and builds node-list blocks that describe lists of connected
// nodes with type codes and delay values.
//
// Architecture overview:
//   - Node lists: a compact descriptor block allocated by OTF_AllocNodeList.
//     Each block holds a count followed by count * 0x10 bytes of payload.
//     All allocated blocks are tracked in g_pOtfNodeListPool[g_nOtfNodeListCount].
//   - Area slots: OTF_AllocSlot allocates a full 0xb0-byte area-node record
//     (same size/layout as static area nodes) via SafeHeap_Alloc, registers it
//     in g_pAreaNodeTable, and calls Area_AddNodeToYBuckets so it participates
//     in hit-testing.
//   - Tooltip/tip nodes: OTF_AreaTip attaches two node-list descriptors to an
//     existing area node — one of type 4 (hover/tooltip trigger) and one of
//     type 6 (activation trigger).  The lists are stored in the node's own
//     descriptor array at offsets +0x15 and +0x06 (tag fields).
//
// Area node record layout (0xb0 bytes, same as AREAS.cpp static nodes):
//   +0x00  INT   x1 (left)
//   +0x04  INT   y1 (top)
//   +0x08  INT   x2 (right)
//   +0x0c  INT   y2 (bottom)
//   +0x10  byte  flags_lo (colour / type byte)
//   +0x11  byte  flags_hi
//   +0x90  INT   z-depth (0x24th int = index 0x24)
//   +0x94  INT   exclude flag (index 0x25, = -1 means "include in hit test")
//   +0x54  INT[15]  tag-type array (indices 0x15..0x23, -1 = empty slot)
//   +0x18  INT[15]  node-list ptr array (indices 0x06..0x14)
//   (remainder zero-initialised)
//
// Node-list block layout (param_1 * 0x10 + 4 bytes):
//   +0x00  INT   count (= param_1 passed to OTF_AllocNodeList)
//   +0x04  [count * 0x10 bytes]  node descriptors (type-specific payload)
//
// Node-list descriptor fields (0x10 bytes each, selected offsets):
//   +0x04  INT   type / command code
//              0x2c4 (708) = tooltip delay config
//              0x178 (376) = hover trigger node
//              0x17a (378) = activation trigger node
//   +0x08  INT   back-reference to another node-list slot index
//   +0x0c  INT   param (e.g. delay value, param_2 from OTF_AreaTip)
//   +0x14  INT   sub-type or range (e.g. 0x70, 399)
//   +0x18  INT   back-reference to delay node-list slot index
//   +0x24  INT   count limit (0x0f = 15 max active triggers)
//   +0x34  INT   threshold constant (0xcd = 205)
//   +0x38  INT   param_3 from OTF_AreaTip (extra data)
//
// Original source: C:\DevStudio\Projects\Crux\ONTHEFLY.cpp
// Address range:   0x004577a0 -- 0x00457c90  (approx)
// Known functions:
//   0x004577a0  OTF_AllocSlot       (already named in Ghidra)
//   0x004578e0  OTF_AllocNodeList
//   0x004579d0  OTF_AreaTip
// Boundary note:
//   0x00457e60 is the first PLAYER.cpp function (Player_SetPalFreezeMode).
//   The gap 0x00457c90 -- 0x00457e60 may contain additional OTF helpers not in
//   the provided address list.

#include "ONTHEFLY.h"
#include "AREAS.h"
#include <windows.h>
#include <string.h>

// ============================================================
//  Globals
// ============================================================

// Node-list pool: array of pointers to allocated OTF_AllocNodeList blocks.
// g_nOtfNodeListCount is the number of currently allocated blocks.
// Each block pointer lives at g_pOtfNodeListPool + index * 4.
void*  g_pOtfNodeListPool   = 0;    // 0x00708ff8  pointer array base
int    g_nOtfNodeListCount  = 0;    // 0x007127f0  next free slot index

// ============================================================
//  External dependencies
// ============================================================

extern "C" {
    // SAFEHEAP: allocate nBytes bytes, tag the block with the source file name
    // and a 4-byte tag taken from *(int*)(tag + 4).
    void* SafeHeap_Alloc(int nTag, const char *pszFile, int nBytes);

    // ERRORS: format + throw a fatal error record
    void* Err_SetRecord3(int nCode, void *pArg, int nExtra);
    void  thunk_FUN_00420e60(int nLine, const char *pszFile);
    void  FUN_00489090(void *pErr, void *pBase);
}

// AREAS: node table and registration
extern int    g_nAreaNodeCount;
extern int*   g_pAreaNodeTable;     // actually int** but kept as int* for Ghidra compat
extern void   Area_AddNodeToYBuckets(int nNodeIdx);

// AREAS CRITICAL_SECTION (0x0070ae78)
extern int    g_nAreaCritSec2;      // 0x0070ae78

// Ghidra-recovered area node count mirror used by ONTHEFLY
extern int    DAT_006ead9c;         // running node-add counter (second copy)

// ============================================================
//  OTF_AllocNodeList  (0x004578e0)
// ============================================================
// Allocate a new node-list block that can hold param_1 node descriptors
// (each 0x10 bytes).  The block is registered in g_pOtfNodeListPool at the
// next free index (g_nOtfNodeListCount) and that index is returned.
//
// Block layout:
//   [0]        = param_1  (count stored as first int)
//   [1..N*4+3] = N * 0x10 bytes of zero-initialised descriptor space
//
// Returns the slot index (= the index into g_pOtfNodeListPool).
int OTF_AllocNodeList(int param_1)
{
    int iSlot = g_nOtfNodeListCount;
    g_nOtfNodeListCount = g_nOtfNodeListCount + 1;

    // Allocate: 4 bytes for the count + param_1 entries * 0x10 bytes each.
    void *pBlock = SafeHeap_Alloc(*(int*)(0x004d57d0) + 4,
                                  "C:\\DevStudio\\Projects\\Crux\\ONTHEFLY.cpp",
                                  param_1 * 0x10 + 4);

    // Store the block pointer and write the count into the first int.
    ((void**)(&g_pOtfNodeListPool))[iSlot] = pBlock;
    *(int*)pBlock = param_1;

    return iSlot;
}

// ============================================================
//  OTF_AreaTip  (0x004579d0)
// ============================================================
// Attach on-the-fly tooltip and activation triggers to area node param_1.
//
//   param_1  — index into g_pAreaNodeTable of the target node
//   param_2  — delay value stored into the hover-trigger descriptor (+0x0c)
//   param_3  — extra data stored into the delay config descriptor (+0x38)
//
// Creates three node-list blocks:
//   "delay" block  (4 entries, type 0x2c4):
//     Configures the hover delay.  Back-refs and thresholds are hard-coded.
//   "hover" block  (1 entry, type 0x178):
//     The actual hover / tooltip trigger.  References the delay block.
//   "activate" block (2 entries, type 0x17a):
//     The activation trigger.  References the delay block.
//
// Both the hover and activate block indices are installed into the node's
// tag-type and node-list-pointer arrays (at the first empty -1 slot for
// type 4 and type 6 respectively).  If both arrays are full (no -1 slot
// found in 15 tries) a fatal error is raised.
void OTF_AreaTip(int param_1, int param_2, int param_3)
{
    // --- Allocate the "delay config" node list (4 entries) ---
    int nDelaySlot = OTF_AllocNodeList(4);
    int *pDelay = (int*)(((void**)(&g_pOtfNodeListPool))[nDelaySlot]);

    // Descriptor at entry 0 (offset +4 within block): delay config
    pDelay[1]  = 0x2c4;      // +0x04  type 708
    pDelay[4]  = 0;           // +0x10
    pDelay[3]  = 0;           // +0x0c
    pDelay[2]  = 0;           // +0x08
    pDelay[5]  = 0x70;        // +0x14  sub-type 112
    pDelay[8]  = 0;           // +0x20
    pDelay[7]  = 0;           // +0x1c
    pDelay[6]  = 0;           // +0x18
    pDelay[9]  = 0x0f;        // +0x24  max active = 15
    pDelay[12] = 0;           // +0x30
    pDelay[11] = 0;           // +0x2c
    pDelay[10] = 0;           // +0x28
    pDelay[13] = 0xcd;        // +0x34  threshold 205
    pDelay[14] = param_3;     // +0x38  caller's extra param
    pDelay[16] = 0;           // +0x40
    pDelay[15] = 0;           // +0x3c

    // --- Allocate the "hover" node list (1 entry) ---
    int nHoverSlot = OTF_AllocNodeList(1);
    int *pHover = (int*)(((void**)(&g_pOtfNodeListPool))[nHoverSlot]);

    pHover[1] = 0x178;        // +0x04  type 376
    pHover[2] = nDelaySlot;   // +0x08  back-ref to delay
    pHover[3] = param_2;      // +0x0c  delay value
    pHover[4] = 0;            // +0x10

    // --- Allocate the "activate" node list (2 entries) ---
    int nActSlot = OTF_AllocNodeList(2);
    int *pAct = (int*)(((void**)(&g_pOtfNodeListPool))[nActSlot]);

    pAct[1]  = 0x17a;         // +0x04  type 378
    pAct[4]  = 0;             // +0x10
    pAct[3]  = 0;             // +0x0c
    pAct[2]  = 0;             // +0x08
    pAct[5]  = 399;           // +0x14  sub-type 399
    pAct[6]  = nDelaySlot;    // +0x18  back-ref to delay
    pAct[8]  = 0;             // +0x20
    pAct[7]  = 0;             // +0x1c

    // --- Install hover slot (type 4) into node param_1 ---
    // Walk the tag-type array (indices 0x15..0x23) for a slot == 4 (existing)
    // or -1 (empty).
    int nIdx;
    int *pNode = (int*)((int**)(&g_pAreaNodeTable))[param_1];

    for (nIdx = 0; nIdx < 0x0f && pNode[nIdx + 0x15] != 4; nIdx++) {}
    if (nIdx > 0x0e) {
        // No type-4 slot found; look for an empty (-1) slot.
        for (nIdx = 0; nIdx < 0x0f && pNode[nIdx + 0x15] != -1; nIdx++) {}
    }
    if (nIdx > 0x0e) {
        // All 15 slots full — fatal error
        thunk_FUN_00420e60(*(int*)(0x004d5818) + 0x39,
                           "C:\\DevStudio\\Projects\\Crux\\ONTHEFLY.cpp");
        void *pErr = Err_SetRecord3(0x27, (void*)0x006eada0, -1);
        FUN_00489090(pErr, (void*)0x004ab3f8);
    }
    pNode[nIdx + 0x15] = 4;           // tag-type = 4 (hover)
    pNode[nIdx + 0x06] = nHoverSlot;  // node-list pointer

    // --- Install activate slot (type 6) into node param_1 ---
    for (nIdx = 0; nIdx < 0x0f && pNode[nIdx + 0x15] != 6; nIdx++) {}
    if (nIdx > 0x0e) {
        for (nIdx = 0; nIdx < 0x0f && pNode[nIdx + 0x15] != -1; nIdx++) {}
    }
    if (nIdx > 0x0e) {
        thunk_FUN_00420e60(*(int*)(0x004d5818) + 0x50,
                           "C:\\DevStudio\\Projects\\Crux\\ONTHEFLY.cpp");
        void *pErr = Err_SetRecord3(0x27, (void*)0x006eada4, -1);
        FUN_00489090(pErr, (void*)0x004ab3f8);
    }
    pNode[nIdx + 0x15] = 6;           // tag-type = 6 (activate)
    pNode[nIdx + 0x06] = nActSlot;    // node-list pointer
}

// ============================================================
//  OTF_AllocSlot  (0x004577a0)  — already named in Ghidra
// ============================================================
// Allocate a full area-node record (0xb0 bytes) and register it in the
// global node table.  This is the primary entry point called from AREAS.cpp
// (and game scripts) to create a dynamic node.
//
//   param_1  x1 (left bound)
//   param_2  y1 (top bound)
//   param_3  x2 (right bound)
//   param_4  y2 (bottom bound)
//   param_5  flags_lo  (low byte stored into node[4] bits 0..7)
//   param_6  flags_hi  (high byte stored into node[4] bits 8..15)
//   param_7  z-depth   (stored at node[0x24])
//
// Returns the node table index of the newly created node.
int OTF_AllocSlot(int x1, int y1, int x2, int y2,
                  unsigned int flags_lo, unsigned int flags_hi, int zDepth)
{
    int iSlot = g_nAreaNodeCount;

    // Allocate 0xb0-byte node record
    int *pNode = (int*)SafeHeap_Alloc(*(int*)(0x004d5758) + 4,
                                      "C:\\DevStudio\\Projects\\Crux\\ONTHEFLY.cpp",
                                      0xb0);

    // Register in global node table
    ((int**)(&g_pAreaNodeTable))[iSlot] = pNode;

    // Fill bounding box
    pNode[0] = x1;
    pNode[1] = y1;
    pNode[2] = x2;
    pNode[3] = y2;

    // Pack flags bytes into node[4]
    pNode[4] = (pNode[4] & 0xffffff00u) | (flags_lo & 0xff);
    pNode[4] = (unsigned int)(pNode[4] & 0xffff0000u) | (unsigned short)(
                   (unsigned short)(pNode[4] & 0x00ffu) | ((flags_hi & 0xff) << 8));

    // Z-depth and exclude sentinel
    pNode[0x24] = zDepth;
    pNode[0x25] = -1;       // include in hit-testing

    // Zero-fill the tag-type array (0x15..0x23) with -1
    memset(&pNode[0x15], -1, 0x3c);
    // Zero-fill the node-list pointer array (0x06..0x14) with -1
    memset(&pNode[0x06], -1, 0x3c);

    // Serialise: increment counts and register in Y-bucket spatial index
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec2);
    g_nAreaNodeCount++;
    DAT_006ead9c++;
    Area_AddNodeToYBuckets(iSlot);
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nAreaCritSec2);

    return iSlot;
}
