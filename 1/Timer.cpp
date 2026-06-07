#include "Timer.h"
#include "Log.h"
#include <vector>
#include <algorithm>

namespace {

// One timer: run `progId` when `counter` reaches 0. `reset` > 0 re-arms (repeat);
// `reset` == 0 is one-shot (dropped after firing). Mirrors the engine's 0x14-byte
// record {progId, counter, hold(unused here), syncFlag(collapsed), resetValue}.
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
            g_fired.push_back(t.progId);
            if (t.reset > 0) { t.counter = t.reset; }   // repeating (0x17e)
            else { t.dead = true; }                      // one-shot (0x178/0x196)
        }
    }
    g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(),
                                  [](const T& t) { return t.dead; }),
                   g_timers.end());
}

int takeFired() {
    if (g_fired.empty()) { return -1; }
    int v = g_fired.front();
    g_fired.erase(g_fired.begin());
    return v;
}

}  // namespace Timer
