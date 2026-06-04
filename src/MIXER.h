#ifndef MIXER_H
#define MIXER_H

// ---------------------------------------------------------------------------
// MIXER.h  —  Software PCM mixer with DirectSound output
// Original: C:\DevStudio\Projects\Crux\MIXER.cpp
// RE offset: 0x004400e0 – 0x004513f0  (fill kernels up to ~0x00451e90)
// ---------------------------------------------------------------------------
// MIXER is the low-level audio output engine for Crux.  It manages:
//
//   - One DirectSound primary or secondary buffer (g_pDSBuffer).
//   - Up to 8 logical mixer channels (g_nMixerChannelTable), sorted by
//     remaining sample count so the shortest-remaining channel is last.
//   - A dedicated background thread (Mixer_ThreadProc, "mx_handle_bufs")
//     that WaitForSingleObject's on a notification event, then Locks the
//     DirectSound buffer, calls g_pfnMixerFillBuf to mix all channels into
//     it, and Unlocks/Plays.
//   - Volume in the range 0..32 (0x20) per channel, stored in the channel
//     table.  Channel 0 (music) applies an additional g_nMixerMusicVolPct
//     scale factor read from CRUX.INI [Sound] MuseVol (default 100).
//   - Pan support: channels in mode 2 carry separate left/right scale
//     factors computed from a pan position (0..100, centre=50).
//   - A family of 8 fill-buffer kernels selected by Mixer_SelectFillFunc
//     based on the DirectSound buffer's sample width and channel count vs.
//     each source channel's format.
//
// DirectSound buffer architecture:
//   Primary buffer mode (DAT_006dc26c==1): DSSCL_WRITEPRIMARY co-op level.
//     Format is negotiated from hardware caps (stereo if available, else mono).
//   Secondary buffer mode (DAT_006dc26c==2): DSSCL_NORMAL, 16-bit stereo
//     16 KB buffer (DAT_006dc064=0x4000), used as fallback.
//
// Init sequence:
//   Mixer_Init → Mixer_InitTables → DirectSoundCreate → SetCooperativeLevel
//   → CreateSoundBuffer → SetFormat → GetCaps → Lock → memset silence →
//   Unlock → Play → WaitForSingleObject(init event, 10 s) → Mixer_ThreadProc.
//   On failure: Mixer_Kick can call Mixer_Reinit to retry.
//
// Channel table layout (at DAT_006dc098, stride 0x30 = 48 bytes each):
//   +0x00  int  channel_id         (0..7)
//   +0x04  void* read_ptr          current PCM read position
//   +0x08  int  volume_scaled      effective volume (after music-vol scale)
//   +0x0c  int  mixing_mode       0=mono-to-mono, 1=stereo-downmix, 2=pan
//   +0x10  int  nsamples_left     remaining output samples
//   +0x14  int  left_vol          pan left scale (mode 2 only)
//   +0x18  int  right_vol         pan right scale (mode 2 only)
//   +0x1c  int  pan_pos           0..100 pan position
//   +0x20  int  src_rate          source sample rate (not currently used for SRC)
//   +0x24  int  done_callback     pointer to completion callback (or 0)
//   +0x28  int  pad[2]            (unused / alignment)
//
// Per-channel state table (at DAT_006dc278, stride 0x28 = 40 bytes each):
//   +0x00  void*  start_ptr        PCM start address
//   +0x04  void*  end_ptr          PCM end address (start + byte_count)
//   +0x08  int    volume           requested volume 0..32
//   +0x0c  int    saved_volume     restored by Mixer_RestoreVolume after fade
//   +0x10  int    is_looping       flag: loop when end reached
//   +0x14  int    flags            bit0=paused, bit1=reserved
//   +0x18  int    pan              pan position 0..100
//   +0x1c  int    done_callback    pointer to completion callback
//   +0x20  int    reserved[2]
//
// Fill-buffer kernel selection (Mixer_SelectFillFunc):
//   DAT_006dc270 = output channels (1=mono, 2=stereo)
//   DAT_006dc264 = output sample width in bytes (1=8-bit, 2=16-bit)
//   DAT_006dc068 = output sample rate (e.g. 0xAC44=44100, 0x5622=22050)
//
//   The 8 kernels cover combinations of:
//     output: U8 stereo or mono, S16 stereo or mono
//     source mix: mono channels, stereo channels (L/R separate), pan mode
//
// Original source: C:\DevStudio\Projects\Crux\MIXER.cpp
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dsound.h>
#include <mmsystem.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define MIXER_MAX_CHANNELS  8       // maximum simultaneous mixer channels
#define MIXER_MAX_VOLUME    32      // maximum volume level (0x20)
#define MIXER_MUSIC_CHAN    0       // channel index reserved for music/themes

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// DirectSound objects
extern IDirectSound*        g_pDSound;        // 0x006dc094  IDirectSound* instance
extern IDirectSoundBuffer*  g_pDSBuffer;      // 0x006dc090  primary/secondary IDirectSoundBuffer*

// Mixer state
extern int  g_nMixerActiveChans;  // 0x006dc3d0  number of entries in active channel table
extern int  g_nMixerBufSize;      // 0x006dc064  DirectSound buffer size in bytes
extern int  g_nMixerSampleRate;   // 0x006dc068  output sample rate (e.g. 44100=0xAC44)
extern int  g_nMixerOutChans;     // 0x006dc270  output channel count (1=mono, 2=stereo)
extern int  g_nMixerBitDepth;     // 0x006dc264  output bytes per sample (1=8-bit, 2=16-bit)
extern int  g_nMixerBufMode;      // 0x006dc26c  1=primary buffer, 2=secondary buffer
extern int  g_nMixerFailed;       // 0x004d3b0c  non-zero if init failed (use silence)
extern int  g_nMixerMusicVolPct;  // 0x004d3b04  music volume percent from INI [Sound] MuseVol
extern int  g_nMixerSilenceByte;  // 0x006dc25c  silence fill value (0x00 for S16, 0x80 for U8)
extern int  g_nMixerBytesPerTick; // 0x006dc268  bytes per output tick (rate/fps * channels * bytes)

// Function pointer to currently selected fill-buffer kernel
// Set by Mixer_SelectFillFunc; called from Mixer_ThreadProc to mix channels.
// Signature: void (*)(void* dsBufPtr, size_t byteCount)
extern void (*g_pfnMixerFillBuf)(void*, size_t);   // 0x006dc238

// Thread / event objects
extern HANDLE  g_hMixerThread;        // 0x007c4ba4  mixer background thread
extern DWORD   g_nMixerThreadId;      // 0x006dc06c  mixer thread ID
extern HANDLE  g_hMixerReadyEvent;    // 0x006dc404  signalled when buffer is ready/needed
extern HANDLE  g_hMixerInitEvent;     // 0x006dc400  signalled once thread is initialised
extern int     g_nMixerStopping;      // 0x006dc23c  non-zero while Mixer_Kick/Reinit is running

// Volume lookup table (2 KB): maps mixed 16-bit value → clamped 8-bit output
extern void*   g_pMixerVolTable;      // 0x006dc240  SafeAlloc'd 0x800 bytes

// Intermediate S16 mix accumulation buffer (for U8 output modes)
extern void*   g_pMixerAccumBuf;      // 0x006dc260  SafeAlloc'd 0x1000 bytes (mono output only)

// Active channel table base and stride
// Active channel table: DAT_006dc098, stride 0x30, up to MIXER_MAX_CHANNELS entries
// Per-channel state: DAT_006dc278, stride 0x28, indexed by channel id 0..7

// Timer handle for the DirectSound buffer-notification timer
extern int  g_nMixerTimerId;          // 0x004d3b08  Theme_SetTimer handle for buffer-tick

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Mixer_Init — one-time initialisation.
//   param_1 = sample rate (e.g. 44100)
//   param_2 = allow stereo (1=yes, 0=force mono)
//   param_3 = allow 16-bit (1=yes, 0=force 8-bit)
//   param_4 = pan enabled (passed to Mixer_SelectFillFunc)
// Calls Mixer_InitTables, DirectSoundCreate, GetCaps, SetCooperativeLevel,
// CreateSoundBuffer, spawns Mixer_ThreadProc, starts Play.
void  Mixer_Init(int sampleRate, int allowStereo, int allow16bit, int allowPan);

// Mixer_Reinit — recreate DirectSound buffer after device loss.
// Named "mx_reinit" in debug trace.  Called by Mixer_Kick.
void  Mixer_Reinit(void);

// Mixer_InitTables — read MuseVol from INI, allocate volume clamp table.
// Named (implicitly) as part of Mixer_Init; also called separately.
void  Mixer_InitTables(void);

// Mixer_Kick — recover from a stalled/lost buffer.
// Named "mx_kick" in debug trace.  Sets g_nMixerStopping=1, Sleeps 10ms,
// releases IDirectSound, calls Mixer_Reinit, clears flag.
void  Mixer_Kick(void);

// Mixer_Stop — hard stop and teardown.
// Calls IDirectSoundBuffer::Stop (vtable +0x48), kills the DirectSound
// timer, closes event handles, terminates mixer thread, calls waveOutReset/Close.
// After return the mixer is fully shut down (g_nMixerFailed remains 0).
void  Mixer_Stop(void);

// Mixer_ThreadProc — DirectSound fill thread.
// Named "mx_handle_bufs" in debug trace.
// Loops: WaitForSingleObject(g_hMixerReadyEvent) → GetStatus → Lock →
// mix channels via g_pfnMixerFillBuf → Unlock → Play → SetEvent.
DWORD WINAPI Mixer_ThreadProc(void);

// Mixer_AddChannel — activate a channel for playback (called "Resume" by THEMES).
//   param_1 = channel id (0..7)
// Inserts the channel into the active list sorted by remaining sample count
// (ascending, so the channel that finishes soonest is at the back).
// Clears the paused flag (bit 0 of flags field).
void  Mixer_AddChannel(int channelId);

// Mixer_RemoveChannel — deactivate a channel (called "Pause" by THEMES).
//   param_1 = channel id (0..7)
// Removes the channel from the active sorted list, sets paused flag (bit 0).
void  Mixer_RemoveChannel(int channelId);

// Mixer_ChannelDone — called when a channel has exhausted all its samples.
//   param_1 = channel id (0..7)
// EnterCriticalSection, clears start/end ptrs, removes from active list,
// decrements looping counter, fires per-channel done callback if set.
void  Mixer_ChannelDone(int channelId);

// Mixer_SetVolume — set volume and/or pan on a channel, with pan processing.
//   param_1 = channel id (0..7)
//   param_2 = volume 0..32, or 0xFFFFFFFF to keep current
//   param_3 = pan 0..100, or -1 to keep current, or 0x32 (50) for centre
// When a pan value is applied, computes left/right scale factors via
// atan2-based panning and sets mixing_mode=2.
void  Mixer_SetVolume(int channelId, unsigned int volume, int pan);

// Mixer_SetVolumeOnly — simplified SetVolume that skips pan recalculation.
//   param_1 = channel id, param_2 = volume 0..32, param_3 = pan (-1=keep)
// Updates volume in channel state and active table, then calls Mixer_SetVolume
// with param_2=0xFFFFFFFF (keep vol) to finalise pan if needed.
void  Mixer_SetVolumeOnly(int channelId, unsigned int volume, int pan);

// Mixer_RestoreVolume — restore channel volume from its saved value.
//   param_1 = channel id
// Copies saved_volume → volume and reapplies it in the active table.
// Called after a temporary fade/duck on secondary channels completes.
void  Mixer_RestoreVolume(int channelId);

// Mixer_SelectFillFunc — choose the PCM fill-buffer kernel.
//   param_1 = source channels (1=mono, 2=stereo)
//   param_2 = pan enabled (0=no, 1=yes)
// Sets g_pfnMixerFillBuf based on output format (g_nMixerBitDepth,
// g_nMixerOutChans, g_nMixerSampleRate) and source format.
void  Mixer_SelectFillFunc(int srcChannels, int panEnabled);

// Mixer_SkipBytes — advance all channel read pointers by N output bytes.
//   param_1 = byte count to skip
// Used to compensate for buffer underruns detected in the fill thread.
void  Mixer_SkipBytes(int byteCount);

// Mixer_UpdateTimers — tick-based bookkeeping (reads timeGetTime, iterates channels).
// Called once per fill-thread iteration before mixing begins.
void  Mixer_UpdateTimers(void);

// Mixer_DsErrToStr — convert a DirectSound HRESULT to a human-readable string.
//   param_1 = HRESULT from a DS API call
// Returns a static string like "DSERR_ALLOCATED", "DS_OK", "Unknown", etc.
const char* Mixer_DsErrToStr(int hr);

// ---------------------------------------------------------------------------
// Fill-buffer kernels (selected via g_pfnMixerFillBuf by Mixer_SelectFillFunc)
// All take (void* dsBufPtr, size_t byteCount).
// "U8" = unsigned 8-bit output (silence=0x80); "S16" = signed 16-bit (silence=0).
// First suffix = output layout; second = source layout.
// ---------------------------------------------------------------------------

// U8 stereo output, mono sources (duplicate L to R)          0x00449450
void Mixer_FillBuf_U8_Stereo_Mono(void* buf, size_t n);
// U8 mono output, mono sources                               0x0044ac50
void Mixer_FillBuf_U8_Mono_Mono(void* buf, size_t n);
// S16 stereo output, stereo sources (L+R separate)           0x0044b760
void Mixer_FillBuf_S16_Stereo_Stereo(short* buf, size_t n);
// S16 stereo output, stereo sources (variant B — wrap)       0x0044cb80
void Mixer_FillBuf_S16_Stereo_StereoB(short* buf, size_t n);
// S16 stereo output, mono sources                            0x0044deb0
void Mixer_FillBuf_S16_Stereo_Mono(short* buf, size_t n);
// U8 stereo output, stereo sources (variant A)               0x0044e8e0
void Mixer_FillBuf_U8_Stereo_StereoA(void* buf, size_t n);
// U8 stereo output, stereo sources (variant B — wrap)        0x0044fe50
void Mixer_FillBuf_U8_Stereo_StereoB(void* buf, size_t n);
// U8 mono output, stereo sources (downmix to mono)           0x004513f0
void Mixer_FillBuf_U8_Mono_StereoMix(void* buf, size_t n);

#endif // MIXER_H
