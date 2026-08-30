#include "Anim.h"
#include "HelpBlit.h"
#include "Area.h"
#include "Log.h"
#include <algorithm>
#include <cstdio>
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
    bool   visible = true;            // legacy show/hide flag (kept; no longer toggled by callers)
    int    freezeCount = 0;           // engine g_anAnimSlotFreezeCount: >0 = HIDDEN (not drawn) AND not
                                      // advanced. Anim_Freeze/0x191 increments, Anim_Unfreeze decrements
                                      // (floor 0), Anim_ResetFreeze/0x195 zeroes, ADD_FROZEN/0x13ba loads
                                      // at 1. It is a COUNT, not a bool, so a balanced FreezeAll/UnfreezeAll
                                      // (0x193/0x194) pair leaves a base-frozen slot still hidden (1->2->1).
                                      // Anim_GetCurrentFrame returns -1 when >0, so the engine skips it.
    bool   paused = false;            // frame-step 0: NOT advanced but STILL DRAWN (Anim_SetFrameStep/0x197,
                                      // FREEZE_ANIM/0x13c, and a finished one-shot resting on its last frame).
    bool   looping = false;           // wrap to 0 at the end (else stop on last frame)
    int    curFrame = 0;
    int    x = 0, y = 0;              // slot screen position
    bool   dumpPending = false;       // engine flag bit 2: queued for dump, still live until processed
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

// Slots marked for dump but not yet freed (engine g_anDumpQueue[50] / g_nDumpQueueCount).
std::vector<int> g_dumpQueue;

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

// --- diagnostics (ANIM_LOG=1) -------------------------------------------------
// Slot state is invisible from the outside, which makes a stuck WAIT_ANIM_END /
// WAIT_FRAME impossible to diagnose from a log. debugSlot() renders one slot's
// full state; animLog() traces every mutation that can strand a wait.
const char* debugSlot(int slot) {
    static char buf[192];
    if (slot < 0 || slot >= MAX_SLOTS) { std::snprintf(buf, sizeof(buf), "slot %d <out of range>", slot); return buf; }
    const Slot& s = g_slots[slot];
    if (!s.active) { std::snprintf(buf, sizeof(buf), "slot %d <inactive>", slot); return buf; }
    std::snprintf(buf, sizeof(buf),
                  "slot %d '%s' frame=%d/%zu stop=%d freeze=%d paused=%d loop=%d group=%d vis=%d cb=%d trig=%d",
                  slot, s.name.c_str(), s.curFrame, s.frames.size(), s.stopFrame, s.freezeCount,
                  (int)s.paused, (int)s.looping, s.groupId, (int)s.visible, s.completionCb, s.triggerFrame);
    return buf;
}

bool logEnabled() {
    static const bool on = std::getenv("ANIM_LOG") != nullptr;
    return on;
}

static void animLog(const char* what, int slot) {
    if (logEnabled()) { Log::info("ANIM %-12s %s", what, debugSlot(slot)); }
}

void reset() {
    for (auto& s : g_slots) { s = Slot{}; }
    g_firedCbs.clear();
    g_dumpQueue.clear();
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
    s.freezeCount = frozen ? 1 : 0;   // ADD_FROZEN (0x13ba): hidden (count 1) until verb-4 RESET_FREEZE
    s.paused = frozen;        // 0x13ba also SetFrameStep(0); harmless for the non-frozen loaders
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
    animLog("add", slot);
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
    // Frame boundary first: free everything marked during the previous frame. The engine
    // drains g_anDumpQueue from its dispatcher, and its trace shows a mark and its dump
    // landing back-to-back with no load between them ("marking for dump: VVI2TOK at 6" /
    // "dumping: VVI2TOK at 6"), so the queue is a per-frame deferral — a marked slot stays
    // drawn for the rest of the frame that marked it, and dies at the next boundary.
    processDumpQueue();
    tickGroups();
    for (int i = 0; i < MAX_SLOTS; ++i) {
        Slot& s = g_slots[i];
        // Engine Anim_HandleFrameTick step 1: skip ONLY frozen slots (freeze count > 0). A
        // PAUSED slot (frame-step 0) is NOT skipped — it is processed with a zero advance, so
        // its stop frame still clears and its completion callback still fires. That is load-
        // bearing: WAIT_ANIM_END on an anim resting (paused) at its last frame must terminate
        // (stop frame == last is already reached -> cleared), not spin forever.
        if (!s.active || s.freezeCount > 0) { continue; }
        // Grouped member that isn't the active one: hold (not drawn, not advanced).
        if (s.groupId >= 0 && s.groupId < MAX_GROUPS && g_groups[s.groupId].active &&
            g_groups[s.groupId].activeSlot != i) { continue; }
        // step: 0 when paused or single-frame (no advance), else 1 (engine g_anAnimSlotStep).
        const int step = (s.paused || (int)s.frames.size() <= 1) ? 0 : 1;
        s.curFrame += step;
        // Engine Anim_HandleFrameTick step 5: when the slot is at/past its armed stop frame,
        // CLEAR the stop frame to -1 (this is what makes Anim_IsAtStopFrame true and ends a
        // WAIT). The slot is not pinned there — it keeps advancing/looping; the script holds
        // position separately with FREEZE_ANIM if it wants to.
        const bool reachedStop = (s.stopFrame >= 0 && s.curFrame >= s.stopFrame);
        if (step != 0 && s.curFrame >= (int)s.frames.size()) {
            if (s.looping) { s.curFrame = 0; }
            else {
                // Anim_HandleFrameTick @0x004059c0 (end-of-anim branch @0x00406140): a
                // LOOPING slot (flags bit 4) rewinds to 0; every other slot falls through
                // to Anim_MarkForDump @0x004061d1 — bit 5 (0x20) only adds a clamp to the
                // last frame on the way. So a one-shot anim DISPOSES OF ITSELF when it
                // finishes; it does not park on its last frame forever (which left the
                // port's VVI2FRMQ / VVI2TOK / VVKCTO2 on screen and replaying).
                s.curFrame = (int)s.frames.size() - 1;   // clamp; still drawn this frame
                s.paused = true;
                markForDump(i);
            }
            // A grouped member whose anim just ended is released so the group re-rolls.
            if (s.groupId >= 0 && s.groupId < MAX_GROUPS && g_groups[s.groupId].activeSlot == i) {
                g_groups[s.groupId].activeSlot = -1;
            }
        }
        if (reachedStop) { s.stopFrame = -1; }
        // Completion callback (ops 0x3c/0x159/0x167/0x185): fire once when the anim reaches
        // its trigger frame, then clear (one-shot). The VM drains g_firedCbs and runs them.
        if (s.completionCb >= 0 && s.triggerFrame >= 0 && s.curFrame >= s.triggerFrame) {
            static const bool loopLog = std::getenv("LOOP_LOG") != nullptr;   // TEMP
            if (loopLog) { Log::info("LOOP cb-fire slot=%d name='%s' prog=%d cur=%d trig=%d looping=%d",
                                     i, s.name.c_str(), s.completionCb, s.curFrame, s.triggerFrame, s.looping); }
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

void clearAllCompletionCallbacks() {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        g_slots[i].completionCb = -1;
        g_slots[i].triggerFrame = -1;
    }
    g_firedCbs.clear();          // drop any that fired this frame but have not run yet
    if (logEnabled()) { Log::info("ANIM clearAllCallbacks (all %d slots)", MAX_SLOTS); }
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

// Anim_MarkForDump @0x00405810: drop the slot's area sprite if it has one, then flag it
// dump-pending and enqueue. The slot stays ACTIVE — it keeps drawing and advancing, and
// a stop frame armed on it still counts — until processDumpQueue() frees it.
void markForDump(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return; }
    Slot& s = g_slots[slot];
    if (s.dumpPending) { return; }                  // already queued
    Area::removeSpriteBySlot(slot);                 // engine: flags & 0x1000 -> Area_RemoveSprite
    s.dumpPending = true;
    g_dumpQueue.push_back(slot);
    animLog("markForDump", slot);
}

// Anim_ProcessDumpQueue @0x004074d0: free every queued slot, then clear the queue.
void processDumpQueue() {
    for (int slot : g_dumpQueue) {
        if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].dumpPending) { freeSlot(slot); }
    }
    g_dumpQueue.clear();
}

void freeSlot(int slot) {
    animLog("free", slot);
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
// else -1.
int getCurrentFrame(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].active && g_slots[slot].freezeCount == 0) {
        return g_slots[slot].curFrame;
    }
    return -1;
}

// Anim_Freeze @0x00407050: increment the freeze count (hide one level).
void freeze(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) { ++g_slots[slot].freezeCount; }
    animLog("freeze", slot);
}

// Anim_Unfreeze @0x004070f0: decrement the freeze count, floored at 0 (reveal one level).
void unfreeze(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS && g_slots[slot].freezeCount > 0) { --g_slots[slot].freezeCount; }
    animLog("unfreeze", slot);
}

// Anim_ResetFreeze @0x004071a0: clear the freeze count outright (fully reveal).
void resetFreeze(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].freezeCount = 0; }
    animLog("resetFreeze", slot);
}

// Anim_FreezeAll @0x00407230 / Anim_UnfreezeAll @0x00407380: bulk freeze/unfreeze, but ONLY
// slots that are on-screen (engine flags&2) or grouped — and by INCREMENT/DECREMENT, not
// reset. The menu balances them: enter-options (prog26) FreezeAll, back-to-game (prog59)
// UnfreezeAll, so a base-hidden flower (count 1, grouped) goes 1->2->1 and stays hidden,
// while a menu anim that was showing goes 0->1->0 and reappears. Our on-screen proxy is the
// `visible` flag; grouped slots always qualify (flowers are grouped via 0x13ba).
void freezeAll() {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        Slot& s = g_slots[i];
        if (s.active && (s.groupId >= 0 || s.visible)) { ++s.freezeCount; }
    }
}

void unfreezeAll() {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        Slot& s = g_slots[i];
        if (s.active && (s.groupId >= 0 || s.visible) && s.freezeCount > 0) { --s.freezeCount; }
    }
}

void setVisible(int slot, bool visible) {
    if (slot >= 0 && slot < MAX_SLOTS) { g_slots[slot].visible = visible; }
}

// Anim_SetStopFrame @0x00405450: arm the frame at which the slot halts. The engine ONLY
// arms it when the slot is on-screen (flags&2), dump-pending (flags&4), or grouped —
// otherwise it is a no-op and the stop frame stays -1. That is load-bearing: WAIT_ANIM_END
// / WAIT_FRAME on a hidden/frozen helper slot must terminate AT ONCE (stopFrame already -1
// -> Anim_IsAtStopFrame true), not spin forever. Our on-screen proxy is "active and not
// frozen" (a frozen slot is hidden — Anim_GetCurrentFrame returns -1, it is not drawn);
// grouped slots also qualify. tick() clears stopFrame back to -1 when the frame is reached.
void setStopFrame(int slot, int frame) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return; }
    Slot& s = g_slots[slot];
    const bool onScreen = s.freezeCount == 0;   // proxy for engine flags&2 (on display list)
    const bool grouped  = s.groupId >= 0;
    if (!(onScreen || grouped)) { return; }     // engine no-op -> stopFrame stays -1
    int last = (int)s.frames.size() - 1;
    if (frame < 0) { frame = -1; }
    else if (frame > last) { frame = last; }
    s.stopFrame = frame;
    animLog("setStopFrame", slot);
}

// Anim_IsAtStopFrame @0x00405610: literally `g_anAnimSlotStopFrame[slot] == -1`. The stop
// frame is the WAIT terminator — tick() resets it to -1 the moment the slot reaches it, and
// a freed/reloaded slot also resets to -1, so a wait whose target gets recycled (e.g. a
// completion callback frees+reloads it as looping) exits instead of hanging. An invalid /
// inactive slot reports "done" so a stale wait can never hang.
bool atStopFrame(int slot) {
    if (slot < 0 || slot >= MAX_SLOTS || !g_slots[slot].active) { return true; }
    return g_slots[slot].stopFrame == -1;
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

// Anim_SetFrameStep(slot, step): 0 pauses auto-advance, nonzero resumes. This is the
// frame-step gate (paused-but-DRAWN), distinct from the freeze count (frozen=hidden).
void setFrameStep(int slot, int step) {
    if (slot < 0 || slot >= MAX_SLOTS) { return; }
    g_slots[slot].paused = (step == 0);
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
        if (!s.active || !s.visible || s.freezeCount > 0) { continue; }   // freeze count>0 -> hidden (not drawn)
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
