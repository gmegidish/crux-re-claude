#include "Cursor.h"
#include "HelpBlit.h"
#include "Log.h"
#include <cstdint>
#include <vector>
#include <strings.h>

namespace {

// One loaded cursor (frame-0 sprite stream + hotspot).
struct Cur {
    std::vector<uint8_t> blob;
    int  hotX = 0;
    int  hotY = 0;
    bool loaded = false;
};

constexpr int kMaxMode = 16;
Cur g_default;            // the default arrow (CSDEF)
Cur g_mode[kMaxMode];     // per-mode cursors (CURSAREA=0/8, CURSEXIT=2, CURSINV=3, CURSHOUR=9)

inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline int32_t  rd32(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

// Parse a type-2 cursor resource's frame-0 sprite into `cur`. Hotspot is (0,0):
// our decodeSprite places the content at local (0,0) and we draw at the mouse,
// matching the engine's net positioning (confirmed against CSDEF).
bool loadInto(Cur& cur, ResArchive& arc, const char* name) {
    cur.loaded = false;
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {
        if (en.type == 2 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (!e) { Log::warn("Cursor: '%s' (type 2) not found", name); return false; }

    std::vector<uint8_t> data = arc.read(*e);
    if (data.size() < 12 || data[0] != 0x10) { Log::warn("Cursor: '%s' malformed", name); return false; }
    const uint8_t* d = data.data();
    int frameCount = rd32(d + 8);
    if (frameCount < 1) { Log::warn("Cursor: '%s' has no frames", name); return false; }

    size_t p = 12;
    if (p + (size_t)frameCount * 8 > data.size()) { Log::warn("Cursor: '%s' truncated frame table", name); return false; }
    int32_t size0 = rd32(d + p + 4);
    size_t blobStart = p + (size_t)frameCount * 8;
    if (size0 < 0 || blobStart + (size_t)size0 > data.size()) { Log::warn("Cursor: '%s' truncated frame-0 blob", name); return false; }

    cur.blob.assign(d + blobStart, d + blobStart + size0);
    cur.hotX = 0;
    cur.hotY = 0;
    cur.loaded = true;
    return true;
}

void drawCur(const Cur& cur, Framebuffer& fb, int x, int y) {
    if (!cur.loaded || cur.blob.empty()) { return; }
    blitHelpImage(cur.blob.data(), cur.blob.size(), fb, x - cur.hotX, y - cur.hotY);
}

}  // namespace

namespace Cursor {

void reset() {
    g_default = Cur{};
    for (auto& c : g_mode) { c = Cur{}; }
}

bool load(ResArchive& arc, const char* name) {
    bool ok = loadInto(g_default, arc, name);
    if (ok) { Log::info("Cursor: default '%s' (%d bytes)", name, (int)g_default.blob.size()); }
    return ok;
}

bool loadMode(ResArchive& arc, int mode, const char* name) {
    if (mode < 0 || mode >= kMaxMode) { return false; }
    return loadInto(g_mode[mode], arc, name);
}

void draw(Framebuffer& fb, int mouseX, int mouseY) {
    drawCur(g_default, fb, mouseX, mouseY);
}

void drawMode(Framebuffer& fb, int mode, int mouseX, int mouseY) {
    if (mode >= 0 && mode < kMaxMode && g_mode[mode].loaded) {
        drawCur(g_mode[mode], fb, mouseX, mouseY);
    } else {
        drawCur(g_default, fb, mouseX, mouseY);   // fall back to the arrow
    }
}

}  // namespace Cursor
