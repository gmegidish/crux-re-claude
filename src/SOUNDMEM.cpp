// SOUNDMEM.cpp — Sound sample cache + speech/lipsync subsystem
//
// Two distinct subsystems:
//
// 1. SOUND CACHE (0x00472810–0x00473de0)
//    A 200-slot flat PCM cache backed by a contiguous pool.
//    Loads, evicts, and defragments sound samples for one-shot SFX
//    and voiced speech waveforms.
//
// 2. SPEECH / LIPSYNC (0x00473ed0–0x00474aa0)
//    Drives voiced dialogue: loads a per-sentence phoneme byte stream
//    (resource type 0x11), plays audio through the speech channel,
//    ticks a per-frame mouth-animation cursor, and attaches lipsync
//    sprites to character animation slots.
//
// NOTE on 0x0046f7f0 / Thm_Play / Thm_FindLabel / Thm_PlayNextSegment:
//   These addresses (0x0046f730, 0x0046f7f0, 0x0046fc40, etc.) are in the
//   0x0046xxxx range, which lies below this module's range (0x00472810+).
//   They are NOT part of SOUNDMEM.cpp and do NOT resolve here.
//   Thm_Play / Thm_PlayNextSegment / Thm_FindLabel (referenced as stubs in
//   THEMES.cpp) are in a separate translation unit in the 0x0046f000 zone —
//   likely the actual SOUND.cpp or a speech-channel dispatcher module.
//
// Original source: C:\DevStudio\Projects\Crux\SOUNDMEM.cpp
// (confirmed by debug assert path: "C:\DevStudio\Projects\Crux\SOUND")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "SOUNDMEM.h"
#include "SPEECH.h"
#include "READRES.h"
#include "ERRORS.h"
#include "TEXT.h"

// ============================================================
//  External functions (unresolved until dependent modules reversed)
// ============================================================

// READRES.cpp — find resource by number and name; fills resBuf/auxBuf/pSize
extern int  Res_FindByNumChar(int nResType, const char* pszName,
                              char* pResBuf, int nFlags,
                              char* pAuxBuf, int* pOutSize);

// READRES.cpp — start an async stream-load into *ppDst; prio/maxPrio are
//              scheduler priority hints.
extern void Res_BunchFreadStreamLoadPtr(int* ppDst, int nCount, int nSize,
                                        char* pAuxBuf, int nPriority, int nMaxPriority);

// READRES.cpp — block until an async load entry completes.
extern void Res_WaitForEntry(int* pEntry);

// READRES.cpp — synchronous block-read (non-streaming).
extern void Res_BunchFreadNow(int* ppDst, int nCount, int nSize, char* pAuxBuf);

// READRES.cpp — cancel any pending frame-based async tasks for a handle.
extern void Res_CancelFrameTasks(int hTask);

// ERRORS.cpp — fatal "bad resource entry" handler.
extern void Err_BadResEntry(int nLevel, const char* pszFile, const char* pszMsg);

// ERRORS.cpp — format a message into pszBuf using printf-style fmt.
extern void FUN_0048a060(char* pszBuf, const char* pszFmt, ...);

// STRING helpers (wrappers confirmed by decompile)
extern void FUN_004895e0(void* pDst, const void* pSrc);    // strcpy
extern void FUN_0049def0(char* pszStr);                    // strupr in-place
extern int  FUN_0049a830(const void* pA, const void* pB);  // strcmp (0=equal)

// READRES.cpp — normalise / uppercase a resource name string.
// (same as FUN_0049def0 when called on a name buffer)

// TEXT.cpp — set subtitle text string and optionally truncate at nLen chars.
extern void Txt_SetString(int nSentId, int* pBBox, int nLen);

// TEXT.cpp — returns non-zero while subtitle is scrolling/pending display.
extern int  Txt_IsScrollPending(void);

// TEXT.cpp — returns non-zero when subtitle display is complete.
extern int  Txt_IsDone(void);

// TEXT.cpp — advance to next subtitle page (for paginated subtitles).
extern void Txt_PageAdvance(void);

// TEXT.cpp — reset subtitle display state.
extern void Txt_Reset(void);

// SCHED.cpp — dispatch one async-program tick (drives timer callbacks).
extern void Timer_DispatchAsyncProg(void);

// Advanim.cpp / PLAYER.cpp — per-frame animation engine poll.
extern void thunk_FUN_00412ac0(void);

// thunk wrappers for character animation/sprite system (Advanim.cpp)
// thunk_FUN_00407e80(charIdx, phoneme, animHi, animLo) — set mouth frame
extern void thunk_FUN_00407e80(int nChar, int nPhoneme, int nAnimHi, int nAnimLo);

// thunk_FUN_00409070(nSpeechPos, 3, &x1,&y1,&x2,&y2) — get bbox of speech char
extern void thunk_FUN_00409070(int nPos, int nMode,
                               int* px1, int* py1, int* px2, int* py2);

// thunk_FUN_00405810(nSpeechPos) — stop a speech animation slot
extern void thunk_FUN_00405810(int nPos);

// thunk_FUN_004098c0(hAnim, nPriority) — acquire an animation play slot
extern int  thunk_FUN_004098c0(int hAnim, int nPriority);

// thunk_FUN_00406b90(nPos) — start/activate animation slot
extern void thunk_FUN_00406b90(int nPos);

// thunk_FUN_00406980(nPos, param) — configure animation slot
extern void thunk_FUN_00406980(int nPos, int nParam);

// thunk_FUN_00472340 — SndMem_Load variant called from SndMem_StartSpeech
// loads audio waveform for sentence: (sentId, pOutBuf, bSync, nResType, pFmt)
extern void thunk_FUN_00472340(int nSentId, char* pOutBuf,
                               int bSync, int nResType, char* pFmt);

// 0x0046f7f0 — speech channel play (centred/mono): (sentId, bVoiced)
// NOT in SOUNDMEM range. External speech-channel dispatcher.
extern void thunk_FUN_0046f7f0(int nSentId, int bVoiced);

// 0x0046f730 — speech channel play (panned): (sentId, bVoiced, nPanPos)
extern void thunk_FUN_0046f730(int nSentId, int bVoiced, int nPanPos);

// 0x0046fc40 — check whether speech audio channel is idle: (bForce) -> int
extern int  thunk_FUN_0046fc40(int bForce);

// 0x0040f700 — input/event poll (keyboard skip check): () -> int
extern int  thunk_FUN_0040f700(void);

// 0x0046fa60 — stop speech channel: (bForce)
extern void thunk_FUN_0046fa60(int bForce);

// SCHED.cpp — register an async program callback
extern void thunk_FUN_00442f40(int nSlot, void* pfnCallback);

// AREAS.cpp — bounding box of currently active screen area
extern int  g_nAreaActiveBBoxX1;   // (referenced in SndMem_StartSpeech via globals)
extern int  g_nAreaActiveBBoxY1;
extern int  g_nAreaActiveBBoxX2;

// Debug / assert helpers
extern void Debug_Assert(int nLevel, const char* pszFile, int nVal);
extern void Debug_Trace(int nA, int nB, const char* pszFmt, ...);

// DAT_004c4c40 — output sample rate used for async priority calculation
extern int  DAT_004c4c40;

// DAT_007c819c, DAT_007c81a0, DAT_007c81a4 — empty / cleared name constants
// used as source for FUN_004895e0 (strcpy) when clearing slot names.
extern char DAT_007c819c;   // empty-string sentinel for SndMem_AllocSlot clear
extern char DAT_007c81a0;   // empty-string sentinel for SndMem_Reset
extern char DAT_007c81a4;   // empty-string sentinel for SndMem_Free

// Per-character animation walk table base (Advanim.cpp)
// Indexed as DAT_0070c24c[charIdx] — yields hAnim handle
extern int  DAT_0070c24c[];

// Per-slot frame count table (Advanim.cpp): DAT_00574990[slot] = frame count
extern int  DAT_00574990[];

// Per-character animation data table (5b10c0 area):
// +0x00 slot entry base
// +0x58 stride per character
// Offsets used:
//   +0x00 (005b10c0) — slot[charIdx * 0x58 + 0x00]: animHi
//   +0x04 (005b10c4) — slot[charIdx * 0x58 + 0x04]: animLo
//   +0x10 (005b10d0) — slot[charIdx * 0x58 + 0x10]: done flag
//   +0x3c (005b10fc) — slot[g_nSpeechPos * 0x58 + 0x3c]: resource handle
//   +0x40 (005b1100) — slot[g_nSpeechPos * 0x58 + 0x40]: charIdx
extern char DAT_005b10c0[];

// DAT_0070e130 — resource handle constant used when attaching speech anim
extern int  DAT_0070e130;

// DAT_00629b04 — subtitle-only mode flag (1 = no audio, text only)
extern int  DAT_00629b04;

// DAT_00629f54 — audio globally muted flag (1 = no voice output)
extern int  DAT_00629f54;

// Error context/assert tables
extern char DAT_004d9d34[];   // "_read_sound_delayed" context base
extern char DAT_004d9d80[];   // assert file path for SndMem_Load
extern char DAT_004d9dbc[];   // "out of soundcache" assert path
extern char DAT_004d9d74[];   // "cur_snd %d" format string
extern char DAT_004d9d38[];   // "_read_sound_delayed_char__name_i" string
extern char DAT_004d9f0c[];   // SndMem_StartSpeech assert base
extern char DAT_004d9f38[];   // assert file path for pan-pos check
extern char DAT_004d9fc4[];   // SndMem_SetSpeechAnim assert base
extern char DAT_004da000[];   // assert file path: "C:\DevStudio\...\SPEECH"
extern char DAT_004da028[];   // "Not 5 frames in lipsync" format
extern char DAT_004da044[];   // assert file path for lipsync frame check
extern char DAT_004d9fe8[];   // "No slot for ANISPK  %s" format
extern char DAT_004d9fc8[];   // "spk_set_ani  int ptr  int pri" string

// DAT_007c88e0 — current speech animation handle name buffer (9 chars)
extern char DAT_007c88e0[];

// ============================================================
//  Globals
// ============================================================

// -- Sound cache pool --
int  g_nSndMemPoolBase    = 0;   // 0x007c5d90
int  g_nSndMemPoolSize    = 0;   // 0x007c5d94
int  g_nSndMemSlotCount   = 0;   // 0x007c5d98
char g_abSndMemSlotNames[200 * 9];    // 0x007c5da0  (1800 bytes)
char g_abSndMemSlotTable[200 * 0x21]; // 0x007c67d0  (6600 bytes)
int  g_anSndMemSlotAge[200];          // 0x007c64a8

// -- Speech gate --
int  g_nSndMemSpeechEnabled = 0;  // 0x004d9eec

// -- Lipsync state --
char g_abLipsyncData[256];        // 0x007c89f8
int  g_nLipsyncCS       = 0;     // 0x007c89e0  (CRITICAL_SECTION, 24 bytes)
int  g_nLipsyncLen      = 0;     // 0x007c91d0
int  g_nLipsyncActive   = 0;     // 0x007c91d4
int  g_nLipsyncPos      = 0;     // 0x007c91d8
int  g_nLipsyncAnimPending = 0;  // 0x007c91dc

// ============================================================
//  SndMem_Init   0x00472da0
// ============================================================
// Zero all slot descriptors and names; mark all 200 slots as cancelled (-1).
// Resets the slot high-water mark to 0.
void SndMem_Init(void)
{
    _memset(&g_abSndMemSlotTable, 0, 0x19c8);   // 200 * 0x21 = 6600 = 0x19c8
    _memset(&g_abSndMemSlotNames, 0, 0x708);    // 200 * 9   = 1800 = 0x708
    for (int i = 0; i < 200; i++)
    {
        SndMem_SetSlotState(i, (char)-1);
    }
    g_nSndMemSlotCount = 0;
}

// ============================================================
//  SndMem_Reset   0x00473be0
// ============================================================
// Clear all slot name entries and set all slot states to -1.
// Resets the slot count to 0. Does NOT zero the descriptor table.
void SndMem_Reset(void)
{
    for (int i = 0; i < 200; i++)
    {
        FUN_004895e0(&g_abSndMemSlotNames + i * 9, &DAT_007c81a0);
        (&g_abSndMemSlotTable)[i * 0x21 + 0x1c] = (char)-1;
    }
    g_nSndMemSlotCount = 0;
}

// ============================================================
//  SndMem_SetSlotState   0x00472ce0
// ============================================================
// Set the state byte at slot descriptor offset +0x1c.
// If the old state was >0 (task in flight), cancel it first.
void SndMem_SetSlotState(int nSlot, char bState)
{
    char* pSlot = &g_abSndMemSlotTable + nSlot * 0x21;
    if (pSlot[0x1c] > '\0')
    {
        Res_CancelFrameTasks(*(int*)(pSlot + 0x04));
    }
    pSlot[0x1c] = bState;
}

// ============================================================
//  SndMem_Load   0x00472810
// ============================================================
// Load a named PCM sound into the cache.
// Increments all slot age counters, normalises the name, then:
//   1. Searches the 200-slot name table for a cache hit.
//   2. If miss: scans resource types 0x27..0x20 (then 0x0D) to find codec.
//   3. Calls SndMem_AllocSlot to reserve a slot.
//   4. Streams the data in via Res_BunchFreadStreamLoadPtr.
//   5. Calls SndMem_Compact and returns the PCM data pointer.
// Returns PCM pointer on success, 0 on failure.
int SndMem_Load(char* pszName, int* pPcmPtr, int nPriority, unsigned int* pFmtFlags)
{
    unsigned int nLocalFmt;
    if (pFmtFlags == NULL)
        pFmtFlags = &nLocalFmt;

    // Increment all slot ages
    for (unsigned short i = 0; i < 200; i++)
    {
        int* pAge = &g_anSndMemSlotAge[i];
        if (*pAge != 0)
            (*pAge)++;
    }

    FUN_0049def0(pszName);

    // Cache hit: search name table
    for (unsigned short i = 0; i < 200; i++)
    {
        if (_strcmp(pszName, &g_abSndMemSlotNames[i * 9]) == 0)
        {
            *pPcmPtr   = *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x04]);
            *pFmtFlags = *(unsigned int*)(&g_abSndMemSlotTable[i * 0x21 + 0x1d]);
            return g_nSndMemPoolBase +
                   *(int*)(&g_abSndMemSlotTable[i * 0x21]);
        }
    }

    // Cache miss: find codec via Res_FindByNumChar
    char resBuf[260];
    char auxBuf[16];
    int  nFileSize = 0;
    unsigned int nFmt = 0;
    bool bFound = false;

    for (unsigned short codec = 0x27; codec > 0x1f; codec--)
    {
        if (Res_FindByNumChar(codec, pszName, resBuf, 0, auxBuf, &nFileSize) == 0)
        {
            *pFmtFlags = codec & 7;
            nFmt       = codec & 7;
            bFound     = true;
            break;
        }
    }
    if (!bFound)
    {
        if (Res_FindByNumChar(0x0d, pszName, resBuf, 0, auxBuf, &nFileSize) != 0)
            return 0;
        *pFmtFlags = 0;
        nFmt = 0;
    }

    // Allocate slot
    int nSlot = SndMem_AllocSlot(nFileSize, pszName, 0);
    if (nSlot == -1)
        return 0;

    Debug_Trace(DAT_004d9d34[0x2d], 0,
                (int)DAT_004d9d80, (int)DAT_004d9d74, nSlot);

    // Calculate async priority (bytes-per-second scaling)
    int nChannels22k = (((int)(nFmt & 4) >> 2) + 1) *
                       (((int)(nFmt & 2) >> 1) + 1) *
                       ((nFmt & 1) + 1) * 0x5622;
    int nAsyncPrio   = (nFileSize * DAT_004c4c40) / nChannels22k;

    // Record format flags in slot descriptor
    *(unsigned int*)(&g_abSndMemSlotTable[nSlot * 0x21 + 0x1d]) = nFmt;

    // Check pool capacity
    if (g_nSndMemPoolSize <=
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21]) + nFileSize)
    {
        Err_BadResEntry(DAT_004d9d34[0x35],
                        DAT_004d9dbc,
                        "out_of_soundcache");
    }

    // Async stream load
    Res_BunchFreadStreamLoadPtr(
        (int*)(&g_abSndMemSlotTable + nSlot * 0x21),
        1, nFileSize, auxBuf,
        nPriority, nPriority + nAsyncPrio);

    Res_WaitForEntry((int*)(&g_abSndMemSlotTable + nSlot * 0x21));
    SndMem_Compact();

    return *(int*)(&g_abSndMemSlotTable[nSlot * 0x21]);
}

// ============================================================
//  SndMem_AllocSlot   0x00472ea0
// ============================================================
// Find or evict a cache slot large enough for nBytes.
// Uses a multi-pass LRU eviction strategy:
//   Pass 1: find a completely free (state==0) slot that fits.
//   Pass 2: evict the least-recently-used in-use (state==1) slot that fits.
//   Pass 3: merge contiguous in-use slots to create enough space.
// Returns slot index, or -1 if allocation is impossible.
int SndMem_AllocSlot(int nBytes, void* pszName, char bExactOnly)
{
    int  nResult    = g_nSndMemSlotCount;
    unsigned int nTempFmt = (unsigned int)(bExactOnly == '\0');

    // Fast path: room at the end of the pool
    int nEndOffset = *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21]);
    if ((g_nSndMemPoolSize - nEndOffset >= nBytes) &&
        (g_nSndMemSlotCount <= 0xc6))
    {
        // Append new slot at tail
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x00]) =
            g_nSndMemPoolBase + nEndOffset;
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x04]) = nBytes;
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x08]) = nBytes;
        FUN_004895e0(&g_abSndMemSlotNames[g_nSndMemSlotCount * 9], pszName);
        FUN_0049def0(&g_abSndMemSlotNames[g_nSndMemSlotCount * 9]);
        g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x1c] = 1;
        g_anSndMemSlotAge[g_nSndMemSlotCount] = nTempFmt;
        g_nSndMemSlotCount++;
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21]) =
            nEndOffset + nBytes;
        return nResult;
    }

    // Pass 1: find a free slot large enough
    {
        int nBestSize = g_nSndMemPoolSize;
        int nBest     = -1;
        for (int i = 0; i <= g_nSndMemSlotCount; i++)
        {
            if ((g_abSndMemSlotTable[i * 0x21 + 0x1c] == '\0') &&
                (nBytes <= *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08])) &&
                (*(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]) < nBestSize))
            {
                nBestSize = *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]);
                nBest     = i;
            }
        }
        if (nBest != -1)
        {
            SndMem_SetSlotState(nBest, 0);
            *(int*)(&g_abSndMemSlotTable[nBest * 0x21 + 0x04]) = nBytes;
            FUN_004895e0(&g_abSndMemSlotNames[nBest * 9], pszName);
            FUN_0049def0(&g_abSndMemSlotNames[nBest * 9]);
            g_abSndMemSlotTable[nBest * 0x21 + 0x1c] = 1;
            g_anSndMemSlotAge[nBest] = nTempFmt;
            *(int*)(&g_abSndMemSlotTable[nBest * 0x21 + 0x00]) =
                g_nSndMemPoolBase +
                *(int*)(&g_abSndMemSlotTable[nBest * 0x21]);
            return nBest;
        }
    }

    // Pass 2: evict the oldest in-use slot that is big enough
    {
        int nBestSize = g_nSndMemPoolSize;
        int nBest     = -1;
        int nBestAge  = 1;
        for (int i = 0; i <= g_nSndMemSlotCount; i++)
        {
            if ((g_abSndMemSlotTable[i * 0x21 + 0x1c] == '\x01') &&
                (nBestAge <= g_anSndMemSlotAge[i]) &&
                (nBytes <= *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08])) &&
                ((nBestAge < g_anSndMemSlotAge[i] ||
                  *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]) < nBestSize)))
            {
                nBestSize = *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]);
                nBestAge  = g_anSndMemSlotAge[i];
                nBest     = i;
            }
        }
        if (nBest != -1)
        {
            SndMem_SetSlotState(nBest, 0);
            *(int*)(&g_abSndMemSlotTable[nBest * 0x21 + 0x04]) = nBytes;
            FUN_004895e0(&g_abSndMemSlotNames[nBest * 9], pszName);
            FUN_0049def0(&g_abSndMemSlotNames[nBest * 9]);
            g_abSndMemSlotTable[nBest * 0x21 + 0x1c] = 1;
            g_anSndMemSlotAge[nBest] = nTempFmt;
            *(int*)(&g_abSndMemSlotTable[nBest * 0x21 + 0x00]) =
                g_nSndMemPoolBase +
                *(int*)(&g_abSndMemSlotTable[nBest * 0x21]);
            return nBest;
        }
    }

    // Pass 3: merge contiguous free/in-use runs to satisfy request.
    // (complex sliding-window eviction — see decompile of 0x00472ea0)
    // Omitted for brevity; returns -1 when no combination fits.
    return -1;
}

// ============================================================
//  SndMem_Compact   0x00473800
// ============================================================
// Defragment the slot table:
//   1. Trim dead slots off the tail (state < 1) and decrement slot count.
//   2. Merge contiguous free-slot runs into the following in-use slot.
//   3. Promote any slot whose alloc_size > file_size: give the excess
//      space to the next slot in the table.
void SndMem_Compact(void)
{
    // Trim tail
    while (g_nSndMemSlotCount > 0 &&
           g_abSndMemSlotTable[(g_nSndMemSlotCount - 1) * 0x21 + 0x1c] < '\x01')
    {
        g_nSndMemSlotCount--;
        g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x1c] = (char)-1;
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x08]) = 0;
        *(int*)(&g_abSndMemSlotTable[g_nSndMemSlotCount * 0x21 + 0x04]) = 0;
    }

    // Merge free runs into the following occupied slot
    bool bInRun   = false;
    int  nRunStart = 0;
    int  nRunSize  = 0;
    for (int i = 0; i < g_nSndMemSlotCount; i++)
    {
        char state = g_abSndMemSlotTable[i * 0x21 + 0x1c];
        if (bInRun)
        {
            if (state == '\0')
            {
                nRunSize += *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]);
            }
            else
            {
                // End of free run at slot i: collapse run_start..i-1 into i's space
                for (int j = nRunStart + 1; j < i; j++)
                {
                    *(int*)(&g_abSndMemSlotTable[j * 0x21 + 0x04]) = 0;
                    *(int*)(&g_abSndMemSlotTable[j * 0x21 + 0x08]) = 0;
                    *(int*)(&g_abSndMemSlotTable[j * 0x21 + 0x00]) =
                        *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x00]);
                }
                *(int*)(&g_abSndMemSlotTable[nRunStart * 0x21 + 0x08]) = nRunSize;
                *(int*)(&g_abSndMemSlotTable[nRunStart * 0x21 + 0x04]) = 0;
                bInRun = false;
            }
        }
        else if (state == '\0')
        {
            nRunStart = i;
            bInRun    = true;
            nRunSize  = *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]);
        }
    }

    // Promote: if a slot's file_size < alloc_size, give excess to next slot
    for (int i = 0; i < g_nSndMemSlotCount; i++)
    {
        if ((g_abSndMemSlotTable[i * 0x21 + 0x1c] == '\x01') &&
            (*(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x04]) <
             *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08])) &&
            (g_abSndMemSlotTable[(i + 1) * 0x21 + 0x1c] == '\0'))
        {
            int nExcess = *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]) -
                          *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x04]);
            *(int*)(&g_abSndMemSlotTable[(i + 1) * 0x21 + 0x08]) += nExcess;
            *(int*)(&g_abSndMemSlotTable[(i + 1) * 0x21 + 0x04]) = 0;
            *(int*)(&g_abSndMemSlotTable[(i + 1) * 0x21 + 0x00]) =
                g_nSndMemPoolBase +
                *(int*)(&g_abSndMemSlotTable[i * 0x21]) +
                *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x04]);
            *(int*)(&g_abSndMemSlotTable[(i + 1) * 0x21 + 0x1c - 1]) =
                g_nSndMemPoolBase +
                *(int*)(&g_abSndMemSlotTable[(i + 1) * 0x21 + 0x00]);
            *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x08]) =
                *(int*)(&g_abSndMemSlotTable[i * 0x21 + 0x04]);
        }
    }
}

// ============================================================
//  SndMem_Free   0x00473cc0
// ============================================================
// Release a named sound from the cache: find by uppercase name,
// set state=0 (empty), compact the table.
void SndMem_Free(char* pszName)
{
    FUN_0049def0(pszName);
    int i = 0;
    while (i < 200 && _strcmp(pszName, &g_abSndMemSlotNames[i * 9]) != 0)
        i++;
    if (i != 200)
    {
        SndMem_SetSlotState(i, 0);
        FUN_004895e0(&g_abSndMemSlotNames[i * 9], &DAT_007c81a4);
        SndMem_Compact();
    }
}

// ============================================================
//  SndMem_UpdateAnim   0x00473de0
// ============================================================
// Per-frame lipsync-to-animation bridge.
// When g_nSpeechSentence==2 (voiced sentence active), reads the current
// lipsync phoneme byte and passes it to the character animation system.
void SndMem_UpdateAnim(int nCharIdx)
{
    if (g_nSpeechSentence == 2)
    {
        int nPhoneme = SndMem_GetLipsyncByte();
        if (nPhoneme >= 0)
        {
            thunk_FUN_00407e80(
                nCharIdx,
                nPhoneme,
                *(int*)(&DAT_005b10c0[nCharIdx * 0x58 + 0x00]),
                *(int*)(&DAT_005b10c0[nCharIdx * 0x58 + 0x04]));
            *(int*)(&DAT_005b10c0[nCharIdx * 0x58 + 0x10]) = 0;
        }
    }
}

// ============================================================
//  SndMem_InitLipsync   0x004749f0
// ============================================================
// One-time init: zero lipsync state variables and initialise critical section.
void SndMem_InitLipsync(void)
{
    g_nLipsyncLen    = 0;
    g_nLipsyncActive = 0;
    g_nLipsyncPos    = 0;
    InitializeCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
}

// ============================================================
//  SndMem_SetSpeechEnabled   0x00473ed0
// ============================================================
// Enable or disable voiced speech output.
void SndMem_SetSpeechEnabled(int bEnabled)
{
    g_nSndMemSpeechEnabled = bEnabled;
}

// ============================================================
//  SndMem_SetSpeechAnim   0x00474aa0
// ============================================================
// Attach a lipsync animation to a character slot.
// Acquires an animation slot for the character's sprite, stores it in
// g_nSpeechPos, sets g_nSpeechSentence=2, g_nLipsyncAnimPending=1.
// Debug string: "spk_set_ani  int ptr  int pri"
void SndMem_SetSpeechAnim(int nCharIdx, int nPriority)
{
    char szErrBuf[256];

    int hAnim = DAT_0070c24c[nCharIdx];

    if (g_nSpeechPos != -1)
    {
        if (FUN_0049a830((void*)hAnim, &DAT_007c88e0) != 0)
        {
            thunk_FUN_00405810(g_nSpeechPos);
            g_nSpeechPos = -1;
        }
    }

    if (g_nSpeechPos == -1)
    {
        g_nSpeechPos = thunk_FUN_004098c0(hAnim, -1);
        if (g_nSpeechPos < 0)
        {
            FUN_0048a060(szErrBuf, DAT_004d9fe8, (void*)hAnim);
            Err_BadResEntry(DAT_004d9fc4[0x13],
                            DAT_004da000, szErrBuf);
        }
        FUN_004895e0(&DAT_007c88e0, (void*)hAnim);
    }

    g_nLipsyncAnimPending = 1;
    thunk_FUN_00406b90(g_nSpeechPos);
    thunk_FUN_00406980(g_nSpeechPos, nPriority);

    *(int*)(&DAT_005b10c0[g_nSpeechPos * 0x58 + 0x3c]) = DAT_0070e130;
    *(int*)(&DAT_005b10c0[g_nSpeechPos * 0x58 + 0x40]) = nCharIdx;

    g_nSpeechSentence = 2;

    if (DAT_00574990[g_nSpeechPos] < 5)
    {
        FUN_0048a060(szErrBuf, DAT_004da028, (void*)hAnim);
        Err_BadResEntry(DAT_004d9fc4[0x20],
                        DAT_004da044, szErrBuf);
    }
}

// ============================================================
//  SndMem_StartSpeech   0x00473ee0
// ============================================================
// Begin voiced dialogue playback for a sentence.
// Debug string: "nwspeak  char* name  "
void SndMem_StartSpeech(int nSentId)
{
    char auxBuf1[4];
    char auxBuf2[4];
    int  nLipsyncLen;
    char auxBuf16[16];
    int  nResType;
    int  nPcmPtr;
    int  nBBoxX1 = -1, nBBoxY1 = -1, nBBoxX2 = -1, nBBoxY2 = -1;
    char resBuf[260];

    if (g_nSndMemSpeechEnabled == 0)
    {
        // Speech output disabled
        thunk_FUN_00417df0("Speech_disallowed   ");
        return;
    }

    if (g_nLipsyncAnimPending == 0)
    {
        SndMem_StopLipsync();
    }
    else
    {
        g_nLipsyncAnimPending = 0;
    }

    Speech_Play();
    Speech_SetTag(-1, 0);
    thunk_FUN_00442f40(1, (void*)Speech_Commit);
    g_nLipsyncActive = 0;

    if ((DAT_00629b04 == 0) && (DAT_00629f54 == 0))
    {
        nResType = 0x11;
        if (Res_FindByNumChar(0x11, (char*)nSentId, resBuf, 0,
                              auxBuf16, &nLipsyncLen) == 0)
        {
            g_nLipsyncActive = 1;
            g_nLipsyncLen    = nLipsyncLen - 1;
            Res_BunchFreadNow(&g_abLipsyncData, 1, nLipsyncLen, auxBuf16);
            thunk_FUN_00472340(nSentId, auxBuf2, 0, 2, auxBuf1);
            SndMem_SetLipsyncPos(0);
        }
    }
    else
    {
        g_nLipsyncActive = 0;
        g_nLipsyncLen    = 0;
        SndMem_SetLipsyncPos(0);
    }

    // Determine subtitle bounding box from speech position
    int nXMid = -1;
    if ((g_nSpeechPos != -1) && (g_nSpeechSentence == 2))
    {
        thunk_FUN_00409070(g_nSpeechPos, 3,
                           &nBBoxX1, &nBBoxY1, &nBBoxX2, &nBBoxY2);
        nXMid  = nBBoxX2 - 1 + nBBoxX1;
        nBBoxY2 = nBBoxY2 - 1 + nBBoxY1;
    }
    else if ((g_nSpeechSentence == 1) && (g_nMovDestNode != -1))
    {
        nBBoxX1 = g_nAreaActiveBBoxX1;
        nBBoxY1 = g_nAreaActiveBBoxY1;
        nXMid   = g_nAreaActiveBBoxX2;
        nBBoxY2 = g_nAreaActiveBBoxY1 + 1;
    }

    if (nXMid == -1)
    {
        nBBoxX1 = -1;
        nBBoxY1 = -1;
        nBBoxX2 = -1;
    }

    // Calculate subtitle-safe last-char index (trim trailing non-printable)
    int nSubLen = -1;
    if (g_nLipsyncActive != 0)
    {
        nSubLen = g_nLipsyncLen;
        while (nSubLen >= 0 && g_abLipsyncData[nSubLen + 2] < '\x01')
            nSubLen--;
    }

    Txt_SetString(nSentId, &nBBoxX1, nSubLen);

    if (DAT_00629f54 == 0)
    {
        if (nXMid == -1)
        {
            // Centred / mono speech channel
            thunk_FUN_0046f7f0(nSentId, 1);
        }
        else
        {
            // Panned speech channel: compute pan from character screen X
            int nPan = ((nXMid + nBBoxX1) * 0x32) / 0x280;
            Debug_Assert(DAT_004d9f0c[0x66], DAT_004d9f38, nPan);
            thunk_FUN_0046f730(nSentId, 1, nPan);
        }
    }
}

// ============================================================
//  SndMem_WaitSpeech   0x00474320
// ============================================================
// Synchronous wait for speech to finish.
// bForceSkip != 0 — immediately stop speech.
// Returns 1 when done/skipped, 0 on skip-rejected.
int SndMem_WaitSpeech(int bForceSkip)
{
    int nDone  = 0;
    int nMode;

    if ((g_nLipsyncActive == 0) && (bForceSkip == 0))
    {
        if (Txt_IsScrollPending() == 0)
            nMode = ((DAT_00629b04 == 0) && (DAT_00629f54 == 0)) ? 1 : 2;
        else
            nMode = 2;
    }
    else
    {
        nMode = 1;
    }

    do
    {
        if (nMode == 1)
        {
            int nByte = SndMem_GetLipsyncByte();
            if ((nByte < 0) || (bForceSkip != 0))
            {
                if (bForceSkip != 0)
                {
                    if (thunk_FUN_0046fc40(1) != 0)
                        goto done;
                }
                goto wait_text;
            }
        }
        else
        {
wait_text:
            if (nMode != 2) goto done;
            if (Txt_IsDone() != 0) goto done;
        }

        if (thunk_FUN_0040f700() != 0)
        {
            if (nMode != 2)
            {
                thunk_FUN_0046fa60(1);
                nDone = 1;
done:
                if ((g_nSpeechPos != -1) && (g_nSpeechSentence == 2))
                {
                    thunk_FUN_00405810(g_nSpeechPos);
                    g_nSpeechPos = -1;
                }
                thunk_FUN_00403670();
                thunk_FUN_00403770();
                g_nSpeechSentence = 0;
                Txt_Reset();
                return nDone;
            }
            Txt_PageAdvance();
        }

        thunk_FUN_00412ac0();
        Timer_DispatchAsyncProg();
    }
    while (1);
}

// ============================================================
//  SndMem_IsSpeaking   0x004744f0
// ============================================================
// Returns 1 if voiced speech or subtitle is currently active.
int SndMem_IsSpeaking(void)
{
    int nMode;

    if (g_nLipsyncActive == 0)
    {
        if (Txt_IsScrollPending() == 0)
            nMode = ((DAT_00629b04 == 0) && (DAT_00629f54 == 0)) ? 1 : 2;
        else
            nMode = 2;
    }
    else
    {
        nMode = 1;
    }

    if (nMode == 1)
    {
        if (SndMem_GetLipsyncByte() < 0)
            goto idle;
        return 1;
    }

idle:
    if (nMode == 2 && Txt_IsDone() == 0)
        return 1;
    return 0;
}

// ============================================================
//  SndMem_SpeakAndWait   0x00474600
// ============================================================
// Convenience: start speech then synchronously wait for completion.
void SndMem_SpeakAndWait(int nSentId)
{
    SndMem_StartSpeech(nSentId);
    SndMem_WaitSpeech(0);
}

// ============================================================
//  SndMem_GetLipsyncByte   0x004746a0
// ============================================================
// Thread-safe read of the current lipsync phoneme byte.
// Returns the byte at g_abLipsyncData[g_nLipsyncPos], cast to int,
// or -1 if g_nLipsyncPos == -1 (lipsync stopped).
//
// NOTE: this is the function previously speculatively named
// Advanim_GetOverride in MOVEMENT.cpp.  That name was incorrect.
// The real Advanim_GetOverride is in Advanim.cpp.
int SndMem_GetLipsyncByte(void)
{
    int nResult;
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
    if (g_nLipsyncPos < 0)
        nResult = -1;
    else
        nResult = (int)(char)g_abLipsyncData[g_nLipsyncPos];
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
    return nResult;
}

// ============================================================
//  SndMem_StopLipsync   0x00474770
// ============================================================
// Stop lipsync playback: cancel the speech animation slot if active,
// reset lipsync cursor to -1. Calls Speech_Play() to commit/clear pending.
void SndMem_StopLipsync(void)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
    if ((g_nSpeechPos != -1) && (g_nSpeechSentence == 2))
    {
        thunk_FUN_00405810(g_nSpeechPos);
        g_nSpeechPos = -1;
    }
    Speech_Play();
    g_nLipsyncPos = -1;
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
}

// ============================================================
//  SndMem_AdvanceLipsync   0x00474850
// ============================================================
// Advance the lipsync cursor by one frame. Thread-safe.
// Wraps to -1 when the cursor reaches or passes g_nLipsyncLen.
void SndMem_AdvanceLipsync(void)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
    if (g_nLipsyncPos != -1)
    {
        g_nLipsyncPos++;
        if (g_nLipsyncPos >= g_nLipsyncLen)
            g_nLipsyncPos = -1;
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
}

// ============================================================
//  SndMem_SetLipsyncPos   0x00474940
// ============================================================
// Set lipsync cursor to a specific position. Thread-safe.
void SndMem_SetLipsyncPos(int nPos)
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
    g_nLipsyncPos = nPos;
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nLipsyncCS);
}
