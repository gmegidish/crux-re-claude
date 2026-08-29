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

// Register a self-resetting timer (op 0x17e Timer_AddWithReset): fires `progId` once after
// `frames` ticks and is then removed, exactly like a one-shot — `frames` is only stowed as
// the re-arm value for resetCounters(). At the tick level this is identical to addOnce; a
// repeat happens because the fired program re-adds the timer. (See Timer.cpp for why.)
void addRepeat(int progId, int frames);

// Timer_ResetCounters: restore each self-resetting timer's countdown from its stored value.
// Engine turn-loop facility; currently unused in the port (kept for faithfulness).
void resetCounters();

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
