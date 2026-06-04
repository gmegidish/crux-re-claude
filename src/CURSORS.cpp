// ---------------------------------------------------------------------------
// CURSORS.cpp  —  Software sprite cursor system
// Original: C:\DevStudio\Projects\Crux\CURSORS.cpp
// RE offsets: 0x00418640 – 0x0041a7b0
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "CURSORS.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

int  g_nCurrentCursor       = -1;  // 0x004c8768
int  g_nCursorSystemEnabled =  0;  // 0x00647c18
int  g_nCursorMode          =  0;  // 0x00646754
int  g_nCursorDrawEnabled   =  0;  // 0x004c8774
int  g_nWin32CursorVisible  =  0;  // 0x004c8770
int  g_nCursorMode4Id       =  0;  // 0x0064674c

int  g_nCursorX             =  0;  // 0x00646734
int  g_nCursorY             =  0;  // 0x00646730
int  g_nCursorWidth         =  0;  // 0x00646760
int  g_nCursorHeight        =  0;  // 0x0064675c
int  g_nCursorImgW          =  0;  // 0x00646744
int  g_nCursorImgH          =  0;  // 0x0064673c
int  g_nCursorHotX          =  0;  // 0x00646710
int  g_nCursorHotY          =  0;  // 0x00646738
int  g_nCursorOffsetX       =  0;  // 0x00647c20
int  g_nCursorOffsetY       =  0;  // 0x00647c24

int  g_nCursorSrcX          =  0;  // 0x00647c38
int  g_nCursorSrcY          =  0;  // 0x00647c3c
int  g_nCursorDstX          =  0;  // 0x00647c0c
int  g_nCursorDstY          =  0;  // 0x00647c08
int  g_nCursorBlitW         =  0;  // 0x00647c10
int  g_nCursorBlitH         =  0;  // 0x00647c14

int* g_pCursorSurface       = 0;   // 0x00646758
int* g_pCursorBgSurface     = 0;   // 0x00646750

int  g_nCursorDirty         =  0;  // 0x00647c28
int  g_nCursorWasVisible    =  0;  // 0x00647c34
int  g_nLastCursorSprite    =  0;  // 0x00646740

HANDLE g_hCursorSyncEvent   = NULL; // 0x00647c2c
int  g_nWin32CursorIdx      =  0;  // 0x004c8e3c
int  g_nWin32Cursor1        =  0;  // 0x006468cc  HCURSOR mode 1
int  g_nWin32Cursor2        =  0;  // 0x0064677c  HCURSOR mode 2
int  g_nWin32CursorWait     =  0;  // 0x006467ac  HCURSOR wait

int* g_pfnCursorPos         = 0;   // 0x00647c1c
int* g_pfnCursorOverride    = 0;   // 0x00647c30

int  g_nCursorCS            =  0;  // 0x00646718  (CRITICAL_SECTION, 24 bytes actual)

int  g_nMouseX              =  0;  // 0x006dc4f0  (shared with input system)
int  g_nMouseY              =  0;  // 0x006dc4f4

// ---------------------------------------------------------------------------
// Debug stubs — no-ops in release build
// ---------------------------------------------------------------------------

// 0x0041a740
void Debug_Assert(int /*line*/, const char* /*file*/, int /*value*/) {}

// 0x0041a760
void Debug_AssertFatal(int /*line*/, const char* /*file*/) {}

// 0x0041a770
void Debug_Trace(int /*line*/, const char* /*file*/, const char* /*msg*/) {}

// 0x0041a790
void Debug_TraceVal(int /*line*/, const char* /*file*/, const char* /*msg*/, int /*val*/) {}

// ---------------------------------------------------------------------------
// External helpers (defined in other modules)
// ---------------------------------------------------------------------------

// DDRAWI.cpp: lock a surface and return a pointer + pitch
extern void* DDI_LockSurface(int surface, int* pitch, int* rows);

// DDRAWI.cpp: unlock a surface after writing
extern void  DDI_UnlockSurface(int surface, void* ptr);

// DDRAWI.cpp: blit one surface rect onto another
extern void  DDI_Blit(int dst, int src, int srcX, int srcY,
                       int w, int h, int dstX, int dstY, int useColorKey);

// DDRAWI.cpp: mark screen region dirty for redraw
extern void  DDI_InvalidateRect(int x1, int y1, int x2, int y2);

// GI.cpp: blit sprite with palette
extern void  BlitCursorSprite(int sprite, void* surface, int x, int y,
                               int stride, int height);

// FILES.cpp: check if a file exists, returns non-zero if yes
extern int   Files_Exists(int type, const char* path, char* outPath,
                           int outLen, void* extra, void* extra2);

// Logging / assert
extern void  Log_Printf(int line, const char* file, const char* fmt, ...);
extern void  Log_Error(int line, const char* file, const char* msg);
extern int   Error_Raise(int code, void* context, int flags);

// ---------------------------------------------------------------------------
// Internal cursor data tables (populated by the loading subsystem)
// ---------------------------------------------------------------------------

// Per-cursor struct (stride 0x30 = 48 bytes), max ~20 cursors.
// Filled in by Curs_LoadCursorSelect / the resource loader.
//   [0]  int animType        — index into animation table
//   [1]  int hotBaseX        — base hotspot X (before frame offset)
//   [2]  int hotBaseY        — base hotspot Y
//   [3]  int hotOffX         — frame-adjusted hotspot X offset
//   [4]  int hotOffY         — frame-adjusted hotspot Y offset
//   (remaining bytes reserved / padding)
static int s_cursorTable[20 * (0x30 / 4)];   // 0x00646784

// Per-cursor sprite slot (stride 0xc = 12 bytes).
//   [0]  int spritePtr       — pointer to current animation frame sprite
//   [1]  int hotX            — hotspot X from sprite header
//   [2]  int hotY            — hotspot Y from sprite header
static int s_cursorSprites[20 * (0xc / 4)];  // 0x00646768

// Animation frame counter per animation type (stride 0x58)
static int s_animFrames[32 * (0x58 / 4)];    // 0x005b10d0

// Frame lookup table: [animType * 0x640 + frameIdx * 4]
static int s_frameLookup[32 * 0x640 / 4];    // 0x004e3b58

// Sprite data array (stride 0x20 = 32 bytes per frame)
//   [0]  int spritePtr
//   [1]  int hotX
//   [2]  int hotY
//   ...
static int s_frameSprites[256 * (0x20 / 4)]; // 0x0051e500
static int s_frameHotX[256];                 // 0x0051e4f0
static int s_frameHotY[256];                 // 0x0051e4f4

// Max frame count per animation type
static int s_maxFrames[32];                  // 0x00574990

// Log CRITICAL_SECTION (used by Curs_InitLog / logging subsystem)
static CRITICAL_SECTION s_csLog;             // 0x00648080
static int s_logInitialized = 0;             // 0x00648098

// ---------------------------------------------------------------------------
// Curs_InitLog — init the debug log and its critical section
// 0x0041a6c0
// ---------------------------------------------------------------------------
void Curs_InitLog()
{
    char path[260];
    // Build path to ADVENT_OUT log file (from engine data directory)
    // FUN_004895e0 / FUN_004895f0 / FUN_0048a660 = path builder + open
    // (exact path-building calls depend on FILES.cpp helpers)

    if (!s_logInitialized) {
        InitializeCriticalSection(&s_csLog);
        s_logInitialized = 1;
    }
}

// ---------------------------------------------------------------------------
// Curs_SetOffset — set additional draw offset X/Y
// 0x00419090
// ---------------------------------------------------------------------------
void Curs_SetOffset(int x, int y)
{
    g_nCursorOffsetX = x;
    g_nCursorOffsetY = y;
}

// ---------------------------------------------------------------------------
// Curs_SetPosCallback — register position-transform callback
// 0x0041a1b0
// ---------------------------------------------------------------------------
void Curs_SetPosCallback(void* fn)
{
    g_pfnCursorPos = (int*)fn;
}

// ---------------------------------------------------------------------------
// Curs_SetOverrideCallback — register cursor ID override callback
// 0x0041a630
// ---------------------------------------------------------------------------
void Curs_SetOverrideCallback(void* fn)
{
    g_pfnCursorOverride = (int*)fn;
}

// ---------------------------------------------------------------------------
// Curs_GetSize — return cursor sprite and buffer dimensions
// 0x0041a240
// Buffer dimensions are always 150×150 (0x96).
// ---------------------------------------------------------------------------
void Curs_GetSize(int* width, int* height, int* bufW, int* bufH)
{
    *bufW   = 0x96;
    *bufH   = 0x96;
    *width  = g_nCursorWidth;
    *height = g_nCursorHeight;
}

// ---------------------------------------------------------------------------
// Curs_DisableDraw / Curs_EnableDraw
// 0x0041a510 / 0x0041a5a0
// ---------------------------------------------------------------------------
void Curs_DisableDraw() { g_nCursorDrawEnabled = 0; }
void Curs_EnableDraw()  { g_nCursorDrawEnabled = 1; }

// ---------------------------------------------------------------------------
// Curs_UpdateAnimFrame — advance animation frame for current cursor
// 0x00418750
// ---------------------------------------------------------------------------
void Curs_UpdateAnimFrame()
{
    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);

    int animType   = s_cursorTable[g_nCurrentCursor * (0x30 / 4) + 0];
    int frameCount = s_animFrames[animType * (0x58 / 4)];
    int frameIdx   = s_frameLookup[animType * (0x640 / 4) + frameCount];

    s_cursorSprites[g_nCurrentCursor * (0xc / 4) + 0] =
        s_frameSprites[frameIdx * (0x20 / 4) + 0];
    s_cursorTable[g_nCurrentCursor * (0x30 / 4) + 3] =
        s_frameHotX[frameIdx] - s_cursorTable[g_nCurrentCursor * (0x30 / 4) + 1];
    s_cursorTable[g_nCurrentCursor * (0x30 / 4) + 4] =
        s_frameHotY[frameIdx] - s_cursorTable[g_nCurrentCursor * (0x30 / 4) + 2];

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_SetPosition — compute screen position relative to hotspot
// 0x00419360
// ---------------------------------------------------------------------------
void Curs_SetPosition(int x, int y)
{
    g_nCursorX    = x - g_nCursorHotX;
    g_nCursorY    = y - g_nCursorHotY;
    g_nCursorImgW = g_nCursorWidth;
    g_nCursorImgH = g_nCursorHeight;
}

// ---------------------------------------------------------------------------
// Curs_GetSurface — get draw surface, optionally via position callback
// 0x00419420
// ---------------------------------------------------------------------------
int Curs_GetSurface(int x, int y)
{
    int surface = (int)g_pCursorSurface;
    if (g_pfnCursorPos) {
        typedef int (*PosCallback)(int, int, int, int);
        ((PosCallback)g_pfnCursorPos)(x, y, (int)g_pCursorSurface,
                                       *(int*)((char*)&g_nCursorX + 4));
        surface = *(int*)((char*)&g_nCursorX + 4); // DAT_00646714 = back buffer
    }
    return surface;
}

// ---------------------------------------------------------------------------
// Curs_SetCursor — core: set cursor ID, blit sprite to g_pCursorSurface
// 0x00418a20
// ---------------------------------------------------------------------------
void Curs_SetCursor(int cursorId)
{
    if (!g_nCursorSystemEnabled)
        return;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);

    Debug_Assert(__LINE__, __FILE__, cursorId);

    // Apply override callback if set
    if (g_pfnCursorOverride) {
        typedef int (*OverrideFn)(int);
        cursorId = ((OverrideFn)g_pfnCursorOverride)(cursorId);
    }

    // Check if cursor is hidden (bit 0 of flags set)
    if ((s_cursorTable[cursorId * (0x30 / 4)] & 1) == 0) {
        if (g_nCurrentCursor != cursorId) {
            g_nCurrentCursor = cursorId;
            Curs_UpdateAnimFrame();
        }
    } else {
        g_nCurrentCursor = -1;
    }

    int sprite = s_cursorSprites[cursorId * (0xc / 4)];

    // Log missing cursor
    if (sprite == 0) {
        char msg[100];
        sprintf(msg, "Missing cursor: %d", cursorId);
        Log_Error(__LINE__, __FILE__, msg);
    }

    // Only redraw if sprite changed
    if (sprite != g_nLastCursorSprite) {
        int pitch, rows;
        void* surface = DDI_LockSurface((int)g_pCursorSurface, &pitch, &rows);
        if (!surface) {
            Debug_Assert(__LINE__, __FILE__, 0);
            LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
            return;
        }

        // Clear cursor buffer and blit new sprite
        for (int row = 0; row < rows; row++)
            memset((char*)surface + row * pitch, 0, 0x96);

        g_nCursorWidth  = (unsigned short)*(short*)((char*)sprite + 1);
        g_nCursorHeight = *(short*)((char*)sprite + 3) + 1;
        g_nCursorHotX   = s_cursorSprites[cursorId * (0xc / 4) + 1] +
                           s_cursorTable[cursorId * (0x30 / 4) + 3];
        g_nCursorHotY   = s_cursorSprites[cursorId * (0xc / 4) + 2] +
                           s_cursorTable[cursorId * (0x30 / 4) + 4];

        BlitCursorSprite(sprite, surface, 0, 0, pitch, 0x96);
        DDI_UnlockSurface((int)g_pCursorSurface, surface);

        g_nLastCursorSprite = sprite;
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_SetCursorByMode — high-level dispatcher based on g_nCursorMode
// 0x00418da0
// ---------------------------------------------------------------------------
void Curs_SetCursorByMode(int cursorId)
{
    if (!g_nCursorSystemEnabled)
        return;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);

    if (g_nCursorMode == 1) {
        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(7);
    } else if (g_nCursorMode == 2) {
        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(0);
    } else if (g_nCursorMode == 3) {
        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(9);
    } else if (g_nCursorMode == 4) {
        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(g_nCursorMode4Id);
    } else if (cursorId == -1) {
        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(1);
    } else {
        // Object-driven mode: look up cursor from game object table
        extern CRITICAL_SECTION g_csObjects;       // 0x0070ae78
        extern int              g_nObjectsEnabled; // 0x007127e8
        extern int*             g_pObjectTable[];  // 0x0070ded8

        EnterCriticalSection(&g_csObjects);
        if (g_nObjectsEnabled == 0 && cursorId >= 0) {
            Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
            Curs_SetCursor(1);
        } else {
            Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
            Debug_Assert(__LINE__, __FILE__, cursorId);
            Debug_Assert(__LINE__, __FILE__, g_nObjectsEnabled);
            // Read cursor ID from object data at offset 0x10
            Curs_SetCursor((int)(char)*(int*)((char*)g_pObjectTable[cursorId] + 0x10));
        }
        LeaveCriticalSection(&g_csObjects);
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_Animate — per-tick animation driver
// 0x004188b0
// ---------------------------------------------------------------------------
void Curs_Animate()
{
    if (!g_nCursorSystemEnabled)
        return;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);

    if (g_nCurrentCursor >= 0) {
        Curs_UpdateAnimFrame();

        int animType   = s_cursorTable[g_nCurrentCursor * (0x30 / 4)];
        int& frameCount = s_animFrames[animType * (0x58 / 4)];
        frameCount++;
        if (frameCount >= s_maxFrames[animType])
            frameCount = 0;

        Debug_Trace(__LINE__, __FILE__, "going_to_curs_set_cursor");
        Curs_SetCursor(g_nCurrentCursor);
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_Restore — restore background rect under the cursor
// 0x004194e0
// ---------------------------------------------------------------------------
void Curs_Restore(int invalidate)
{
    if (!g_nCursorDirty)
        return;

    if (invalidate) {
        // Compute visible region clamped to screen
        int x1 = (g_nCursorX < 0) ? 0 : (unsigned int)g_nCursorX;
        int y1 = (g_nCursorY < 0) ? 0 : (unsigned int)g_nCursorY;
        int x2 = (g_nCursorX - 1 + g_nCursorImgW < 0x27f)
                    ? g_nCursorX - 1 + g_nCursorImgW : 0x27f;
        int y2 = (g_nCursorY - 1 + g_nCursorImgH < 0x1df)
                    ? g_nCursorY - 1 + g_nCursorImgH : 0x1df;

        if (x1 < 0x280 && y1 < 0x1e0 && x2 >= 0 && y2 >= 0 &&
            x1 < x2 && y1 < y2)
        {
            DDI_InvalidateRect(x1, y1, x2, y2);
        }

        // Invalidate out-of-bound strips
        if (g_nCursorX - 1 + g_nCursorImgW > 0x27f)
            DDI_InvalidateRect(
                (g_nCursorX < 0x280) ? g_nCursorX : 0x280,
                g_nCursorY,
                g_nCursorX - 1 + g_nCursorImgW,
                g_nCursorY - 1 + g_nCursorImgH);
        if (g_nCursorY - 1 + g_nCursorImgH > 0x1df)
            DDI_InvalidateRect(
                g_nCursorX,
                (g_nCursorY < 0x1e0) ? g_nCursorY : 0x1e0,
                g_nCursorX - 1 + g_nCursorImgW,
                g_nCursorY - 1 + g_nCursorImgH);
        if (g_nCursorX < 0)
            DDI_InvalidateRect(g_nCursorX, g_nCursorY,
                                (g_nCursorX - 1 + g_nCursorImgW < -1)
                                    ? g_nCursorX - 1 + g_nCursorImgW : -1,
                                g_nCursorY - 1 + g_nCursorImgH);
        if (g_nCursorY < 0)
            DDI_InvalidateRect(g_nCursorX, g_nCursorY,
                                g_nCursorX - 1 + g_nCursorImgW,
                                (g_nCursorY - 1 + g_nCursorImgH < -1)
                                    ? g_nCursorY - 1 + g_nCursorImgH : -1);
    }

    g_nCursorDirty = 0;
}

// ---------------------------------------------------------------------------
// Curs_Update — update cursor position and blit
// 0x00419230
// ---------------------------------------------------------------------------
void Curs_Update(int x, int y)
{
    if (g_nWin32CursorVisible)
        return;

    Curs_SetPosition(x, y);
    int surface = Curs_GetSurface(g_nCursorX, g_nCursorY);

    if (g_nCursorDrawEnabled) {
        DDI_Blit((int)g_pCursorBgSurface, 0,
                 g_nCursorX + g_nCursorOffsetX,
                 g_nCursorY + g_nCursorOffsetY,
                 g_nCursorWidth, g_nCursorHeight,
                 0, 0, 1);
        g_nCursorDirty = 1;
    }
}

// ---------------------------------------------------------------------------
// Curs_Tick — per-frame tick: animate then update position
// 0x00419130
// ---------------------------------------------------------------------------
void Curs_Tick()
{
    extern int g_nGamePaused; // 0x00629dd0

    if (g_nGamePaused || !g_nCursorSystemEnabled)
        return;
    if (WaitForSingleObject(g_hCursorSyncEvent, 0) != WAIT_OBJECT_0)
        return;

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
    Curs_Restore(1);
    Curs_Update(g_nMouseX, g_nMouseY);
    SetEvent(g_hCursorSyncEvent);
    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_PutOnPage — blit cursor onto a page surface
// 0x00419a20
// ---------------------------------------------------------------------------
void Curs_PutOnPage(int surface)
{
    if (WaitForSingleObject(g_hCursorSyncEvent, 10000) != WAIT_OBJECT_0) {
        Log_Error(__LINE__, __FILE__, "Curs_PutOnPage timeout");
        Error_Raise(0x18, NULL, -1);
    }

    if (!g_nCursorDrawEnabled) {
        g_nCursorWasVisible = 0;
        return;
    }

    EnterCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
    Curs_Restore(0);
    Curs_SetPosition(g_nMouseX, g_nMouseY);
    int drawSurface = Curs_GetSurface(g_nCursorX, g_nCursorY);

    if (!g_nWin32CursorVisible && g_nCursorDrawEnabled) {
        DDI_Blit((int)g_pCursorBgSurface, 0,
                 g_nCursorX + g_nCursorOffsetX,
                 g_nCursorY + g_nCursorOffsetY,
                 g_nCursorWidth, g_nCursorHeight,
                 0, 0, 1);
        g_nCursorDirty = 1;
    }

    // Compute clip offsets for edges
    g_nCursorSrcX = (g_nCursorX < 0) ? -g_nCursorX : 0;
    g_nCursorSrcY = (g_nCursorY < 0) ? -g_nCursorY : 0;

    if (g_nCursorSrcX < g_nCursorWidth && g_nCursorSrcY < g_nCursorHeight) {
        unsigned int dstX = (unsigned int)(g_nCursorX + g_nCursorSrcX);
        unsigned int dstY = (unsigned int)(g_nCursorY + g_nCursorSrcY);

        int w = g_nCursorImgW - g_nCursorSrcX;
        if (w > (int)(0x280 - dstX)) w = 0x280 - dstX;
        g_nCursorBlitW = w;

        int h = g_nCursorImgH - g_nCursorSrcY;
        if (h > (int)(0x1e0 - dstY)) h = 0x1e0 - dstY;
        g_nCursorBlitH = h;

        g_nCursorDstX = (dstX >= 1) ? dstX : 0;
        g_nCursorDstY = (dstY >= 1) ? dstY : 0;

        if (w > 0 && h > 0) {
            g_nCursorWasVisible = 1;

            // Save background then blit cursor sprite
            DDI_Blit((int)g_pCursorBgSurface, surface,
                     g_nCursorSrcX, g_nCursorSrcY, w, h,
                     g_nCursorDstX, g_nCursorDstY, 0);
            if (!g_nWin32CursorVisible)
                DDI_Blit(surface, drawSurface,
                         g_nCursorDstX, g_nCursorDstY, w, h,
                         g_nCursorSrcX, g_nCursorSrcY, 1);
        }
    } else {
        g_nCursorWasVisible = 0;
    }

    LeaveCriticalSection((LPCRITICAL_SECTION)&g_nCursorCS);
}

// ---------------------------------------------------------------------------
// Curs_RestoreFromPage — restore background; signal sync event
// 0x00419e80
// ---------------------------------------------------------------------------
void Curs_RestoreFromPage(int surface)
{
    if (g_nCursorWasVisible) {
        DDI_Blit(surface, (int)g_pCursorBgSurface,
                 g_nCursorDstX, g_nCursorDstY,
                 g_nCursorBlitW, g_nCursorBlitH,
                 g_nCursorSrcX, g_nCursorSrcY, 0);
    }
    SetEvent(g_hCursorSyncEvent);
}

// ---------------------------------------------------------------------------
// Curs_Shutdown — wait for sync event then shut down cursor rendering
// 0x00419f70
// ---------------------------------------------------------------------------
void Curs_Shutdown()
{
    if (g_hCursorSyncEvent)
        WaitForSingleObject(g_hCursorSyncEvent, 1000);
    Curs_DisableDraw();
}

// ---------------------------------------------------------------------------
// Curs_ShowWin32 — show Win32 system cursor
// 0x0041a020
// ---------------------------------------------------------------------------
void Curs_ShowWin32()
{
    extern int g_nGamePaused; // 0x00629dd0
    if (!g_nWin32CursorVisible && !g_nGamePaused) {
        ShowCursor(TRUE);
        g_nWin32CursorVisible = 1;
        Curs_Restore(1);
    }
}

// ---------------------------------------------------------------------------
// Curs_HideWin32 — hide Win32 system cursor
// 0x0041a0e0
// ---------------------------------------------------------------------------
void Curs_HideWin32()
{
    extern int g_nGamePaused;
    if (g_nWin32CursorVisible && !g_nGamePaused) {
        ShowCursor(FALSE);
        g_nWin32CursorVisible = 0;
        Curs_Update(g_nMouseX, g_nMouseY);
    }
}

// ---------------------------------------------------------------------------
// Curs_ForceRestore — force background restore
// 0x0041a480
// ---------------------------------------------------------------------------
void Curs_ForceRestore()
{
    Curs_Restore(1);
}

// ---------------------------------------------------------------------------
// Curs_SetWin32Cursor — set Win32 HCURSOR based on mode
// 0x0041a2f0
// ---------------------------------------------------------------------------
void Curs_SetWin32Cursor(int idx)
{
    if (!g_nCursorSystemEnabled)
        return;

    if (idx == -1)
        idx = g_nWin32CursorIdx;
    g_nWin32CursorIdx = idx;

    if (g_nCursorMode == 1)
        SetCursor((HCURSOR)g_nWin32Cursor1);
    else if (g_nCursorMode == 2)
        SetCursor((HCURSOR)g_nWin32Cursor2);
    else
        SetCursor((HCURSOR)*(int*)((char*)&g_nWin32Cursor2 + idx * 0xc));
}

// ---------------------------------------------------------------------------
// Curs_SetWaitCursor — set busy/wait cursor immediately
// 0x0041a3f0
// ---------------------------------------------------------------------------
void Curs_SetWaitCursor()
{
    SetCursor((HCURSOR)g_nWin32CursorWait);
}

// ---------------------------------------------------------------------------
// Curs_LoadCursorSelect — load cursor resource, select type by file existence
// 0x00418640
// ---------------------------------------------------------------------------
void Curs_LoadCursorSelect(int cursorId, int resId, int x, int y)
{
    char outPath[260];
    char extra[16];

    // Try to locate the cursor file (type 7 = cursor resource)
    int exists = Files_Exists(7, NULL, outPath, 0, extra, NULL);

    if (exists == 0) {
        // File not found: use type 2 (generic cursor)
        extern void Curs_LoadCursor(int, int, int, int, int);
        Curs_LoadCursor(cursorId, resId, 2, x, y);
    } else {
        // File found: use type 1 (sprite cursor)
        extern void Curs_LoadCursor(int, int, int, int, int);
        Curs_LoadCursor(cursorId, resId, 1, x, y);
    }
}

// ---------------------------------------------------------------------------
// DDI_CreateOffscreenSurf — create a DirectDraw offscreen surface
// 0x0041a7b0
// ---------------------------------------------------------------------------

// Surface slot arrays (defined in DDRAWI.cpp)
extern int* g_pDDSurfaces[10];   // 0x006480c0  DirectDraw surface pointers
extern int  g_nSurfaceW[10];     // 0x006480e8  widths
extern int  g_nSurfaceH[10];     // 0x006480ec  heights
extern int  g_nSurfaceColorKey[10]; // 0x006480f0
extern int  g_nSurfaceVideo[10]; // 0x006480f4
extern int  g_nSurfacePitch[10]; // stride info
extern int* g_pDDDevice;         // 0x00648220  primary DirectDraw device

int DDI_CreateOffscreenSurf(int width, int height, int colorKey, int isVideo)
{
    // Find a free surface slot
    int slot = 0;
    while (g_pDDSurfaces[slot] != NULL && slot < 10)
        slot++;

    if (slot == 10)
        return -1;

    // Fill DDSURFACEDESC (0x6c bytes)
    struct { int size, flags, height, width, pitch; int caps; char rest[0x60]; } desc;
    memset(&desc, 0, sizeof(desc));
    desc.size   = 0x6c;
    desc.flags  = 7;
    desc.height = height;
    desc.width  = width;
    if (isVideo)
        desc.caps = 0x840;
    else
        desc.caps = 0x40;

    // Create via IDirectDraw::CreateSurface vtable call
    typedef int (__stdcall *CreateSurface_t)(void*, void*, void**, void*);
    int hr = ((CreateSurface_t**)g_pDDDevice)[0][6](
                g_pDDDevice, &desc, &g_pDDSurfaces[slot], NULL);
    if (hr != 0) {
        Log_Error(__LINE__, __FILE__, "DDI_CreateOffscreenSurf: CreateSurface failed");
        Error_Raise(0x1a, NULL, -1);
    }

    // Lock and zero-fill
    int pitch;
    void* ptr = DDI_LockSurface(slot, &pitch, NULL);
    if (!ptr) {
        Log_Error(__LINE__, __FILE__, "DDI_CreateOffscreenSurf: Lock failed");
        Error_Raise(0x1b, NULL, -1);
    }

    for (int row = 0; row < height; row++)
        memset((char*)ptr + row * pitch, 0, width);

    DDI_UnlockSurface(slot, ptr);

    // Set color key if requested
    if (colorKey) {
        struct { int low, high; } ck = { 0, 0 };
        typedef int (__stdcall *SetColorKey_t)(void*, DWORD, void*);
        ((SetColorKey_t**)g_pDDSurfaces[slot])[0][0x1d](g_pDDSurfaces[slot], 8, &ck);
    }

    g_nSurfaceW[slot]        = width;
    g_nSurfaceH[slot]        = height;
    g_nSurfaceColorKey[slot] = colorKey;
    g_nSurfaceVideo[slot]    = isVideo;

    return slot;
}

// ===========================================================================
//  Mouse input handler (0x00451e90 – 0x004528c0)
//
//  A small thread-safe mouse driver layered on top of the Win32 message pump.
//  It translates WM_MOUSEMOVE / WM_*BUTTON* messages into a logical button
//  state (g_nMouseButtons) plus a mouse position (g_nMouseX/Y) relative to a
//  configurable origin, and synthesises double-click events using a Win32
//  timer started on button-down.  Two auto-reset events let the render /
//  game thread observe move and button-state transitions.
// ===========================================================================

// --- Mouse state globals ---------------------------------------------------
int    g_nMouseDblClickTimer    = -1;   // 0x004d5058  dbl-click timer ID (-1 none)
int    g_nMouseButtons          =  0;   // 0x006dc4f8  logical button/click state
int    g_nMouseClickX           =  0;   // 0x006dc4fc  X at last button-down
int    g_nMouseClickY           =  0;   // 0x006dc500  Y at last button-down
void*  g_pMouseCS               =  0;   // 0x006dc508  CRITICAL_SECTION (opaque)
int    g_nMouseScreenX          =  0;   // 0x006dc520  GetCursorPos POINT.x
int    g_nMouseScreenY          =  0;   // 0x006dc524  GetCursorPos POINT.y
int    g_nMouseBtnDownMask      =  0;   // 0x006dc528  raw button bitmask
HANDLE g_pMouseStateEvent       =  0;   // 0x006dc52c  state-changed event
HANDLE g_pMouseMoveEvent        =  0;   // 0x006dc530  mouse-move event
int    g_nMouseDblClickEnabled  =  1;   // 0x006dc534  from [Mouse]DoubleClick INI
int    g_nMouseOriginX          =  0;   // 0x006dc538  origin offset X
int    g_nMouseOriginY          =  0;   // 0x006dc53c  origin offset Y

// The CRITICAL_SECTION lives at g_pMouseCS; expose it typed for the Win32 API.
#define MOUSE_CS ((LPCRITICAL_SECTION)&g_pMouseCS)

// --- External helpers ------------------------------------------------------
extern int   Theme_SetTimer(void* fn, int periodMs);  // start a periodic timer
extern void  Theme_KillTimer(int timerId);            // stop a timer
extern void  Win_CloseHandle(void* phandle);          // CloseHandle + null
extern char  g_abIniPath[];                           // game INI file path

// The hit-test / double-click timer callbacks (jump-table thunks in the
// original; resolved at link time).
extern int   Mouse_HitTestCursor(void);   // func_0x004010e1 — cursor under pointer
extern void  Mouse_DblClickTick(void);    // func_0x00401b90 — dbl-click timer proc

// ---------------------------------------------------------------------------
// Curs_CancelDblClickTimer (0x00451e90)
//   Cancel a pending double-click timer and commit the recorded click as a
//   single click at the original press position.
// ---------------------------------------------------------------------------
void Curs_CancelDblClickTimer()
{
    EnterCriticalSection(MOUSE_CS);
    if (g_nMouseDblClickTimer != -1)
    {
        Theme_KillTimer(g_nMouseDblClickTimer);
        g_nMouseDblClickTimer = -1;
        g_nMouseButtons = 1;             // commit a left click
        g_nMouseX = g_nMouseClickX;
        g_nMouseY = g_nMouseClickY;
        SetEvent(g_pMouseStateEvent);
    }
    LeaveCriticalSection(MOUSE_CS);
}

// ---------------------------------------------------------------------------
// Curs_SetMouseOrigin (0x00451f90)
//   Set the origin offset subtracted from raw WM_MOUSEMOVE coordinates.
// ---------------------------------------------------------------------------
void Curs_SetMouseOrigin(int x, int y)
{
    g_nMouseOriginX = x;
    g_nMouseOriginY = y;
}

// ---------------------------------------------------------------------------
// Curs_HandleMouseMsg (0x00452030)
//   Main mouse message handler.  Called from the window proc with the Win32
//   mouse message id, wParam button flags, and packed lParam coordinates.
//   Maintains g_nMouseButtons / g_nMouseX/Y and drives double-click timing.
// ---------------------------------------------------------------------------
int Curs_HandleMouseMsg(unsigned int msg, unsigned int wParam, unsigned int lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE: // 0x200
        g_nMouseButtons = 0;
        g_nMouseX = (int)(lParam & 0xffff) - g_nMouseOriginX;
        g_nMouseY = (int)(lParam >> 16)    - g_nMouseOriginY;
        SetEvent(g_pMouseMoveEvent);

        // Re-evaluate the cursor under the pointer and tick the cursor anim.
        Curs_SetCursorByMode(Mouse_HitTestCursor());
        Curs_Tick();

        EnterCriticalSection(MOUSE_CS);
        {
            int dx = g_nMouseX - g_nMouseClickX;
            int dy = g_nMouseY - g_nMouseClickY;
            // If no timer is pending, or the pointer stayed within the
            // double-click radius (<201 px^2), keep tracking button bits.
            if (g_nMouseDblClickTimer == -1 || dx * dx + dy * dy < 0xc9)
            {
                if (g_nMouseDblClickTimer == -1)
                    g_nMouseButtons = (wParam & 1) != 0;   // left held?
                LeaveCriticalSection(MOUSE_CS);
                g_nMouseButtons |= (wParam & 2) ? 2 : 0;   // right held?
            }
            else
            {
                // Pointer moved too far: drag, not a double-click.
                Mouse_DblClickTick();
                LeaveCriticalSection(MOUSE_CS);
            }
        }
        break;

    case WM_LBUTTONDOWN: // 0x201
        g_nMouseBtnDownMask |= 1;
        g_nMouseButtons = 0;
        EnterCriticalSection(MOUSE_CS);
        if (g_nMouseDblClickEnabled == 0)
        {
            // Double-click disabled: emit click immediately via timer proc.
            g_nMouseClickX = g_nMouseX;
            g_nMouseClickY = g_nMouseY;
            g_nMouseDblClickTimer =
                Theme_SetTimer((void*)&Mouse_DblClickTick,
                               (int)(1000 / GetDoubleClickTime()));
            Mouse_DblClickTick();
        }
        else if (g_nMouseDblClickTimer == -1)
        {
            // Arm the double-click timer; a second down within the window
            // is detected by the timer proc.
            g_nMouseClickX = g_nMouseX;
            g_nMouseClickY = g_nMouseY;
            g_nMouseDblClickTimer =
                Theme_SetTimer((void*)&Mouse_DblClickTick,
                               (int)(1000 / GetDoubleClickTime()));
        }
        LeaveCriticalSection(MOUSE_CS);
        break;

    case WM_LBUTTONUP: // 0x202
        g_nMouseBtnDownMask &= ~1;
        g_nMouseButtons = (wParam & 2) ? 2 : 0;
        break;

    case WM_LBUTTONDBLCLK: // 0x203
        EnterCriticalSection(MOUSE_CS);
        if (g_nMouseDblClickTimer != -1)
        {
            Theme_KillTimer(g_nMouseDblClickTimer);
            g_nMouseDblClickTimer = -1;
            g_nMouseButtons = 0x100;       // double-click flag
        }
        LeaveCriticalSection(MOUSE_CS);
        break;

    case WM_RBUTTONDOWN: // 0x204
        g_nMouseBtnDownMask |= 2;
        g_nMouseButtons = ((wParam & 1) != 0) + 2;
        break;

    case WM_RBUTTONUP: // 0x205
        g_nMouseBtnDownMask &= ~2;
        g_nMouseButtons = (wParam & 1) != 0;
        break;
    }

    // (left|right) == 3 is not a meaningful combined state; clear it.
    if (g_nMouseButtons == 3)
        g_nMouseButtons = 0;

    SetEvent(g_pMouseStateEvent);
    return 0;
}

// ---------------------------------------------------------------------------
// Curs_InitMouse (0x00452440)
//   Initialise the mouse subsystem: critical section, the two sync events,
//   and the [Mouse]DoubleClick INI option (default enabled).
// ---------------------------------------------------------------------------
int Curs_InitMouse()
{
    InitializeCriticalSection(MOUSE_CS);
    g_pMouseStateEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    g_pMouseMoveEvent  = CreateEventA(NULL, FALSE, FALSE, NULL);
    g_nMouseDblClickEnabled =
        GetPrivateProfileIntA("Mouse", "DoubleClick", 1, g_abIniPath);
    return 0;
}

// ---------------------------------------------------------------------------
// Curs_CloseMouseEvents (0x00452520)
//   Close the two mouse sync event handles.
// ---------------------------------------------------------------------------
void Curs_CloseMouseEvents()
{
    Win_CloseHandle(&g_pMouseStateEvent);
    Win_CloseHandle(&g_pMouseMoveEvent);
}

// ---------------------------------------------------------------------------
// Curs_GetMouseState (0x00452770)
//   Return the current screen cursor position and logical button state.
// ---------------------------------------------------------------------------
void Curs_GetMouseState(int* outX, int* outY, int* outButtons)
{
    GetCursorPos((LPPOINT)&g_nMouseScreenX);
    *outX       = g_nMouseScreenX;
    *outY       = g_nMouseScreenY;
    *outButtons = g_nMouseButtons;
}

// ---------------------------------------------------------------------------
// Curs_Nop1..Curs_Nop5 (0x004525c0, 0x00452650, 0x004526e0, 0x00452830,
//                       0x004528c0)
//   Empty stubs (SEH frame setup/teardown only — no body in the release
//   build; debug-only helpers whose contents were macro-stripped).
// ---------------------------------------------------------------------------
void Curs_Nop1() {}
void Curs_Nop2() {}
void Curs_Nop3() {}
void Curs_Nop4() {}
void Curs_Nop5() {}
