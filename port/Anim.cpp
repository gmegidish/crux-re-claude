#include "Anim.h"
#include "HelpBlit.h"
#include "Log.h"
#include <cstring>
#include <strings.h>
#include <string>
#include <vector>

namespace {

struct Frame {
    int16_t x = 0, y = 0;             // per-frame offset (added to the slot position)
    std::vector<uint8_t> blob;        // sprite stream (decodeSprite input)
};

struct Slot {
    bool   active = false;            // slot in use
    bool   visible = true;            // drawn each frame
    bool   frozen = false;            // frame advance disabled
    bool   looping = false;           // wrap to 0 at the end (else stop on last frame)
    int    curFrame = 0;
    int    x = 0, y = 0;              // slot screen position
    std::string name;
    std::vector<Frame> frames;
};

Slot g_slots[Anim::MAX_SLOTS];

inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline int16_t  rs16(const uint8_t* p) { return (int16_t)rd16(p); }
inline int32_t  rd32(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

int findFreeSlot() {
    for (int i = 0; i < Anim::MAX_SLOTS; ++i) {
        if (!g_slots[i].active) { return i; }
    }
    return -1;
}

// Load and parse a type-7 .ANI resource into `s`. Returns false on bad data.
bool loadAni(ResArchive& arc, const char* name, Slot& s) {
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {
        if (en.type == 7 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (!e) { Log::warn("Anim: '%s' (type 7) not found", name); return false; }

    std::vector<uint8_t> blob = arc.read(*e);
    if (blob.size() < 12) { Log::warn("Anim: '%s' too small", name); return false; }
    const uint8_t* d = blob.data();
    if (d[0] != 0x10) { Log::warn("Anim: '%s' bad magic 0x%02x", name, d[0]); return false; }

    int frameCount = rd32(d + 8);
    if (frameCount < 0 || frameCount > 400) { Log::warn("Anim: '%s' bad frameCount %d", name, frameCount); return false; }

    // frame table: frameCount x {s16 x, s16 y, s32 size}, then the blobs.
    size_t p = 12;
    if (p + (size_t)frameCount * 8 > blob.size()) { Log::warn("Anim: '%s' truncated frame table", name); return false; }
    s.frames.resize(frameCount);
    std::vector<int32_t> sizes(frameCount);
    for (int i = 0; i < frameCount; ++i) {
        s.frames[i].x = rs16(d + p);
        s.frames[i].y = rs16(d + p + 2);
        sizes[i]      = rd32(d + p + 4);
        p += 8;
    }
    for (int i = 0; i < frameCount; ++i) {
        int32_t sz = sizes[i];
        if (sz < 0 || p + (size_t)sz > blob.size()) { Log::warn("Anim: '%s' truncated frame %d blob", name, i); return false; }
        s.frames[i].blob.assign(d + p, d + p + sz);
        p += sz;
    }
    return true;
}

}  // namespace

namespace Anim {

void reset() {
    for (auto& s : g_slots) { s = Slot{}; }
}

bool blitResourceFrame0(ResArchive& arc, Framebuffer& fb, const char* name, int type, int x, int y) {
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {
        if (en.type == type && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (!e) { Log::warn("Anim: resource '%s' (type %d) not found", name, type); return false; }

    std::vector<uint8_t> blob = arc.read(*e);
    if (blob.size() < 20) { Log::warn("Anim: '%s' too small", name); return false; }
    const uint8_t* d = blob.data();
    if (d[0] != 0x10) { Log::warn("Anim: '%s' bad magic 0x%02x", name, d[0]); return false; }
    int frameCount = rd32(d + 8);
    if (frameCount < 1) { Log::warn("Anim: '%s' no frames", name); return false; }

    int16_t fx = rs16(d + 12);
    int16_t fy = rs16(d + 12 + 2);
    int32_t sz = rd32(d + 12 + 4);
    size_t blobStart = 12 + (size_t)frameCount * 8;
    if (sz < 0 || blobStart + (size_t)sz > blob.size()) { Log::warn("Anim: '%s' truncated frame 0", name); return false; }

    blitHelpImage(d + blobStart, sz, fb, x + fx, y + fy);
    return true;
}

int addByName(ResArchive& arc, const char* name, bool looping, bool frozen) {
    int slot = findFreeSlot();
    if (slot < 0) { Log::warn("Anim: no free slots for '%s'", name); return -1; }
    Slot& s = g_slots[slot];
    s = Slot{};
    s.name = name;
    if (!loadAni(arc, name, s)) { s = Slot{}; return -1; }
    s.active = true;
    s.visible = true;
    s.frozen = frozen;
    s.looping = looping;
    s.curFrame = 0;
    return slot;
}

// Advance every active, non-frozen, multi-frame anim by one frame. Looping anims
// wrap to 0 at the end; one-shot anims stop (freeze) on the last frame. Called
// once per animation tick from the render loop.
void tick() {
    for (auto& s : g_slots) {
        if (!s.active || s.frozen || (int)s.frames.size() <= 1) { continue; }
        ++s.curFrame;
        if (s.curFrame >= (int)s.frames.size()) {
            if (s.looping) { s.curFrame = 0; }
            else { s.curFrame = (int)s.frames.size() - 1; s.frozen = true; }
        }
    }
}

int findByName(const char* name) {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (g_slots[i].active && strcasecmp(g_slots[i].name.c_str(), name) == 0) { return i; }
    }
    return -1;
}

void freeSlot(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot] = Slot{}; }
}

void setPosition(int slot, int x, int y) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].x = x; g_slots[slot].y = y; }
}

void setCurrentFrame(int slot, int frame) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active) {
        if (frame >= 0 && frame < (int)g_slots[slot].frames.size()) { g_slots[slot].curFrame = frame; }
    }
}

void freeze(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].frozen = true; }
}

void resetFreeze(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].frozen = false; }
}

void setVisible(int slot, bool visible) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].visible = visible; }
}

bool active(int slot) {
    return slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active;
}

const char* slotName(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return ""; }
    return g_slots[slot].name.c_str();
}

bool frameBounds(int slot, int& x, int& y, int& w, int& h) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return false; }
    const Slot& s = g_slots[slot];
    if (s.curFrame < 0 || s.curFrame >= (int)s.frames.size()) { return false; }
    const Frame& f = s.frames[s.curFrame];
    if (f.blob.empty()) { return false; }

    // Paint the frame onto a private scratch surface (index 0 = empty), then take
    // the bounding box of the touched pixels. Reuses the real decoder so the rect
    // matches exactly what drawAll() paints.
    static Framebuffer scratch;
    scratch.clear(0);
    blitHelpImage(f.blob.data(), f.blob.size(), scratch, s.x + f.x, s.y + f.y);

    const uint8_t* px = scratch.pixels();
    int minX = Framebuffer::W, minY = Framebuffer::H, maxX = -1, maxY = -1;
    for (int yy = 0; yy < Framebuffer::H; ++yy) {
        for (int xx = 0; xx < Framebuffer::W; ++xx) {
            if (px[yy * Framebuffer::W + xx] != 0) {
                if (xx < minX) { minX = xx; }
                if (xx > maxX) { maxX = xx; }
                if (yy < minY) { minY = yy; }
                if (yy > maxY) { maxY = yy; }
            }
        }
    }
    if (maxX < 0) { return false; }            // blank frame
    x = minX; y = minY; w = maxX - minX + 1; h = maxY - minY + 1;
    return true;
}

void drawAll(Framebuffer& fb) {
    for (auto& s : g_slots) {
        if (!s.active || !s.visible) { continue; }
        if (s.curFrame < 0 || s.curFrame >= (int)s.frames.size()) { continue; }
        const Frame& f = s.frames[s.curFrame];
        if (f.blob.empty()) { continue; }
        blitHelpImage(f.blob.data(), f.blob.size(), fb, s.x + f.x, s.y + f.y);
    }
}

}  // namespace Anim
