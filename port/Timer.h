// Timer.h — async/sync script-timer subsystem (port of CRUX.EXE TIMERS.cpp).
//
// A timer counts down `frames` anim-ticks, then runs a script program. The engine
// keeps a 30-entry table (Timer_AddSync/AddAsync/AddWithReset @0x0047e3a0..) ticked by
// Timer_Tick_Callback (registered on Anim's tick list): each tick decrements the
// counter and, when it elapses, queues the program (Timer_AddSyncProg /
// Theme_RegisterAsyncProg) and drops a one-shot timer. The sync/async distinction only
// changes *when* the queued program runs in the engine's frame; the port runs both the
// same way (RunProg drains takeFired() after each tick), so they collapse to one path.
#pragma once

namespace Timer {

// Register a one-shot timer: run script program `progId` after `frames` ticks.
// (ops 0x178 Timer_AddSync / 0x196 Timer_AddAsync — both one-shot here.)
void addOnce(int progId, int frames);

// Register a repeating timer: run `progId` every `frames` ticks (op 0x17e
// Timer_AddWithReset — the engine stows `frames` as the re-arm value).
void addRepeat(int progId, int frames);

// Remove every timer registered for `progId` (op 0x18f Timer_Kill).
void kill(int progId);

// Drop all timers (call on area change).
void clear();

// Advance all timers one tick and queue any that elapsed. Call once per anim tick
// (the engine ticks timers from Anim's tick-callback list).
void tick();

// Pop the next fired program id (FIFO), or -1 if none. Drained by RunProg each frame.
int  takeFired();

}  // namespace Timer
