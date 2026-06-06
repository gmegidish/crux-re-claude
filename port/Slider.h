// Slider.h — animated UI slider widgets (clean-room port of SLIDER.cpp).
//
// A slider is a draggable thumb rendered by an animation slot (Anim_*). Its
// per-frame position is a linear remap of the thumb's pixel travel; the value it
// reports is a second linear remap of the same travel onto an integer range. See
// ../src/SLIDER.h for the full SliderEntry layout and the frame<->value model.
//
// The engine overlays SliderEntry on the shared 20-entry sound-channel table; the
// port keeps a clean private SliderEntry slots[20] array instead (no overlay).
// id == -1 resolves to the "current" slider (set by setCurrent). Up to 20
// sliders may exist at once (SLIDER_MAX).
//
// trackClicked()/drag() are blocking in the engine (they loop while the left
// mouse button is held). The port runs the VM synchronously, so these take the
// Display (for live mouse/button state) and a pump-one-frame callback that
// advances+renders the world. In headless/non-realtime mode there is no input to
// wait on, so they don't loop: they return the slider's CURRENT value via the
// real ValueFromPos math (honest — no fake constant).
#pragma once
#include <functional>

class Display;

namespace Slider {

constexpr int SLIDER_MAX = 20;

// Allocate a free slider bound to anim slot `animSlot`. flags bit0 -> vertical.
// Returns the slider id (0..19), or -1 if no slot is free.
int  add(int animSlot, unsigned int flags);

// Free a slider and reset its value/pixel ranges.
void remove(int id);

// Select the slider used when later calls pass id == -1.
void setCurrent(int id);

// Reposition the thumb anchor to (x,y); recomputes the anim offset.
void setPosition(int id, int x, int y);

// Set the reported value range / pixel range / max per-tick click step.
void setValueRange(int id, int valueLo, int valueHi);
void setPixelRange(int id, int pixelLo, int pixelHi);
void setMaxStep(int id, int maxStep);

// Place the thumb directly from an integer value (clamps to the range).
void setValue(int id, int value);

// Blocking in the engine: walk the thumb toward / drag it under the mouse while
// the button is held, then return the resulting value. `pumpFrame` advances and
// renders one frame (returns false to abort the loop early, e.g. quit/area
// change). In non-realtime/headless mode these return ValueFromPos at the thumb's
// current position without looping.
int  trackClicked(int id, Display& disp, const std::function<bool()>& pumpFrame);
int  drag(int id, Display& disp, const std::function<bool()>& pumpFrame);

// Free every slider — call on area change (mirrors Anim::reset()).
void clearAll();

}  // namespace Slider
