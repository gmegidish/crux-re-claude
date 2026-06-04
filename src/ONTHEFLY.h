// ---------------------------------------------------------------------------
// ONTHEFLY.h  —  Dynamic on-the-fly area node creation
//
// ONTHEFLY is a spatial subsystem for creating area nodes and node-list
// descriptors at runtime, as opposed to the static nodes loaded from room
// resource files.  Despite the name, it is NOT an audio decompressor — it
// creates clickable/hoverable game-world nodes "on the fly".
//
// The three public entry points match the stubs referenced in THEMES.h
// (which mistakenly attributed them to audio decoding):
//   ONTHEFLY::FindCodec  — does not exist; THEMES.h stub was misleading.
//   ONTHEFLY::StartDecode — does not exist; THEMES.h stubs were misleading.
//   ONTHEFLY::Flush      — does not exist; THEMES.h stubs were misleading.
//
// The actual THEMES.h addresses (0x0045d4e0, 0x0045db60, 0x0045e6f0) are
// READRES.cpp functions (Res_FindByNumChar, Res_BunchFreadLoadPtr,
// Res_WaitForEntry), not ONTHEFLY.
//
// Original source: C:\DevStudio\Projects\Crux\ONTHEFLY.cpp
// Address range:   0x004577a0 -- ~0x00457c90
// ---------------------------------------------------------------------------
#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// OTF node-list pool: base address of the pointer array.  Each entry is a
// void* pointing to an OTF_AllocNodeList block.  Indexed 0..g_nOtfNodeListCount-1.
extern void* g_pOtfNodeListPool;        // 0x00708ff8

// Next free slot in g_pOtfNodeListPool.
extern int   g_nOtfNodeListCount;       // 0x007127f0

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

// 0x004577a0  Allocate a full 0xb0-byte area-node record and register it in
//   g_pAreaNodeTable.  Fills the bounding box (x1,y1,x2,y2), packs flags_lo
//   and flags_hi into node[4], sets the z-depth at node[0x24], initialises
//   both the tag-type array (indices 0x15..0x23) and the node-list pointer
//   array (indices 0x06..0x14) to -1, then calls Area_AddNodeToYBuckets under
//   a CRITICAL_SECTION.  Returns the new node's table index.
int  OTF_AllocSlot(int x1, int y1, int x2, int y2,
                   unsigned int flags_lo, unsigned int flags_hi, int zDepth);

// 0x004578e0  Allocate a node-list block that holds param_1 descriptors
//   (each 0x10 bytes).  Block is registered in g_pOtfNodeListPool.
//   Returns the slot index.
int  OTF_AllocNodeList(int nCount);

// 0x004579d0  Attach hover (type 4) and activation (type 6) node-list
//   descriptors to the area node at index param_1.
//   param_2 = delay value for the hover trigger.
//   param_3 = extra parameter stored in the delay config block.
//   Creates three node-list blocks (delay config, hover, activate) and
//   installs them in the node's tag-type/node-list-pointer arrays at the
//   first available -1 slot.  Raises a fatal error if all 15 slots are full.
void OTF_AreaTip(int nNodeIdx, int nDelay, int nExtra);
