#ifndef SLIDER_H
#define SLIDER_H

// ---------------------------------------------------------------------------
// SLIDER.h  —  Animated UI slider widgets
// Original: C:\DevStudio\Projects\Crux\SLIDER.cpp
// RE offsets: 0x00470780 – 0x004727xx
// ---------------------------------------------------------------------------
// A slider is a draggable thumb whose on-screen position is driven by an
// animation slot (Anim_*).  Up to 20 sliders may exist at once; their state
// lives in the shared channel table g_nSndChannelTable (0x007c5910), the same
// 20-entry / 0x30-byte-stride array used by the sound mixer's channel records.
//
// Slider entry layout (stride 0x30 bytes, indexed by slider id 0..19):
//
//   +0x00  nFlags         bit0 = in use, bit1 = vertical orientation
//   +0x04  nAnimSlot      animation slot id that renders the thumb
//   +0x08  nFrameCount    number of frames in the thumb animation
//   +0x0c  nThumbX        thumb position along X (pixel coordinate / value)
//   +0x10  nThumbY        thumb position along Y (pixel coordinate / value)
//   +0x14  nAnchorX       anim anchor X (frame-top-left correction)
//   +0x18  nAnchorY       anim anchor Y
//   +0x1c  nValueLo       reported value at the low end of the track
//   +0x20  nValueHi       reported value at the high end of the track
//   +0x24  nPixelLo       pixel coordinate of the low end of the track
//   +0x28  nPixelHi       pixel coordinate of the high end of the track
//   +0x2c  nMaxStep       max pixels the thumb may jump per tick on a click
//
// Data model (anim frame <-> slider value)
// -----------------------------------------
// The thumb's travel runs from nPixelLo..nPixelHi pixels.  Three mappings tie
// the three representations together:
//
//   span      = (nPixelHi - nPixelLo) +/- 1            (signed, inclusive)
//   frame     = (pos - nPixelLo) * nFrameCount / span  -> Anim_SetCurrentFrame
//   value     = ((nValueHi - nValueLo + 1) * (pos - nPixelLo)) / span + nValueLo
//
// So the animation frame is a linear remap of pixel travel onto the frame
// range, and the reported value is a linear remap of pixel travel onto the
// integer value range.  The horizontal axis uses nThumbX as "pos"; vertical
// sliders (flag bit1) use nThumbY instead.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Slider entry record (overlays the shared channel table)
// ---------------------------------------------------------------------------
struct SliderEntry              // 0x30 bytes
{
    int nFlags;                 // +0x00  bit0=in use, bit1=vertical
    int nAnimSlot;              // +0x04  animation slot id
    int nFrameCount;            // +0x08  frames in thumb animation
    int nThumbX;                // +0x0c  thumb X (pixels / value axis)
    int nThumbY;                // +0x10  thumb Y
    int nAnchorX;               // +0x14  anim anchor X
    int nAnchorY;               // +0x18  anim anchor Y
    int nValueLo;               // +0x1c  value at track low end
    int nValueHi;               // +0x20  value at track high end
    int nPixelLo;               // +0x24  pixel at track low end
    int nPixelHi;               // +0x28  pixel at track high end
    int nMaxStep;               // +0x2c  max pixels per click tick
};

#define SLIDER_MAX        20    // entries in the shared channel table
#define SLIDER_FLAG_USED  0x01  // +0x00 bit0
#define SLIDER_FLAG_VERT  0x02  // +0x00 bit1 (vertical orientation)

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Shared 20-entry channel table (also used by the sound mixer).  The slider
// system overlays a SliderEntry on each 0x30-byte record.
extern int g_nSndChannelTable;  // 0x007c5910  (table base; index * 0x0c ints)

extern int g_nSliderCurrent;    // 0x004d9a1c  default slider id (used when -1 is passed)

extern int g_nSliderRefX;       // 0x007d668c  reference X for click-track direction
extern int g_nSliderRefY;       // 0x007d6690  reference Y for click-track direction

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// 0x00470780  allocate a slider channel bound to an animation slot.
//   nAnimSlot : animation slot rendering the thumb
//   nFlags    : bit0 ignored (set internally), bit1 -> vertical orientation
//   returns   : slider id (0..19)
int  Slider_Add(int nAnimSlot, unsigned int nFlags);

// 0x004709f0  free a slider channel and reset its value/pixel ranges.
void Slider_Remove(int nSlider);

// 0x00470b30  set the "current" slider used when later calls pass id -1.
void Slider_SetCurrent(int nSlider);

// 0x00470bc0  reposition the thumb anchor; recomputes the anim offset.
void Slider_SetPosition(int nSlider, int nX, int nY);

// 0x00470d40  set the reported value range (lo, hi) for the track.
void Slider_SetValueRange(int nSlider, int nValueLo, int nValueHi);

// 0x00470e00  set the pixel range (lo, hi) the thumb travels across.
void Slider_SetPixelRange(int nSlider, int nPixelLo, int nPixelHi);

// 0x00470ec0  set the max pixels the thumb may jump per click tick.
void Slider_SetMaxStep(int nSlider, int nMaxStep);

// 0x00470f60  handle a click on the track: step the thumb toward the mouse,
//             ticking frames until release or the mouse is reached.
//             returns the resulting slider value.
int  Slider_TrackClicked(int nSlider);

// 0x00471820  drag the thumb while the mouse button is held (mouse inside the
//             thumb rect).  returns the resulting slider value.
int  Slider_Drag(int nSlider);

// 0x00471fe0  set the thumb directly from a value; clamps to the range.
void Slider_SetValue(int nSlider, int nValue);

#endif // SLIDER_H
