// SPEECH.cpp — Subtitle / voiced dialogue subsystem
//
// Manages sentence text + duration data (loaded from SENTENCE.BIN), a set of
// INI-driven subtitle font settings, an offscreen DirectDraw surface for
// compositing on-screen text, and a per-channel played/skipped state table.
//
// The call chain for voiced diary playback:
//   Gran_DiaryPlay (Graninv.cpp)
//     -> Speech_SetTag(nSentId, nChannel)   [queue it]
//     -> Speech_Play()                       [mark interrupted + commit]
//          -> Speech_Commit()               [record state, fire trigger, reset]
//
// SENTENCE.BIN layout (binary):
//   uint32  count              — number of sentences
//   uint32  flags?             — read but not used if 0
//   Per sentence (count times):
//     uint32  textLen           — byte length of the text string
//     char[]  text              — textLen bytes, NUL-terminated by caller
//     uint32  extraLen?         — optional skip field (present if flags != 0)
//     uint32  durLen            — byte length of the duration field
//     char[]  duration          — durLen bytes, NUL-terminated by caller
//
// Original source: C:\DevStudio\Projects\Crux\SPEECH.cpp

#include "SPEECH.h"
#include <windows.h>

// ============================================================
//  Cross-module stubs (unresolved until dependent modules are reversed)
// ============================================================

// FILES.cpp / READRES.cpp — open a binary data file, return file handle
extern int  Files_Open(void* pCtx, void* pPathSpec, void* pAltPath, const char* pszName);

// FILES.cpp / READRES.cpp — read nCount*nSize bytes from file handle
extern int  Files_Read(void* pDst, int nCount, int nSize, int hFile);

// FILES.cpp / READRES.cpp — seek forward nBytes in file handle
extern void Files_Seek(int hFile, int nBytes, int nWhence);

// FILES.cpp / READRES.cpp — close a file handle
extern void Files_Close(int hFile);

// Memalloc.cpp / SAFEHEAP.cpp — allocate nSize bytes, tagged with pszTag
extern int* Mem_Alloc(void* pTag, void* pAltTag, int nSize);

// GI.cpp — create a DirectDraw offscreen surface of given dimensions
extern int  DDI_CreateOffscreenSurf(int nWidth, int nHeight, int nFlags, int nExtra);

// ERRORS.cpp — assert / fatal-error handler
extern void Debug_Assert(int nLevel, const char* pszFile, const char* pszMsg);

// TEXT.cpp — set the active font family constant
extern void Text_SetFontFamily(int nFamily);

// TEXT.cpp — set RGB subtitle text colour
extern void Text_SetColour(int nR, int nG, int nB);

// THEMES.cpp — queue an async callback (thunk_FUN_0047d5b0 = Theme_RegisterAsyncProg)
extern void Theme_RegisterAsyncProg(int nFnPtr);

// ============================================================
//  Globals
// ============================================================

// 007c91cc — current sentence handle (getter/setter pair)
int   g_nSpeechSentence    = 0;

// 007c91c8 — playback position/cursor; -1 = at start
int   g_nSpeechPos         = 0;

// 004da0a0 — pending sentence ID queued by Speech_SetTag; -1 = nothing queued
int   g_nSpeechPendingId   = -1;

// 004da0a4 — channel index for the pending sentence
int   g_nSpeechChannel     = 0;

// 007c91e0 — set to 1 by Speech_Play before calling Speech_Commit
//            signals that the sentence was interrupted / skipped
int   g_nSpeechInterrupted = 0;

// 0070fa38 — per-channel state table; 0 = skipped, 1 = fully played
//            indexed as int array: g_anSpeechPlayed + nChannel * 4
int   g_anSpeechPlayed     = 0;

// 007cc7d8 — total number of sentences in SENTENCE.BIN
int   g_nSpeechCount       = 0;

// 007c93bc — array of char* pointers to sentence text strings
int * g_pSpeechTexts       = 0;

// 007ca78c — array of char* pointers to sentence duration strings
int * g_pSpeechDurations   = 0;

// 007cd7c4 — set to 1 after Speech_Init completes
int   g_nSpeechInitialized = 0;

// 007ca874 — GDI charset constant from INI [Text] Charset
int   g_nSpeechCharset     = 0;

// 007ca878 — text alignment: 0=left, 1=center, 2=right
int   g_nSpeechAlign       = 1;

// 004da16c — subtitle font point size (default 0x12 = 18pt)
int   g_nSpeechFontSize    = 0x12;

// 007cd7c8 — font weight flags (INI value << 17)
uint  g_dwSpeechFontWeight = 0;

// 007cd7cc — GDI FF_* constant:
//   0 -> FF_DECORATIVE (0x10), 1 -> FF_SWISS (0x50), 2 -> FF_SCRIPT (0x30),
//   3 -> FF_MODERN (0x40),     4 -> FF_ROMAN (0x20), 5 -> FF_DONTCARE (0x00)
int   g_nSpeechFontFamily  = 0;

// 007ca368 — font face name, 32-byte buffer
char *g_szSpeechFontFace   = 0;

// 007cd7b0 — 1 if drop-shadow is enabled on subtitle text
int   g_nSpeechShadow      = 1;

// 007ca360 — DirectDraw offscreen surface handle for subtitle compositing (640×480)
int   g_nSpeechSurface     = 0;

// 007ca394 — colour table pointer used by subtitle blitter
int   g_nSpeechColorTable  = 0;

// 007ca884 — Y scanline at which the subtitle bar is drawn (0xb4 = 180)
int   g_nSpeechSubtitleY   = 0xb4;

// 007cd7c0 — subtitle display countdown / remaining frame counter
int   g_nSpeechDisplayTimer = 0;

// 007cc7f0 — index of the sentence currently on-screen (-1 = none)
int   g_nSpeechCurDisplay  = -1;

// ============================================================
//  External data referenced by Speech_Init
// ============================================================

// INI configuration file path (global, owner: Winmain.cpp or FILES.cpp)
extern char DAT_006299c0[];   // 006299c0 — path to CRUX.INI

// File context descriptors used by Files_Open (owner: FILES.cpp / READRES.cpp)
// Two alternative paths for SENTENCE.BIN are tried in sequence.
extern void DAT_004da190[];   // 004da190 — primary file-open context
extern void DAT_007d6248[];   // 007d6248 — primary alt-path descriptor
extern void DAT_004da1ac[];   // 004da1ac — secondary file-open context
extern void DAT_007d6468[];   // 007d6468 — secondary alt-path descriptor
extern void DAT_004da198[];   // 004da198 — read-mode param for primary open
extern void DAT_004da1b4[];   // 004da1b4 — read-mode param for secondary open

// Default font face buffer address (owner: TEXT.cpp or this file)
extern char DAT_007cd7e8[];   // 007cd7e8 — 32-byte font-face-name default buffer

// Error context block (owner: ERRORS.cpp / Winmain.cpp)
extern char DAT_004da170[];   // 004da170 — base of error assert context array

// Exception object prototype (owner: Except.cpp)
extern void DAT_007cd7dc[];   // 007cd7dc — exception object for text-alloc failure
extern void DAT_007cd7e0[];   // 007cd7e0 — exception object for sentence alloc failure
extern void DAT_007cd7e4[];   // 007cd7e4 — exception object for duration alloc failure

// Exception throw helper (owner: Except.cpp)  thunk_FUN_0041fad0
extern void* Except_Throw(int nCode, void* pExcObj, int nExtra);
// Exception handler  thunk_FUN_00420e60
extern void  Except_Setup(void* pCtx, const char* pszFile);
// Exception finalise  thunk_FUN_00489090 / FUN_00489090
extern void  Except_End(void* pLocal, void* pProto);

// Default colour table constant (owner: GI.cpp or Winmain.cpp)
extern int   DAT_004c4c40;    // 004c4c40 — default subtitle colour table handle

// ============================================================
//  Speech_SetSentence   0x00474cf0
// ============================================================
// Store a sentence handle; retrieved later by Speech_GetSentence.
void Speech_SetSentence(int nSentence)
{
    g_nSpeechSentence = nSentence;
}

// ============================================================
//  Speech_GetSentence   0x00474d80
// ============================================================
// Return the current sentence handle.
// NOTE: MOVEMENT.cpp's Mov_Update calls this via thunk and was previously
// mapped as Advanim_HasOverride() — that mapping was incorrect.
// The real Advanim_HasOverride lives elsewhere in Advanim.cpp.
int Speech_GetSentence(void)
{
    return g_nSpeechSentence;
}

// ============================================================
//  Speech_ResetPos   0x00474e10
// ============================================================
// Reset the speech playback position cursor to -1 (beginning).
void Speech_ResetPos(void)
{
    g_nSpeechPos = -1;
}

// ============================================================
//  Speech_SetTag   0x00474ea0
// ============================================================
// Queue a sentence ID + channel index for the next Speech_Commit/Speech_Play call.
void Speech_SetTag(int nSentId, int nChannel)
{
    g_nSpeechPendingId = nSentId;
    g_nSpeechChannel   = nChannel;
}

// ============================================================
//  Speech_Commit   0x00474f40
// ============================================================
// Finalise the currently queued sentence:
//   - If g_nSpeechInterrupted == 0: record as played (1) in g_anSpeechPlayed[channel]
//   - If g_nSpeechInterrupted != 0: record as skipped (0)
//   - Fire the associated sentence timer/trigger
//   - Reset pending state
void Speech_Commit(void)
{
    if (g_nSpeechPendingId != -1)
    {
        // Update the per-channel played/skipped state table.
        // The table base is g_anSpeechPlayed (0x0070fa38); each slot is 4 bytes.
        int* pTable = &g_anSpeechPlayed;
        if (g_nSpeechInterrupted == 0)
            pTable[g_nSpeechChannel] = 0;   // interrupted / skipped
        else
            pTable[g_nSpeechChannel] = 1;   // fully played

        // Fire the timer/trigger for this sentence ID (owner: TIMERS.cpp).
        Theme_RegisterAsyncProg(g_nSpeechPendingId);
    }

    g_nSpeechPendingId   = -1;
    g_nSpeechInterrupted = 0;
}

// ============================================================
//  Speech_Play   0x00475030
// ============================================================
// Play / commit the queued sentence, recording it as interrupted.
// Called from Gran_DiaryPlay in Graninv.cpp as PlaySpeech(szTag).
//
// NOTE: Despite the "Play" name, this function marks the sentence as
// *interrupted* (g_nSpeechInterrupted = 1) before committing.
// The logical flow is: Speech_SetTag queues a sentence, then the
// actual audio play is driven elsewhere (SOUNDMEM/MIXER); Speech_Play
// is the "start + schedule cleanup" call that records the event.
void Speech_Play(void)
{
    if (g_nSpeechPendingId != -1)
    {
        g_nSpeechInterrupted = 1;
        Speech_Commit();
    }
}

// ============================================================
//  Speech_Init   0x004750d0
// ============================================================
// Initialise the speech / subtitle subsystem:
//   1. Load sentence text + duration tables from SENTENCE.BIN
//      (tries two alternative paths in the resource file system).
//   2. Read subtitle font settings from INI:
//        [Text] Charset, Align, <size key>, <weight key>, FontFamily,
//        FontFace, Shadow
//   3. Create a 640×480 offscreen DirectDraw surface for subtitle compositing.
//   4. Set initial display state (timer=0, curDisplay=-1).
//
// String references extracted from decompile:
//   s_txt_init___004da174     — "txt_init  " (debug/trace tag)
//   s_SENTENCE_BIN_004da180   — "SENTENCE.BIN"
//   s_C__DevStudio_..._004da1b8 — assert file path for text-pointer alloc
//   s_C__DevStudio_..._004da1dc — assert file path for duration-pointer alloc
//   s_C__DevStudio_..._004da200 — assert file path for alloc-fail assert
//   s_C__DevStudio_..._004da224 — per-sentence text alloc assert
//   s_C__DevStudio_..._004da248 — per-sentence text-alloc-fail assert
//   s_C__DevStudio_..._004da26c — per-sentence duration alloc assert
//   s_C__DevStudio_..._004da290 — per-sentence duration-alloc-fail assert
//   s_C__DevStudio_..._004da2b4 — offscreen-surface-fail assert
//   s_Offscreen_surfs_004da2d8  — "Offscreen surfs"
//   s_Charset_004da2e8          — "Charset"   (INI key)
//   s_Align_004da2f8            — "Align"     (INI key)
//   s_FontFamily_004da324       — "FontFamily" (INI key)
//   s_FontFace_004da338         — "FontFace"   (INI key)
//   s_Shadow_004da34c           — "Shadow"     (INI key)
void Speech_Init(void)
{
    int   hFile     = 0;
    int   nVersion  = 0;

    // --- Load SENTENCE.BIN ---
    // Try primary resource path; fall back to secondary.
    hFile = Files_Open(&DAT_004da190, &DAT_007d6248, &DAT_004da198,
                       "SENTENCE.BIN");
    if (hFile == 0)
    {
        hFile = Files_Open(&DAT_004da1ac, &DAT_007d6468, &DAT_004da1b4,
                           "SENTENCE.BIN");
    }

    if (hFile == 0)
    {
        g_nSpeechCount = 0;
    }
    else
    {
        // Read sentence count.
        Files_Read(&g_nSpeechCount, 1, 4, hFile);

        // Read version/flags field; if 0 and re-read count matches 4 bytes, re-read count.
        if (g_nSpeechCount == 0)
        {
            if (Files_Read(&nVersion, 1, 4, hFile) == 4)
                Files_Read(&g_nSpeechCount, 1, 4, hFile);
        }

        // Allocate pointer arrays for text and duration strings.
        g_pSpeechTexts = Mem_Alloc(
            (void*)((char*)DAT_004da170 + 0x1d),
            (void*)"C:\\DevStudio\\Projects\\Crux\\TEXT\\...",
            g_nSpeechCount << 2);

        g_pSpeechDurations = Mem_Alloc(
            (void*)((char*)DAT_004da170 + 0x1e),
            (void*)"C:\\DevStudio\\Projects\\Crux\\TEXT\\...",
            g_nSpeechCount << 2);

        if (g_pSpeechTexts == 0 || g_pSpeechDurations == 0)
        {
            // Fatal: out of memory for sentence pointer arrays.
            Except_Setup((void*)((char*)DAT_004da170 + 0x21),
                         "C:\\DevStudio\\Projects\\Crux\\TEXT\\...");
            void* pEx = Except_Throw(0, &DAT_007cd7dc, -1);
            Except_End(pEx, &DAT_004ab3f8);
        }

        // Read each sentence's text and duration.
        for (int i = 0; i < g_nSpeechCount; i++)
        {
            int nTextLen = 0;
            Files_Read(&nTextLen, 1, 4, hFile);

            // Allocate and read text string.
            g_pSpeechTexts[i] = (int)(intptr_t)Mem_Alloc(
                (void*)((char*)DAT_004da170 + 0x26),
                (void*)"C:\\DevStudio\\Projects\\Crux\\TEXT\\...",
                nTextLen + 1);

            if (g_pSpeechTexts[i] == 0)
            {
                Except_Setup((void*)((char*)DAT_004da170 + 0x29),
                             "C:\\DevStudio\\Projects\\Crux\\TEXT\\...");
                void* pEx = Except_Throw(0, &DAT_007cd7e0, -1);
                Except_End(pEx, &DAT_004ab3f8);
            }

            Files_Read((void*)g_pSpeechTexts[i], 1, nTextLen, hFile);
            ((char*)g_pSpeechTexts[i])[nTextLen] = '\0';

            // If file has a version/extra field, skip it.
            if (nVersion > 0)
            {
                int nSkip = 0;
                Files_Read(&nSkip, 1, 4, hFile);
                Files_Seek(hFile, nSkip, 1);
            }

            int nDurLen = 0;
            Files_Read(&nDurLen, 1, 4, hFile);

            // Allocate and read duration string.
            g_pSpeechDurations[i] = (int)(intptr_t)Mem_Alloc(
                (void*)((char*)DAT_004da170 + 0x33),
                (void*)"C:\\DevStudio\\Projects\\Crux\\TEXT\\...",
                nDurLen + 1);

            if (g_pSpeechDurations[i] == 0)
            {
                Except_Setup((void*)((char*)DAT_004da170 + 0x36),
                             "C:\\DevStudio\\Projects\\Crux\\TEXT\\...");
                void* pEx = Except_Throw(0, &DAT_007cd7e4, -1);
                Except_End(pEx, &DAT_004ab3f8);
            }

            Files_Read((void*)g_pSpeechDurations[i], 1, nDurLen, hFile);
            ((char*)g_pSpeechDurations[i])[nDurLen] = '\0';
        }

        Files_Close(hFile);
    }

    // --- Colour table and subtitle bar position ---
    g_nSpeechColorTable = DAT_004c4c40;
    g_nSpeechSubtitleY  = 0xb4;   // y = 180

    // --- Offscreen surface for subtitle compositing (640×480) ---
    g_nSpeechSurface = DDI_CreateOffscreenSurf(0x280, 0x1e0, 0, 0);
    if (g_nSpeechSurface < 0)
    {
        Except_Setup((void*)((char*)DAT_004da170 + 0x43),
                     "C:\\DevStudio\\Projects\\Crux\\TEXT\\...");
        void* pEx = Except_Throw(0x1d, (void*)"Offscreen surfs", -1);
        Except_End(pEx, &DAT_004ab3f8);
    }

    g_nSpeechInitialized = 1;

    // --- Read subtitle font settings from INI ---
    g_nSpeechCharset = GetPrivateProfileIntA(
        "Text", "Charset", 0, DAT_006299c0);

    g_nSpeechAlign = GetPrivateProfileIntA(
        "Text", "Align", 1, DAT_006299c0);
    // Clamp to 0/1/2 (left/center/right):
    if      (g_nSpeechAlign == 0) g_nSpeechAlign = 0;
    else if (g_nSpeechAlign == 1) g_nSpeechAlign = 1;
    else if (g_nSpeechAlign == 2) g_nSpeechAlign = 2;

    g_nSpeechFontSize = GetPrivateProfileIntA(
        "Text", "Size", 0x12, DAT_006299c0);   // default 18pt

    g_dwSpeechFontWeight = (UINT)GetPrivateProfileIntA(
        "Text", "Weight", 0, DAT_006299c0) << 17;

    // Map INI FontFamily 0-5 to GDI FF_* constants:
    int nFamily = GetPrivateProfileIntA(
        "Text", "FontFamily", 0, DAT_006299c0);
    switch (nFamily)
    {
    case 0: g_nSpeechFontFamily = 0x10; break;  // FF_DECORATIVE
    case 1: g_nSpeechFontFamily = 0x50; break;  // FF_SWISS
    case 2: g_nSpeechFontFamily = 0x30; break;  // FF_SCRIPT
    case 3: g_nSpeechFontFamily = 0x40; break;  // FF_MODERN
    case 4: g_nSpeechFontFamily = 0x20; break;  // FF_ROMAN
    case 5: g_nSpeechFontFamily = 0x00; break;  // FF_DONTCARE
    }

    GetPrivateProfileStringA(
        "Text", "FontFace", DAT_007cd7e8,
        g_szSpeechFontFace, 0x20, DAT_006299c0);

    g_nSpeechShadow = GetPrivateProfileIntA(
        "Text", "Shadow", 1, DAT_006299c0);

    // Apply font family to the TEXT subsystem.
    Text_SetFontFamily(g_nSpeechFontFamily);

    // Set default subtitle text colour (near-white: R=63, G=63, B=63).
    Text_SetColour(0x3f, 0x3f, 0x3f);

    // --- Display state ---
    g_nSpeechDisplayTimer = 0;
    g_nSpeechCurDisplay   = -1;
}
