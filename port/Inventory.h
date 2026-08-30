// Inventory.h — the player's item list (engine INVMANG.cpp + Adv_AddInvItem/Adv_RemoveInvItem).
//
// Script-visible model only. The engine keeps rich per-item state (icon atlas slot, drag
// flags, GV list-view row) in INVMANG/Graninv; scripts touch just three things — add an
// item, remove one, and test whether it is held (ops 0xc / 0xd / 0x11) — which is what
// gates puzzle progress. The GV panel that DRAWS the inventory is not ported (see Gv.h),
// so nothing here renders; it exists so `IF_INV_HAS` can answer correctly.
//
// Items are identified by the tag the script passes. The engine resolves that through
// Inv_GetByTag (@0x0043d4e0) into an item pointer; the port keeps the tag itself, since
// it has no item records to point at.
#pragma once
#include <vector>

namespace Inventory {

// Adv_AddInvItem (@0x00411b40) — append if not already held (the engine's list holds one
// entry per item). Ordering is insertion order, which is what Adv_ScrollInv(9999) walks.
void add(int tag);

// Adv_RemoveInvItem (@0x00411ea1-thunk) — drop the item if held; no-op otherwise.
void remove(int tag);

bool has(int tag);

// g_nCurrentItem (0x007d67b4): the item a script means by the name "_current". Ops 0xc/0xd/
// 0x11 substitute it when the addressed object is literally named "_current".
void setCurrent(int tag);
int  current();

const std::vector<int>& items();
void clear();                     // new game / load

}  // namespace Inventory
