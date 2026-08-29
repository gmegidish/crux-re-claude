#include "Timer.h"
#include "Log.h"
#include <vector>
#include <algorithm>
#include <cstdlib>

static const bool g_loopLog = std::getenv("LOOP_LOG") != nullptr;   // TEMP loop diagnostic

namespace {

// One timer: run `progId` when `counter` reaches 0. Mirrors the engine's 0x14-byte
// record {progId, counter, hold(unused here), syncFlag(collapsed), resetValue}.
// IMPORTANT (engine Timer_TickCallback @0x0047e500): a timer is REMOVED the moment it
// fires — EVERY timer is one-shot at the tick level, including Timer_AddWithReset (0x17e).
// `reset` (resetVal) is NOT re-armed by the tick; it is only restored by the separate
// Timer_ResetCounters @0x0047ec10 (resetCounters() below), which the engine's turn loop
// calls and we currently do not. So a "repeating" script timer repeats ONLY because the
// fired program re-adds it (e.g. the vvk conversation: prog22 arms a completion-cb to
// prog23, prog23 re-adds the prog22 timer). Re-arming in tick() instead made the timer
// immortal AND let each cycle stack another -> unbounded accumulation (endless animation).
struct T { int progId; int counter; int reset; bool dead; };

std::vector<T>   g_timers;
std::vector<int> g_fired;       // program ids that elapsed this tick, FIFO

const int kMaxTimers = 30;      // engine cap (Err "Too many timers" at >= 0x1d+1)

}  // namespace

namespace Timer {

void addOnce(int progId, int frames) {
    if ((int)g_timers.size() >= kMaxTimers) { Log::warn("Timer: too many timers (cap %d)", kMaxTimers); return; }
    g_timers.push_back({ progId, frames, 0, false });
}

void addRepeat(int progId, int frames) {
    if ((int)g_timers.size() >= kMaxTimers) { Log::warn("Timer: too many timers (cap %d)", kMaxTimers); return; }
    if (g_loopLog) { Log::info("LOOP timer-add prog=%d frames=%d (total now %d)", progId, frames, (int)g_timers.size() + 1); }
    g_timers.push_back({ progId, frames, frames, false });
}

void kill(int progId) {
    for (T& t : g_timers) {
        if (t.progId == progId) { t.dead = true; }
    }
}

void clear() {
    g_timers.clear();
    g_fired.clear();
}

void tick() {
    for (T& t : g_timers) {
        if (t.dead) { continue; }
        if (--t.counter <= 0) {
            if (g_loopLog) { Log::info("LOOP timer-fire prog=%d", t.progId); }
            g_fired.push_back(t.progId);
            t.dead = true;            // engine Timer_TickCallback removes EVERY timer on fire
        }
    }
    g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(),
                                  [](const T& t) { return t.dead; }),
                   g_timers.end());
}

// Timer_ResetCounters @0x0047ec10: re-arm self-resetting timers (resetVal != 0) without
// removing/re-adding them. The engine's turn loop calls this; we don't model that loop yet,
// so this is currently unused — script-driven re-adds (the common case) cover the repeats.
void resetCounters() {
    for (T& t : g_timers) {
        if (t.reset != 0) { t.counter = t.reset; }
    }
}

int takeFired() {
    if (g_fired.empty()) { return -1; }
    int v = g_fired.front();
    g_fired.erase(g_fired.begin());
    return v;
}

}  // namespace Timer
