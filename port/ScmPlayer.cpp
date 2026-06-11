#include "ScmPlayer.h"
#include "Scm.h"
#include "Sprite.h"
#include "Audio.h"
#include "Sentence.h"
#include "TextRender.h"
#include "Log.h"
#include <SDL.h>
#include <strings.h>   // strcasecmp
#include <cstdlib>     // getenv
#include <string>
#include <cstring>

// Apply a 0x02 palette chunk: [u8 startIdx][u8 pad][ (size-2) bytes 6-bit RGB ].
static void applyPalette(Framebuffer& fb, const uint8_t* d, uint32_t size) {
    if (size < 2) return;
    int start = d[0];
    int n = (int)(size - 2) / 3;                 // RGB triples
    auto exp6 = [](uint8_t v){ v &= 0x3f; return (uint8_t)((v << 2) | (v >> 4)); };
    for (int i = 0; i < n && start + i < 256; ++i)
        fb.setPaletteEntry(start + i, exp6(d[2 + i*3]), exp6(d[2 + i*3 + 1]), exp6(d[2 + i*3 + 2]));
}

bool playScmByName(ResArchive& arc, Display& disp, Framebuffer& fb, const char* name) {
    // Resolve the type-16 entry (names repeat across types).
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries())
        if (en.type == 16 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    if (!e) { Log::warn("SCM '%s' (type 16) not found", name); return true; }

    Scm scm;
    if (!scm.parse(arc.read(*e))) { Log::error("SCM '%s': parse failed", name); return true; }

    int rate = scm.rate() > 0 ? scm.rate() : 9;
    int delayMs = 1000 / (rate > 0 ? rate : 9);
    Log::info("play SCM %s: %zu frames @ ~%d fps", name, scm.frames(), rate);

    // Audio only when we're pacing to real time (a real window). In turbo/headless
    // we render flat-out, so queued audio would have no relation to the visuals.
    const bool wantAudio = disp.isRealtime() && Audio::isOpen();
    int audioChunks = 0; size_t audioBytes = 0;
    if (wantAudio) Audio::reset();   // start clean (the previous clip already drained + reset)
    const Uint32 startTicks = SDL_GetTicks();

    std::string subtitle;   // current subtitle (CP1255), set by 0x1000 cues, drawn each frame
    Framebuffer composite;  // scratch: video frame + subtitle overlay (keeps fb clean)
    bool skipped = false;   // user skipped → cut immediately, don't wait for the audio tail

    for (size_t f = 0; f < scm.frames(); ++f) {
        const auto& fr = scm.frame(f);
        for (uint32_t c = 0; c < fr.count; ++c) {
            const auto& ch = scm.chunk(fr.first + c);
            const uint8_t* d = scm.payload(ch);
            // The engine derives the audio format from (chunk param >> 8):
            // bit0=16-bit, bit1=stereo, bit2=44100 (Player_RenderFrame/StartMusicLoop).
            const int fmt = ch.param >> 8;
            if (ch.type == Scm::PALETTE)    applyPalette(fb, d, ch.size);
            else if (ch.type == Scm::VIDEO) decodeSprite(d, ch.size, fb);
            else if (wantAudio && ch.type >= 0x40 && ch.type <= 0x43) {
                Audio::queue(Audio::MUSIC, d, ch.size, fmt);   // music
                ++audioChunks; audioBytes += ch.size;
            }
            else if (wantAudio && ch.type >= 0x80 && ch.type <= 0x83) {
                // Bunch-audio voices are CONCURRENT: the engine plays each on its own
                // mixer channel (id = chunk param low byte, 0..2), summed. Routing
                // them all to one channel would serialize them -> growing delay.
                int voice = ch.param & 0xff;
                if (voice > 2) { voice = 2; }
                Audio::queue(Audio::VOICE0 + voice, d, ch.size, fmt);
                ++audioChunks; audioBytes += ch.size;
            }
            else if (ch.type == Scm::TEXT) {
                // Subtitle cue: payload starts with a NUL-terminated SENTENCE.BIN key
                // (e.g. "3372"). Look it up and show it until the next cue / video end.
                std::string key;
                for (uint32_t i = 0; i < ch.size && d[i] != 0; ++i) { key += (char)d[i]; }
                const std::string* heb = key.empty() ? nullptr : Sentence::lookup(key.c_str());
                subtitle = heb ? *heb : std::string();
            }
            // 0x100 speech markers: music/lipsync control (handled elsewhere)
        }

        // Composite the subtitle onto a scratch buffer, leaving the persistent
        // (delta-coded) video frame `fb` clean. Drawing onto fb directly would let
        // later video deltas overwrite the subtitle pixels, and a changed/cleared
        // line couldn't be erased (it'd be baked into fb).
        Framebuffer* shown = &fb;
        if (TextRender::ready() && !subtitle.empty()) {
            std::memcpy(composite.pixels(), fb.pixels(), (size_t)fb.width() * fb.height());
            composite.setPaletteRGB(fb.palette());
            TextRender::drawSentence(composite, subtitle, composite.width() / 2, composite.height() - 24);
            shown = &composite;
        }
        disp.present(*shown);

        // Headless verification: dump the LAST frame of each SCM when SCM_DUMP is set.
        if (std::getenv("SCM_DUMP") && f + 1 == scm.frames()) {
            char path[80];
            std::snprintf(path, sizeof path, "%s_mid.ppm", name);
            shown->savePPM(path);
        }

        if (!disp.isHeadless()) {
            PumpResult pr = disp.pump();
            if (pr == PumpResult::Quit) return false;    // window closed → quit program
            if (pr == PumpResult::Skip) { skipped = true; break; }   // key/click → skip this video
            if (disp.isRealtime()) {
                // Pace to a wall-clock target so video stays locked to the audio
                // (which plays at its own real-time rate), instead of drifting by
                // however long each frame's decode took.
                Uint32 target = startTicks + (Uint32)((f + 1) * delayMs);
                Uint32 now = SDL_GetTicks();
                if (now < target) SDL_Delay(target - now);
            }
        }
    }
    Log::info("SCM %s: queued %d audio chunk(s), %zu bytes", name, audioChunks, audioBytes);
    // A clip's audio can be much longer than its video (e.g. VVKBLACK: 0.5s video / ~27s of
    // narration). Hold on the last frame until this clip's audio finishes before moving on,
    // so the next clip doesn't cut or overlap it. Then flush so the next clip starts clean.
    // (A skipped clip cuts immediately — no tail.)
    if (wantAudio && !skipped) {
        while (Audio::queuedSamples(Audio::MUSIC)  > 0 || Audio::queuedSamples(Audio::VOICE0) > 0 ||
               Audio::queuedSamples(Audio::VOICE1) > 0 || Audio::queuedSamples(Audio::VOICE2) > 0) {
            PumpResult pr = disp.pump();
            if (pr == PumpResult::Quit) return false;
            if (pr == PumpResult::Skip) break;             // user skips the audio tail too
            SDL_Delay(33);
        }
    }
    if (wantAudio) Audio::reset();   // clip + its audio done → clean slate for the next clip
    return true;
}
