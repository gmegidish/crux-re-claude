// Theme.h — room-music streaming subsystem (clean-room port of THEMES.cpp + the
// Thm_* helpers in TEXT.cpp).
//
// Music is streaming PCM, not MIDI. A "theme" is a type-12 (0xc) resource holding
// a small music program: a table of PCM segment-cue names (type-13 resources, raw
// 8-bit-unsigned mono @ 22050 Hz) plus a command/event bytecode the sequencer
// walks to decide which cue plays next. Theme_SetRoom gates whether the current
// track runs; the track identity is a theme name set by Thm_Play.
//
// The original uses a background thread + two Win32 events + critical sections to
// stay ahead of the soundcard. Under our SDL2 callback mixer that collapses to a
// single polled advance() plus two booleans (segFinished / execCommand).
#pragma once
#include "ResArchive.h"
#include <cstdint>
#include <string>
#include <vector>

// A parsed type-12 theme file (the per-track music program).
struct ThemeFile {
    // 20-byte command record (compiled into seg-ops by the sequencer).
    struct Command {
        int type;               // 1..6
        int arg;                // segment-name index / duration ms / ...
        int cnt;                // repeat count (type 1)
        int evtStart, evtEnd;   // event-table range owned by this command
    };
    // 16-byte event record (a named music transition).
    struct Event {
        int transMode;          // 1,2,3,4,5,7
        int nameIdx;            // event-name index
        int themeIdx;           // theme-name index (-1 = keep current track)
        int extIdx;             // label / import-label index (-1 = none)
    };

    std::vector<std::string> segNames;       // PCM cue names (type-13)
    std::vector<std::string> themeNames;     // event target track names
    std::vector<std::string> labels;         // entry-point labels (e.g. start, loop)
    std::vector<int>         labelOffsets;   // command index each label jumps to
    std::vector<std::string> importLabels;   // extension / variant names
    std::vector<std::string> eventNames;     // names scripts fire via MusicEvent
    std::vector<Command>     commands;
    std::vector<Event>       events;

    // Parse the type-12 resource `name`. Returns false if missing/malformed.
    bool load(ResArchive& arc, const char* name);

    // Command index for a label name (-1 if not found); used as the play cursor.
    int labelOffset(const char* label) const;
};

namespace Theme {

// Mark the system ready and remember the archive to load cues from (mirrors
// Theme_Init setting g_nThmReady=1; the background thread/pool are replaced by
// the polled advance()).
void init(ResArchive& arc);
bool ready();

// --- Script-facing control (the engine's Theme_*/Thm_* surface) ---

// Theme_SetRoom (op 0x84e): gate/cut. roomId 0 = none, 1 = "same room" sentinel.
// First non-zero room restarts the current track; any later change stops music.
void setRoom(int roomId);
int  getRoom();

// Thm_Play (op 0x14): load theme `track`, reset the sequencer to `label` (or the
// top when null), and begin streaming when a room is active.
void play(const char* track, const char* label);

// Theme_MusicEvent (op 0x16c, THEMES.cpp @0x00479c20): fire a named music-transition
// event against the current track. The engine looks the name up in the track's
// event table and, per the matched record's transition mode, switches/queues a new
// segment or target track. Returns early if the system isn't ready.
void musicEvent(const char* eventName);

// Theme_StopMusic (op 0x1a): stop playback and reset the current track to the
// idle baseline.
void stopMusic();

// Theme_RestartCurrentTrack: replay the current track if it differs from the
// idle baseline.
void restartCurrentTrack();

void setVolume(int v);     // Theme_SetVolume, 0..64
int  getVolume();
void fadeOut(int ms);      // Theme_FadeOut: ramp to silence over ms, then stop
bool isFading();           // Theme_IsFading: a fade-out is in progress

// Per-frame pump: keep the THEME audio channel fed by streaming the next cue(s)
// when it runs low, and service fades. Replaces the engine's background thread.
void advance();

// Debug: dry-walk the current track's command sequence, logging each cue / volume
// / loop without touching audio. For verifying the sequencer.
void debugWalk(int steps);

}  // namespace Theme
