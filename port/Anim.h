// Anim.h — animation-slot subsystem (clean-room port of Advanim.cpp).
//
// An animation is a type-7 resource: [u8 0x10][u16 ver=1][u16 W][u16 H]
// [u8 maskFlag][u32 frameCount], then frameCount x {s16 x, s16 y, s32 size},
// then each frame's sprite blob (a Help_BlitImage strip/BANI stream — decoded by
// decodeSprite). A slot holds the loaded frames + a screen position + current
// frame; drawAll() blits every visible slot's current frame at
// (slotX + frame.x, slotY + frame.y).
//
// A global pool mirrors the engine's 150 parallel-array slots; ops in RunProg
// (0x13/0x13ba/0x19/0x3d...) and the Adv_RunScene render loop both use it.
#pragma once
#include "ResArchive.h"
#include "Framebuffer.h"
#include <cstdint>

namespace Anim {

constexpr int MAX_SLOTS = 150;

// Free every slot (called when entering a new area).
void reset();

// Load a single-frame anim-format sprite resource (e.g. a type-6 area background)
// and blit its frame 0 at (x,y) + the frame's own offset. Returns false if the
// resource isn't found or is malformed. Does not allocate a slot.
bool blitResourceFrame0(ResArchive& arc, Framebuffer& fb, const char* name, int type, int x, int y);

// Load a type-7 anim by resource name into a free slot; returns the slot or -1.
// `looping` sets the loop flag; `frozen` freezes at frame 0 (static display).
int addByName(ResArchive& arc, const char* name, bool looping, bool frozen);

int  findByName(const char* name);           // slot with this name, or -1
void freeSlot(int slot);

// --- DEFERRED DUMP (engine Anim_MarkForDump @0x00405810 / Anim_ProcessDumpQueue
//     @0x004074d0, g_anDumpQueue[50]) ---
// The engine does NOT free an anim when a script asks it to (op 0x13). It sets the
// slot's dump-pending flag and queues it; the slot stays active, drawn and advancing
// until the queue is processed. The trace of the real engine shows the ordering:
//
//     marking for dump: VVI2FRMQ at 6
//     Adding animation: VVI2SHTC/7
//     Finished adding: VVI2SHTC/7
//     dumping: VVI2FRMQ at 6
//
// i.e. the outgoing anim is kept alive until its replacement has finished loading —
// so processDumpQueue() runs at the end of addByName(). (The engine also drives it
// from a command dispatcher at 0x00403294; that path isn't modelled.)
void markForDump(int slot);                  // Anim_MarkForDump: flag + enqueue, do not free
void processDumpQueue();                     // Anim_ProcessDumpQueue: free everything queued

// --- per-slot STOP FRAME (engine Anim_SetStopFrame @0x004054b0 / g_anAnimSlotStopFrame) ---
// Set the frame at which `slot` should halt auto-advance. tick() does NOT advance a
// slot past its stop frame (it clamps curFrame to stopFrame and leaves the slot's
// getCurrentFrame valid — i.e. NOT frozen). Pass -1 to clear (no stop frame).
// Used by WAIT_ANIM_END (0x1f) and WAIT_FRAME (0x13b).
void setStopFrame(int slot, int frame);
// True once `slot` has reached (its curFrame >=) its stop frame. Mirrors the engine's
// Anim_IsAtStopFrame loop terminator. False if no stop frame is set / slot inactive.
bool atStopFrame(int slot);

// --- per-slot Z / draw-order key (engine Anim_SetWalkTableBase @0x00406980 sets
//     g_nCharWalkTableBase, which Anim_CompareByZ @0x0040796e uses to z-sort the draw
//     order). Set by op 0x137 SET_WALKTABLE. drawAll() sorts by this (lower = behind). ---
void setZBase(int slot, int base);

// --- ANIM GROUPS (engine Anim_StartGroup @0x00409260 / Anim_AddToGroup @0x00409460,
//     consumed by Anim_BuildDrawOrder @0x00407708) ---
// A group is a set of mutually-exclusive member slots; only the group's ACTIVE member
// is drawn/advanced. startGroup opens a group of `size` members with switch chance
// `triggerPct` (% per tick) and no active member. The next `size` anim-add calls
// (addByName) auto-join the open group. Each tick, a group with no active member rolls
// rand()%100 < triggerPct; on success it picks a random member as active (engine RNG is
// an LCG, std::rand() per op 0x1c precedent). When the active member's anim ends it is
// released (active -> none).
void startGroup(int size, int triggerPct);
// True while an open group is still waiting for members (drives addByName auto-join).
bool groupOpen();

// --- per-slot completion callback / trigger frame (engine g_anAnimSlotTriggerFrame +
//     the slot's script-trigger; ops 0x3c set-id / 0x159 fire-at-end / 0x167 set-frame /
//     0x185 fire-at-frame). When tick() advances `slot` to its triggerFrame, the slot's
//     completionCb (a script program id) is queued; the VM drains it via takeFiredCallback
//     and runs it. One-shot: cleared after firing. ---
void setTriggerFrame(int slot, int frame);            // 0x167: where to fire (no cb change)
void setCompletionCallback(int slot, int progId, int frame);  // 0x159/0x185: cb + frame; progId<0 clears
// Pop the next fired callback program id (FIFO), or -1 if none. Drained each frame by RunProg.
int  takeFiredCallback();
void setPosition(int slot, int x, int y);
void setCurrentFrame(int slot, int frame);
void freeze(int slot);                       // Anim_Freeze: freeze count + 1 (hide one level)
void unfreeze(int slot);                     // Anim_Unfreeze: freeze count - 1, floored at 0
void resetFreeze(int slot);                  // Anim_ResetFreeze: freeze count -> 0 (fully reveal)
void freezeAll();                            // Anim_FreezeAll (0x193): inc on-screen/grouped slots
void unfreezeAll();                          // Anim_UnfreezeAll (0x194): dec on-screen/grouped slots
void setVisible(int slot, bool visible);     // show/hide in drawAll

bool        active(int slot);                 // is this slot in use?
const char* slotName(int slot);               // resource name, or "" if inactive

// --- accessors used by the SLIDER subsystem (Slider.cpp) ---
// Number of frames in `slot`'s animation (== engine g_anAnimFrameCount[slot]);
// 0 if the slot is inactive.
int  frameCount(int slot);
// The CURRENT frame's per-frame {x,y} offset (frame.x/frame.y) — matches engine
// Anim_GetFrameTopLeft. Writes (0,0) for an inactive/out-of-range slot.
void getFrameTopLeft(int slot, int& x, int& y);
// The slot's screen position (slotX/slotY == engine g_anAnimSlotX/Y[slot]).
void slotPos(int slot, int& x, int& y);
// Engine Anim_SetFrameStep(slot, step): 0 pauses auto-advance (freeze), nonzero
// resumes it. The port has no per-tick step magnitude, so we model the only two
// values the slider uses: 0 = freeze, !=0 = resetFreeze.
void setFrameStep(int slot, int step);

// Current frame index of an active, non-frozen slot; -1 if the slot is inactive
// or frozen (matches Anim_GetCurrentFrame @0x004019fb, which returns -1 unless
// (flags & active) && freezeCount == 0).
int         getCurrentFrame(int slot);

// On-screen bounding box of `slot`'s current frame (the painted pixels), in
// framebuffer coordinates. Returns false for an empty/blank frame. Used to turn
// a flower anim into a hover hotspot.
bool frameBounds(int slot, int& x, int& y, int& w, int& h);

// Advance every active, non-frozen, multi-frame anim by one frame (looping anims
// wrap; one-shot anims stop on the last frame). Call once per animation tick.
void tick();

// One-line state dump of a slot, for diagnostics (wait watchdog, ANIM_LOG).
// Returns a static buffer: name, curFrame/frameCount, stopFrame, freezeCount,
// paused, looping, groupId. Safe on an inactive/out-of-range slot.
const char* debugSlot(int slot);

// Blit every active+visible slot's current frame into fb (lowest add order
// first — the engine z-sorts; for the static menu, add order suffices).
void drawAll(Framebuffer& fb);

}  // namespace Anim
