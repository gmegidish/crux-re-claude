#include "Anim.h"
#include "HelpBlit.h"
#include "Log.h"
#include <algorithm>
#include <cstdlib>
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
    int    stopFrame = -1;            // halt auto-advance at this frame (-1 = none); g_anAnimSlotStopFrame
    int    zBase = 0;                 // Z-sort / draw-order key (g_nCharWalkTableBase, op 0x137)
    int    groupId = -1;              // group this slot belongs to (-1 = none); g_anAnimSlotGroupId
    int    triggerFrame = -1;        // fire the completion callback when curFrame reaches this (op 0x167); g_anAnimSlotTriggerFrame
    int    completionCb = -1;        // script program id to run when triggerFrame is reached (ops 0x3c/0x159/0x185); -1 = none
    std::string name;
    std::vector<Frame> frames;
};

Slot g_slots[Anim::MAX_SLOTS];

// Completion-callback programs whose anims reached their trigger frame this tick. The
// VM drains these (Anim::takeFiredCallback) and runs each — the engine's Anim_HandleFrameTick
// -> script-trigger dispatch. A queue keeps Anim decoupled from RunProg.
std::vector<int> g_firedCbs;

// --- anim-group state (mirrors engine globals g_anGroupSize / g_anGroupTriggerPct /
//     g_anGroupActiveSlot / g_nGroupCount / g_nGroupMemberTemp). A group is a set of
//     mutually-exclusive member slots; only its active member is drawn/advanced. ---
constexpr int MAX_GROUPS = 16;
struct Group {
    bool active = false;              // group slot in use
    int  size = 0;                    // member count (g_anGroupSize)
    int  triggerPct = 1;             // switch chance %/tick (g_anGroupTriggerPct)
    int  activeSlot = -1;             // currently-drawn member, or -1 (g_anGroupActiveSlot)
    int  members[10] = {0};           // member anim slots (g_abGroupMembers; engine stride 10)
    int  filled = 0;                  // members registered so far
};
Group g_groups[MAX_GROUPS];
int   g_openGroup = -1;               // group currently accepting members (g_nGroupCount cursor)

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
    g_firedCbs.clear();
    for (auto& g : g_groups) { g = Group{}; }
    g_openGroup = -1;
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
    s.stopFrame = -1;
    s.groupId = -1;

    // Auto-join the open group, if any (engine Anim_AddToGroup, called by the anim-add
    // dispatcher cases right after Anim_StartGroup). The group keeps the slot number;
    // when full, the group closes.
    if (g_openGroup >= 0 && g_openGroup < MAX_GROUPS) {
        Group& g = g_groups[g_openGroup];
        if (g.active && g.filled < g.size && g.filled < 10) {
            g.members[g.filled] = slot;
            ++g.filled;
            s.groupId = g_openGroup;
            if (g.filled >= g.size) { g_openGroup = -1; }   // group full -> close (g_nGroupCount++)
        }
    }
    return slot;
}

// Per-tick group switching (engine Anim_BuildDrawOrder @0x00407708): for each group
// with no active member, roll rand()%100 < triggerPct; on success pick a random member
// as the active one (engine uses an LCG == rand(); op 0x1c sets the std::rand precedent).
static void tickGroups() {
    for (auto& g : g_groups) {
        if (!g.active || g.filled <= 0) { continue; }
        if (g.activeSlot != -1) { continue; }               // a member is already showing
        if (std::rand() % 100 < g.triggerPct) {
            int idx = std::rand() % g.filled;
            int slot = g.members[idx];
            // Engine only activates a member whose slot is still active; wrap its frame
            // if it ran off the end.
            if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active) {
                g.activeSlot = slot;
                if (g_slots[slot].curFrame >= (int)g_slots[slot].frames.size()) {
                    g_slots[slot].curFrame = 0;
                }
            }
        }
    }
}

// Advance every active, non-frozen, multi-frame anim by one frame. Looping anims wrap
// to 0 at the end; one-shot anims stop (freeze) on the last frame. A per-slot stop frame
// halts advancement at that frame (without freezing — getCurrentFrame stays valid). A
// grouped slot only advances while it is its group's ACTIVE member; when its (non-loop)
// anim ends, the group releases it (active -> none) so the next tick can re-roll. Called
// once per animation tick from the render loop.
void tick() {
    tickGroups();
    for (int i = 0; i < MAX_SLOTS; ++i) {
        Slot& s = g_slots[i];
        if (!s.active || s.frozen || (int)s.frames.size() <= 1) { continue; }
        // Grouped member that isn't the active one: hold (not drawn, not advanced).
        if (s.groupId >= 0 && s.groupId < MAX_GROUPS && g_groups[s.groupId].active &&
            g_groups[s.groupId].activeSlot != i) { continue; }
        // Honor a stop frame: don't advance past it.
        if (s.stopFrame >= 0 && s.curFrame >= s.stopFrame) { continue; }
        ++s.curFrame;
        if (s.stopFrame >= 0 && s.curFrame >= s.stopFrame) {
            s.curFrame = s.stopFrame;
        } else if (s.curFrame >= (int)s.frames.size()) {
            if (s.looping) { s.curFrame = 0; }
            else { s.curFrame = (int)s.frames.size() - 1; s.frozen = true; }
            // A grouped member whose anim just ended is released so the group re-rolls.
            if (s.groupId >= 0 && s.groupId < MAX_GROUPS && g_groups[s.groupId].activeSlot == i) {
                g_groups[s.groupId].activeSlot = -1;
            }
        }
        // Completion callback (ops 0x3c/0x159/0x167/0x185): fire once when the anim reaches
        // its trigger frame, then clear (one-shot). The VM drains g_firedCbs and runs them.
        if (s.completionCb >= 0 && s.triggerFrame >= 0 && s.curFrame >= s.triggerFrame) {
            g_firedCbs.push_back(s.completionCb);
            s.completionCb = -1;
            s.triggerFrame = -1;
        }
    }
}

void setTriggerFrame(int slot, int frame) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active) { g_slots[slot].triggerFrame = frame; }
}

void setCompletionCallback(int slot, int progId, int frame) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active) {
        g_slots[slot].completionCb = progId;
        g_slots[slot].triggerFrame = frame;
    }
}

int takeFiredCallback() {
    if (g_firedCbs.empty()) { return -1; }
    int v = g_firedCbs.front();
    g_firedCbs.erase(g_firedCbs.begin());
    return v;
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

// Anim_GetCurrentFrame @0x004019fb: curFrame iff (flags & active) && freezeCount==0,
// else -1. Our `frozen` bool stands in for the engine's freezeCount != 0.
int getCurrentFrame(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active && !g_slots[slot].frozen) {
        return g_slots[slot].curFrame;
    }
    return -1;
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

// Anim_SetStopFrame @0x004054b0: stash the frame at which the slot halts. (Engine guards
// on active/group membership; we just require a valid active slot.)
void setStopFrame(int slot, int frame) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return; }
    int last = (int)g_slots[slot].frames.size() - 1;
    // Clamp to a reachable frame so a play-to-frame wait can always terminate (the engine
    // ignores an out-of-range stop frame; clamping is the equivalent safe resting target).
    if (frame < 0) { frame = -1; }
    else if (frame > last) { frame = last; }
    g_slots[slot].stopFrame = frame;
}

bool atStopFrame(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return false; }
    const Slot& s = g_slots[slot];
    if (s.stopFrame < 0) { return false; }
    return s.curFrame >= s.stopFrame;
}

// Anim_SetWalkTableBase @0x00406980: per-slot value that the engine ALSO uses as the
// draw-order Z key (Anim_CompareByZ @0x0040796e sorts the draw list by it, ascending).
// The port models that: drawAll() z-sorts by this value, so op 0x137 affects rendering.
void setZBase(int slot, int base) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].zBase = base; }
}

// Anim_StartGroup @0x00409260: open a new group of `size` members, switch chance
// `triggerPct` (%, min 1), no active member yet. The next `size` addByName calls join it.
void startGroup(int size, int triggerPct) {
    int gi = -1;
    for (int i = 0; i < MAX_GROUPS; ++i) {
        if (!g_groups[i].active) { gi = i; break; }
    }
    if (gi < 0) { Log::warn("Anim: no free anim group slots"); return; }
    Group& g = g_groups[gi];
    g = Group{};
    g.active = true;
    g.size = (size < 1) ? 1 : (size > 10 ? 10 : size);
    g.triggerPct = (triggerPct == 0) ? 1 : triggerPct;
    g.activeSlot = -1;
    g.filled = 0;
    g_openGroup = gi;
}

bool groupOpen() {
    return g_openGroup >= 0;
}

bool active(int slot) {
    return slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active;
}

const char* slotName(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return ""; }
    return g_slots[slot].name.c_str();
}

int frameCount(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return 0; }
    return (int)g_slots[slot].frames.size();
}

void getFrameTopLeft(int slot, int& x, int& y) {
    x = 0;
    y = 0;
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return; }
    const Slot& s = g_slots[slot];
    if (s.curFrame < 0 || s.curFrame >= (int)s.frames.size()) { return; }
    x = s.frames[s.curFrame].x;
    y = s.frames[s.curFrame].y;
}

void slotPos(int slot, int& x, int& y) {
    x = 0;
    y = 0;
    if (slot < 0 || slot >= MAX_SLOTS) { return; }
    x = g_slots[slot].x;
    y = g_slots[slot].y;
}

void setFrameStep(int slot, int step) {
    if (slot < 0 || slot >= MAX_SLOTS) { return; }
    if (step == 0) { g_slots[slot].frozen = true; }
    else { g_slots[slot].frozen = false; }
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

// Build the draw order (engine Anim_BuildDrawOrder): collect the active+visible slots,
// skipping grouped members that are NOT their group's active member, then z-sort by the
// per-slot Z key (g_nCharWalkTableBase via op 0x137), lowest first (drawn furthest back).
// std::stable_sort keeps add order among equal keys (the prior behavior for un-z'd anims).
void drawAll(Framebuffer& fb) {
    int order[MAX_SLOTS];
    int n = 0;
    for (int i = 0; i < MAX_SLOTS; ++i) {
        const Slot& s = g_slots[i];
        if (!s.active || !s.visible) { continue; }
        if (s.curFrame < 0 || s.curFrame >= (int)s.frames.size()) { continue; }
        if (s.frames[s.curFrame].blob.empty()) { continue; }
        // A grouped slot is drawn only when it is its group's active member.
        if (s.groupId >= 0 && s.groupId < MAX_GROUPS && g_groups[s.groupId].active &&
            g_groups[s.groupId].activeSlot != i) { continue; }
        order[n++] = i;
    }
    std::stable_sort(order, order + n,
                     [](int a, int b) { return g_slots[a].zBase < g_slots[b].zBase; });
    for (int k = 0; k < n; ++k) {
        const Slot& s = g_slots[order[k]];
        const Frame& f = s.frames[s.curFrame];
        blitHelpImage(f.blob.data(), f.blob.size(), fb, s.x + f.x, s.y + f.y);
    }
}

}  // namespace Anim
