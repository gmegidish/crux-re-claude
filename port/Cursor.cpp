#include "Cursor.h"
#include "HelpBlit.h"
#include "Log.h"
#include <SDL.h>
#include <cstdint>
#include <vector>
#include <strings.h>

namespace {

// One loaded cursor: all frame sprite streams. Most cursors are single-frame, but the
// engine prefers a TYPE-7 cursor resource over the type-2 one (Curs_LoadCursorSelect), and
// several of those are animated — e.g. CURSEXIT is a 10-frame opening/closing door, CURSHOUR
// a 13-frame hourglass. We hold every frame and cycle them when drawing.
struct Cur {
    std::vector<std::vector<uint8_t>> frames;   // per-frame Help_BlitImage streams
    bool loaded = false;
};

constexpr int kMaxMode = 16;
constexpr uint32_t kFrameMs = 100;   // ~10 fps cursor animation
Cur g_default;            // the default arrow (CSDEF)
Cur g_mode[kMaxMode];     // per-mode cursors (CURSAREA=0/8, CURSEXIT=2, CURSINV=3, CURSHOUR=9)

inline int32_t  rd32(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

// Load a cursor by name, preferring a type-7 (often multi-frame/animated) resource and
// falling back to type-2 (Curs_LoadCursorSelect: "use type 1 if the .7 file exists, else
// type 2"). Stores every frame's sprite stream. Hotspot is (0,0): decodeSprite places the
// content at local (0,0) and we draw at the mouse, matching the engine (confirmed vs CSDEF).
bool loadInto(Cur& cur, ResArchive& arc, const char* name) {
    cur = Cur{};
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {                         // prefer animated type-7
        if (en.type == 7 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (!e) {
        for (const auto& en : arc.entries()) {                     // fall back to type-2
            if (en.type == 2 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
        }
    }
    if (!e) { Log::warn("Cursor: '%s' (type 7/2) not found", name); return false; }

    std::vector<uint8_t> data = arc.read(*e);
    if (data.size() < 12 || data[0] != 0x10) { Log::warn("Cursor: '%s' malformed", name); return false; }
    const uint8_t* d = data.data();
    int frameCount = rd32(d + 8);
    if (frameCount < 1) { Log::warn("Cursor: '%s' has no frames", name); return false; }

    size_t tbl = 12, off = tbl + (size_t)frameCount * 8;
    if (off > data.size()) { Log::warn("Cursor: '%s' truncated frame table", name); return false; }
    for (int i = 0; i < frameCount; ++i) {
        int32_t sz = rd32(d + tbl + (size_t)i * 8 + 4);
        if (sz < 0 || off + (size_t)sz > data.size()) { break; }
        cur.frames.emplace_back(d + off, d + off + sz);
        off += (size_t)sz;
    }
    if (cur.frames.empty()) { Log::warn("Cursor: '%s' no frame blobs", name); return false; }
    if (cur.frames.size() > 1) {
        Log::info("Cursor: '%s' loaded (type %d, %d frames, animated)", name, e->type, (int)cur.frames.size());
    }
    cur.loaded = true;
    return true;
}

void drawCur(const Cur& cur, Framebuffer& fb, int x, int y) {
    if (!cur.loaded || cur.frames.empty()) { return; }
    size_t idx = (cur.frames.size() > 1) ? (SDL_GetTicks() / kFrameMs) % cur.frames.size() : 0;
    const std::vector<uint8_t>& blob = cur.frames[idx];
    blitHelpImage(blob.data(), blob.size(), fb, x, y);
}

}  // namespace

namespace Cursor {

void reset() {
    g_default = Cur{};
    for (auto& c : g_mode) { c = Cur{}; }
}

bool load(ResArchive& arc, const char* name) {
    bool ok = loadInto(g_default, arc, name);
    if (ok) { Log::info("Cursor: default '%s' (%d frame(s))", name, (int)g_default.frames.size()); }
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
