// Cursor.h — software mouse-cursor module (clean-room port of CURSORS.cpp).
//
// Cursors are TYPE-2 resources in the archive (e.g. "CSDEF", "CURSDRAG",
// "CURSINV", "CURSHOUR"). They share the anim/Help_BlitImage container:
//   [u8 0x10][u16 ver=1][u16 W][u16 H][u8 maskFlag][u32 frameCount]
//   then frameCount x {s16 x, s16 y, s32 size}, then each frame's sprite blob.
// The per-frame {s16 x, s16 y} is the cursor HOTSPOT: the original engine's
// Curs_SetPosition (0x00419360) computes the draw origin as
//   g_nCursorX = mouseX - hotX;  g_nCursorY = mouseY - hotY;
// (see src/CURSORS.cpp). We load frame 0, keep its blob + hotspot, and blit
// with decodeSprite at (mouseX - hotX, mouseY - hotY). CSDEF is the default
// arrow (cursor id 1 in the engine).
#pragma once
#include "ResArchive.h"
#include "Framebuffer.h"

namespace Cursor {

// Load a type-2 cursor sprite by name (e.g. "CSDEF"). Copies frame 0's sprite
// blob and records its hotspot. Returns false (and logs) if not found / bad.
bool load(ResArchive& arc, const char* name);

// Blit the default cursor with its hotspot at (mouseX, mouseY) via decodeSprite.
// No-op if nothing is loaded. Bounds-safe (decodeSprite clips to the framebuffer).
void draw(Framebuffer& fb, int mouseX, int mouseY);

// Clear all loaded cursors (default + per-mode).
void reset();

// Load a cursor into a mode slot (the engine's cursor ids: 0/8=CURSAREA,
// 2=CURSEXIT, 3=CURSINV, 9=CURSHOUR). Used to show a context cursor over areas.
bool loadMode(ResArchive& arc, int mode, const char* name);

// Draw the cursor for `mode` (the hovered area's cursor id), falling back to the
// default arrow when that mode isn't loaded or mode < 0 (nothing hovered).
void drawMode(Framebuffer& fb, int mode, int mouseX, int mouseY);

}  // namespace Cursor
