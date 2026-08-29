// Slider.cpp — animated UI slider widgets (clean-room port of ../src/SLIDER.cpp).
//
// Faithful port of the engine's frame<->value linear-remap model. Every config/
// state op is real. The two blocking interactive ops (trackClicked/drag) run a
// real per-frame loop while the left button is held when the Display is realtime;
// in headless/non-realtime mode they compute and return the real current value
// (ValueFromPos) without looping (there is no input to wait on). See Slider.h.
#include "Slider.h"
#include "Anim.h"
#include "Display.h"
#include "Log.h"

namespace {

// SliderEntry — mirrors ../src/SLIDER.h (port keeps a clean private array; the
// engine overlays this on the shared sound-channel table).
struct SliderEntry {
    int flags = 0;          // bit0 = in use, bit1 = vertical orientation
    int animSlot = -1;      // animation slot rendering the thumb
    int frameCount = 0;     // frames in the thumb animation
    int thumbX = 0;         // thumb X (pixels / value axis)
    int thumbY = 0;         // thumb Y
    int anchorX = 0;        // anim anchor X (frame-top-left correction)
    int anchorY = 0;        // anim anchor Y
    int valueLo = 0;        // value at track low end
    int valueHi = 0;        // value at track high end
    int pixelLo = 0;        // pixel at track low end
    int pixelHi = 0;        // pixel at track high end
    int maxStep = 0;        // max pixels per click tick
};

constexpr int FLAG_USED = 0x01;   // bit0
constexpr int FLAG_VERT = 0x02;   // bit1 (vertical orientation)

SliderEntry g_slots[Slider::SLIDER_MAX];
int g_current = 0;        // 0x004d9a1c — slider used when id == -1
int g_refX = 0;           // 0x007d668c — reference X for click-track direction
int g_refY = 0;           // 0x007d6690 — reference Y for click-track direction

// Resolve a slider id (-1 -> current) to its entry, or nullptr if out of range.
SliderEntry* resolve(int id) {
    if (id == -1) { id = g_current; }
    if (id < 0 || id >= Slider::SLIDER_MAX) {
        Log::warn("Slider: id %d out of range", id);
        return nullptr;
    }
    return &g_slots[id];
}

// Signed, inclusive pixel span of the track (matches the original (hi-lo) +/- 1
// idiom that keeps the span non-zero for division).
int span(const SliderEntry* s) {
    int diff = s->pixelHi - s->pixelLo;
    // ((hi <= lo) - 1 & 2) - 1  ==  +1 when hi>lo, -1 when hi<=lo
    return diff + ((((s->pixelHi <= s->pixelLo) - 1) & 2) - 1);
}

// Push the animation slot to match the thumb's current position.
void syncAnim(const SliderEntry* s, int pos, int sp) {
    int topX, topY;
    Anim::setCurrentFrame(s->animSlot, ((pos - s->pixelLo) * s->frameCount) / sp);
    Anim::getFrameTopLeft(s->animSlot, topX, topY);
    Anim::setPosition(s->animSlot,
                      (s->thumbX + s->anchorX) - topX,
                      (s->thumbY + s->anchorY) - topY);
}

// Map the thumb's current pixel position to the reported integer value.
int valueFromPos(const SliderEntry* s, int pos, int sp) {
    return ((s->valueHi - s->valueLo + 1) * (pos - s->pixelLo)) / sp + s->valueLo;
}

}  // namespace

namespace Slider {

// 0x00470780  Slider_Add: allocate a free slider, bind it to an anim slot and
// seed its thumb position/anchor from the slot's current frame.
int add(int animSlot, unsigned int flags) {
    int i = 0;
    while (i < SLIDER_MAX && (g_slots[i].flags & FLAG_USED)) { ++i; }
    if (i == SLIDER_MAX) {
        // Engine raises a fatal "out of slider channels" resource error; the
        // port degrades gracefully.
        Log::warn("Slider: out of slider channels");
        return -1;
    }

    SliderEntry* s = &g_slots[i];
    s->flags |= FLAG_USED;
    s->flags = (s->flags & ~FLAG_VERT) | ((flags & 1) << 1);

    s->animSlot   = animSlot;
    s->frameCount = Anim::frameCount(animSlot);

    Anim::setFrameStep(animSlot, 0);
    Anim::setCurrentFrame(animSlot, 0);
    s->maxStep = 3;

    int topX, topY;
    Anim::getFrameTopLeft(animSlot, topX, topY);
    int slotX, slotY;
    Anim::slotPos(animSlot, slotX, slotY);
    s->anchorX = slotX + topX;
    s->thumbX  = s->anchorX;
    s->anchorY = slotY + topY;
    s->thumbY  = s->anchorY;

    return i;
}

// 0x004709f0  Slider_Remove: free a slider and reset its value/pixel ranges.
void remove(int id) {
    if (id >= 0 && id < SLIDER_MAX) {
        SliderEntry* s = &g_slots[id];
        s->flags &= ~FLAG_USED;
        s->flags &= ~FLAG_VERT;
        s->animSlot = -1;
        s->valueHi  = 0;
        s->valueLo  = 0;
        s->pixelHi  = 0;
        s->pixelLo  = 0;
    }
}

// 0x00470b30  Slider_SetCurrent.
void setCurrent(int id) {
    g_current = id;
}

// 0x00470bc0  Slider_SetPosition: move the thumb anchor to (x,y); recompute the
// anim offset so the thumb's frame stays registered to the new anchor.
void setPosition(int id, int x, int y) {
    SliderEntry* s = resolve(id);
    if (!s) { return; }

    int topX, topY;
    Anim::getFrameTopLeft(s->animSlot, topX, topY);

    s->anchorX = s->thumbX - x;
    s->anchorY = s->thumbY - y;
    s->thumbX  = x;
    s->thumbY  = y;

    Anim::setPosition(s->animSlot,
                      (s->thumbX + s->anchorX) - topX,
                      (s->thumbY + s->anchorY) - topY);
}

// 0x00470d40  Slider_SetValueRange.
void setValueRange(int id, int valueLo, int valueHi) {
    SliderEntry* s = resolve(id);
    if (!s) { return; }
    s->valueLo = valueLo;
    s->valueHi = valueHi;
}

// 0x00470e00  Slider_SetPixelRange.
void setPixelRange(int id, int pixelLo, int pixelHi) {
    SliderEntry* s = resolve(id);
    if (!s) { return; }
    s->pixelLo = pixelLo;
    s->pixelHi = pixelHi;
}

// 0x00470ec0  Slider_SetMaxStep.
void setMaxStep(int id, int maxStep) {
    SliderEntry* s = resolve(id);
    if (!s) { return; }
    s->maxStep = maxStep;
}

// 0x00471fe0  Slider_SetValue: place the thumb directly from an integer value.
// Endpoints snap exactly to the track ends; intermediate values map linearly
// (with a half-step bias to land the thumb in the middle of its value cell).
void setValue(int id, int value) {
    SliderEntry* s = resolve(id);
    if (!s) { return; }

    // Vertical sliders position the thumb on Y, horizontal on X.
    int* pPos = (s->flags & FLAG_VERT) ? &s->thumbY : &s->thumbX;

    if ((value < s->valueLo && value < s->valueHi) ||
        (s->valueLo < value && s->valueHi < value)) {
        // Engine raises Err_BadResEntry "Value of slider out of range"; the port
        // logs and clamps via the same math below.
        Log::warn("Slider: value %d out of range [%d,%d]", value, s->valueLo, s->valueHi);
    }

    int sp = span(s);

    if (value == s->valueLo) {
        *pPos = s->pixelLo;
    } else if (value == s->valueHi) {
        *pPos = s->pixelHi;
    } else {
        int cells = (s->valueHi - s->valueLo) + 1;
        *pPos = ((value - s->valueLo) * sp) / cells + s->pixelLo + (sp / cells) / 2;
    }

    int topX, topY;
    Anim::setCurrentFrame(s->animSlot, ((*pPos - s->pixelLo) * s->frameCount) / sp);
    Anim::getFrameTopLeft(s->animSlot, topX, topY);
    Anim::setPosition(s->animSlot,
                      (s->thumbX + s->anchorX) - topX,
                      (s->thumbY + s->anchorY) - topY);
}

// 0x00470f60  Slider_TrackClicked: walk the thumb toward the mouse, at most
// maxStep pixels per tick, ticking animation frames until the mouse is reached or
// the button is released. Returns the resulting value. Horizontal sliders track
// mouseX; vertical sliders track mouseY.
// Wait watchdog: the two interactive ops below block while the left button is held.
// If the release is never observed (or the thumb can never reach the cursor) they spin
// silently, which is indistinguishable from a hung game. Log the state periodically
// instead — the loop keeps running exactly as the engine's does.
static void watchdog(int& frames, const char* what, const SliderEntry* s, Display& disp) {
    constexpr int WATCHDOG_FRAMES = 90;   // ~10s at the 9fps anim tick
    if (++frames % WATCHDOG_FRAMES != 0) { return; }
    Log::warn("SLIDER STUCK %s %dframes: thumb=(%d,%d) mouse=(%d,%d) pixel=[%d..%d] "
              "value=[%d..%d] maxStep=%d anim=%s held=%d",
              what, frames, s->thumbX, s->thumbY, disp.mouseX(), disp.mouseY(),
              s->pixelLo, s->pixelHi, s->valueLo, s->valueHi, s->maxStep,
              Anim::debugSlot(s->animSlot), (int)disp.leftButtonHeld());
}

int trackClicked(int id, Display& disp, const std::function<bool()>& pumpFrame) {
    SliderEntry* s = resolve(id);
    if (!s) { return 0; }
    int sp = span(s);

    // Headless / non-realtime: no live input to wait on. Return the real current
    // value (ValueFromPos at the thumb's current position) without looping.
    if (!disp.isRealtime()) {
        int pos = (s->flags & FLAG_VERT) ? s->thumbY : s->thumbX;
        return valueFromPos(s, pos, sp);
    }

    int dir = 1;   // walk direction sign
    int frames = 0;   // wait-watchdog counter

    if ((s->flags & FLAG_VERT) == 0) {
        // Horizontal: thumb travels along X toward mouseX.
        if (g_refX < s->thumbX) { dir = -1; }

        while (dir * s->thumbX < dir * disp.mouseX()) {
            int mx = disp.mouseX();
            int hi = (s->pixelLo < s->pixelHi) ? s->pixelHi : s->pixelLo;
            int lo = (s->pixelLo < s->pixelHi) ? s->pixelLo : s->pixelHi;

            if (mx <= hi && lo <= mx) {
                int want = dir * mx - dir * s->thumbX;
                int step = (s->maxStep < want) ? s->maxStep : want;
                s->thumbX += step * dir;

                int clampLo = (s->pixelLo < s->thumbX) ? s->thumbX : s->pixelLo;
                if (s->pixelHi < clampLo) {
                    s->thumbX = s->pixelHi;
                } else {
                    s->thumbX = (s->pixelLo < s->thumbX) ? s->thumbX : s->pixelLo;
                }

                syncAnim(s, s->thumbX, sp);
            }

            watchdog(frames, "trackClicked-h", s, disp);
            if (!pumpFrame()) { break; }              // quit / area change
            if (!disp.leftButtonHeld()) { dir = 0; }  // button released -> stop
        }

        return valueFromPos(s, s->thumbX, sp);
    } else {
        // Vertical: thumb travels along Y toward mouseY.
        if (g_refY < s->thumbY) { dir = -1; }

        while (dir * s->thumbY < dir * disp.mouseY()) {
            int my = disp.mouseY();
            int hi = (s->pixelLo < s->pixelHi) ? s->pixelHi : s->pixelLo;
            int lo = (s->pixelLo < s->pixelHi) ? s->pixelLo : s->pixelHi;

            // (original tests mouseX against the pixel bounds here)
            int mx = disp.mouseX();
            if (mx <= hi && lo <= mx) {
                int want = dir * my - dir * s->thumbY;
                int step = (s->maxStep < want) ? s->maxStep : want;
                s->thumbY += step * dir;

                int clampLo = (s->pixelLo < s->thumbY) ? s->thumbY : s->pixelLo;
                if (s->pixelHi < clampLo) {
                    s->thumbY = s->pixelHi;
                } else {
                    s->thumbY = (s->pixelLo < s->thumbY) ? s->thumbY : s->pixelLo;
                }

                syncAnim(s, s->thumbY, sp);
            }

            watchdog(frames, "trackClicked-v", s, disp);
            if (!pumpFrame()) { break; }
            if (!disp.leftButtonHeld()) { dir = 0; }
        }

        return valueFromPos(s, s->thumbY, sp);
    }
}

// 0x00471820  Slider_Drag: while the button is held, snap the thumb to the mouse
// (clamped to the track) whenever the cursor is inside the thumb's frame rect,
// and resync the animation. Returns the resulting value on release.
int drag(int id, Display& disp, const std::function<bool()>& pumpFrame) {
    SliderEntry* s = resolve(id);
    if (!s) { return 0; }
    int frames = 0;   // wait-watchdog counter
    int sp = span(s);

    // Headless / non-realtime: no live input to wait on. Return the real current
    // value (ValueFromPos at the thumb's current position) without looping.
    if (!disp.isRealtime()) {
        int pos = (s->flags & FLAG_VERT) ? s->thumbY : s->thumbX;
        return valueFromPos(s, pos, sp);
    }

    if ((s->flags & FLAG_VERT) == 0) {
        // Horizontal drag.
        while (disp.leftButtonHeld()) {
            watchdog(frames, "drag", s, disp);
            int rx, ry, rw, rh;
            if (Anim::frameBounds(s->animSlot, rx, ry, rw, rh)) {
                int my = disp.mouseY();
                if (ry < my && my < ry + rh) {
                    s->thumbX = disp.mouseX();

                    int hi = (s->pixelLo < s->pixelHi) ? s->pixelHi : s->pixelLo;
                    int lo = (s->pixelHi < s->pixelLo) ? s->pixelHi : s->pixelLo;
                    int clamped = (lo < s->thumbX)
                                      ? s->thumbX
                                      : ((s->pixelHi < s->pixelLo) ? s->pixelHi : s->pixelLo);
                    if (hi < clamped) {
                        s->thumbX = (s->pixelLo < s->pixelHi) ? s->pixelHi : s->pixelLo;
                    } else {
                        s->thumbX = clamped;
                    }

                    syncAnim(s, s->thumbX, sp);
                }
            }

            if (!pumpFrame()) { break; }
        }

        return valueFromPos(s, s->thumbX, sp);
    } else {
        // Vertical drag.
        while (disp.leftButtonHeld()) {
            watchdog(frames, "drag", s, disp);
            int rx, ry, rw, rh;
            if (Anim::frameBounds(s->animSlot, rx, ry, rw, rh)) {
                int mx = disp.mouseX();
                if (rx < mx && mx < rx + rw) {
                    s->thumbY = disp.mouseY();

                    int lo = (s->pixelLo < s->thumbY) ? s->thumbY : s->pixelLo;
                    if (s->pixelHi < lo) {
                        s->thumbY = s->pixelHi;
                    } else {
                        s->thumbY = (s->pixelLo < s->thumbY) ? s->thumbY : s->pixelLo;
                    }

                    syncAnim(s, s->thumbY, sp);
                }
            }

            if (!pumpFrame()) { break; }
        }

        return valueFromPos(s, s->thumbY, sp);
    }
}

// Free every slider — called on area change.
void clearAll() {
    for (auto& s : g_slots) { s = SliderEntry{}; }
    g_current = 0;
    g_refX = 0;
    g_refY = 0;
}

}  // namespace Slider
