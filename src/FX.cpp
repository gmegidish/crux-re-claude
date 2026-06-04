// FX.cpp — sound-effect (SFX) playback layer
//
// This is a *sound* module, not a visual one (the "FX" path in the binary is
// C:\DevStudio\Projects\Crux\FX.cpp).  It wraps MIXER.cpp / SOUNDMEM.cpp and
// provides:
//   - one-shot effects on a pool of channels 4..6 (Fx_PlayAnyChar, 0x0042ac80,
//     which physically lives in the CURSORS.cpp translation unit and is only
//     declared here), and
//   - a single looping background effect on channel 3 (everything below).
//
// The loop works by re-arming Mixer's per-channel done-callback: when the
// sound on channel 3 finishes, the mixer calls Fx_LoopCallback, which re-reads
// the stored waveform and plays it again.  g_nFxLoop gates this so a stop
// request (Fx_StopLoop) lets the loop die naturally.
//
// See FX.h for the SndMem_ReadSound format-word decoding used throughout.

#include "FX.h"

typedef unsigned int uint;

// ---- externs (other translation units) ----------------------------------

extern int  g_nMixerActiveChanIds;   // MIXER.cpp 0x006dc3d8 — per-id active flags (int[])

int  SndMem_ReadSound(char* pszName, int* pnLen, unsigned char bAlloc,
                      unsigned char nFlags, unsigned int* pnFmt);   // SLIDER.cpp 0x00472340
void Mixer_PlayChannel(int nChannel, int pData, int nLen, int bStereo,
                       int nBits, int nRate, int nVolume, int nLoops);
void Mixer_SetChannelDoneCallback(int nChannel, void (*pfn)(int));
void Mixer_ClearChannelDoneCallback(int nChannel);
void Snd_Stop(int nChannel);                                       // SETPAL.h 0x0046fa60

void Adv_Tick(void);                                               // ADVENT.h
void Timer_DispatchAsyncProg(void);                                // TIMERS.h
void Debug_Trace(int nLine, const char* pszFile, const char* pszMsg, ...);

// CRT helpers (other TUs)
char* _strlwr(char* psz);          // 0x0049def0
char* _strcpy(char* pszDst, const char* pszSrc);   // 0x004895e0
int   _strcmp(const char* a, const char* b);

// ---- globals (owned by this module) --------------------------------------

int  g_nFxLoop;          // 0x006b8ea8
char g_szFxName[256];    // 0x006b8da8
int  g_nFxVolume;        // 0x004ce89c

// --------------------------------------------------------------------------
// Common helper: read a sound by name and play it on a channel, decoding the
// format word into the mixer's (stereo, bits, rate) arguments.  Returns the
// data pointer (0 on failure / NULL sound).
// --------------------------------------------------------------------------
static int Fx_StartOnChannel(char* pszName, int nChannel, int nLoops)
{
    unsigned int fmt = 0;
    int          len = 0;
    int          pData = SndMem_ReadSound(pszName, &len, 0, 2, &fmt);
    if (pData == 0)
        return 0;

    Mixer_PlayChannel(nChannel, pData, len,
                      (int)(fmt & 2) >> 1,                       // stereo
                      (fmt & 1) * 8 + 8,                         // bits per sample
                      ((int)(fmt & 4) >> 2) * 0x5622 + 0x5622,   // sample rate
                      g_nFxVolume, nLoops);
    return pData;
}

// --------------------------------------------------------------------------
// 0x0042ae10  Fx_PlayChar — start the named effect looping on channel 3.
// Arms the loop callback only if g_nFxLoop is still set.
// --------------------------------------------------------------------------
void Fx_PlayChar(char* pszName)
{
    unsigned int fmt = 0;
    int          len = 0;
    int          pData = SndMem_ReadSound(pszName, &len, 0, 2, &fmt);

    Debug_Trace(0 /*line*/ + 7, "C:\\DevStudio\\Projects\\Crux\\FX.cpp", "att=%d", fmt);

    if (pData == 0)
    {
        Debug_Trace(0 /*line*/ + 10, "C:\\DevStudio\\Projects\\Crux\\FX.cpp", "NULL !!");
        return;
    }

    Mixer_PlayChannel(3, pData, len,
                      (int)(fmt & 2) >> 1,
                      (fmt & 1) * 8 + 8,
                      ((int)(fmt & 4) >> 2) * 0x5622 + 0x5622,
                      g_nFxVolume, -1);
    if (g_nFxLoop != 0)
        Mixer_SetChannelDoneCallback(3, Fx_LoopCallback);
}

// --------------------------------------------------------------------------
// 0x0042af90  Fx_LoopCallback — mixer done-callback for the looping effect.
// Re-reads g_szFxName and replays it on the same channel, re-arming itself,
// as long as the loop is still active.
// --------------------------------------------------------------------------
void Fx_LoopCallback(int nChannel)
{
    if (g_nFxLoop == 0)
        return;

    unsigned int fmt = 0;
    int          len = 0;
    int          pData = SndMem_ReadSound(g_szFxName, &len, 0, 2, &fmt);
    if (pData == 0)
        return;

    Mixer_PlayChannel(nChannel, pData, len,
                      (int)(fmt & 2) >> 1,
                      (fmt & 1) * 8 + 8,
                      ((int)(fmt & 4) >> 2) * 0x5622 + 0x5622,
                      g_nFxVolume, -1);
    Mixer_SetChannelDoneCallback(nChannel, Fx_LoopCallback);
}

// --------------------------------------------------------------------------
// 0x0042b0c0  Fx_WaitChannel3 — block until channel 3's active-flag clears,
// pumping the adventure tick + async timers so the engine keeps running.
// --------------------------------------------------------------------------
void Fx_WaitChannel3(void)
{
    while ((&g_nMixerActiveChanIds)[3] == 1)
    {
        Adv_Tick();
        Timer_DispatchAsyncProg();
    }
}

// --------------------------------------------------------------------------
// 0x0042b170  Fx_PlayCharRestart — identical to Fx_PlayChar but tagged with a
// different debug-trace string ("fx_play_0_char").  Used to (re)start the
// looping effect on channel 3.
// --------------------------------------------------------------------------
void Fx_PlayCharRestart(char* pszName)
{
    unsigned int fmt = 0;
    int          len = 0;
    int          pData = SndMem_ReadSound(pszName, &len, 0, 2, &fmt);

    Debug_Trace(0 /*line*/ + 7, "C:\\DevStudio\\Projects\\Crux\\FX.cpp", "att=%d", fmt);

    if (pData == 0)
    {
        Debug_Trace(0 /*line*/ + 10, "C:\\DevStudio\\Projects\\Crux\\FX.cpp", "NULL !!");
        return;
    }

    Mixer_PlayChannel(3, pData, len,
                      (int)(fmt & 2) >> 1,
                      (fmt & 1) * 8 + 8,
                      ((int)(fmt & 4) >> 2) * 0x5622 + 0x5622,
                      g_nFxVolume, -1);
    if (g_nFxLoop != 0)
        Mixer_SetChannelDoneCallback(3, Fx_LoopCallback);
}

// --------------------------------------------------------------------------
// 0x0042b2f0  Fx_Play — public entry point for the looping effect.
// Lowercases the name; if it differs from the one already looping (or nothing
// is looping), clears the channel-3 callback, stores the new name, marks the
// loop active, and starts it.
// --------------------------------------------------------------------------
void Fx_Play(char* pszName)
{
    _strlwr(pszName);

    if (g_nFxLoop == 0 || _strcmp(pszName, g_szFxName) != 0)
    {
        Mixer_ClearChannelDoneCallback(3);
        _strcpy(g_szFxName, pszName);
        g_nFxLoop = 1;
        Fx_PlayChar(pszName);
    }
}

// --------------------------------------------------------------------------
// 0x0042b3e0  Fx_StopLoop — stop looping: clear the flag so the callback no
// longer re-arms, and remove the channel-3 done-callback.
// --------------------------------------------------------------------------
void Fx_StopLoop(void)
{
    g_nFxLoop = 0;
    Mixer_ClearChannelDoneCallback(3);
}

// --------------------------------------------------------------------------
// 0x0042b480  Fx_Stop — full stop: stop looping then silence channel 3.
// --------------------------------------------------------------------------
void Fx_Stop(void)
{
    Fx_StopLoop();
    Snd_Stop(3);
}

// --------------------------------------------------------------------------
// 0x0042b510  Fx_ClearCallback — drop only the channel-3 done-callback
// (leaves g_nFxLoop / g_szFxName intact for a later resume).
// --------------------------------------------------------------------------
void Fx_ClearCallback(void)
{
    Mixer_ClearChannelDoneCallback(3);
}

// --------------------------------------------------------------------------
// 0x0042b5a0  Fx_Resume — replay the stored looping effect on channel 3 and
// re-arm the loop callback.  Uses volume 0x32 (50) for the (one) replay loop
// count argument, restarting the previously-stored g_szFxName.
// --------------------------------------------------------------------------
void Fx_Resume(void)
{
    if (g_nFxLoop == 0)
        return;

    unsigned int fmt = 0;
    int          len = 0;
    int          pData = SndMem_ReadSound(g_szFxName, &len, 0, 2, &fmt);
    if (pData == 0)
        return;

    Mixer_PlayChannel(3, pData, len,
                      (int)(fmt & 2) >> 1,
                      (fmt & 1) * 8 + 8,
                      ((int)(fmt & 4) >> 2) * 0x5622 + 0x5622,
                      g_nFxVolume, 0x32);
    Mixer_SetChannelDoneCallback(3, Fx_LoopCallback);
}

// --------------------------------------------------------------------------
// 0x0042b6d0  Fx_SetVolume — store the pan/volume parameter handed to every
// Mixer_PlayChannel call made by this module.
// --------------------------------------------------------------------------
void Fx_SetVolume(int nVolume)
{
    g_nFxVolume = nVolume;
}
