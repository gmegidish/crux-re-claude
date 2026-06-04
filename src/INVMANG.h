#pragma once

// ---------------------------------------------------------------------------
// INVMANG.cpp -- Inventory texture-atlas manager + item-list lookup
//
// Manages a compact texture atlas of up to INV_MAX_SLOTS (50) item icon
// bitmaps that are packed into a single pool buffer (g_pTexPool, max 29000
// bytes).  Each slot carries: in_use flag, pixel-data size, pool offset,
// width, and height.  A per-item mapping array (g_anItemSlot[]) maps an item
// index to its atlas slot; -1 means not loaded.
//
// Also provides two linear-search helpers used by Graninv/script layer:
//   Inv_GetByTag  -- find item by integer tag (matches field at offset 0x00
//                    of the item object)
//   Inv_GetByName -- find item by name string (matches field at offset 0xd0
//                    of the item object)
//
// Original source: C:\DevStudio\Projects\Crux\INVMANG.cpp
// Address range:   0x0043c7fd -- 0x0043d6cb
// ---------------------------------------------------------------------------

// ---- constants ------------------------------------------------------------

#define INV_MAX_SLOTS   50      // maximum texture atlas slots
#define INV_POOL_MAX  29000     // maximum bytes in g_pTexPool

// ---- atlas slot table (g_abInvSlots) -- 9-byte stride --------------------
//
//  Byte offset within one slot entry:
//    [0]   char  in_use      — 0 = free, 1 = occupied
//    [1-2] short size        — byte length of pixel data for this slot
//    [3-4] short pool_offset — byte offset from g_pTexPool where data lives
//    [5-6] short width       — pixel width of the item icon
//    [7-8] short height      — pixel height of the item icon

// ---- globals (defined in INVMANG.cpp) ------------------------------------

extern int   g_anItemSlot[];   // 0x006d95f0  per-item atlas-slot index; -1 = not loaded
extern char  g_abInvSlots[];   // 0x006d9dc0  raw slot table (9 bytes × INV_MAX_SLOTS)
extern int   g_nItemCount;     // 0x0070b5c0  total number of items in the item list
extern void *g_apItems[];      // 0x0070d6f0  array of item object pointers
extern void *g_pTexPool;       // 0x007114cc  base of texture pixel data pool

// ---- functions ------------------------------------------------------------

// Inv_GetResource -- get (and lazy-load) the texture resource for item nItemIdx.
// Allocates an atlas slot on first call, reads pixel data from the resource
// file, and stores W/H.  Returns a pointer into g_pTexPool at the slot's
// pool_offset.  On subsequent calls the slot is already occupied; returns the
// same pointer immediately.
//   param_2 / param_3 -- receive item width and height (out parameters)
//   param_4           -- flags forwarded to the resource loader (e.g. palette mode)
int  Inv_GetResource(int nItemIdx, unsigned int *pW, unsigned int *pH, char bFlags);

// Inv_AllocSlot -- find or free space for nSize bytes in the texture pool.
// Scans the slot table for a free slot or one that fits nSize.  When no slot
// fits, compacts occupied slots (slides pixel data down, fixes up all
// g_anItemSlot back-references) until there is room.  Returns the allocated
// slot index (low byte of return value).
int  Inv_AllocSlot(int nSize);

// Inv_FreeSlot -- release the atlas slot for item nItemIdx.
// Zeroes the in_use byte of the slot and resets g_anItemSlot[nItemIdx] to -1.
void Inv_FreeSlot(int nItemIdx);

// Inv_GetByTag -- find item index whose tag integer equals nTag.
// Compares nTag against the first int field (offset 0x00) of each item object
// pointer in g_apItems[0..g_nItemCount-1].  Returns the index or -1.
int  Inv_GetByTag(int nTag);

// Inv_GetByName -- find item index whose name string matches pszName.
// Compares pszName against the string at offset 0xd0 of each item object
// pointer in g_apItems[0..g_nItemCount-1].  Returns the index or -1.
int  Inv_GetByName(const char *pszName);
