#include "Inventory.h"
#include "Log.h"
#include <algorithm>

namespace {
std::vector<int> g_items;
int g_current = -1;               // 0x007d67b4
}

namespace Inventory {

void add(int tag) {
    if (std::find(g_items.begin(), g_items.end(), tag) != g_items.end()) {
        Log::info("INV: item %d already held", tag);
        return;
    }
    g_items.push_back(tag);
    Log::info("INV: + item %d (%zu held)", tag, g_items.size());
}

void remove(int tag) {
    auto it = std::find(g_items.begin(), g_items.end(), tag);
    if (it == g_items.end()) {
        Log::info("INV: item %d not held, nothing to remove", tag);
        return;
    }
    g_items.erase(it);
    Log::info("INV: - item %d (%zu held)", tag, g_items.size());
}

bool has(int tag) { return std::find(g_items.begin(), g_items.end(), tag) != g_items.end(); }

void setCurrent(int tag) { g_current = tag; }
int  current() { return g_current; }

const std::vector<int>& items() { return g_items; }
void clear() { g_items.clear(); g_current = -1; }

}  // namespace Inventory
