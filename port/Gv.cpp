#include "Gv.h"
#include "Log.h"

namespace {
bool g_enabled = false;          // g_nGVEnabled
int  g_destroyHandler = -1;      // _DAT_004cf8ec (script id; -1 = none)
bool g_invOpen = false;          // GV panel up? (ops 0x9c4 / 0x9c7 / 0x9c9)
}

namespace Gv {

void setEnabled(bool on) { g_enabled = on; }
bool enabled() { return g_enabled; }

void setDestroyHandler(int prog) { g_destroyHandler = prog; }
int  destroyHandler() { return g_destroyHandler; }

void setInventoryOpen(bool on) {
    if (g_invOpen == on) { return; }
    g_invOpen = on;
    Log::info("GV: inventory panel %s (not rendered — see Gv.h)", on ? "OPEN" : "CLOSED");
}

bool inventoryOpen() { return g_invOpen; }

void tickInventory() { }   // the engine pumps its window here; the port has no window

void reset() {
    g_invOpen = false; g_enabled = false; g_destroyHandler = -1; }

}  // namespace Gv
