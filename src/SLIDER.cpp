// ---------------------------------------------------------------------------
// SLIDER.cpp  —  Animated UI slider widgets
// Original: C:\DevStudio\Projects\Crux\SLIDER.cpp
// RE offsets: 0x00470780 – 0x004727xx
// ---------------------------------------------------------------------------
// A slider is a draggable thumb rendered by an animation slot (Anim_*).  Its
// per-frame position is a linear remap of the thumb's pixel travel; the value
// it reports is a second linear remap of the same travel onto an integer
// range.  See SLIDER.h for the full entry layout and the frame<->value model.
//
// State lives in the shared 20-entry channel table g_nSndChannelTable
// (0x007c5910, 0x30-byte stride), overlaid by SliderEntry.
// ---------------------------------------------------------------------------

#include "SLIDER.h"
#include "Advanim.h"   // Anim_* helpers, g_anAnimSlotX/Y, g_anAnimFrameCount
#include "ADVENT.h"    // Adv_TickFramesNoAsync, Adv_WaitForMouseNoAsync
#include "CURSORS.h"   // g_nMouseX, g_nMouseY, g_nMouseBtnDownMask
#include "ERRORS.h"    // Err_* / Debug_* / Res_* helpers

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

int g_nSliderCurrent = 0;   // 0x004d9a1c
int g_nSliderRefX    = 0;   // 0x007d668c
int g_nSliderRefY    = 0;   // 0x007d6690

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Resolve a SliderEntry from a slider id, mapping -1 to the current slider.
static SliderEntry* Slider_Resolve(int nSlider)
{
    if (nSlider == -1)
        nSlider = g_nSliderCurrent;
    return (SliderEntry*)((char*)&g_nSndChannelTable + nSlider * 0x30);
}

// Signed, inclusive pixel span of the track (matches the original
// (hi - lo) +/- 1 idiom that keeps the span non-zero for division).
static int Slider_Span(const SliderEntry* s)
{
    int diff = s->nPixelHi - s->nPixelLo;
    // ((hi <= lo) - 1 & 2) - 1  ==  +1 when hi>lo, -1 when hi<=lo
    return diff + ((((s->nPixelHi <= s->nPixelLo) - 1) & 2) - 1);
}

// Push the animation slot to match the thumb's current position: pick the
// frame from the pixel travel, then offset the slot so the thumb lands on
// (nThumbX, nThumbY) given the frame's top-left correction.
static void Slider_SyncAnim(const SliderEntry* s, int pos, int span)
{
    int topX, topY;
    Anim_SetCurrentFrame(s->nAnimSlot,
                         ((pos - s->nPixelLo) * s->nFrameCount) / span);
    Anim_GetFrameTopLeft(s->nAnimSlot, &topX, &topY);
    Anim_SetPosition(s->nAnimSlot,
                     (s->nThumbX + s->nAnchorX) - topX,
                     (s->nThumbY + s->nAnchorY) - topY);
}

// Map the thumb's current pixel position to the reported integer value.
static int Slider_ValueFromPos(const SliderEntry* s, int pos, int span)
{
    return ((s->nValueHi - s->nValueLo + 1) * (pos - s->nPixelLo)) / span
           + s->nValueLo;
}

// ---------------------------------------------------------------------------
// 0x00470780  Slider_Add
// Allocate a free slider channel, bind it to an animation slot and seed its
// thumb position/anchor from the slot's current frame.
// ---------------------------------------------------------------------------
int Slider_Add(int nAnimSlot, unsigned int nFlags)
{
    int i = 0;
    while (i < SLIDER_MAX &&
           (((int*)&g_nSndChannelTable)[i * 0x0c] & SLIDER_FLAG_USED))
    {
        ++i;
    }

    if (i == SLIDER_MAX)
    {
        // Out of slider channels — fatal resource error.
        Err_SetRecord3(0x1d, "Sliders", -1);
    }

    SliderEntry* s = (SliderEntry*)((char*)&g_nSndChannelTable + i * 0x30);

    s->nFlags = (s->nFlags | SLIDER_FLAG_USED);
    s->nFlags = (s->nFlags & ~SLIDER_FLAG_VERT) | ((nFlags & 1) << 1);

    s->nAnimSlot   = nAnimSlot;
    s->nFrameCount = g_anAnimFrameCount[nAnimSlot];

    Anim_SetFrameStep(nAnimSlot, 0);
    Anim_SetCurrentFrame(nAnimSlot, 0);
    s->nMaxStep = 3;

    int topX, topY;
    Anim_GetFrameTopLeft(nAnimSlot, &topX, &topY);
    s->nAnchorX = g_anAnimSlotX[nAnimSlot] + topX;
    s->nThumbX  = s->nAnchorX;
    s->nAnchorY = g_anAnimSlotY[nAnimSlot] + topY;
    s->nThumbY  = s->nAnchorY;

    return i;
}

// ---------------------------------------------------------------------------
// 0x004709f0  Slider_Remove
// Free a slider channel and reset its value/pixel ranges.
// ---------------------------------------------------------------------------
void Slider_Remove(int nSlider)
{
    if (nSlider >= 0 && nSlider < SLIDER_MAX)
    {
        SliderEntry* s = (SliderEntry*)((char*)&g_nSndChannelTable + nSlider * 0x30);
        s->nFlags &= ~SLIDER_FLAG_USED;
        s->nFlags &= ~SLIDER_FLAG_VERT;
        s->nAnimSlot = -1;
        s->nValueHi  = 0;
        s->nValueLo  = 0;
        s->nPixelHi  = 0;
        s->nPixelLo  = 0;
    }
}

// ---------------------------------------------------------------------------
// 0x00470b30  Slider_SetCurrent
// Select the slider used when later calls pass id -1.
// ---------------------------------------------------------------------------
void Slider_SetCurrent(int nSlider)
{
    g_nSliderCurrent = nSlider;
}

// ---------------------------------------------------------------------------
// 0x00470bc0  Slider_SetPosition
// Move the thumb anchor to (nX, nY); recompute the anim offset so the thumb's
// frame stays registered to the new anchor.
// ---------------------------------------------------------------------------
void Slider_SetPosition(int nSlider, int nX, int nY)
{
    SliderEntry* s = Slider_Resolve(nSlider);

    int topX, topY;
    Anim_GetFrameTopLeft(s->nAnimSlot, &topX, &topY);

    s->nAnchorX = s->nThumbX - nX;
    s->nAnchorY = s->nThumbY - nY;
    s->nThumbX  = nX;
    s->nThumbY  = nY;

    Anim_SetPosition(s->nAnimSlot,
                     (s->nThumbX + s->nAnchorX) - topX,
                     (s->nThumbY + s->nAnchorY) - topY);
}

// ---------------------------------------------------------------------------
// 0x00470d40  Slider_SetValueRange
// Set the integer value reported at the low / high ends of the track.
// ---------------------------------------------------------------------------
void Slider_SetValueRange(int nSlider, int nValueLo, int nValueHi)
{
    SliderEntry* s = Slider_Resolve(nSlider);
    s->nValueLo = nValueLo;
    s->nValueHi = nValueHi;
}

// ---------------------------------------------------------------------------
// 0x00470e00  Slider_SetPixelRange
// Set the pixel coordinates of the low / high ends of the track.
// ---------------------------------------------------------------------------
void Slider_SetPixelRange(int nSlider, int nPixelLo, int nPixelHi)
{
    SliderEntry* s = Slider_Resolve(nSlider);
    s->nPixelLo = nPixelLo;
    s->nPixelHi = nPixelHi;
}

// ---------------------------------------------------------------------------
// 0x00470ec0  Slider_SetMaxStep
// Set the max pixels the thumb may jump per tick while a click is held.
// ---------------------------------------------------------------------------
void Slider_SetMaxStep(int nSlider, int nMaxStep)
{
    SliderEntry* s = Slider_Resolve(nSlider);
    s->nMaxStep = nMaxStep;
}

// ---------------------------------------------------------------------------
// 0x00470f60  Slider_TrackClicked
// Handle a click somewhere on the track: walk the thumb toward the mouse, at
// most nMaxStep pixels per tick, ticking animation frames until the mouse is
// reached or the button is released.  Returns the resulting value.
// Horizontal sliders track g_nMouseX; vertical sliders track g_nMouseY.
// ---------------------------------------------------------------------------
int Slider_TrackClicked(int nSlider)
{
    SliderEntry* s = Slider_Resolve(nSlider);
    int span = Slider_Span(s);
    int dir  = 1;   // walk direction sign

    if ((s->nFlags & SLIDER_FLAG_VERT) == 0)
    {
        // Horizontal: thumb travels along X toward g_nMouseX.
        if (g_nSliderRefX < s->nThumbX)
            dir = -1;

        while (dir * s->nThumbX < dir * g_nMouseX)
        {
            int hi = (s->nPixelLo < s->nPixelHi) ? s->nPixelHi : s->nPixelLo;
            int lo = (s->nPixelLo < s->nPixelHi) ? s->nPixelLo : s->nPixelHi;

            if (g_nMouseX <= hi && lo <= g_nMouseX)
            {
                // Step toward the mouse, capped by nMaxStep.
                int want = dir * g_nMouseX - dir * s->nThumbX;
                int step = (s->nMaxStep < want) ? s->nMaxStep : want;
                s->nThumbX += step * dir;

                // Clamp the thumb to the pixel track.
                int clampLo = (s->nPixelLo < s->nThumbX) ? s->nThumbX : s->nPixelLo;
                if (s->nPixelHi < clampLo)
                    s->nThumbX = s->nPixelHi;
                else
                    s->nThumbX = (s->nPixelLo < s->nThumbX) ? s->nThumbX : s->nPixelLo;

                Slider_SyncAnim(s, s->nThumbX, span);
            }

            Adv_TickFramesNoAsync(1);
            if ((g_nMouseBtnDownMask & 1) == 0)
                dir = 0;   // button released -> stop the loop
        }

        return Slider_ValueFromPos(s, s->nThumbX, span);
    }
    else
    {
        // Vertical: thumb travels along Y toward g_nMouseY.
        if (g_nSliderRefY < s->nThumbY)
            dir = -1;

        while (dir * s->nThumbY < dir * g_nMouseY)
        {
            int hi = (s->nPixelLo < s->nPixelHi) ? s->nPixelHi : s->nPixelLo;
            int lo = (s->nPixelLo < s->nPixelHi) ? s->nPixelLo : s->nPixelHi;

            // (original tests g_nMouseX against the pixel bounds here)
            if (g_nMouseX <= hi && lo <= g_nMouseX)
            {
                int want = dir * g_nMouseY - dir * s->nThumbY;
                int step = (s->nMaxStep < want) ? s->nMaxStep : want;
                s->nThumbY += step * dir;

                int clampLo = (s->nPixelLo < s->nThumbY) ? s->nThumbY : s->nPixelLo;
                if (s->nPixelHi < clampLo)
                    s->nThumbY = s->nPixelHi;
                else
                    s->nThumbY = (s->nPixelLo < s->nThumbY) ? s->nThumbY : s->nPixelLo;

                Slider_SyncAnim(s, s->nThumbY, span);
            }

            Adv_TickFramesNoAsync(1);
            if ((g_nMouseBtnDownMask & 1) == 0)
                dir = 0;
        }

        return Slider_ValueFromPos(s, s->nThumbY, span);
    }
}

// ---------------------------------------------------------------------------
// 0x00471820  Slider_Drag
// Drag the thumb while the mouse button is held: as long as the cursor is
// inside the thumb's frame rect, snap the thumb to the mouse (clamped to the
// track) and resync the animation.  Returns the resulting value on release.
// ---------------------------------------------------------------------------
int Slider_Drag(int nSlider)
{
    SliderEntry* s = Slider_Resolve(nSlider);
    int span = Slider_Span(s);

    if ((s->nFlags & SLIDER_FLAG_VERT) == 0)
    {
        // Horizontal drag.
        while (g_nMouseBtnDownMask & 1)
        {
            int rx, ry, rw, rh;
            Anim_GetCurrentFrameRect(s->nAnimSlot, &rx, &ry, &rw, &rh);

            if (ry < g_nMouseY && g_nMouseY < ry + rh)
            {
                s->nThumbX = g_nMouseX;

                // Clamp nThumbX into [min(lo,hi), max(lo,hi)].
                int hi = (s->nPixelLo < s->nPixelHi) ? s->nPixelHi : s->nPixelLo;
                int lo = (s->nPixelHi < s->nPixelLo) ? s->nPixelHi : s->nPixelLo;
                int clamped = (lo < s->nThumbX)
                                  ? s->nThumbX
                                  : ((s->nPixelHi < s->nPixelLo) ? s->nPixelHi : s->nPixelLo);
                if (hi < clamped)
                    s->nThumbX = (s->nPixelLo < s->nPixelHi) ? s->nPixelHi : s->nPixelLo;
                else
                    s->nThumbX = clamped;

                Slider_SyncAnim(s, s->nThumbX, span);
            }

            Adv_WaitForMouseNoAsync();
        }

        return Slider_ValueFromPos(s, s->nThumbX, span);
    }
    else
    {
        // Vertical drag.
        while (g_nMouseBtnDownMask & 1)
        {
            int rx, ry, rw, rh;
            Anim_GetCurrentFrameRect(s->nAnimSlot, &rx, &ry, &rw, &rh);

            if (rx < g_nMouseX && g_nMouseX < rx + rw)
            {
                s->nThumbY = g_nMouseY;

                int lo = (s->nPixelLo < s->nThumbY) ? s->nThumbY : s->nPixelLo;
                if (s->nPixelHi < lo)
                    s->nThumbY = s->nPixelHi;
                else
                    s->nThumbY = (s->nPixelLo < s->nThumbY) ? s->nThumbY : s->nPixelLo;

                Slider_SyncAnim(s, s->nThumbY, span);
            }

            Adv_WaitForMouseNoAsync();
        }

        return Slider_ValueFromPos(s, s->nThumbY, span);
    }
}

// ---------------------------------------------------------------------------
// 0x00471fe0  Slider_SetValue
// Place the thumb directly from an integer value.  Endpoints snap exactly to
// the track ends; intermediate values map linearly (with a half-step bias to
// land the thumb in the middle of its value cell).
// ---------------------------------------------------------------------------
void Slider_SetValue(int nSlider, int nValue)
{
    SliderEntry* s = Slider_Resolve(nSlider);

    // Vertical sliders position the thumb on Y, horizontal on X.
    int* pPos = (s->nFlags & SLIDER_FLAG_VERT) ? &s->nThumbY : &s->nThumbX;

    if ((nValue < s->nValueLo && nValue < s->nValueHi) ||
        (s->nValueLo < nValue && s->nValueHi < nValue))
    {
        Err_BadResEntry(0, "C:\\DevStudio\\Projects\\Crux\\SLIDER.cpp",
                        "Value of slider out of range");
    }

    int span = Slider_Span(s);

    if (nValue == s->nValueLo)
    {
        *pPos = s->nPixelLo;
    }
    else if (nValue == s->nValueHi)
    {
        *pPos = s->nPixelHi;
    }
    else
    {
        int cells = (s->nValueHi - s->nValueLo) + 1;
        *pPos = ((nValue - s->nValueLo) * span) / cells
                + s->nPixelLo
                + (span / cells) / 2;
    }

    int topX, topY;
    Anim_SetCurrentFrame(s->nAnimSlot,
                         ((*pPos - s->nPixelLo) * s->nFrameCount) / span);
    Anim_GetFrameTopLeft(s->nAnimSlot, &topX, &topY);
    Anim_SetPosition(s->nAnimSlot,
                     (s->nThumbX + s->nAnchorX) - topX,
                     (s->nThumbY + s->nAnchorY) - topY);
}

// ---------------------------------------------------------------------------
// 0x00472340  SndMem_ReadSound  (resides in this translation unit)
// NOTE: not a slider routine.  This is the sound-cache loader "_read_sound":
// it ages all sound-memory slots, checks the cache by name, and on a miss
// streams the waveform in from the resource pool.  It physically lives at the
// end of the SLIDER object file in the binary, so it is documented here.
//   pszName : sound resource name
//   pnLen   : [out] decoded length in bytes
//   bAlloc  : SndMem allocation hint
//   nFlags  : bit0 set -> lookup only (no load on miss)
//   pnFmt   : [out] format/flags (bit0 8/16, bit1 mono/stereo, bit2 rate)
//   returns : pointer (as offset) to the cached sound data, or 0 on failure
// ---------------------------------------------------------------------------
int SndMem_ReadSound(char* pszName, int* pnLen, unsigned char bAlloc,
                     unsigned char nFlags, unsigned int* pnFmt)
{
    // Age every populated sound-memory slot by one tick.
    for (unsigned short i = 0; i < 200; ++i)
        if (g_anSndMemSlotAge[i] != 0)
            g_anSndMemSlotAge[i] += 1;

    Snd_NormalizeName(pszName);   // FUN_0049def0

    // Cache lookup by name.
    for (unsigned short i = 0; i < 200; ++i)
    {
        if (strcmp(pszName, g_abSndMemSlotNames + (unsigned int)i * 9) == 0)
        {
            char* rec = g_abSndMemSlotTable + (unsigned int)i * 0x21;
            *pnLen = *(int*)(rec + 0x14);
            *pnFmt = *(unsigned int*)(rec + 0x1d);
            return g_nSndMemPoolBase + *(int*)(rec + 0x10);
        }
    }

    if (nFlags & 1)
        return 0;   // lookup-only: cache miss reports failure

    // Cache miss: locate the resource (numbers 0x27..0x20, then 0x0d).
    unsigned short num;
    int found = 0;
    for (num = 0x27; num > 0x1f; --num)
    {
        char dummy[16];
        char pathBuf[260];
        found = Res_FindByNumChar(num, pszName, pathBuf, 0, dummy, pnLen);
        if (found == 0)
        {
            *pnFmt = num & 7;
            break;
        }
    }

    char dummy[16];
    char pathBuf[260];
    if (num == 0x1f)
    {
        if (Res_FindByNumChar(0x0d, pszName, pathBuf, 0, dummy, pnLen) != 0)
            return 0;
        *pnFmt = 0;
    }

    int slot = SndMem_AllocSlot(*pnLen, pszName, bAlloc);
    if (slot == -1)
        return 0;

    Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\SOUND.cpp",
                "cur snd=%d", slot);

    char* rec = g_abSndMemSlotTable + slot * 0x21;
    *(unsigned int*)(rec + 0x1d) = *pnFmt;

    if (g_nSndMemPoolSize <= *(int*)(rec + 0x10) + *pnLen)
        Err_BadResEntry(0, "C:\\DevStudio\\Projects\\Crux\\SOUND.cpp",
                        "out of soundcache");

    // Stream the waveform into the slot, scaled by the format multipliers.
    unsigned int fmt = *pnFmt;
    int rawBytes = *pnLen * g_nSndSampleSize;   // DAT_004c4c40
    int divisor  = ((((int)(fmt & 4) >> 2) + 1) *
                    (((int)(fmt & 2) >> 1) + 1) *
                    ((fmt & 1) + 1) * 0x5622);
    Res_BunchFreadStreamLoadPtr(rec, 1, *pnLen, dummy, 0, rawBytes / divisor);
    Res_WaitForEntry(rec);
    SndMem_Compact();

    return *(int*)rec;
}
