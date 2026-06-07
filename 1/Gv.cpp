#include "Gv.h"

namespace {
bool g_enabled = false;          // g_nGVEnabled
int  g_destroyHandler = -1;      // _DAT_004cf8ec (script id; -1 = none)
}

namespace Gv {

void setEnabled(bool on) { g_enabled = on; }
bool enabled() { return g_enabled; }

void setDestroyHandler(int prog) { g_destroyHandler = prog; }
int  destroyHandler() { return g_destroyHandler; }

void reset() { g_enabled = false; g_destroyHandler = -1; }

}  // namespace Gv
