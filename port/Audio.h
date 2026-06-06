// Audio.h — minimal SDL2 software mixer for SCM playback.
//
// The engine outputs 22050 Hz / 16-bit (ADVENT.INI [Sound]) and does NO
// sample-rate conversion, so SCM PCM is already 22050 Hz mono. SCM audio chunks:
//   0x40-0x43  music   |  0x80-0x83  bunch audio/sfx   |  0x100  speech (markers)
// The chunk `param` encodes the sample format; bit 0x100 = 16-bit (else 8-bit
// unsigned, silence = 0x80). A video can carry music AND a sfx stream at once
// (e.g. BRDFWR3), so we mix several logical channels rather than a single FIFO.
//
// Implementation: an SDL audio callback sums all channels into a mono S16 buffer.
// Each channel is an append-only S16 sample queue with a read cursor; chunks are
// converted to S16 on enqueue. reset() clears everything between SCMs.
#pragma once
#include <cstdint>
#include <cstddef>

namespace Audio {

constexpr int MUSIC  = 0;  // 0x40-0x43 SCM music
constexpr int VOICE0 = 1;  // 0x80-0x83 SCM bunch-audio voices: the engine plays up to
constexpr int VOICE1 = 2;  // 3 concurrent voice channels (mixer ids 1/2/3), summed; the
constexpr int VOICE2 = 3;  // channel index is the chunk's param LOW byte (not the type).
constexpr int THEME  = 4;  // room-music sequencer (persists across SCM reset())
constexpr int SFX    = 5;  // looping room sound-effect (op 0x15 PLAY_SOUND, engine FX
                           // channel 3); persists across SCM reset(), cleared on area
                           // change / op 0x16f (Fx_StopLoop) / op 0x132 (stop tracked).

bool open();
void close();
bool isOpen();

// Master output gain from a DirectSound millibel attenuation in [-10000, 0]
// (op 0x84d / Mixer_SetMasterVolume): gain = 10^(mb/2000), so 0 = full, -10000 ~ mute.
void setMasterVolumeMillibels(int mb);

// Per-channel linear gain (0..1). Theme volume 0..64 maps to v/64; fades ramp it.
void setChannelGain(int channel, float gain);

// Unplayed S16 samples still queued on `channel` (the sequencer's low-water mark).
size_t queuedSamples(int channel);

// Clear a single channel's queue (e.g. Theme::stopMusic). reset() clears only the
// SCM channels (MUSIC/SFX), leaving THEME for the Theme subsystem to manage.
void clearChannel(int channel);

// Append a chunk to a logical channel. `fmt` is the engine's format word
// (chunk param >> 8): bit0 = 16-bit (else 8-bit unsigned), bit1 = stereo (else
// mono), bit2 = 44100 Hz (else 22050). The source is converted to the device's
// mono 22050 S16 (stereo downmixed, 44100 decimated).
void queue(int channel, const uint8_t* data, size_t bytes, int fmt);

// Mark a channel as looping: when its queue drains, the read cursor wraps to the
// start instead of stopping (used for the op-0x15 room SFX). clearChannel clears it.
void setLoop(int channel, bool loop);

// Clear all channels (called when a new SCM starts).
void reset();

}  // namespace Audio
