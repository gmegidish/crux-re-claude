#include "Audio.h"
#include "Log.h"
#include <SDL.h>
#include <vector>
#include <cstddef>
#include <cstring>
#include <cmath>

namespace {

constexpr int kChannels = 6;   // music, 3 SCM voices, theme, + spare

struct Chan {
    std::vector<int16_t> buf;   // S16 mono samples
    size_t pos = 0;             // read cursor (advanced by the audio callback)
    float  gain = 1.0f;         // per-channel linear gain
    bool   loop = false;        // when drained, wrap pos to 0 instead of stopping
};

SDL_AudioDeviceID g_dev = 0;
bool g_tried = false;
Chan g_chan[kChannels];
float g_masterGain = 1.0f;      // global output gain (op 0x84d)

// Audio thread: sum all channels (each scaled by its gain), apply master gain,
// clamp to S16.
void mixCallback(void* /*ud*/, Uint8* stream, int len) {
    int16_t* out = reinterpret_cast<int16_t*>(stream);
    const int n = len / (int)sizeof(int16_t);
    for (int i = 0; i < n; ++i) {
        float sum = 0.0f;
        for (auto& c : g_chan) {
            if (c.loop && !c.buf.empty() && c.pos >= c.buf.size()) c.pos = 0;  // wrap looping SFX
            if (c.pos < c.buf.size()) sum += c.gain * (float)c.buf[c.pos++];
        }
        sum *= g_masterGain;
        if (sum >  32767.0f) sum =  32767.0f;
        if (sum < -32768.0f) sum = -32768.0f;
        out[i] = (int16_t)sum;
    }
}

}  // namespace

namespace Audio {

bool open() {
    if (g_dev) return true;
    if (g_tried) return false;
    g_tried = true;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        Log::warn("Audio: SDL_InitSubSystem(AUDIO) failed (%s) — silent", SDL_GetError());
        return false;
    }
    SDL_AudioSpec want{}, have{};
    want.freq     = 22050;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 1024;
    want.callback = mixCallback;

    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!g_dev) {
        Log::warn("Audio: SDL_OpenAudioDevice failed (%s) — silent", SDL_GetError());
        return false;
    }
    Log::info("Audio: open %d Hz, %d-bit, %d ch (software mixer, %d channels)",
              have.freq, SDL_AUDIO_BITSIZE(have.format), have.channels, kChannels);
    SDL_PauseAudioDevice(g_dev, 0);
    return true;
}

void close() {
    if (g_dev) { SDL_CloseAudioDevice(g_dev); g_dev = 0; }
}

bool isOpen() { return g_dev != 0; }

// One source sample -> S16, at byte offset `i`.
static inline int sampleAt(const uint8_t* p, bool b16) {
    if (b16) { int16_t v; std::memcpy(&v, p, 2); return v; }
    return ((int)*p - 128) << 8;             // 8-bit unsigned -> S16
}

void queue(int channel, const uint8_t* data, size_t bytes, int fmt) {
    if (!g_dev || channel < 0 || channel >= kChannels || !data || !bytes) return;
    const bool b16    = (fmt & 1) != 0;      // 16-bit else 8-bit
    const bool stereo = (fmt & 2) != 0;      // stereo else mono
    const bool r44    = (fmt & 4) != 0;      // 44100 else 22050
    const int  bps    = b16 ? 2 : 1;         // bytes per sample
    const int  chans  = stereo ? 2 : 1;
    const int  frameB = bps * chans;         // bytes per frame
    const int  decim  = r44 ? 2 : 1;         // 44100 -> drop every other frame (-> 22050)
    const size_t frames = bytes / frameB;

    SDL_LockAudioDevice(g_dev);
    Chan& ch = g_chan[channel];
    // Drop already-played samples so a continuously-streamed channel (THEME)
    // stays bounded rather than growing for the whole track.
    if (ch.pos > 0) { ch.buf.erase(ch.buf.begin(), ch.buf.begin() + ch.pos); ch.pos = 0; }
    auto& buf = ch.buf;
    buf.reserve(buf.size() + frames / decim + 1);
    for (size_t f = 0; f < frames; f += decim) {
        const uint8_t* p = data + f * frameB;
        int s = sampleAt(p, b16);
        if (stereo) s = (s + sampleAt(p + bps, b16)) >> 1;   // downmix L+R
        buf.push_back((int16_t)s);
    }
    SDL_UnlockAudioDevice(g_dev);
}

void reset() {
    if (!g_dev) return;
    SDL_LockAudioDevice(g_dev);
    // Clear the SCM channels (music + the 3 voices); THEME is owned by the Theme
    // subsystem and must survive an SCM reset.
    for (int c : { MUSIC, VOICE0, VOICE1, VOICE2 }) { g_chan[c].buf.clear(); g_chan[c].pos = 0; }
    SDL_UnlockAudioDevice(g_dev);
}

void setMasterVolumeMillibels(int mb) {
    if (mb > 0) { mb = 0; }
    if (mb < -10000) { mb = -10000; }
    g_masterGain = std::pow(10.0f, (float)mb / 2000.0f);   // hundredths-of-dB -> linear
}

void setChannelGain(int channel, float gain) {
    if (channel < 0 || channel >= kChannels) { return; }
    if (gain < 0.0f) { gain = 0.0f; }
    if (g_dev) { SDL_LockAudioDevice(g_dev); }
    g_chan[channel].gain = gain;
    if (g_dev) { SDL_UnlockAudioDevice(g_dev); }
}

size_t queuedSamples(int channel) {
    if (channel < 0 || channel >= kChannels) { return 0; }
    if (!g_dev) { return 0; }
    SDL_LockAudioDevice(g_dev);
    const Chan& c = g_chan[channel];
    size_t remaining = (c.pos < c.buf.size()) ? (c.buf.size() - c.pos) : 0;
    SDL_UnlockAudioDevice(g_dev);
    return remaining;
}

void clearChannel(int channel) {
    if (channel < 0 || channel >= kChannels || !g_dev) { return; }
    SDL_LockAudioDevice(g_dev);
    g_chan[channel].buf.clear();
    g_chan[channel].pos = 0;
    g_chan[channel].loop = false;
    SDL_UnlockAudioDevice(g_dev);
}

void setLoop(int channel, bool loop) {
    if (channel < 0 || channel >= kChannels || !g_dev) { return; }
    SDL_LockAudioDevice(g_dev);
    g_chan[channel].loop = loop;
    SDL_UnlockAudioDevice(g_dev);
}

}  // namespace Audio
