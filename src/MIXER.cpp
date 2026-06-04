// MIXER.cpp — Software PCM mixer with DirectSound output
//
// Low-level audio mixing engine for Crux.  Manages one DirectSound
// primary or secondary buffer, up to 8 simultaneous PCM channels, volume,
// pan, and a background fill thread.
//
// Architecture overview:
//   - Mixer_Init creates the DirectSound device and buffer, spawns
//     Mixer_ThreadProc, and starts playback.
//   - Mixer_ThreadProc ("mx_handle_bufs") runs forever: waits on
//     g_hMixerReadyEvent, checks buffer status, Locks a chunk of the
//     DirectSound buffer, calls g_pfnMixerFillBuf to mix all active
//     channels into it, Unlocks, Plays.
//   - Channels are kept in a sorted active list (DAT_006dc098) ordered by
//     remaining-sample-count ascending so the mixer always processes the
//     soonest-to-finish channel last (and can call Mixer_ChannelDone on
//     it mid-fill).
//   - Volume per channel: 0..32. Channel 0 (music) is further scaled by
//     g_nMixerMusicVolPct (read from [Sound] MuseVol in CRUX.INI, default 100).
//   - Pan: mixing_mode==2 stores separate left/right scale factors computed
//     via atan-based formula; mode 0 = mono dup, mode 1 = stereo downmix.
//   - g_pfnMixerFillBuf is set by Mixer_SelectFillFunc to one of 8 kernels
//     depending on the output format (8/16-bit, mono/stereo) and the source
//     format of the active channels.
//   - The intermediate accumulation buffer (g_pMixerAccumBuf, 0x1000 bytes)
//     holds 16-bit working values when the output is 8-bit; the vol table
//     (g_pMixerVolTable, 0x800 bytes) maps those back to clamped U8.
//
// Buffer size management:
//   Primary buffer: size from IDirectSoundBuffer::GetCaps (DAT_006dc064).
//   Secondary buffer: fixed 0x4000 (16 KB).
//   The thread fills up to 0x800 bytes per Lock call to keep latency low.
//
// "mx_" debug names found in error strings confirm original source identifiers.
//
// Original source: C:\DevStudio\Projects\Crux\MIXER.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dsound.h>
#include <mmsystem.h>
#include <string.h>
#include "MIXER.h"

// ============================================================
//  Globals
// ============================================================

// DirectSound objects --------------------------------------------------------
IDirectSound*        g_pDSound       = NULL;   // 0x006dc094
IDirectSoundBuffer*  g_pDSBuffer     = NULL;   // 0x006dc090

// Mixer state ----------------------------------------------------------------
int  g_nMixerActiveChans   = 0;        // 0x006dc3d0  count of entries in active channel table
int  g_nMixerBufSize       = 0;        // 0x006dc064  DS buffer size in bytes (from GetCaps or 0x4000)
int  g_nMixerSampleRate    = 0;        // 0x006dc068  output sample rate (44100, 22050, ...)
int  g_nMixerOutChans      = 0;        // 0x006dc270  1=mono, 2=stereo
int  g_nMixerBitDepth      = 0;        // 0x006dc264  1=8-bit, 2=16-bit
int  g_nMixerBufMode       = 0;        // 0x006dc26c  1=primary buffer, 2=secondary buffer
int  g_nMixerFailed        = 1;        // 0x004d3b0c  1 until init succeeds
int  g_nMixerMusicVolPct   = 100;      // 0x004d3b04  [Sound] MuseVol percent (default 100)
int  g_nMixerSilenceByte   = 0x80;     // 0x006dc25c  0x80 for U8 output, 0x00 for S16
int  g_nMixerBytesPerTick  = 0;        // 0x006dc268  bytes output per fill tick

void (*g_pfnMixerFillBuf)(void*, size_t) = NULL;  // 0x006dc238  selected fill kernel

// Thread / event objects -----------------------------------------------------
HANDLE  g_hMixerThread     = NULL;     // 0x007c4ba4
DWORD   g_nMixerThreadId   = 0;        // 0x006dc06c
HANDLE  g_hMixerReadyEvent = NULL;     // 0x006dc404  thread signals "ready for next fill"
HANDLE  g_hMixerInitEvent  = NULL;     // 0x006dc400  main thread waits on this at startup
int     g_nMixerStopping   = 0;        // 0x006dc23c  1 during Kick/Reinit

// Volume table / accumulation buffer ----------------------------------------
void*   g_pMixerVolTable   = NULL;     // 0x006dc240  0x800-byte U8 clamp LUT
void*   g_pMixerAccumBuf   = NULL;     // 0x006dc260  0x1000-byte S16 accumulation buffer

// Fill cursor / timing -------------------------------------------------------
// (Internal globals used by Mixer_ThreadProc — not all exposed in header)
// DAT_006dc414  = fill write cursor in DS buffer
// DAT_006dc410  = previous hardware read cursor
// DAT_006dc408  = exit flag for thread (set by Mixer_Stop)
// DAT_006dc418  = current fill chunk size (adaptive)
// DAT_006dc41c  = last timeGetTime() for missed-buffer compensation
// DAT_006dc3f8  = count of looping channels
// DAT_006dc3b8  = CRITICAL_SECTION for channel table
// DAT_006dc420  = 1 when primary buffer setup failed (fall back to secondary)
// DAT_006dc424  = event name string "MixerReady"
// DAT_006dc428  = event name string "MixerInit"

// Timer (multimedia timer handle from THEMES timer wrapper) ------------------
int  g_nMixerTimerId = 0;              // 0x004d3b08  buffer-notification timer id

// ============================================================
//  Internal channel tables (reconstructed from raw DAT_ offsets)
// ============================================================
//
// Active-channel mixing table.  One entry per currently-playing voice,
// kept sorted ascending by remaining sample count.  Base 0x006dc098,
// stride 0x30 (48) bytes = 12 dwords.  Field offsets (relative to base):
//   +0x00 (0x098) chanId            logical channel id (0..7)
//   +0x04 (0x09c) pSrc              current source PCM read pointer
//   +0x08 (0x0a0) samplesLeft       remaining sample count (sort key)
//   +0x0c (0x0a4) vol               scaled mix volume
//   +0x10 (0x0a8) mode              0=mono-dup, 1=stereo-downmix, 2=pan
//   +0x14 (0x0ac) srcRate           source sample rate
//   +0x18 (0x0b0) srcBits           8 (0x08) or 16 (0x10)
//   +0x1c (0x0b4) (reserved)
//   +0x18 (0x0b8) panPos            pan position (0..100)  [overlaps; see DAT_006dc0b8]
//   +0x24 (0x0bc) volL              left pan scale
//   +0x28 (0x0c0) volR              right pan scale
//   +0x2c (0x0c4) srcRateDiv        rate divisor for sample-count decrement
struct MixerChannel {
    int      chanId;        // 0x098
    unsigned char* pSrc;    // 0x09c
    int      samplesLeft;   // 0x0a0
    int      vol;           // 0x0a4
    int      mode;          // 0x0a8
    int      srcRate;       // 0x0ac
    int      srcBits;       // 0x0b0
    int      pad_b4;        // 0x0b4
    int      panPos;        // 0x0b8
    int      volL;          // 0x0bc
    int      volR;          // 0x0c0
    int      srcRateDiv;    // 0x0c4
};

extern MixerChannel g_MixerChans[8];          // 0x006dc098  active (sorted) list

// Per-logical-channel state table.  Base 0x006dc278, stride 0x28 (40 bytes).
//   +0x00 (0x278) pcmStart          PCM buffer start pointer
//   +0x04 (0x27c) pcmEnd            PCM buffer end pointer
//   +0x08 (0x280) vol               current volume (0..32)
//   +0x0c (0x284) volBackup         saved volume for restore after duck
//   +0x10 (0x288) flags             bit0=paused, bit2=looping
//   +0x14 (0x28c) noPanFlag         1 => pan forced off
//   +0x24 (0x29c) panPos            requested pan position
struct MixerChannelState {
    void*    pcmStart;      // 0x278
    void*    pcmEnd;        // 0x27c
    int      vol;           // 0x280
    int      volBackup;     // 0x284
    unsigned flags;         // 0x288
    int      noPanFlag;     // 0x28c
    int      pad_290[3];    // 0x290..0x298
    int      panPos;        // 0x29c
};

extern MixerChannelState g_MixerState[8];     // 0x006dc278
extern void (*g_MixerDoneCb[8])(int);         // 0x006dc070  per-channel done callbacks
extern int  g_nMixerActiveChanIds;            // 0x006dc3d8  active-flag array (per id)
extern int  g_nMixerLoopCount;                // 0x006dc3f8  number of looping channels
extern CRITICAL_SECTION g_MixerChanCS;        // 0x006dc3b8

// Internal fill-thread cursors/timers
extern int   g_nMixerFillCursor;              // 0x006dc414
extern int   g_nMixerPrevWriteCur;            // 0x006dc410
extern int   g_nMixerPrevPlayCur;             // 0x006dc40c
extern int   g_nMixerExitFlag;                // 0x006dc408
extern int   g_nMixerFillChunk;               // 0x006dc418
extern DWORD g_nMixerLastTick;                // 0x006dc41c
extern int   g_nMixerSecBufFailed;            // 0x006dc420
extern int   g_nMixerLastPlayPos;             // 0x006dc060
extern char  g_szMixerReadyEvtName[];         // 0x006dc424  "MixerReady"
extern char  g_szMixerInitEvtName[];          // 0x006dc428  "MixerInit"
extern char  g_szMixerWaveName[];             // 0x006dc42c

// DirectSound format descriptor scratch (WAVEFORMATEX + DSBUFFERDESC area at 0x006dc248)
extern unsigned char g_MixerDSBufDesc[];      // 0x006dc248

// Volume/MuseVol from CRUX.INI
extern int  DAT_004d3b04;                     // [Sound] MuseVol percent (== g_nMixerMusicVolPct)
extern int  DAT_004c4c40;                     // buffer-tick divisor (timer rate)

// Cross-module helpers (declared elsewhere) ---------------------------------
extern "C" {
    HWND  g_nHwndMain;                                   // main window handle
    void  Win_BringToFront(HWND hwnd);
    void  Debug_Assert(int line, const char* file, int code);
    void  Debug_Trace(int line, const char* file, const char* fmt, ...);
    void  thunk_FUN_00420e60(int line, const char* file);
    void* Err_SetRecord3(int code, const char* msg, int arg);
    void  FUN_00489090(void* rec, void* tag);
    void* SafeHeap_Alloc(int line, const char* file, size_t size);
    int   Theme_SetTimer(void* proc, int ms);
    double FUN_0048b1c4(double x);                        // atan-style helper
    long  __ftol(void);                                  // CRT double->long
}

// The remaining three fill kernels referenced by Mixer_SelectFillFunc are
// implemented elsewhere in the build (44.1 kHz S16 variants and the U8/44k
// stereo path); forward-declare them so the selector can take their address.
void Mixer_FillBuf_S16_Stereo_StereoA(short* buf, size_t n);
void Mixer_FillBuf_S16_Stereo_Mono44k(short* buf, size_t n);
void Mixer_FillBuf_U8_Stereo_StereoB44k(void* buf, size_t n);

// Forward declarations for kernels/helpers used before their definition.
void Mixer_FillBuf_U8_Stereo_Mono(void* buf, size_t n);
void Mixer_FillBuf_U8_Mono_Mono(void* buf, size_t n);
void Mixer_FillBuf_S16_Stereo_Stereo(short* buf, size_t n);
void Mixer_FillBuf_S16_Stereo_StereoB(short* buf, size_t n);
void Mixer_FillBuf_S16_Stereo_Mono(short* buf, size_t n);
void Mixer_FillBuf_U8_Stereo_StereoA(void* buf, size_t n);
void Mixer_FillBuf_U8_Stereo_StereoB(void* buf, size_t n);
void Mixer_FillBuf_U8_Mono_StereoMix(void* buf, size_t n);
void Mixer_InitTables(void);
const char* Mixer_DsErrToStr(int hr);
void Mixer_ChannelDone(int channelId);
void Mixer_RestoreVolume(int channelId);
void Mixer_SkipBytes(int byteCount);
void Mixer_UpdateTimers(void);
void Mixer_SelectFillFunc(int srcChannels, int panEnabled);

// ============================================================
//  Mixer_InitTables  (0x00440ee0)
// ============================================================
// Named "mx_init_tables" (not in debug strings, inferred).
// Reads MuseVol from CRUX.INI [Sound], allocates the 0x800-byte volume
// clamping LUT (g_pMixerVolTable) and fills it:
//   For index 0..0x3FF: table[0x400+i] = clamp(i*0x7F>>8, 0x7F) - 0x80
//                        table[0x400-i] = -0x80 - above
// This maps a signed 10-bit mixed value back to a clamped signed-8 sample.
void Mixer_InitTables(void)
{
    extern char* g_szIniPath;   // CRUX.INI path

    DAT_004d3b04 = GetPrivateProfileIntA("Sound", "MuseVol", 100, g_szIniPath);

    char* table = (char*)SafeHeap_Alloc(0, "MIXER.cpp", 0x800);
    g_pMixerVolTable = table;

    for (int i = 0; i < 0x400; i++) {
        int v = (i * 0x7F + ((i * 0x7F) >> 31 & 0xFF)) >> 8;
        if (v > 0x7F) v = 0x7F;
        table[0x400 + i] = (char)((char)v - 0x80);
        table[0x400 - i] = (char)(-0x80 - (char)v);
    }
}

// ============================================================
//  Mixer_Init  (0x00441690)
// ============================================================
// Named "mx_init_int_rate_int_fst_int_f16_..." in debug trace.
// Full mixer initialisation:
//   1. Save sample rate (g_nMixerSampleRate = param_1)
//   2. Init all 8 channel volumes to 0x1F (31)
//   3. InitializeCriticalSection for channel table
//   4. Mixer_InitTables (volume LUT + MuseVol)
//   5. DirectSoundCreate(NULL, &g_pDSound, NULL)
//   6. IDirectSound::GetCaps → determine stereo/mono, 8/16-bit capability
//   7. IDirectSound::SetCooperativeLevel(hwnd, DSSCL_WRITEPRIMARY)
//       → on success: create primary buffer (DSBCAPS_PRIMARYBUFFER),
//         SetFormat, GetCaps to get buffer size
//       → on failure: fall back to DSSCL_NORMAL + secondary buffer (16-bit stereo 16KB)
//   8. Compute g_nMixerBytesPerTick, g_nMixerSilenceByte
//   9. Mixer_SelectFillFunc(g_nMixerBitDepth, param_4)
//  10. Create g_hMixerReadyEvent, g_hMixerInitEvent
//  11. CreateThread → Mixer_ThreadProc
//  12. IDirectSoundBuffer::Lock (full buffer), memset silence, Unlock
//  13. IDirectSoundBuffer::Play(0, 0, DSBPLAY_LOOPING)
//  14. WaitForSingleObject(g_hMixerInitEvent, 10000)  ← thread signals it's running
//  15. if mono output: SafeHeap_Alloc 0x1000 bytes for g_pMixerAccumBuf
void Mixer_Init(int sampleRate, int allowStereo, int allow16bit, int allowPan)
{
    DSCAPS         dsCaps;
    DSBUFFERDESC   bufDesc;
    WAVEFORMATEX   fmt;
    DSBCAPS        bufCaps;
    HRESULT        hr;

    g_nMixerSampleRate = sampleRate;

    // Default all 8 logical channel volumes to 0x1F (31).
    for (int ch = 0; ch < 8; ch++)
        g_MixerState[ch].vol = 0x1f;

    InitializeCriticalSection(&g_MixerChanCS);
    Mixer_InitTables();
    Win_BringToFront(g_nHwndMain);

    hr = DirectSoundCreate(0, &g_pDSound, 0);
    if (hr != 0) {
        // DSERR_ALLOCATED / DSERR_NODRIVER get specific error records.
        if (hr == -0x7787fff6) {                // DSERR_ALLOCATED
            thunk_FUN_00420e60(0, "MIXER.cpp");
            void* rec = Err_SetRecord3(0x26, Mixer_DsErrToStr(hr), 2);
            FUN_00489090(rec, (void*)0x004ab3f8);
        } else if (hr == -0x7787ff88) {         // DSERR_NODRIVER
            thunk_FUN_00420e60(0, "MIXER.cpp");
            void* rec = Err_SetRecord3(0x26, Mixer_DsErrToStr(hr), 1);
            FUN_00489090(rec, (void*)0x004ab3f8);
        }
        g_nMixerFailed = 1;
    } else {
        // Query device caps.
        dsCaps.dwSize = sizeof(DSCAPS);
        hr = g_pDSound->GetCaps(&dsCaps);
        if (hr == 0) {
            if (dsCaps.dwFlags & DSCAPS_PRIMARYSTEREO) {     // (bit 2)
                g_nMixerBitDepth = 2;
                if (allowStereo == 0) g_nMixerBitDepth = 1;
            } else if (dsCaps.dwFlags & DSCAPS_PRIMARYMONO) {// (bit 1)
                g_nMixerBitDepth = 1;
            }
        } else {
            Debug_Assert(0, "MIXER.cpp", hr);
            Debug_Trace(0, "MIXER.cpp", "Error getting DSound caps");
            g_nMixerFailed = 1;
        }

        if (hr == 0) {
            // Try exclusive write-primary cooperative level first.
            hr = g_pDSound->SetCooperativeLevel(g_nHwndMain, DSSCL_WRITEPRIMARY);
            if (hr == 0) {
                Debug_Trace(0, "MIXER.cpp", "Initing mixer on primary buffer");
                g_nMixerBufMode = 1;

                memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
                bufDesc.dwSize        = sizeof(DSBUFFERDESC);
                bufDesc.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME; // 0x81
                bufDesc.dwBufferBytes = 0;
                bufDesc.lpwfxFormat   = NULL;

                hr = g_pDSound->CreateSoundBuffer(&bufDesc, &g_pDSBuffer, 0);
                if (hr == 0) {
                    fmt.wFormatTag = WAVE_FORMAT_PCM;
                    fmt.nChannels  = (WORD)g_nMixerBitDepth;
                    fmt.nSamplesPerSec = sampleRate;

                    if (dsCaps.dwFlags & 8) {            // 16-bit capable
                        g_nMixerOutChans = 2;
                        if (allow16bit == 0) g_nMixerOutChans = 1;
                    } else if (dsCaps.dwFlags & 4) {     // 8-bit only
                        g_nMixerOutChans = 1;
                    } else {
                        thunk_FUN_00420e60(0, "MIXER.cpp");
                        void* rec = Err_SetRecord3(0x25, "Invalid format", 0xffffffff);
                        FUN_00489090(rec, (void*)0x004ab3f8);
                    }

                    g_nMixerBytesPerTick = (3 - g_nMixerBitDepth)
                                         * ((g_nMixerSampleRate == 0x5622) + 1)
                                         * ((g_nMixerOutChans == 1) + 1);
                    fmt.nAvgBytesPerSec = sampleRate * g_nMixerOutChans * g_nMixerBitDepth;
                    fmt.nBlockAlign     = (WORD)(g_nMixerOutChans * g_nMixerBitDepth);
                    fmt.wBitsPerSample  = (WORD)(g_nMixerOutChans << 3);
                    fmt.cbSize          = 0;

                    hr = g_pDSBuffer->SetFormat(&fmt);
                    if (hr == 0) {
                        bufCaps.dwSize = sizeof(DSBCAPS);
                        g_pDSBuffer->GetCaps(&bufCaps);
                        g_nMixerBufSize = bufCaps.dwBufferBytes;
                        g_nMixerFailed  = 0;
                    } else {
                        Debug_Trace(0, "MIXER.cpp", "Error setting primary buffer format");
                        g_nMixerSecBufFailed = 1;
                        g_pDSBuffer->Release();
                    }
                } else {
                    Debug_Assert(0, "MIXER.cpp", hr);
                    Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                    Debug_Trace(0, "MIXER.cpp", "Error creating primary buffer");
                    g_nMixerSecBufFailed = 1;
                }
            } else {
                Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                Debug_Trace(0, "MIXER.cpp", "Error setting WRITEPRIMARY coop level");
                g_nMixerSecBufFailed = 1;
            }

            // Fall back to a secondary buffer at normal cooperative level.
            if (g_nMixerSecBufFailed != 0) {
                Win_BringToFront(g_nHwndMain);
                hr = g_pDSound->SetCooperativeLevel(g_nHwndMain, DSSCL_NORMAL);
                if (hr == 0) {
                    Debug_Trace(0, "MIXER.cpp", "Initing mixer on secondary buffer");
                    g_nMixerBufMode = 2;

                    memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
                    bufDesc.dwSize        = sizeof(DSBUFFERDESC);
                    bufDesc.dwFlags       = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2; // 0xe0
                    g_nMixerBufSize       = 0x4000;
                    bufDesc.dwBufferBytes = 0x4000;

                    fmt.wFormatTag     = WAVE_FORMAT_PCM;
                    fmt.nChannels      = (WORD)g_nMixerBitDepth;
                    fmt.nSamplesPerSec = sampleRate;
                    g_nMixerOutChans   = 2;
                    g_nMixerBytesPerTick = (3 - g_nMixerBitDepth) * ((g_nMixerSampleRate == 0x5622) + 1);
                    fmt.nAvgBytesPerSec = sampleRate * 2 * g_nMixerBitDepth;
                    fmt.nBlockAlign     = (WORD)(g_nMixerBitDepth * 2);
                    fmt.wBitsPerSample  = 0x10;
                    bufDesc.lpwfxFormat = &fmt;

                    hr = g_pDSound->CreateSoundBuffer(&bufDesc, &g_pDSBuffer, 0);
                    if (hr != 0) {
                        Debug_Assert(0, "MIXER.cpp", hr);
                        Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                        Debug_Trace(0, "MIXER.cpp", "Error creating secondary buffer");
                    }
                    g_nMixerFailed = (hr != 0);
                } else {
                    Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                    g_nMixerFailed = 1;
                    Debug_Trace(0, "MIXER.cpp", "Error setting normal priority");
                }
            }
        }
    }

    if (g_nMixerFailed != 0) {
        thunk_FUN_00420e60(0, "MIXER.cpp");
        void* rec = Err_SetRecord3(0x26, Mixer_DsErrToStr(hr), 0xffffffff);
        FUN_00489090(rec, (void*)0x004ab3f8);
    }

    Mixer_SelectFillFunc(g_nMixerBitDepth, allowPan);

    g_hMixerReadyEvent = CreateEventA(NULL, 0, 0, g_szMixerReadyEvtName);
    g_hMixerInitEvent  = CreateEventA(NULL, 0, 0, g_szMixerInitEvtName);
    g_hMixerThread     = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Mixer_ThreadProc,
                                      NULL, 0, &g_nMixerThreadId);
    Debug_Trace(0, "MIXER.cpp", "Mixer thread ID is %x", g_nMixerThreadId);

    if (g_hMixerThread == NULL) {
        Debug_Trace(0, "MIXER.cpp", "Error creating mixer thread");
        g_nMixerFailed = 1;
    } else {
        // Prime the whole buffer with silence, then start looping playback.
        void  *p1, *p2;
        DWORD  s1, s2;
        hr = g_pDSBuffer->Lock(0, g_nMixerBufSize, &p1, &s1, &p2, &s2, 0);
        if (hr == 0) {
            if      (g_nMixerOutChans == 2) g_nMixerSilenceByte = 0;
            else if (g_nMixerOutChans == 1) g_nMixerSilenceByte = 0x80;
            memset(p1, g_nMixerSilenceByte, s1);
            if (p2 != NULL)
                memset(p2, g_nMixerSilenceByte, s2);
            g_pDSBuffer->Unlock(p1, s1, p2, s2);

            g_nMixerTimerId = Theme_SetTimer((void*)Mixer_ThreadProc, DAT_004c4c40 << 1);

            hr = g_pDSBuffer->Play(0, 0, DSBPLAY_LOOPING);
            if (hr != 0) {
                thunk_FUN_00420e60(0, "MIXER.cpp");
                void* rec = Err_SetRecord3(0x25, Mixer_DsErrToStr(hr), 0xffffffff);
                FUN_00489090(rec, (void*)0x004ab3f8);
            }

            // Wait for the fill thread to signal it is running.
            DWORD wr = WaitForSingleObject(g_hMixerReadyEvent, 10000);
            if (wr == WAIT_TIMEOUT) {
                thunk_FUN_00420e60(0, "MIXER.cpp");
                void* rec = Err_SetRecord3(0x18, g_szMixerWaveName, 0xffffffff);
                FUN_00489090(rec, (void*)0x004ab3f8);
            }
        }

        // Mono output needs the 0x1000-byte S16 accumulation scratch.
        if (g_nMixerOutChans == 1)
            g_pMixerAccumBuf = SafeHeap_Alloc(0, "MIXER.cpp", 0x1000);
    }
}

// ============================================================
//  Mixer_Reinit  (0x004410d0)
// ============================================================
// Named "mx_reinit" in debug trace (s_mx_reinit___004d403c).
// Called after device loss to recreate the DirectSound buffer without
// re-reading INI.  Re-uses existing g_nMixerSampleRate, g_nMixerOutChans,
// g_nMixerBitDepth.  Tries primary buffer first; falls back to secondary.
// On failure, sets g_nMixerFailed=1.
void Mixer_Reinit(void)
{
    DSBUFFERDESC bufDesc;
    WAVEFORMATEX fmt;
    DSBCAPS      bufCaps;
    HRESULT      hr;

    int savedRate = g_nMixerSampleRate;

    // Pre-build the primary-path format from the existing settings.
    fmt.wFormatTag     = WAVE_FORMAT_PCM;
    fmt.nChannels      = (WORD)g_nMixerBitDepth;
    fmt.nSamplesPerSec = g_nMixerSampleRate;
    fmt.nAvgBytesPerSec = g_nMixerSampleRate * g_nMixerOutChans * g_nMixerBitDepth;
    fmt.nBlockAlign    = (WORD)(g_nMixerOutChans * g_nMixerBitDepth);
    fmt.wBitsPerSample = (WORD)(g_nMixerOutChans << 3);

    Win_BringToFront(g_nHwndMain);

    hr = DirectSoundCreate(0, &g_pDSound, 0);
    if (hr != 0) {
        g_nMixerFailed = 1;
        thunk_FUN_00420e60(0, "MIXER.cpp");
        void* rec = Err_SetRecord3(0x25, Mixer_DsErrToStr(hr), 0xffffffff);
        FUN_00489090(rec, (void*)0x004ab3f8);
        return;
    }

    int coopErr = 0;
    if (g_nMixerSecBufFailed == 0 &&
        (hr = g_pDSound->SetCooperativeLevel(g_nHwndMain, DSSCL_WRITEPRIMARY),
         coopErr = hr, hr == 0)) {
        // Primary buffer path.
        memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
        bufDesc.dwSize        = sizeof(DSBUFFERDESC);
        bufDesc.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME; // 0x81
        bufDesc.dwBufferBytes = 0;
        bufDesc.lpwfxFormat   = NULL;

        hr = g_pDSound->CreateSoundBuffer(&bufDesc, &g_pDSBuffer, 0);
        if (hr == 0) {
            hr = g_pDSBuffer->SetFormat(&fmt);
            if (hr == 0) {
                bufCaps.dwSize = sizeof(DSBCAPS);
                g_pDSBuffer->GetCaps(&bufCaps);
                g_nMixerBufSize = bufCaps.dwBufferBytes;
                g_nMixerFailed  = 0;
            } else {
                Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                Debug_Trace(0, "MIXER.cpp", "Error setting primary buffer format");
                g_nMixerSecBufFailed = 1;
                g_pDSBuffer->Release();
            }
        } else {
            Debug_Trace(0, "MIXER.cpp", "Error creating primary buffer");
            g_nMixerSecBufFailed = 1;
        }
    } else {
        hr = coopErr;
        Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
        Debug_Trace(0, "MIXER.cpp", "Error setting WRITEPRIMARY coop level");
        g_nMixerSecBufFailed = 1;
    }

    if (g_nMixerSecBufFailed != 0) {
        Win_BringToFront(g_nHwndMain);
        hr = g_pDSound->SetCooperativeLevel(g_nHwndMain, DSSCL_NORMAL);
        if (hr == 0) {
            memset(&bufDesc, 0, sizeof(DSBUFFERDESC));
            bufDesc.dwSize        = sizeof(DSBUFFERDESC);
            bufDesc.dwFlags       = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2; // 0xe0
            g_nMixerBufSize       = 0x4000;
            bufDesc.dwBufferBytes = 0x4000;

            fmt.wFormatTag     = WAVE_FORMAT_PCM;
            fmt.nChannels      = (WORD)g_nMixerBitDepth;
            fmt.nSamplesPerSec = savedRate;
            fmt.nAvgBytesPerSec = savedRate * g_nMixerOutChans * g_nMixerBitDepth;
            fmt.nBlockAlign    = (WORD)(g_nMixerOutChans * g_nMixerBitDepth);
            fmt.wBitsPerSample = (WORD)(g_nMixerOutChans << 3);
            bufDesc.lpwfxFormat = &fmt;

            hr = g_pDSound->CreateSoundBuffer(&bufDesc, &g_pDSBuffer, 0);
            if (hr != 0) {
                Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                Debug_Trace(0, "MIXER.cpp", "Error creating secondary buffer");
            }
            g_nMixerFailed = (hr != 0);
        } else {
            Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
            g_nMixerFailed = 1;
            Debug_Trace(0, "MIXER.cpp", "Error setting normal priority");
        }
    }
}

// ============================================================
//  Mixer_Kick  (0x00440da0)
// ============================================================
// Named "mx_kick" in debug trace (s_mx_kick___004d3fa0).
// Recovery entry point called when the buffer is detected as stalled.
// Sets g_nMixerStopping=1, Sleep(10), releases IDirectSound
// (IDirectSound::Release via vtable +8), calls Mixer_Reinit,
// clears g_nMixerStopping.
void Mixer_Kick(void)
{
    if (g_nMixerFailed != 0)
        return;
    g_nMixerStopping = 1;
    Sleep(10);
    g_pDSound->Release();          // IDirectSound::Release (vtable +8)
    Mixer_Reinit();
    g_nMixerStopping = 0;
}

// ============================================================
//  Mixer_Stop  (0x00442430)
// ============================================================
// Hard shutdown.  Only executes once (guarded by DAT_004d3b0c flag check):
//   1. IDirectSoundBuffer::Stop (vtable +0x48)
//   2. Theme_KillTimer(g_nMixerTimerId) — kills the buffer-tick timer
//   3. CloseHandle(g_hMixerInitEvent), CloseHandle(g_hMixerReadyEvent)
//   4. If g_hMixerThread != NULL and caller != mixer thread:
//       TerminateThread / CloseHandle / set NULL
//   5. waveOutReset / waveOutClose on DAT_006dc3fc (HWAVEOUT, if open)
// After this the DirectSound buffer and thread are gone.
void Mixer_Stop(void)
{
    extern void Theme_KillTimer(int id);
    extern HWAVEOUT g_hMixerWaveOut;    // 0x006dc3fc

    if (g_nMixerFailed != 0)            // guard: only stop once (DAT_004d3b0c)
        return;
    g_nMixerFailed = 1;

    g_pDSBuffer->Stop();               // IDirectSoundBuffer::Stop (vtable +0x48)
    Theme_KillTimer(g_nMixerTimerId);

    CloseHandle(g_hMixerInitEvent);   g_hMixerInitEvent  = NULL;
    CloseHandle(g_hMixerReadyEvent);  g_hMixerReadyEvent = NULL;

    DWORD tid = GetCurrentThreadId();
    if (g_hMixerThread != NULL && tid != g_nMixerThreadId) {
        TerminateThread(g_hMixerThread, 0);
        CloseHandle(g_hMixerThread);
        g_hMixerThread = NULL;
    }

    if (g_hMixerWaveOut != NULL) {
        waveOutReset(g_hMixerWaveOut);
        waveOutClose(g_hMixerWaveOut);
        g_hMixerWaveOut = NULL;
    }
}

// ============================================================
//  Mixer_ThreadProc  (0x004400e0)
// ============================================================
// Named "mx_handle_bufs" in debug trace (s_mx_handle_bufs_HWAVE_notInUse__004d3e08).
// Background DirectSound fill thread.  Runs until g_nMixerStopping != 0
// or g_nMixerFailed != 0.
//
// Loop:
//   1. WaitForSingleObject(g_hMixerReadyEvent, INFINITE)
//   2. If g_nMixerStopping: skip to SetEvent, return
//   3. IDirectSoundBuffer::GetStatus → if DSBSTATUS_BUFFERLOST:
//        IDirectSoundBuffer::Restore, re-Play
//   4. IDirectSoundBuffer::GetCurrentPosition(&playPos, &writePos)
//   5. Compute fill window: from DAT_006dc414 (last fill end) to
//      writePos + MIXER_PREFILL_AHEAD (200ms worth of bytes), clamped.
//   6. Missed buffer detection: compare DAT_006dc410 (prev write cursor) to
//      current; if gap > expected call Mixer_SkipBytes(gap) and log "missed %d".
//   7. Adaptive fill chunk: max of (last-chunk*3/4) and (bytes-since-last-tick*2).
//   8. While bytes remain to fill (up to 0x800 per Lock):
//        a. IDirectSoundBuffer::Lock(offset, size, &ptr1, &sz1, &ptr2, &sz2, 0)
//        b. Mixer_UpdateTimers()
//        c. For each locked segment (ptr1/sz1, ptr2/sz2):
//             - While last channel's remaining*stride < segment bytes:
//                 fill up to that channel's end, Mixer_ChannelDone(chan)
//             - g_pfnMixerFillBuf(ptr, remaining)
//        d. IDirectSoundBuffer::Unlock(ptr1, sz1, ptr2, sz2)
//        e. Advance DAT_006dc414 by filled size (mod buffer size)
//   9. SetEvent(g_hMixerReadyEvent) — signal thread is done filling
//  10. if g_nMixerStopping: return 0
DWORD WINAPI Mixer_ThreadProc(void)
{
    unsigned char silenceBuf[2048];     // fallback scratch when Lock fails
    DWORD  prevTick = 0;
    int    stoppedOnce = 0;
    unsigned chunkSize = 0;             // bytes to fill this round (local_44)
    unsigned missed = 0;                // local_2c

    for (;;) {
        WaitForSingleObject(g_hMixerInitEvent, INFINITE);

        missed = 0;
        if (g_nMixerStopping == 0) {
            DWORD status = 0;
            HRESULT hr = g_pDSBuffer->GetStatus(&status);
            if (hr != 0)
                Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));

            // Buffer lost: restore and restart.
            if (status & DSBSTATUS_BUFFERLOST) {
                if (g_pDSBuffer->Restore() != 0)
                    goto do_fill;
                hr = g_pDSBuffer->Play(0, 0, DSBPLAY_LOOPING);
                if (hr != 0)
                    Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
                hr = g_pDSBuffer->GetStatus(&status);
                if (hr != 0)
                    Debug_Trace(0, "MIXER.cpp", Mixer_DsErrToStr(hr));
            }

            // Not playing? log, restart, mark stopped.
            if ((status & DSBSTATUS_PLAYING) == 0) {
                Debug_Trace(0, "MIXER.cpp", "stopped playing at %d", timeGetTime());
                g_pDSBuffer->Play(0, 0, DSBPLAY_LOOPING);
                stoppedOnce = 1;
            }

            DWORD playPos = 0, writePos = 0;
            g_pDSBuffer->GetCurrentPosition(&playPos, &writePos);

            if (stoppedOnce != 0) {
                g_nMixerPrevWriteCur =
                    (((g_nMixerBufSize + (int)writePos) - g_nMixerLastPlayPos)
                     - g_nMixerBitDepth * 200 * g_nMixerOutChans) % g_nMixerBufSize;
                g_nMixerPrevPlayCur =
                    ((g_nMixerBufSize + (int)playPos) - g_nMixerLastPlayPos) % g_nMixerBufSize;
            }

            // Aim 200ms ahead of the hardware write cursor (8-byte aligned).
            writePos = ((writePos + g_nMixerBitDepth * 200 * g_nMixerOutChans) % g_nMixerBufSize)
                       & 0xfffffff8;

            int played = (int)(((playPos + g_nMixerBufSize) - g_nMixerPrevPlayCur) % g_nMixerBufSize);

            int compensate = 0;
            if (g_nMixerLastTick == 0) {
                compensate = 0;
            } else {
                prevTick = g_nMixerLastTick;
                g_nMixerLastTick = timeGetTime();
                compensate = (int)((g_nMixerLastTick - prevTick)
                                   * g_nMixerSampleRate * g_nMixerOutChans * g_nMixerBitDepth) / 1000;
                played += (compensate / g_nMixerBufSize) * g_nMixerBufSize;
                Debug_Trace(0, "MIXER.cpp", "%d %d %d", compensate, g_nMixerLastTick, prevTick);
            }
            g_nMixerLastPlayPos = played;

            // Missed-buffer detection between previous and current write cursor.
            if ((((g_nMixerFillCursor < g_nMixerPrevWriteCur) && ((int)writePos < g_nMixerPrevWriteCur)) ||
                 ((g_nMixerPrevWriteCur < g_nMixerFillCursor) && (g_nMixerPrevWriteCur < (int)writePos))) &&
                (g_nMixerFillCursor < (int)writePos)) {
                missed = (int)writePos - g_nMixerFillCursor;
            } else if (((int)writePos < g_nMixerPrevWriteCur) && (g_nMixerPrevWriteCur < g_nMixerFillCursor)) {
                missed = ((int)writePos + g_nMixerBufSize) - g_nMixerFillCursor;
            }
            if (compensate != 0)
                missed = compensate;
            if (missed != 0) {
                Debug_Trace(0, "MIXER.cpp", "missed %d", missed);
                Mixer_SkipBytes((int)missed);
                g_nMixerFillCursor = (int)writePos;
            }
            g_nMixerPrevWriteCur = (int)writePos;
            g_nMixerPrevPlayCur  = (int)playPos;

            // Adaptive fill chunk: max(prev*3/4, played*2), clamped to one tick min,
            // buffer size max.
            int lo = played * 2 * g_nMixerSilenceByte;
            int oneTick = (g_nMixerSampleRate * g_nMixerBitDepth * g_nMixerOutChans) / DAT_004c4c40;
            unsigned want = (lo <= oneTick) ? (unsigned)oneTick : (unsigned)lo;
            unsigned three4 = (unsigned)(((g_nMixerFillChunk * 3) + (((g_nMixerFillChunk * 3) >> 31) & 3)) >> 2);
            unsigned target = (int)three4 < (int)want ? want : three4;
            unsigned clamped = ((int)target < g_nMixerBufSize) ? target : (unsigned)g_nMixerBufSize;
            chunkSize = clamped;
            g_nMixerFillChunk = (int)clamped;
        }

    do_fill:
        if ((((g_nMixerFillCursor - (int)g_nMixerPrevWriteCur) <= (int)chunkSize) &&
             (((int)g_nMixerPrevWriteCur < g_nMixerFillCursor) ||
              ((g_nMixerFillCursor + g_nMixerBufSize) <= (int)(g_nMixerPrevWriteCur + chunkSize)))) ||
            (missed != 0)) {

            // Total bytes to write this round (8-byte aligned).
            unsigned remaining =
                (((g_nMixerPrevWriteCur + chunkSize) - g_nMixerFillCursor) + g_nMixerBufSize)
                % g_nMixerBufSize & 0xfffffff8;

            while ((int)remaining > 0) {
                unsigned block = (remaining < 0x800) ? remaining : 0x800;
                remaining -= block;

                void  *p1 = NULL, *p2 = NULL;
                DWORD  s1 = 0, s2 = 0;
                HRESULT hr = g_pDSBuffer->Lock(g_nMixerFillCursor, block,
                                               &p1, &s1, &p2, &s2, 0);
                if (hr != 0) {
                    Debug_Assert(0, "MIXER.cpp", hr);
                    p1 = silenceBuf;
                    s1 = block;
                    p2 = NULL;
                    s2 = 0;
                }

                for (int seg = 0; seg < 2; seg++) {
                    unsigned char* dst;
                    unsigned       segBytes;
                    if (seg == 0) { dst = (unsigned char*)p1; segBytes = s1; }
                    else          { dst = (unsigned char*)p2; segBytes = s2; }

                    Mixer_UpdateTimers();

                    // If the soonest-to-finish channel ends within this segment,
                    // fill up to its end, retire it, then continue.
                    while (g_nMixerActiveChans != 0 &&
                           (g_MixerChans[g_nMixerActiveChans - 1].samplesLeft *
                            g_MixerChans[g_nMixerActiveChans - 1].srcRateDiv
                            < (int)(segBytes * g_nMixerBytesPerTick))) {
                        int part = (g_MixerChans[g_nMixerActiveChans - 1].samplesLeft *
                                    g_MixerChans[g_nMixerActiveChans - 1].srcRateDiv)
                                   / g_nMixerBytesPerTick;
                        if (part < 0)
                            Debug_Trace(0, "MIXER.cpp", "", timeGetTime());
                        g_pfnMixerFillBuf(dst, part);
                        dst      += part;
                        segBytes -= part;
                        Mixer_ChannelDone(g_MixerChans[g_nMixerActiveChans - 1].chanId);
                    }
                    if ((int)segBytes > 0)
                        g_pfnMixerFillBuf(dst, segBytes);
                }

                if (p1 != silenceBuf)
                    g_pDSBuffer->Unlock(p1, s1, p2, s2);

                g_nMixerFillCursor = (g_nMixerFillCursor + block) % g_nMixerBufSize;
            }

            SetEvent(g_hMixerReadyEvent);
            if (g_nMixerExitFlag != 0)
                return 0;
        }
    }
}

// ============================================================
//  Mixer_SkipBytes  (0x00440b60)
// ============================================================
// Advances each active channel's read pointer by param_1 output bytes
// (scaled by the channel's source rate vs. output rate, via DAT_006dc268).
// Also decrements each channel's remaining-sample counter.  If a channel
// reaches zero, calls Mixer_ChannelDone on it.
void Mixer_SkipBytes(int byteCount)
{
    for (int i = g_nMixerActiveChans - 1; i >= 0; i--) {
        int adv = (byteCount * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
        g_MixerChans[i].pSrc        += adv;
        g_MixerChans[i].samplesLeft -= adv;
    }
    for (int i = 0; i < g_nMixerActiveChans; i++) {
        if (g_MixerChans[i].samplesLeft < 0)
            Mixer_ChannelDone(g_MixerChans[i].chanId);
    }
}

// ============================================================
//  Mixer_UpdateTimers  (0x00440cf0)
// ============================================================
// Called once per fill-thread iteration.  Reads timeGetTime() for
// timing bookkeeping and iterates the active channel list (body is
// effectively empty in this build — placeholder for rate adaptation).
void Mixer_UpdateTimers(void)
{
    timeGetTime();
    for (int i = 0; i < g_nMixerActiveChans; i++) {
        // (rate-adaptation placeholder — empty body in this build)
    }
}

// ============================================================
//  Mixer_ChannelDone  (0x004425a0)
// ============================================================
// Called when a channel has exhausted all its samples.
//   1. EnterCriticalSection(channel table CS)
//   2. Clear start_ptr and end_ptr for this channel
//   3. Remove channel from the sorted active list (shift entries down)
//   4. If channel was looping (bit 2 of flags set):
//        decrement g_nMixerLoopCount (DAT_006dc3f8)
//        call Mixer_RestoreVolume on all other channels
//   5. Clear DAT_006dc3d8[channelId] (active flag array)
//   6. If done_callback is set: fire it(channelId), clear it
//   7. LeaveCriticalSection
void Mixer_ChannelDone(int channelId)
{
    if (g_MixerState[channelId].pcmStart == 0)
        return;

    EnterCriticalSection(&g_MixerChanCS);

    // Clear this channel's PCM pointers.
    g_MixerState[channelId].pcmStart = 0;
    g_MixerState[channelId].pcmEnd   = 0;

    // Find it in the sorted active list.
    int i = 0;
    while (g_MixerChans[i].chanId != channelId && i < g_nMixerActiveChans)
        i++;

    if (i < g_nMixerActiveChans) {
        g_nMixerActiveChans--;
        // Shift the tail of the active list down over the removed entry.
        for (; i < g_nMixerActiveChans; i++)
            g_MixerChans[i] = g_MixerChans[i + 1];
    }

    // If this voice was looping (bit 2), restore ducked volume on the others.
    if (((g_MixerState[channelId].flags << 0x1e) >> 0x1f) & 0xff) {
        g_nMixerLoopCount--;
        for (i = 0; i < 8; i++)
            if (i != channelId)
                Mixer_RestoreVolume(i);
    }

    (&g_nMixerActiveChanIds)[channelId] = 0;

    // Fire the one-shot done callback if registered.
    if (g_MixerDoneCb[channelId] != 0) {
        void (*cb)(int) = g_MixerDoneCb[channelId];
        g_MixerDoneCb[channelId] = 0;
        cb(channelId);
    }

    LeaveCriticalSection(&g_MixerChanCS);
}

// ============================================================
//  Mixer_SetVolume  (0x00443060)
// ============================================================
// Sets volume and/or pan on a logical channel.
//   param_1 = channel id (0..7)
//   param_2 = new volume 0..32, clamped; 0xFFFFFFFF = keep current
//   param_3 = pan position 0..100, -1 = keep current, 0x32 = centre (50)
//
// Stores volume in channel state table (DAT_006dc280+id*0x28).
// Stores pan in DAT_006dc29c+id*0x28 (unless forced to -1 by looping flag).
//
// If channel is in the active list and (mode==2 OR (mode!=1 AND pan!=0x32 AND pan!=-1)):
//   Compute pan scale factors using atan formula:
//     angle = 20000 / ((100-pan)^2 + pan^2)    (via FUN_0048b1c4 = atan2)
//     left_vol  = volume * atan_val * (100-pan) / 10000
//     right_vol = volume * atan_val * pan       / 10000
//   Set mixing_mode = 2
void Mixer_SetVolume(int channelId, unsigned int volume, int pan)
{
    unsigned int savedVol;

    if (volume == 0xffffffff) {
        savedVol = (unsigned int)g_MixerState[channelId].vol;
    } else {
        // Clamp to 0..0x20 (negative -> 0).
        unsigned int v = ((int)volume < 0 ? 0u : ~0u) & volume;
        if ((int)v > 0x20) v = 0x20;
        volume = v;
        g_MixerState[channelId].vol       = (int)v;
        g_MixerState[channelId].volBackup = (int)v;
        savedVol = v;
    }

    // A looping channel cannot be panned.
    if (g_MixerState[channelId].noPanFlag == 1)
        pan = -1;
    if (pan != -1)
        g_MixerState[channelId].panPos = pan;

    for (int i = 0; i < g_nMixerActiveChans; i++) {
        if (g_MixerChans[i].chanId != channelId)
            continue;

        if (volume != 0xffffffff) {
            if (channelId == 0)
                g_MixerChans[i].vol = (int)(volume * (unsigned)DAT_004d3b04) / 100;
            else
                g_MixerChans[i].vol = (int)volume;
        }

        if (g_MixerChans[i].mode == 2 ||
            (g_MixerChans[i].mode != 1 && pan != 0x32 && pan != -1)) {
            if (pan != -1)
                g_MixerChans[i].panPos = pan;
            unsigned int v = (volume == 0xffffffff) ? savedVol : volume;

            int p = g_MixerChans[i].panPos;
            // angle ~ atan-based weighting (FUN_0048b1c4 == atan helper, __ftol rounds)
            FUN_0048b1c4((double)(int)(20000 / (long long)((100 - p) * (100 - p) + p * p)));
            int a = (int)__ftol();
            g_MixerChans[i].volL = (int)((int)v * a * (100 - p)) / 10000;
            g_MixerChans[i].volR = (int)((int)v * a * p) / 10000;
            g_MixerChans[i].mode = 2;
        }
        return;
    }
}

// ============================================================
//  Mixer_SetVolumeOnly  (0x004433b0)
// ============================================================
// Simplified volume setter: clamps param_2 to 0..+inf (no negative),
// stores it in DAT_006dc280+id*0x28 and DAT_006dc284+id*0x28,
// updates volume_scaled in active table directly, then calls
// Mixer_SetVolume(id, 0xFFFFFFFF, param_3) to handle pan side-effects.
void Mixer_SetVolumeOnly(int channelId, unsigned int volume, int pan)
{
    // Clamp negative to 0 (no upper clamp here).
    volume = (((int)volume < 0 ? 0u : ~0u)) & volume;
    g_MixerState[channelId].vol       = (int)volume;
    g_MixerState[channelId].volBackup = (int)volume;

    for (int i = 0; i < g_nMixerActiveChans; i++) {
        if (g_MixerChans[i].chanId == channelId) {
            if (channelId == 0)
                g_MixerChans[i].vol = (int)(volume * (unsigned)DAT_004d3b04) / 100;
            else
                g_MixerChans[i].vol = (int)volume;
            break;
        }
    }

    // Re-apply pan side effects without touching volume again.
    Mixer_SetVolume(channelId, 0xffffffff, pan);
}

// ============================================================
//  Mixer_RestoreVolume  (0x004436e0)
// ============================================================
// Restores the channel's main volume from the saved backup (DAT_006dc284).
// Called after a temporary duck ends (e.g. when a looping channel finishes).
// Also re-applies the scaled value in the active channel table:
//   if channelId==0: active_vol = (saved_vol * g_nMixerMusicVolPct) / 100
//   else:            active_vol = saved_vol
void Mixer_RestoreVolume(int channelId)
{
    int saved = g_MixerState[channelId].volBackup;
    g_MixerState[channelId].vol = saved;
    for (int i = 0; i < g_nMixerActiveChans; i++) {
        if (g_MixerChans[i].chanId == channelId) {
            if (channelId == 0)
                g_MixerChans[i].vol = (saved * DAT_004d3b04) / 100;
            else
                g_MixerChans[i].vol = saved;
        }
    }
}

// ============================================================
//  Mixer_RemoveChannel  (0x00443820)
// ============================================================
// THEMES calls this as "Pause": temporarily removes a channel from the
// active mix list without clearing its PCM pointers.
// Finds the channel by id in the sorted active list, removes it (shift
// down), sets the paused flag (bit 0 of DAT_006dc288+id*0x28).
void Mixer_RemoveChannel(int channelId)
{
    int i = 0;
    while (g_MixerChans[i].chanId != channelId && i < g_nMixerActiveChans)
        i++;

    if (i != g_nMixerActiveChans) {
        g_nMixerActiveChans--;
        for (; i < g_nMixerActiveChans; i++)
            g_MixerChans[i] = g_MixerChans[i + 1];
        // Mark the logical channel paused (bit 0).
        g_MixerState[channelId].flags |= 1;
    }
}

// ============================================================
//  Mixer_AddChannel  (0x00443970)
// ============================================================
// THEMES calls this as "Resume": re-inserts a channel into the active
// mix list, sorted in ascending order of remaining-sample-count so the
// soonest-to-finish channel is at the tail (index g_nMixerActiveChans-1).
//
// If not failed and channel's paused flag (bit 0) is set:
//   Binary-insert into sorted active list by samples_left.
//   Re-compute active_vol from current volume:
//     if channelId==0: active_vol = (vol * g_nMixerMusicVolPct) / 100
//   Set read_ptr to current PCM start, samples_left = (end-start) byte count.
//   Increment g_nMixerActiveChans.
//   Clear paused flag.
void Mixer_AddChannel(int channelId)
{
    // Total sample count = (pcmEnd - pcmStart) + 1.
    int samples = (int)((unsigned char*)g_MixerState[channelId].pcmEnd
                      - (unsigned char*)g_MixerState[channelId].pcmStart) + 1;

    // Only if the mixer is alive and this channel is currently paused (bit 0).
    if (g_nMixerFailed != 1 && ((int)(g_MixerState[channelId].flags << 0x1f) < 0)) {
        // Insertion sort: find slot keeping the list ascending by samplesLeft.
        int slot = g_nMixerActiveChans;
        while (slot > 0 && g_MixerChans[slot - 1].samplesLeft < samples) {
            g_MixerChans[slot] = g_MixerChans[slot - 1];
            slot--;
        }

        g_MixerChans[slot].chanId      = channelId;
        g_MixerChans[slot].pSrc        = (unsigned char*)g_MixerState[channelId].pcmStart;
        g_MixerChans[slot].samplesLeft = samples;
        if (channelId == 0)
            g_MixerChans[slot].vol = (g_MixerState[0].vol * DAT_004d3b04) / 100;
        else
            g_MixerChans[slot].vol = g_MixerState[channelId].vol;

        g_nMixerActiveChans++;
        g_MixerState[channelId].flags &= 0xfffffffe;   // clear paused bit
    }
}

// ============================================================
//  Mixer_SelectFillFunc  (0x00444350)
// ============================================================
// Selects the appropriate fill-buffer kernel and stores it in g_pfnMixerFillBuf.
// Decision tree based on (g_nMixerSampleRate, g_nMixerOutChans, param_1, param_2):
//
//   if sampleRate == 0xAC44 (44100):
//     if outChans == 2 (stereo output):
//       if srcChans == 2:
//         if pan==0 → Mixer_FillBuf_U8_Stereo_Mono   (0x00449450)   [note: confusing name — 44k stereo+pan=0 path]
//         else      → Mixer_FillBuf_U8_Stereo_StereoA (0x0044e8e0)  [not exact — see below]
//       else (srcChans==1):
//         → Mixer_FillBuf_U8_Mono_Mono (0x0044ac50)
//     else (mono output):
//       → Mixer_FillBuf_U8_Mono_Mono (0x0044ac50)
//   else (22050 or other):
//     if outChans == 2:
//       if srcChans == 2:
//         if pan==0 → Mixer_FillBuf_S16_Stereo_Stereo (0x0044b760)
//         else      → Mixer_FillBuf_S16_Stereo_StereoB (0x0044cb80)
//       else → Mixer_FillBuf_S16_Stereo_Mono (0x0044deb0)
//     else (mono):
//       if srcChans == 2:
//         if pan==0 → Mixer_FillBuf_U8_Stereo_StereoA (0x0044e8e0)
//         else      → Mixer_FillBuf_U8_Stereo_StereoB (0x0044fe50)
//       else → Mixer_FillBuf_U8_Mono_StereoMix (0x004513f0)
//
// Note: the actual selection branches on DAT_006dc068, DAT_006dc270,
// param_1 (srcChans), and param_2 (pan flag).
void Mixer_SelectFillFunc(int srcChannels, int panEnabled)
{
    typedef void (*FillFn)(void*, size_t);

    if (g_nMixerSampleRate == 0xac44) {           // 44100 Hz
        if (g_nMixerOutChans == 2) {              // stereo output
            if (srcChannels == 2) {
                if (panEnabled == 0)
                    g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_StereoA;
                else
                    g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_StereoB;
            } else {
                g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_Mono44k;
            }
        } else if (srcChannels == 2) {
            if (panEnabled == 0)
                g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Stereo_Mono;
            else
                g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Stereo_StereoB44k;
        } else {
            g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Mono_Mono;
        }
    } else if (g_nMixerOutChans == 2) {           // stereo output, non-44.1k
        if (srcChannels == 2) {
            if (panEnabled == 0)
                g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_Stereo;
            else
                g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_StereoB;
        } else {
            g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_S16_Stereo_Mono;
        }
    } else if (srcChannels == 2) {                // mono output, non-44.1k
        if (panEnabled == 0)
            g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Stereo_StereoA;
        else
            g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Stereo_StereoB;
    } else {
        g_pfnMixerFillBuf = (FillFn)Mixer_FillBuf_U8_Mono_StereoMix;
    }
}

// ============================================================
//  Mixer_DsErrToStr  (0x004440c0)
// ============================================================
// Converts a DirectSound HRESULT to a static English string.
// Known values: DS_OK, DSERR_ALLOCATED, DSERR_ALREADYINITIALIZED,
//   DSERR_BADFORMAT, DSERR_BUFFERLOST, DSERR_CONTROLUNAVAIL,
//   DSERR_GENERIC, DSERR_INVALIDCALL, DSERR_INVALIDPARAM,
//   DSERR_NOAGGREGATION, DSERR_NODRIVER, DSERR_NOINTERFACE,
//   DSERR_OTHERAPPHASPRIO, DSERR_OUTOFMEMORY, DSERR_PRIOLEVELNEEDED,
//   DSERR_UNINITIALIZED, DSERR_UNSUPPORTED.
// Returns "Unknown" for unrecognised codes.
const char* Mixer_DsErrToStr(int hr)
{
    // Faithful translation of the original signed-constant comparison tree.
    switch (hr) {
    case 0:            return "DS_OK";
    case -0x7787fff6:  return "DSERR_ALLOCATED";
    case -0x7787ffce:  return "DSERR_INVALIDCALL";
    case -0x7787ffe2:  return "DSERR_CONTROLUNAVAIL";
    case -0x7787ff9c:  return "DSERR_BADFORMAT";
    case -0x7787ffba:  return "DSERR_PRIOLEVELNEEDED";
    case -0x7787ff7e:  return "DSERR_ALREADYINITIALIZED";
    case -0x7787ff88:  return "DSERR_NODRIVER";
    case -0x7787ff60:  return "DSERR_OTHERAPPHASPRIO";
    case -0x7787ff6a:  return "DSERR_BUFFERLOST";
    case -0x7787ff56:  return "DSERR_UNINITIALIZED";
    case -0x7fffbffb:  return "DSERR_GENERIC";
    case -0x7fffbfff:  return "DSERR_UNSUPPORTED";
    case -0x7ff8fff2:  return "DSERR_OUTOFMEMORY";
    case -0x7ffbfef0:  return "DSERR_NOAGGREGATION";
    case -0x7ff8ffa9:  return "DSERR_INVALIDPARAM";
    default:           return "Unknown";
    }
}

// ============================================================
//  Fill-buffer kernels  (0x00449450 – 0x004513f0)
// ============================================================
// All kernels share the same logic structure:
//   1. Initialise output pointer from g_pMixerAccumBuf (U8 modes) or
//      directly from param_1 (S16 modes).
//   2. For channel 0 (first active entry, DAT_006dc0a8==0):
//      Mix its samples into the buffer with its volume scale.
//   3. For channels 1..N (subsequent entries, DAT_006dc0a8 loop):
//      Add-mix into the buffer (+=).
//   4. U8 modes: convert the S16 accumulation buffer back to U8 by
//      indexing g_pMixerVolTable and store into param_1.
//   5. Update each channel's remaining-sample counter:
//      chan_samples_left[i] -= (param_2 * g_nMixerBytesPerTick) / chan_src_rate[i]
//
// Mixing modes per channel (DAT_006dc0a8 field):
//   0 = mono source → duplicate to both L/R (or use as mono)
//   1 = stereo source → downmix L+R, advance ptr by 2 samples per output
//   2 = pan mode → separate L/R scale factors (DAT_006dc0bc / DAT_006dc0c0)
//
// Source bit depth (DAT_006dc0b0):
//   0x08 = 8-bit unsigned (subtract 0x80 to get signed)
//   0x10 = 16-bit signed  (read as short*)
//
// Source channel stride (DAT_006dc0ac vs DAT_006dc068):
//   if src_channels == out_channels: stride = bytes_per_sample
//   else: stride = bytes_per_sample * out_channels  (skip to keep in phase)

// ------------------------------------------------------------------
// Shared helpers for the fill kernels below.
//
// Each kernel's body in the original binary is fully hand-unrolled into a
// 3x3x{1,2} ladder (mode {0,1,2} x rate {equal,resample} x bits {U8,S16})
// for both the channel-0 prime pass and the channel-1..N add-mix pass.
// The implementations below are the algorithmically-equivalent rolled loops;
// the per-iteration arithmetic matches the decompiler output exactly:
//
//   S16 source sample s, volume v:   out = (short)((v*s + ((v*s>>31)&0xff)) >> 8)
//     (the (>>31 & 0xff) term is the C truncating-divide-toward-zero
//      rounding bias the compiler emitted for "/256")
//   U8  source sample b, volume v:   out = (short)v * (b - 0x80)
//
// For 8-bit *output* (U8 modes) channel 0 stores its value biased by +0x8000
// into the S16 accumulation buffer (g_pMixerAccumBuf); later channels do += .
// Finally the accumulator is mapped back to U8 via g_pMixerVolTable[acc>>5].
// For 16-bit output the kernels write straight into the DS buffer.
//
// Source advance ("stride"): when the source rate equals the output rate
// (srcRate==g_nMixerSampleRate) the read pointer steps by one source frame;
// otherwise it steps by one frame scaled by the output channel count to stay
// in phase with the decimated output (matches the +2/+4/+6/+8 byte steps).
// ------------------------------------------------------------------

namespace {

// S16-source scaled sample, reproducing the decompiler's "/256" idiom.
inline short MixScaleS16(int vol, short s)
{
    int p = vol * (int)s;
    return (short)((p + ((p >> 31) & 0xff)) >> 8);
}

// U8-source scaled sample (no >>8; the table/volume already account for range).
inline short MixScaleU8(int vol, unsigned char b)
{
    return (short)((short)vol * ((int)b - 0x80));
}

} // namespace

void Mixer_FillBuf_U8_Stereo_Mono(void* buf, size_t n)
{
    // U8 stereo output (44100 path). Sources mixed into the S16 accumulator,
    // each output frame written to both L and R; non-matching rates expand to
    // two output frames per source sample.  Original is hand-unrolled; this is
    // the equivalent loop.
    unsigned short* acc    = (unsigned short*)g_pMixerAccumBuf;
    unsigned short* accEnd = acc + n;

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }
    Debug_Assert(0, "C:\\DevStudio\\Projects\\Crux\\MIXER.cpp", (int)n);

    // ---- channel 0: prime accumulator (store with +0x8000 bias) ----
    {
        MixerChannel* c = &g_MixerChans[0];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < accEnd) {
            unsigned short l, r;
            if (c->mode == 2) {
                // pan: separate L/R scale
                if (s16) { short s = *(short*)c->pSrc;
                           l = MixScaleS16(c->volL, s) + 0x8000;
                           r = MixScaleS16(c->volR, s) + 0x8000; }
                else     { unsigned char b = *c->pSrc;
                           l = MixScaleU8(c->volL, b) + 0x8000;
                           r = MixScaleU8(c->volR, b) + 0x8000; }
                c->pSrc += s16 ? 2 : 1;
            } else {
                // mode 0/1 both collapse to a single mono value here
                if (s16) { l = MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000; c->pSrc += 2; }
                else     { l = MixScaleU8(c->vol, *c->pSrc) + 0x8000;        c->pSrc += 1; }
                r = l;
            }
            p[0] = l; p[1] = r;
            p += 2;
            if (!matched) { p[-2 + 2] = l; p[-1 + 2] = r; p[0] = l; p[1] = r; p += 2;
                            // resampled path duplicates the same frame again
            }
        }
    }

    // ---- channels 1..N: add-mix into accumulator ----
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        int step = matched ? 2 : 4;
        while (p < accEnd) {
            unsigned short l, r;
            if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc;
                           l = (unsigned short)MixScaleS16(c->volL, s);
                           r = (unsigned short)MixScaleS16(c->volR, s); c->pSrc += 2; }
                else     { unsigned char b = *c->pSrc;
                           l = MixScaleU8(c->volL, b); r = MixScaleU8(c->volR, b); c->pSrc += 1; }
            } else {
                if (s16) { short s = *(short*)c->pSrc; l = r = (unsigned short)MixScaleS16(c->vol, s); c->pSrc += 2; }
                else     { unsigned char b = *c->pSrc;  l = r = (unsigned short)MixScaleU8(c->vol, b); c->pSrc += 1; }
            }
            p[0] += l; p[1] += r;
            if (!matched) { p[2] += l; p[3] += r; }
            p += step;
        }
    }

    // ---- convert accumulator to U8 stereo via the clamp LUT ----
    {
        const unsigned char* lut = (const unsigned char*)g_pMixerVolTable;
        unsigned char* out = (unsigned char*)buf;
        unsigned short* p = acc;
        for (unsigned char* o = out; o < out + n; o += 2) {
            o[1] = lut[(unsigned)p[0] >> 5];
            o[0] = lut[(unsigned)p[1] >> 5];
            p += 2;
        }
    }

    // ---- advance each channel's remaining-sample counter ----
    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_U8_Mono_Mono(void* buf, size_t n)
{
    // U8 mono output. One accumulator entry per output sample.  Mode 1 (stereo
    // source) downmixes L+R.  Original is hand-unrolled; this is the equivalent
    // loop.
    unsigned short* acc    = (unsigned short*)g_pMixerAccumBuf;
    unsigned short* accEnd = acc + n;

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        // mode 2 (pan) collapses to the same single-channel path as mode 0
        if (c->mode == 1) {
            // stereo source -> mono downmix
            while (p < accEnd) {
                if (s16) { int sum = c->vol * ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]);
                           *p = (unsigned short)((short)((sum + ((sum >> 31) & 0xff)) >> 8) + 0x8000);
                           c->pSrc += 4; }
                else     { *p = (unsigned short)((short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1]) + 0x8000);
                           c->pSrc += 2; }
                ++p;
            }
        } else {
            while (p < accEnd) {
                if (s16) { *p = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += 2; }
                else     { *p = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000);          c->pSrc += 1; }
                ++p;
                if (!matched) { p[0] = p[-1]; ++p; }
            }
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        // mode 0 and 2 use the single-sample add path; mode 1 downmixes
        if (c->mode == 1) {
            while (p < accEnd) {
                if (s16) { int sum = c->vol * ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]);
                           *p += (unsigned short)((short)((sum + ((sum >> 31) & 0xff)) >> 8)); c->pSrc += 4; }
                else     { *p += (unsigned short)((short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1])); c->pSrc += 2; }
                ++p;
            }
        } else {
            while (p < accEnd) {
                short v;
                if (s16) { v = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += 2; }
                else     { v = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += 1; }
                *p += (unsigned short)v;
                ++p;
                if (!matched) { *p += (unsigned short)v; ++p; }
            }
        }
    }

    // convert accumulator -> U8 mono
    {
        const unsigned char* lut = (const unsigned char*)g_pMixerVolTable;
        unsigned char* out = (unsigned char*)buf;
        unsigned short* p = acc;
        for (unsigned char* o = out; o < out + n; ++o) { *o = lut[(unsigned)*p >> 5]; ++p; }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_S16_Stereo_Stereo(short* buf, size_t n)
{
    // S16 stereo output, stereo/mono sources, pan disabled.  Writes directly
    // into the DS buffer (no accumulator).  Mode 1 reads consecutive L/R source
    // samples.  Original is hand-unrolled; this is the equivalent loop.
    short* end = (short*)((char*)buf + n);

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0: store (no accumulator bias for 16-bit output)
    {
        MixerChannel* c = &g_MixerChans[0];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < end) {
            if (c->mode == 1) {
                // interleaved stereo source: L then R
                if (s16) { p[0] = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += 2;
                           p[1] = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 6; }
                else     { p[0] = MixScaleU8(c->vol, *c->pSrc); c->pSrc += 1;
                           p[1] = MixScaleU8(c->vol, *c->pSrc); c->pSrc += matched ? 1 : 3; }
            } else {
                short v;
                if (c->mode == 2) {
                    if (s16) { short s = *(short*)c->pSrc; p[0] = MixScaleS16(c->volL, s); p[1] = MixScaleS16(c->volR, s); c->pSrc += matched ? 2 : 4; }
                    else     { unsigned char b = *c->pSrc; p[0] = MixScaleU8(c->volL, b); p[1] = MixScaleU8(c->volR, b); c->pSrc += matched ? 1 : 2; }
                    p += 2; continue;
                }
                if (s16) { v = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] = p[1] = v;
            }
            p += 2;
        }
    }

    // channels 1..N: add-mix
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < end) {
            if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] += MixScaleS16(c->volL, s); p[1] += MixScaleS16(c->volR, s); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] += MixScaleU8(c->volL, b); p[1] += MixScaleU8(c->volR, b); c->pSrc += matched ? 1 : 2; }
            } else if (c->mode == 1) {
                if (s16) { p[0] += MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += 2;
                           p[1] += MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 6; }
                else     { p[0] += MixScaleU8(c->vol, *c->pSrc); c->pSrc += 1;
                           p[1] += MixScaleU8(c->vol, *c->pSrc); c->pSrc += matched ? 1 : 3; }
            } else {
                short v;
                if (s16) { v = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] += v; p[1] += v;
            }
            p += 2;
        }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_S16_Stereo_StereoB(short* buf, size_t n)
{
    // S16 stereo output, pan!=0 variant: like _Stereo but L/R are swapped in the
    // stereo-source path (reads the R sample first) and pan factors apply.
    // Original is hand-unrolled; this is the equivalent loop.
    short* end = (short*)((char*)buf + n);

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < end) {
            if (c->mode == 1) {
                // stereo source, swapped: out L = src R, out R = src L
                if (s16) { p[0] = MixScaleS16(c->vol, ((short*)c->pSrc)[1]);
                           p[1] = MixScaleS16(c->vol, ((short*)c->pSrc)[0]); c->pSrc += matched ? 4 : 8; }
                else     { p[0] = MixScaleU8(c->vol, c->pSrc[1]);
                           p[1] = MixScaleU8(c->vol, c->pSrc[0]); c->pSrc += matched ? 2 : 4; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] = MixScaleS16(c->volR, s); p[1] = MixScaleS16(c->volL, s); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] = MixScaleU8(c->volR, b); p[1] = MixScaleU8(c->volL, b); c->pSrc += matched ? 1 : 2; }
            } else {
                short v;
                if (s16) { v = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] = p[1] = v;
            }
            p += 2;
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < end) {
            if (c->mode == 1) {
                if (s16) { p[0] += MixScaleS16(c->vol, ((short*)c->pSrc)[1]);
                           p[1] += MixScaleS16(c->vol, ((short*)c->pSrc)[0]); c->pSrc += matched ? 4 : 8; }
                else     { p[0] += MixScaleU8(c->vol, c->pSrc[1]);
                           p[1] += MixScaleU8(c->vol, c->pSrc[0]); c->pSrc += matched ? 2 : 4; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] += MixScaleS16(c->volR, s); p[1] += MixScaleS16(c->volL, s); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] += MixScaleU8(c->volR, b); p[1] += MixScaleU8(c->volL, b); c->pSrc += matched ? 1 : 2; }
            } else {
                short v;
                if (s16) { v = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] += v; p[1] += v;
            }
            p += 2;
        }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_S16_Stereo_Mono(short* buf, size_t n)
{
    // S16 *mono* output buffer (one short per output sample).  Mode 1 downmixes
    // stereo sources; mode 0/2 use a single sample.  Original is hand-unrolled;
    // this is the equivalent loop.
    short* end = (short*)((char*)buf + n);

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        // mode 2 falls through to the mode-0 single-sample path
        if (c->mode == 1) {
            while (p < end) {
                if (s16) { int sum = c->vol * ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]);
                           *p = (short)((sum + ((sum >> 31) & 0xff)) >> 8); c->pSrc += matched ? 4 : 8; }
                else     { *p = (short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1]); c->pSrc += matched ? 2 : 4; }
                ++p;
            }
        } else {
            while (p < end) {
                if (s16) { *p = MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { *p = MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                ++p;
            }
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        short* p = buf;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        if (c->mode == 1) {
            while (p < end) {
                if (s16) { int sum = ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]) * c->vol;
                           *p += (short)((sum + ((sum >> 31) & 0xff)) >> 8); c->pSrc += matched ? 4 : 8; }
                else     { *p += (short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1]); c->pSrc += matched ? 2 : 4; }
                ++p;
            }
        } else {
            while (p < end) {
                if (s16) { *p += MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { *p += MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                ++p;
            }
        }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_U8_Stereo_StereoA(void* buf, size_t n)
{
    // U8 stereo output (non-44100 path), stereo sources, pan=0.  Accumulates
    // into the S16 buffer (+0x8000 bias on channel 0) then converts via the LUT.
    // Output is two accumulator entries per output frame.  Original is
    // hand-unrolled; this is the equivalent loop.
    unsigned short* acc    = (unsigned short*)g_pMixerAccumBuf;
    unsigned short* accEnd = acc + n;

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < accEnd) {
            if (c->mode == 1) {
                if (s16) { p[0] = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += 2;
                           p[1] = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += matched ? 2 : 6; }
                else     { p[0] = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000); c->pSrc += 1;
                           p[1] = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000); c->pSrc += matched ? 1 : 3; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] = (unsigned short)(MixScaleS16(c->volL, s) + 0x8000); p[1] = (unsigned short)(MixScaleS16(c->volR, s) + 0x8000); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] = (unsigned short)(MixScaleU8(c->volL, b) + 0x8000); p[1] = (unsigned short)(MixScaleU8(c->volR, b) + 0x8000); c->pSrc += matched ? 1 : 2; }
            } else {
                unsigned short v;
                if (s16) { v = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += matched ? 2 : 4; }
                else     { v = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000);          c->pSrc += matched ? 1 : 2; }
                p[0] = p[1] = v;
            }
            p += 2;
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < accEnd) {
            if (c->mode == 1) {
                if (s16) { p[0] += (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += 2;
                           p[1] += (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 6; }
                else     { p[0] += (unsigned short)MixScaleU8(c->vol, *c->pSrc); c->pSrc += 1;
                           p[1] += (unsigned short)MixScaleU8(c->vol, *c->pSrc); c->pSrc += matched ? 1 : 3; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] += (unsigned short)MixScaleS16(c->volL, s); p[1] += (unsigned short)MixScaleS16(c->volR, s); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] += (unsigned short)MixScaleU8(c->volL, b); p[1] += (unsigned short)MixScaleU8(c->volR, b); c->pSrc += matched ? 1 : 2; }
            } else {
                unsigned short v;
                if (s16) { v = (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = (unsigned short)MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] += v; p[1] += v;
            }
            p += 2;
        }
    }

    // convert accumulator -> U8 stereo (this kernel walks 1 byte per acc entry)
    {
        const unsigned char* lut = (const unsigned char*)g_pMixerVolTable;
        unsigned char* out = (unsigned char*)buf;
        unsigned short* p = acc;
        for (unsigned char* o = out; o < out + n; ++o) { *o = lut[(unsigned)*p >> 5]; ++p; }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_U8_Stereo_StereoB(void* buf, size_t n)
{
    // U8 stereo output (non-44100 path), stereo sources, pan!=0.  Identical to
    // _StereoA for the accumulate phase but the final LUT conversion swaps L/R
    // (matching the 2-byte-stride writeback).  Original is hand-unrolled; this
    // is the equivalent loop.
    unsigned short* acc    = (unsigned short*)g_pMixerAccumBuf;
    unsigned short* accEnd = acc + n;

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < accEnd) {
            if (c->mode == 1) {
                if (s16) { p[0] = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += 2;
                           p[1] = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += matched ? 2 : 6; }
                else     { p[0] = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000); c->pSrc += 1;
                           p[1] = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000); c->pSrc += matched ? 1 : 3; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] = (unsigned short)(MixScaleS16(c->volL, s) + 0x8000); p[1] = (unsigned short)(MixScaleS16(c->volR, s) + 0x8000); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] = (unsigned short)(MixScaleU8(c->volL, b) + 0x8000); p[1] = (unsigned short)(MixScaleU8(c->volR, b) + 0x8000); c->pSrc += matched ? 1 : 2; }
            } else {
                unsigned short v;
                if (s16) { v = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += matched ? 2 : 4; }
                else     { v = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000);          c->pSrc += matched ? 1 : 2; }
                p[0] = p[1] = v;
            }
            p += 2;
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        while (p < accEnd) {
            if (c->mode == 1) {
                if (s16) { p[0] += (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += 2;
                           p[1] += (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 6; }
                else     { p[0] += (unsigned short)MixScaleU8(c->vol, *c->pSrc); c->pSrc += 1;
                           p[1] += (unsigned short)MixScaleU8(c->vol, *c->pSrc); c->pSrc += matched ? 1 : 3; }
            } else if (c->mode == 2) {
                if (s16) { short s = *(short*)c->pSrc; p[0] += (unsigned short)MixScaleS16(c->volL, s); p[1] += (unsigned short)MixScaleS16(c->volR, s); c->pSrc += matched ? 2 : 4; }
                else     { unsigned char b = *c->pSrc; p[0] += (unsigned short)MixScaleU8(c->volL, b); p[1] += (unsigned short)MixScaleU8(c->volR, b); c->pSrc += matched ? 1 : 2; }
            } else {
                unsigned short v;
                if (s16) { v = (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { v = (unsigned short)MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                p[0] += v; p[1] += v;
            }
            p += 2;
        }
    }

    // convert accumulator -> U8 stereo, swapping L/R (2-byte stride)
    {
        const unsigned char* lut = (const unsigned char*)g_pMixerVolTable;
        unsigned char* out = (unsigned char*)buf;
        unsigned short* p = acc;
        for (unsigned char* o = out; o < out + n; o += 2) {
            o[1] = lut[(unsigned)p[0] >> 5];
            o[0] = lut[(unsigned)p[1] >> 5];
            p += 2;
        }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}

void Mixer_FillBuf_U8_Mono_StereoMix(void* buf, size_t n)
{
    // U8 mono output (non-44100 path).  Sources may be stereo (mode 1, downmixed
    // L+R) or mono (mode 0/2).  Single accumulator entry per output sample, then
    // LUT conversion.  Original is hand-unrolled; this is the equivalent loop.
    unsigned short* acc    = (unsigned short*)g_pMixerAccumBuf;
    unsigned short* accEnd = acc + n;

    if (g_nMixerActiveChans == 0) {
        memset(buf, g_nMixerSilenceByte, n);
        return;
    }

    // channel 0
    {
        MixerChannel* c = &g_MixerChans[0];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        if (c->mode == 1) {
            while (p < accEnd) {
                if (s16) { int sum = c->vol * ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]);
                           *p = (unsigned short)((short)((sum + ((sum >> 31) & 0xff)) >> 8) + 0x8000); c->pSrc += matched ? 4 : 8; }
                else     { *p = (unsigned short)((short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1]) + 0x8000); c->pSrc += matched ? 2 : 4; }
                ++p;
            }
        } else {
            while (p < accEnd) {
                if (s16) { *p = (unsigned short)(MixScaleS16(c->vol, *(short*)c->pSrc) + 0x8000); c->pSrc += matched ? 2 : 4; }
                else     { *p = (unsigned short)(MixScaleU8(c->vol, *c->pSrc) + 0x8000);          c->pSrc += matched ? 1 : 2; }
                ++p;
            }
        }
    }

    // channels 1..N
    for (int i = 1; i < g_nMixerActiveChans; ++i) {
        MixerChannel* c = &g_MixerChans[i];
        unsigned short* p = acc;
        bool matched = (c->srcRate == g_nMixerSampleRate);
        bool s16     = (c->srcBits == 0x10);
        if (c->mode == 1) {
            while (p < accEnd) {
                if (s16) { int sum = c->vol * ((int)((short*)c->pSrc)[0] + (int)((short*)c->pSrc)[1]);
                           *p += (unsigned short)((short)((sum + ((sum >> 31) & 0xff)) >> 8)); c->pSrc += matched ? 4 : 8; }
                else     { *p += (unsigned short)((short)c->vol * (((int)c->pSrc[0] - 0x100) + c->pSrc[1])); c->pSrc += matched ? 2 : 4; }
                ++p;
            }
        } else {
            while (p < accEnd) {
                if (s16) { *p += (unsigned short)MixScaleS16(c->vol, *(short*)c->pSrc); c->pSrc += matched ? 2 : 4; }
                else     { *p += (unsigned short)MixScaleU8(c->vol, *c->pSrc);          c->pSrc += matched ? 1 : 2; }
                ++p;
            }
        }
    }

    // convert accumulator -> U8 mono
    {
        const unsigned char* lut = (const unsigned char*)g_pMixerVolTable;
        unsigned char* out = (unsigned char*)buf;
        unsigned short* p = acc;
        for (unsigned char* o = out; o < out + n; ++o) { *o = lut[(unsigned)*p >> 5]; ++p; }
    }

    for (int i = 0; i < g_nMixerActiveChans; ++i)
        g_MixerChans[i].samplesLeft -= (int)(n * g_nMixerBytesPerTick) / g_MixerChans[i].srcRateDiv;
}
