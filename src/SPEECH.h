#pragma once
// SPEECH.cpp — Subtitle / voiced dialogue subsystem
//
// Manages:
//   - Loading sentence text + duration tables from SENTENCE.BIN
//   - INI-driven subtitle font configuration (face, size, family, align, shadow)
//   - An offscreen DirectDraw surface for compositing subtitles
//   - Per-channel played/skipped state tracking for voiced dialogue
//
// Public API called by Gran_DiaryPlay (Graninv.cpp):
//   Speech_SetTag(nSentId, nChannel)   — queue a sentence for playback
//   Speech_Play()                      — commit the queued sentence (marks interrupted)
//   Speech_Commit()                    — finalise; records played/skipped state
//
// Original source: C:\DevStudio\Projects\Crux\SPEECH.cpp

#include <windows.h>

// ============================================================
//  Globals (defined in SPEECH.cpp)
// ============================================================

extern int   g_nSpeechSentence;      // 007c91cc — current sentence handle
extern int   g_nSpeechPos;           // 007c91c8 — playback position/cursor (-1 = reset)
extern int   g_nSpeechPendingId;     // 004da0a0 — queued sentence ID (-1 = none)
extern int   g_nSpeechChannel;       // 004da0a4 — channel/slot for pending sentence
extern int   g_nSpeechInterrupted;   // 007c91e0 — interrupt flag (1 = skipped by Speech_Play)
extern int   g_anSpeechPlayed;       // 0070fa38 — per-channel played state table (0=skipped, 1=played)
extern int   g_nSpeechCount;         // 007cc7d8 — total sentences loaded from SENTENCE.BIN
extern int * g_pSpeechTexts;         // 007c93bc — array of char* sentence text strings
extern int * g_pSpeechDurations;     // 007ca78c — array of sentence duration values
extern int   g_nSpeechInitialized;   // 007cd7c4 — 1 after Speech_Init succeeds
extern int   g_nSpeechCharset;       // 007ca874 — GDI font charset (from INI [Text] Charset)
extern int   g_nSpeechAlign;         // 007ca878 — text alignment 0=left 1=center 2=right
extern int   g_nSpeechFontSize;      // 004da16c — font point size (default 18)
extern uint  g_dwSpeechFontWeight;   // 007cd7c8 — font weight flags (INI value << 17)
extern int   g_nSpeechFontFamily;    // 007cd7cc — GDI FF_* constant for subtitle font
extern char *g_szSpeechFontFace;     // 007ca368 — font face name string (32 bytes)
extern int   g_nSpeechShadow;        // 007cd7b0 — 1 if text shadow is enabled
extern int   g_nSpeechSurface;       // 007ca360 — DirectDraw offscreen surface handle
extern int   g_nSpeechColorTable;    // 007ca394 — colour table pointer for subtitle rendering
extern int   g_nSpeechSubtitleY;     // 007ca884 — Y scanline for subtitle bar (0xb4 = 180)
extern int   g_nSpeechDisplayTimer;  // 007cd7c0 — subtitle display countdown / frame counter
extern int   g_nSpeechCurDisplay;    // 007cc7f0 — currently displayed sentence index (-1 = none)

// ============================================================
//  Functions
// ============================================================

// Speech_Init — load SENTENCE.BIN, configure subtitle font, create offscreen surface.
void Speech_Init(void);

// Speech_SetSentence — store a sentence handle (used by the subtitle display system).
void Speech_SetSentence(int nSentence);

// Speech_GetSentence — return the current stored sentence handle.
int  Speech_GetSentence(void);

// Speech_ResetPos — reset speech playback position to -1 (seek to start).
void Speech_ResetPos(void);

// Speech_SetTag — queue a sentence ID + channel for the next playback event.
void Speech_SetTag(int nSentId, int nChannel);

// Speech_Commit — finalise a queued sentence: records played/skipped state in
//   g_anSpeechPlayed[], fires the associated timer/trigger, resets pending state.
void Speech_Commit(void);

// Speech_Play — play the queued sentence (called from Gran_DiaryPlay).
//   Sets the interrupt flag then calls Speech_Commit to do the real work.
void Speech_Play(void);
