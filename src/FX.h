#pragma once

// FX.cpp — sound-effect (SFX) playback layer
//
// Despite the "FX" name, this module is purely *sound* effects, not visual.
// (Source path in the binary: C:\DevStudio\Projects\Crux\FX.cpp)
//
// It sits on top of MIXER.cpp / SOUNDMEM.cpp and manages two things:
//
//   1. One-shot effects on a pool of channels 4..6 (Fx_PlayAnyChar, defined
//      in CURSORS.cpp's translation unit at 0x0042ac80) — finds the first
//      free channel and plays the named sound once.
//
//   2. A single *looping* background effect on channel 3.  Fx_Play stores the
//      effect name in g_szFxName, sets g_nFxLoop, and starts it; when the
//      channel finishes, Mixer's done-callback (Fx_LoopCallback) re-reads the
//      stored sound and re-plays it, giving a seamless loop.  Fx_StopLoop /
//      Fx_Stop tear it down.
//
// SndMem_ReadSound returns a pointer to the decoded waveform plus a format
// word; the bit-twiddling that turns that word into Mixer_PlayChannel args is:
//   (fmt & 2) >> 1            -> stereo flag (0 = mono, 1 = stereo)
//   (fmt & 1) * 8 + 8         -> bits per sample (8 or 16)
//   ((fmt & 4) >> 2)*0x5622 + 0x5622  -> sample rate (0x5622 or 0xAC44 = 22050/44100 Hz)

// -------------------------------------------------------------------------
// Globals
// -------------------------------------------------------------------------

// Non-zero while a looping effect is active on channel 3.        0x006b8ea8
extern int  g_nFxLoop;

// Name (lowercased) of the currently-looping effect.             0x006b8da8
extern char g_szFxName[256];

// Pan/volume parameter handed to every Mixer_PlayChannel call.   0x004ce89c
extern int  g_nFxVolume;

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

// One-shot SFX on a free channel 4..6 (lives in CURSORS.cpp TU).  0x0042ac80
void Fx_PlayAnyChar(char* pszName);

// Start the named sound looping on channel 3, arming the done-callback. 0x0042ae10
void Fx_PlayChar(char* pszName);

// Mixer channel-done callback: re-plays g_szFxName to continue the loop. 0x0042af90
void Fx_LoopCallback(int nChannel);

// Block (pumping Adv/timers) until channel 3's active-flag clears. 0x0042b0c0
void Fx_WaitChannel3(void);

// Variant of Fx_PlayChar with a different debug-trace tag.        0x0042b170
void Fx_PlayCharRestart(char* pszName);

// Public entry: lowercase the name, store it, mark looping, start. 0x0042b2f0
void Fx_Play(char* pszName);

// Stop looping: clear g_nFxLoop and the channel-3 done-callback.  0x0042b3e0
void Fx_StopLoop(void);

// Full stop: Fx_StopLoop + Snd_Stop(3).                           0x0042b480
void Fx_Stop(void);

// Clear only the channel-3 done-callback.                         0x0042b510
void Fx_ClearCallback(void);

// Re-play the stored looping effect (used to resume after a pause). 0x0042b5a0
void Fx_Resume(void);

// Set g_nFxVolume (pan/volume param for subsequent playback).     0x0042b6d0
void Fx_SetVolume(int nVolume);
