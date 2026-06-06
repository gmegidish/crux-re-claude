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
void setPosition(int slot, int x, int y);
void setCurrentFrame(int slot, int frame);
void freeze(int slot);
void resetFreeze(int slot);                  // fully unfreeze (freeze count -> 0)
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

// Blit every active+visible slot's current frame into fb (lowest add order
// first — the engine z-sorts; for the static menu, add order suffices).
void drawAll(Framebuffer& fb);

}  // namespace Anim
