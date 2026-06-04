#include "Theme.h"
#include "Audio.h"
#include "Log.h"
#include <cstring>
#include <strings.h>

namespace {

// Bounds-checked little-endian reader over the type-12 blob.
struct Rd {
    const uint8_t* p;
    const uint8_t* end;
    bool ok = true;

    uint32_t u32() {
        if (p + 4 > end) { ok = false; return 0; }
        uint32_t v = (uint32_t)p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
        p += 4;
        return v;
    }
    // A theme string: [u32 len][len bytes]. The blob keeps a trailing NUL inside
    // the length, so trim at the first NUL.
    std::string str() {
        uint32_t l = u32();
        if (!ok || p + l > end) { ok = false; return {}; }
        std::string s((const char*)p, l);
        p += l;
        size_t z = s.find('\0');
        if (z != std::string::npos) { s.resize(z); }
        return s;
    }
};

bool readStringTable(Rd& rd, std::vector<std::string>& out) {
    uint32_t n = rd.u32();
    if (!rd.ok) { return false; }
    out.reserve(n);
    for (uint32_t i = 0; i < n && rd.ok; ++i) { out.push_back(rd.str()); }
    return rd.ok;
}

}  // namespace

bool ThemeFile::load(ResArchive& arc, const char* name) {
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {
        if (en.type == 12 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (e == nullptr) { Log::warn("Theme: '%s' (type 12) not found", name); return false; }

    std::vector<uint8_t> blob = arc.read(*e);
    Rd rd{ blob.data(), blob.data() + blob.size() };

    // Tables, in load order (Thm_LoadTheme / Thm_ReadTable).
    if (!readStringTable(rd, segNames))    { Log::warn("Theme '%s': bad segNames", name); return false; }
    if (!readStringTable(rd, themeNames))  { Log::warn("Theme '%s': bad themeNames", name); return false; }
    if (!readStringTable(rd, labels))      { Log::warn("Theme '%s': bad labels", name); return false; }

    // One command-table offset per label (the jump/loop target).
    labelOffsets.resize(labels.size());
    for (size_t i = 0; i < labels.size() && rd.ok; ++i) { labelOffsets[i] = (int)rd.u32(); }

    if (!readStringTable(rd, importLabels)) { Log::warn("Theme '%s': bad importLabels", name); return false; }
    if (!readStringTable(rd, eventNames))   { Log::warn("Theme '%s': bad eventNames", name); return false; }

    uint32_t nCmd = rd.u32();
    if (rd.ok) { commands.resize(nCmd); }
    for (uint32_t i = 0; i < nCmd && rd.ok; ++i) {
        Command& c = commands[i];
        c.type     = (int)rd.u32();
        c.arg      = (int)rd.u32();
        c.cnt      = (int)rd.u32();
        c.evtStart = (int)rd.u32();
        c.evtEnd   = (int)rd.u32();
    }

    uint32_t nEvt = rd.u32();
    if (rd.ok) { events.resize(nEvt); }
    for (uint32_t i = 0; i < nEvt && rd.ok; ++i) {
        Event& ev = events[i];
        ev.transMode = (int)rd.u32();
        ev.nameIdx   = (int)rd.u32();
        ev.themeIdx  = (int)rd.u32();
        ev.extIdx    = (int)rd.u32();
    }

    if (!rd.ok) { Log::warn("Theme '%s': truncated", name); return false; }
    Log::info("Theme '%s': %zu seg, %zu theme, %zu label, %zu imp, %zu evtName, %zu cmd, %zu evt",
              name, segNames.size(), themeNames.size(), labels.size(),
              importLabels.size(), eventNames.size(), commands.size(), events.size());
    return true;
}

int ThemeFile::labelOffset(const char* label) const {
    for (size_t i = 0; i < labels.size(); ++i) {
        if (strcasecmp(labels[i].c_str(), label) == 0) {
            return (i < labelOffsets.size()) ? labelOffsets[i] : -1;
        }
    }
    return -1;
}

// ===========================================================================
// Sequencer + control. The engine sequences cues on a background thread that
// keeps a 6-deep lookahead "stack" full and preloads PCM into a ring pool. With
// our synchronous Audio::queue (which copies+converts PCM on enqueue) that whole
// pipeline collapses: we walk the command table directly and stream one cue at a
// time whenever the THEME channel runs low (Theme::advance).
// ===========================================================================
namespace {

ResArchive* g_arc = nullptr;
bool g_ready = false;

ThemeFile   g_theme;                 // currently loaded track
bool        g_active = false;        // music playing (g_nThemeMusicActive)
int         g_room = 0;              // g_nThemeCurrentRoom (0=none, 1=same sentinel)
std::string g_currentTrack;          // g_szThemeCurrentTrack
std::string g_nextTrack;             // g_szThemeNextTrack (idle baseline)
int         g_volume = 64;           // 0..64
bool        g_dryRun = false;        // debugWalk: log the cue sequence, skip audio

// Sequencer cursor over the command table. PushSegOp pre-increments, so -1 = top.
int g_cursor = -1;
int g_repeatSeg = 0;                 // active type-1 multi-play segment...
int g_repeatLeft = 0;                // ...and remaining repeats

// Fade-out: linear gain ramp over a number of advance() frames, then stop.
int g_fadeFrames = 0;                // remaining frames (0 = not fading)
int g_fadeTotal  = 0;

// Stream ahead by ~this many S16 samples (≈1s at 22050) before fetching the next.
constexpr size_t kLowWaterSamples = 22050;

// Apply the current volume (0..64) to the THEME channel, scaled by any fade.
void applyGain() {
    float g = (float)g_volume / 64.0f;
    if (g_fadeTotal > 0) { g *= (float)g_fadeFrames / (float)g_fadeTotal; }
    Audio::setChannelGain(Audio::THEME, g);
}

// Load segment `segIdx`'s PCM and append it to the THEME channel. The cue is a
// resource named segNames[segIdx]; SegToMem probes codec types 0x27..0x20
// (format = type & 7) then raw type 0x0d (format 0).
bool queueSegment(int segIdx) {
    if (segIdx < 0 || segIdx >= (int)g_theme.segNames.size() || g_arc == nullptr) { return false; }
    const std::string& nm = g_theme.segNames[segIdx];
    if (g_dryRun) { Log::info("  PLAY seg %d '%s'", segIdx, nm.c_str()); return true; }

    const ResEntry* found = nullptr;
    int fmt = 0;
    for (const auto& en : g_arc->entries()) {
        if (strcasecmp(en.name.c_str(), nm.c_str()) != 0) { continue; }
        if (en.type >= 0x20 && en.type <= 0x27) {        // prefer the codec-tagged cue
            if (found == nullptr || en.type > found->type) { found = &en; fmt = en.type & 7; }
        } else if (en.type == 0x0d && found == nullptr) { // raw PCM fallback
            found = &en; fmt = 0;
        }
    }
    if (found == nullptr) { Log::warn("Theme: cue '%s' not found", nm.c_str()); return false; }

    std::vector<uint8_t> pcm = g_arc->read(*found);
    Audio::queue(Audio::THEME, pcm.data(), pcm.size(), fmt);
    return true;
}

// Append `ms` of silence (8-bit-unsigned mono 22050 = 22050 bytes/s, 0x80).
void queueSilence(int ms) {
    if (ms <= 0) { return; }
    size_t bytes = (size_t)ms * 22050 / 1000;
    std::vector<uint8_t> sil(bytes, 0x80);
    Audio::queue(Audio::THEME, sil.data(), sil.size(), 0);
}

// Walk the command table and stream exactly one cue (or silence). Non-playback
// commands (set-volume, loop-back, set-fade) are applied as side effects and the
// walk continues. Returns false when the music stops (end-of-table / STOP).
// Mirrors Theme_PushSegOp's command decode + Theme_PrepNextSeg's skip loop.
bool produceOne() {
    for (int guard = 0; guard <= (int)g_theme.commands.size() + 4; ++guard) {
        if (g_repeatLeft > 0) {                 // continuing a multi-play
            --g_repeatLeft;
            return queueSegment(g_repeatSeg);
        }
        ++g_cursor;                             // PushSegOp pre-increments
        if (g_cursor < 0 || g_cursor >= (int)g_theme.commands.size()) {
            Theme::stopMusic();                 // end of command table
            return false;
        }
        const ThemeFile::Command& c = g_theme.commands[g_cursor];
        switch (c.type) {
        case 1:                                 // multi-segment play: seg arg, cnt times
            if (c.cnt <= 0) { break; }
            g_repeatSeg = c.arg;
            g_repeatLeft = c.cnt - 1;
            return queueSegment(c.arg);
        case 2:                                 // set volume (Theme_SetVolume)
            if (g_dryRun) { Log::info("  SETVOL %d", c.arg); }
            Theme::setVolume(c.arg);
            break;
        case 3: {                               // branch on dot in the cue name
            const std::string& nm = (c.arg >= 0 && c.arg < (int)g_theme.segNames.size())
                                        ? g_theme.segNames[c.arg] : std::string();
            if (nm.find('.') == std::string::npos) {     // no extension -> loop back
                int tgt = (c.arg >= 0 && c.arg < (int)g_theme.labelOffsets.size())
                              ? g_theme.labelOffsets[c.arg] : -1;
                if (tgt < 0) { Theme::stopMusic(); return false; }
                if (g_dryRun) { Log::info("  LOOP -> cmd %d", tgt); }
                g_cursor = tgt - 1;
            } else {                                     // extension -> sub-theme switch
                std::string base = nm.substr(0, nm.find('.'));
                std::string ext  = nm.substr(nm.find('.') + 1);
                Theme::play(base.c_str(), ext.c_str());
                return g_active;
            }
            break;
        }
        case 4:                                 // timed silence (ms)
            queueSilence(c.arg);
            return true;
        case 5:                                 // stop music
            Theme::stopMusic();
            return false;
        case 6:                                 // set fade duration (advisory)
            break;
        default:
            break;
        }
    }
    Log::warn("Theme: command walk made no progress (malformed loop?)");
    Theme::stopMusic();
    return false;
}

}  // namespace

namespace Theme {

void init(ResArchive& arc) {
    g_arc = &arc;
    g_ready = true;
    g_room = 0;
    g_active = false;
    g_currentTrack.clear();
    g_nextTrack.clear();
    g_volume = 64;
}

bool ready() { return g_ready; }

void setVolume(int v) {
    if (v < 0) { v = 0; }
    if (v > 64) { v = 64; }
    g_volume = v;
    applyGain();
}

int getVolume() { return g_volume; }

void play(const char* track, const char* label) {
    if (!g_ready || track == nullptr) { return; }
    if (!g_theme.load(*g_arc, track)) { return; }
    g_currentTrack = track;

    Audio::clearChannel(Audio::THEME);          // stop whatever was playing
    g_repeatLeft = 0;
    g_fadeFrames = g_fadeTotal = 0;
    applyGain();

    int off = (label != nullptr && *label) ? g_theme.labelOffset(label) : -1;
    g_cursor = (off >= 0) ? off - 1 : -1;       // -1 = top of command table

    // Thm_Play only begins streaming once a room is active (the room gate).
    g_active = (g_room != 0);
}

void stopMusic() {
    g_active = false;
    g_currentTrack = g_nextTrack;               // engine's Current = Next on stop
    g_fadeFrames = g_fadeTotal = 0;
    Audio::clearChannel(Audio::THEME);
}

void restartCurrentTrack() {
    if (strcasecmp(g_currentTrack.c_str(), g_nextTrack.c_str()) != 0) {
        play(g_currentTrack.c_str(), nullptr);
    }
}

void setRoom(int roomId) {
    if (!g_ready) { return; }
    if (g_room == 0) {
        if (roomId != 0) { g_room = roomId; restartCurrentTrack(); }
    } else if (roomId != 1) {
        g_room = roomId;
        stopMusic();
    }
}

int getRoom() { return g_room; }

void fadeOut(int ms) {
    if (!g_active) { return; }
    g_fadeTotal = g_fadeFrames = (ms > 0) ? (ms / 33 + 1) : 1;   // ~30 fps frames
}

bool isFading() { return g_fadeTotal > 0; }

void advance() {
    if (!g_ready || !g_active) { return; }

    if (g_fadeTotal > 0) {                       // service an in-progress fade
        applyGain();
        if (--g_fadeFrames <= 0) { stopMusic(); return; }
    }

    int guard = 0;
    while (g_active && Audio::queuedSamples(Audio::THEME) < kLowWaterSamples) {
        if (!produceOne()) { break; }
        if (++guard > 256) { break; }            // never spin forever in one frame
    }
}

void debugWalk(int steps) {
    g_dryRun = true;
    for (int i = 0; i < steps && g_active; ++i) {
        if (!produceOne()) { Log::info("  STOP"); break; }
    }
    g_dryRun = false;
}

}  // namespace Theme
