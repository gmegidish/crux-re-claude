// Gv.h — game-view / verb-toolbar (GV_*) subsystem state (clean-room port of the
// GV_* helpers in GranInv.cpp).
//
// The GV is the on-screen verb/inventory toolbar. Its rendering + click dispatch
// aren't ported yet, but scripts configure its state via opcodes. We hold that
// state faithfully (mirrors g_nGVEnabled / the destroy-handler global) so the
// future toolbar can consume it — the ops aren't stubs, just write-only for now.
#pragma once

namespace Gv {

void setEnabled(bool on);          // GV_SetEnabled (op 0x918)
bool enabled();

void setDestroyHandler(int prog);  // GV_SetDestroyHandler (op 0x9c6) — script id
int  destroyHandler();

void reset();                      // clear state (on area teardown)

}  // namespace Gv
