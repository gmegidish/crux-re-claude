#pragma once
// SOUNDMEM.cpp — Sound sample cache + speech/lipsync subsystem
//
// Two distinct subsystems share this translation unit:
//
// 1. SOUND CACHE (SndMem_*cache* group, 0x00472810–0x00473de0)
//    A flat 200-slot PCM cache backed by a single contiguous memory pool
//    (g_nSndMemPoolBase / g_nSndMemPoolSize).  Each slot holds:
//      - a 9-char normalised name key (g_abSndMemSlotNames[slot][9])
//      - a descriptor record in g_abSndMemSlotTable (33 bytes/slot):
//          +0x00 PCM ptr (int)      — g_nSndMemPoolBase + offset
//          +0x04 task handle (int)  — async Res_BunchFreadStreamLoadPtr handle
//          +0x10 file size (int)    — byte length of PCM data
//          +0x14 alloc size (int)   — allocated bytes in pool
//          +0x1c format flags (uint)— bit0=mono/stereo, bit1=8/16bit, bit2=stereo
//          +0x20 state byte (char)  — 0=empty, 1=in-use, -1=cancelled/dead
//      - an age counter in g_anSndMemSlotAge[slot] for LRU eviction
//    SndMem_Load is the main entry: looks up name in cache, allocates a slot
//    via SndMem_AllocSlot, async-loads via Res_BunchFreadStreamLoadPtr.
//    SndMem_Compact defragments the slot table after frees.
//    SndMem_UpdateAnim bridges the cache to the character lipsync: when
//    g_nSpeechSentence==2 it calls SndMem_GetLipsyncByte to read the current
//    phoneme and passes it to the character animation system.
//
// 2. SPEECH / LIPSYNC (SndMem_*speech*/*lipsync* group, 0x00473ed0–0x00474aa0)
//    Drives voiced dialogue playback with per-frame mouth animation.
//    - A lipsync byte stream (g_abLipsyncData, resource type 0x11) is loaded
//      alongside the audio waveform when a sentence begins.
//    - g_nLipsyncPos is an atomic cursor advanced by SndMem_AdvanceLipsync
//      each frame; SndMem_GetLipsyncByte returns the current phoneme byte
//      (or -1 if cursor is at -1 = stopped).
//    - SndMem_SetSpeechAnim(charIdx, param) attaches a lipsync animation
//      sprite to a character slot, sets g_nSpeechSentence=2.
//    - SndMem_StartSpeech(sentId) is the main "speak" call: loads phoneme
//      data, routes audio via thunk_FUN_0046f7f0 (speech channel play) or
//      thunk_FUN_0046f730 (panned channel play), sets subtitle text via
//      Txt_SetString.
//    - SndMem_WaitSpeech / SndMem_IsSpeaking / SndMem_SpeakAndWait provide
//      synchronous-wait wrappers.
//
// NOTE on 0x0046f7f0:
//   This address is OUTSIDE the SOUNDMEM range and is NOT one of the 19 functions
//   reversed here.  It falls in the 0x0046f000–0x00472000 area — a separate
//   module (likely SCHED.cpp or a speech-channel play stub).  THEMES.cpp stubs
//   Thm_Play / Thm_PlayNextSegment / Thm_FindLabel also resolve OUTSIDE this
//   range (they are in that same 0x0046xxxx zone).  Those three THEMES stubs
//   do NOT resolve here.
//
// Original source: C:\DevStudio\Projects\Crux\SOUNDMEM.cpp
// (inferred from debug strings: "C:\DevStudio\Projects\Crux\SOUND")

#include <windows.h>

// ============================================================
//  Globals (defined in SOUNDMEM.cpp)
// ============================================================

// -- Sound cache pool --
extern int  g_nSndMemPoolBase;      // 0x007c5d90  PCM pool base address
extern int  g_nSndMemPoolSize;      // 0x007c5d94  PCM pool total size in bytes
extern int  g_nSndMemSlotCount;     // 0x007c5d98  active slot high-water mark (0..199)
extern char g_abSndMemSlotNames[];  // 0x007c5da0  200 x 9-char name table
extern char g_abSndMemSlotTable[];  // 0x007c67d0  200 x 33-byte slot descriptor table
extern int  g_anSndMemSlotAge[];    // 0x007c64a8  int[200] LRU age counters

// -- Speech enabled gate --
extern int  g_nSndMemSpeechEnabled; // 0x004d9eec  0 = speech disallowed

// -- Lipsync state (protected by g_nLipsyncCS) --
extern char g_abLipsyncData[];      // 0x007c89f8  phoneme byte stream (up to 256 bytes)
extern int  g_nLipsyncCS;           // 0x007c89e0  CRITICAL_SECTION (24 bytes)
extern int  g_nLipsyncLen;          // 0x007c91d0  length of phoneme stream
extern int  g_nLipsyncActive;       // 0x007c91d4  1 = phoneme data loaded
extern int  g_nLipsyncPos;          // 0x007c91d8  current cursor (-1 = stopped)
extern int  g_nLipsyncAnimPending;  // 0x007c91dc  1 = new speech anim just attached

// ============================================================
//  Sound cache API
// ============================================================

// SndMem_Init — zero all 200 slot descriptors, names, and age counters.
//   Sets all slot states to -1 (cancelled/dead) and resets the slot count.
void SndMem_Init(void);

// SndMem_Reset — full reset: clear slot names to empty, set all states to -1,
//   reset slot count. Lighter than Init (does not memset descriptor table).
void SndMem_Reset(void);

// SndMem_Load — load a named PCM sound into the cache.
//   param_1 = resource name (will be normalised to uppercase)
//   param_2 = pointer to receive PCM data pointer (filled on success)
//   param_3 = async priority / load context
//   param_4 = pointer to receive format flags (bit0=mono, bit1=stereo, bit2=22kHz)
//             if NULL, uses a local variable.
//   Returns PCM data pointer (same as *param_2) on success, 0 on failure.
//   Side effect: increments age counter on all existing slots each call.
int SndMem_Load(char* pszName, int* pPcmPtr, int nPriority, unsigned int* pFmtFlags);

// SndMem_AllocSlot — find or evict a cache slot large enough for nBytes bytes.
//   param_1 = required byte count
//   param_2 = resource name (for slot naming)
//   param_3 = 0 = allow evicting in-use slots; non-0 = only evict free slots
//   Returns slot index (0..199) on success, -1 if none available.
int SndMem_AllocSlot(int nBytes, void* pszName, char bExactOnly);

// SndMem_SetSlotState — set the state byte for slot nSlot.
//   param_2: 0=empty, 1=in-use, -1=cancelled
//   If old state was > 0 (busy), calls Res_CancelFrameTasks first.
void SndMem_SetSlotState(int nSlot, char bState);

// SndMem_Compact — defragment the slot table.
//   Trims dead slots off the tail, merges contiguous free regions,
//   promotes any slot whose data has not been fully consumed.
void SndMem_Compact(void);

// SndMem_Free — release a named sound from the cache.
//   Searches by name, sets state=0 (empty), then calls SndMem_Compact.
void SndMem_Free(char* pszName);

// SndMem_UpdateAnim — per-frame lipsync-to-animation bridge.
//   If g_nSpeechSentence==2: calls SndMem_GetLipsyncByte to read the current
//   phoneme, then passes it to the character animation system via
//   thunk_FUN_00407e80(charIdx, phoneme, animHi, animLo).
void SndMem_UpdateAnim(int nCharIdx);

// ============================================================
//  Speech / lipsync API
// ============================================================

// SndMem_InitLipsync — one-time startup: zero lipsync counters,
//   InitializeCriticalSection(&g_nLipsyncCS).
void SndMem_InitLipsync(void);

// SndMem_SetSpeechEnabled — enable or disable voiced speech output.
//   Stores param_1 in g_nSndMemSpeechEnabled (0 = disallowed).
void SndMem_SetSpeechEnabled(int bEnabled);

// SndMem_SetSpeechAnim — attach a lipsync animation to a character.
//   param_1 = character slot index (into g_pCharWalkTable / DAT_0070c24c)
//   param_2 = animation priority or handle
//   Acquires a free animation slot for the character's sprite, stores it in
//   g_nSpeechPos, sets g_nSpeechSentence=2, sets g_nLipsyncAnimPending=1.
//   Debug string: "spk_set_ani  int ptr  int pri"
void SndMem_SetSpeechAnim(int nCharIdx, int nPriority);

// SndMem_StartSpeech — begin voiced dialogue playback for sentence nSentId.
//   - Checks g_nSndMemSpeechEnabled; logs "Speech_disallowed" if 0.
//   - If g_nLipsyncAnimPending: calls SndMem_StopLipsync; otherwise clears flag.
//   - Calls Speech_Play() + Speech_SetTag(-1, 0) + schedules Speech_Commit.
//   - Loads phoneme stream (resource type 0x11) into g_abLipsyncData.
//   - Calls thunk_FUN_00472340 (SndMem_Load variant) to load audio waveform.
//   - Calls SndMem_SetLipsyncPos(0) to start cursor.
//   - Routes audio to speech channel: panned via thunk_FUN_0046f730 if position
//     is known, else centred via thunk_FUN_0046f7f0.
//   - Sets subtitle text via Txt_SetString.
//   Debug string: "nwspeak  char* name  "
void SndMem_StartSpeech(int nSentId);

// SndMem_WaitSpeech — synchronous wait for speech to finish.
//   param_1: 0 = wait normally; non-0 = force-skip.
//   Polls SndMem_GetLipsyncByte / Txt_IsDone in a loop, dispatching
//   timer events. Returns 1 when speech is done, 0 otherwise.
int SndMem_WaitSpeech(int bForceSkip);

// SndMem_IsSpeaking — non-blocking query: is speech currently playing?
//   Returns 1 if lipsync or subtitle display is active, 0 if idle.
int SndMem_IsSpeaking(void);

// SndMem_SpeakAndWait — convenience: SndMem_StartSpeech + SndMem_WaitSpeech(0).
void SndMem_SpeakAndWait(int nSentId);

// SndMem_GetLipsyncByte — thread-safe read of current phoneme byte.
//   Returns (int)(char)g_abLipsyncData[g_nLipsyncPos], or -1 if g_nLipsyncPos==-1.
int SndMem_GetLipsyncByte(void);

// SndMem_StopLipsync — stop lipsync playback.
//   EnterCS; if g_nSpeechPos valid: stop character animation, g_nSpeechPos=-1.
//   Calls Speech_Play(); sets g_nLipsyncPos=-1. LeaveCS.
void SndMem_StopLipsync(void);

// SndMem_AdvanceLipsync — advance the lipsync cursor by one frame.
//   Thread-safe: EnterCS, g_nLipsyncPos++, wraps to -1 when >= g_nLipsyncLen.
void SndMem_AdvanceLipsync(void);

// SndMem_SetLipsyncPos — set the lipsync cursor to a specific position.
//   Thread-safe: EnterCS, g_nLipsyncPos = nPos. LeaveCS.
void SndMem_SetLipsyncPos(int nPos);
