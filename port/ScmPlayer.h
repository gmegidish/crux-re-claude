// ScmPlayer.h — play a named SCM video resource to the display.
//
// Resolves an SCM by name (type-16 resource), parses the container and renders
// every frame (palette chunks + video sprites) to the 8-bit framebuffer, then
// presents. This is the back-end the RunProg op 0x71 (RUN_SCENE) drives, and
// what the original engine reaches via RunProg_PlayScmWithPaletteGuard ->
// Player_PlayScm.
#pragma once
#include "ResArchive.h"
#include "Display.h"
#include "Framebuffer.h"

// Play SCM resource `name`. Returns false if the user asked to quit (window
// close), true otherwise (including "not found", which is logged).
bool playScmByName(ResArchive& arc, Display& disp, Framebuffer& fb, const char* name);
