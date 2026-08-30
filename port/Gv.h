// Gv.h — the GV floating inventory window (GV_* helpers in Graninv.cpp).
//
// "GV" = the *view* onto the Graninv (GRANular INVentory) — the window, not the item
// list. Read off the engine's own debug strings, which carry the original names:
// gv_init / gv_open / gv_close / gv_show / gv_hide / gv_winproc / gv_listview_winproc /
// gv_addbutton / gv_clipwin / gv_init_drag / gv_dodrag / gv_rotate, plus
// `gv_inv_update(int)` — "inv" appears as a separate token there, so GV itself is not
// "inventory". They live in Graninv.cpp beside the gran_* item logic, which also carries
// the error string "Graninv not open" for this window. (The "G" is inference; "view" is
// what the code does.)
//
// In the engine it is a NATIVE WIN95 WINDOW, not game-drawn art:
//   - class "GVClass", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | 0x00CF0000
//     (title bar, system menu, resizable frame, min/max boxes), title from
//     GetItemName("INVNAME"), pushed to HWND_TOPMOST.
//   - a SysListView32 in LVS_ICON | LVS_AUTOARRANGE — an Explorer-style large-icon grid —
//     over a 32x32 ILC_COLORDDB ImageList whose icons are the game's own item sprites
//     converted by WinRes_Sma2Icon; plus a flat CreateToolbarEx strip at CCS_TOP.
//   - owned by the main window and pumped on its own thread (critical-section
//     synchronised), so it is MODELESS: the game keeps running behind it.
// Dragging an item out of it is the game's core interaction (gv_dodrag / gv_rotate spin
// the dragged bitmap toward the drag direction; GV_CanDrop validates the target).
//
// None of that is ported: ScummVM/SDL have no native widgets, so a faithful port has to
// DRAW this panel. We keep the script-visible state only (open/closed, enabled, destroy
// handler) so the ops are real rather than stubs, and a future renderer can consume it.
#pragma once

namespace Gv {

void setEnabled(bool on);          // GV_SetEnabled (op 0x918)
bool enabled();

void setDestroyHandler(int prog);  // GV_SetDestroyHandler (op 0x9c6) — script id
int  destroyHandler();

// GV_OpenInventory (op 0x9c4) / GV_CloseInventory (0x9c9) / GV_HideAndClean (0x9c7):
// whether the floating inventory panel is up. In the engine this is a real Win95 popup
// window (class "GVClass", WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME, HWND_TOPMOST)
// holding a SysListView32 in LVS_ICON mode plus a flat toolbar, pumped on its own thread.
// The port draws no panel, so this is state only — but scripts branch on it, and it tells
// a future renderer when to show.
void setInventoryOpen(bool on);
bool inventoryOpen();

// GV_TickInventory (op 0x9c8): one service tick of the panel (the engine's message pump
// + icon refresh). Nothing to pump here; kept so the opcode is not an error.
void tickInventory();

void reset();                      // clear state (on area teardown)

}  // namespace Gv
