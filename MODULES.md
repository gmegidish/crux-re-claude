# CRUX.EXE Module Notes

Per-file knowledge base for the RE project. Updated as each file is reversed.

Status legend: ✅ complete · 🔶 partial · ⬜ not started

---

## ✅ CURSORS.cpp — Software sprite cursor
**Address range:** `0x00418640 – 0x0041a7b0`  
**Functions:** 30/30  
**Output:** `src/CURSORS.cpp`, `src/CURSORS.h`

The game runs entirely in a DirectDraw 320×480 (640×480 logical) surface and replaces the OS cursor with a software-drawn sprite. This module owns the full cursor lifecycle: loading cursor graphics from the resource system, blitting the cursor to the back-buffer each frame using a save-behind mechanism, erasing it on the next frame, and switching between up to 16 named cursor shapes. `Curs_SetWin32Cursor` switches to a Win32 cursor (used inside the GV inventory window). Mode 4 is a special "pick-up" mode that drives the drag-and-drop system in Graninv.cpp.

**Key globals:** `g_nCurrentCursor`, `g_nCursorMode`, `g_nCursorX/Y`, `g_nCursorDrawEnabled`  
**Depends on:** GI.cpp (DDI surface ops), Img.cpp (blit helpers)  
**Used by:** Graninv.cpp (`Curs_SetWin32Cursor`, `Curs_DisableDraw/EnableDraw`)

---

## ✅ Graninv.cpp — Granular inventory window + item interactions
**Address range:** `0x00430f5b – 0x004390eb`  
**Functions:** 67/67  
**Output:** `src/Graninv.cpp`, `src/Graninv.h`

Two distinct subsystems share this file:

### GV — Floating inventory panel (functions 0x00431210–0x00433370)
A detachable Win32 tool window (`GVClass`) that displays inventory items as large icons in a `SysListView32`. A `CreateToolbarEx` toolbar sits at the top, populated via `GV_AddButton`. When the player drags an item, `GV_LoadDragGraphics` loads source and destination item bitmaps into off-screen DirectDraw surfaces, and `GV_DragUpdate` renders them each tick rotated towards the drag direction via `GV_RotateBitmap` (2D rotation matrix in software). `GV_CanDrop` validates slot compatibility. The window runs on a separate Win32 message pump thread and synchronises with the game via a critical section.

**Key GV globals:** `g_pGVWindow` (HWND), `g_pGVListView`, `g_pGVToolbar`, `g_pGVImageList`, `g_nGVOpen`, `g_nGVEnabled`, `g_nGVDragStartX/Y`, `g_nGVDragCurX/Y`

### Gran — Per-item mini-interactions (functions 0x00434160–0x00439050)
Six independent item-level mini-systems:

| Subsystem | Functions | What it does |
|-----------|-----------|--------------|
| **Cube viewer** | `Gran_ShowCube`, `Gran_FaceToIdx`, `Gran_FreeCube`, `Gran_ResetCube`, `Gran_LoadItem`, `Gran_DrawItem` | Encodes 8 face values (1–8 → 2-bit pairs) into a 32-bit cube ID, loads `CUB_XXXXX` resource |
| **Drag input** | `Gran_SetCapture`, `Gran_ReleaseCapture`, `Gran_GetAngleDist` | Waits for a mouse drag, returns angle (degrees×10) and pixel distance |
| **Animation layers** | `Gran_ClearAnims`, `Gran_SetAnim`, `Gran_PlayAnim`, `Gran_StartAnim`, `Gran_SetAnimHandle`, `Gran_SetTrigger`, `Gran_CheckAnimDone`, `Gran_SetMovHandle`, `Gran_StartWait`, `Gran_EndWait` | 3-layer 12×12 animation handle table; fires a script trigger when animations complete |
| **Palette / blit** | `Gran_ConvPal`, `Gran_BlitToScreen`, `Gran_Dissolve` | Palette cross-fade for inventory cut-scenes; scanline-interleaved blit in 3 passes; Fisher-Yates shuffle dissolve reveal |
| **Tape / diary** | `Gran_InitTape`, `Gran_TapeCommand`, `Gran_TapeFF`, `Gran_TapeRew`, `Gran_DiaryPlay`, `Gran_SetTapeState`, `Gran_LoadTapeData`, `Gran_SaveTapeData` | VCR-style tape transport over 8000-slot position table; plays speech via `Gran_DiaryPlay`; Villa tapes (slots 800–820) advance as a group |
| **Board game** | `Gran_InitBoard`, `Gran_UpdateBoard`, `Gran_GetPosParity`, `Gran_RestoreBoardRow`, `Gran_AdvancePiece`, `Gran_MoveAlien`, `Gran_CalcBoardMove`, `Gran_UpdateGrannyPos` | 6-piece board game where "Granny" (NPC) chases alien pieces across a 6×7 parity grid; `Gran_MoveAlien` uses time-seeded randomness biased by a preferred-direction heuristic |
| **Slider** | `Gran_InitSlider`, `Gran_SetSliderRange`, `Gran_UpdateSlider`, `Gran_EndSlider`, `Gran_StopSlider` | Horizontal drag-slider mapped to an animation frame range |
| **Help queue** | `Gran_InitHelpQueue`, `Gran_AddHelp`, `Gran_RemoveHelp`, `Gran_ShiftHelp` | FIFO queue (max 60 items) of help-text item IDs; owner item tracks the active head |

**⚠ Speculative cross-module calls:** ~34 helper calls in `src/Graninv.cpp` use invented names (`GetItemByTag`, `BlitResource`, `FireTimer`, `PickUpItem`, `LoadResourceByTag`, `LockSurface`, etc.) because GI.cpp, INVMANG.cpp, PLAYER.cpp, and SCHED.cpp are not yet reversed. These will need updating.

**Depends on:** GI.cpp (surface ops, GI window resize), INVMANG.cpp (item list, slot flags), PLAYER.cpp (item pickup), SCHED.cpp (event loop, timer fire), SPEECH.cpp (diary playback), Img.cpp (blit), SETPAL.cpp (palette ops), READRES.cpp (resource load/free)  
**Used by:** ADVENT.cpp (likely — inventory scripting), THEMES.cpp

---

## 🔶 Img.cpp — Image blitting and RLE compression
**Address range:** `0x0043a1b0 – 0x0043c2ff`  
**Functions:** 18/19  
**Output:** `src/Img.cpp`, `src/Img.h`

Core 2D blit library. Provides plain and RLE-compressed blits (opaque and transparent), horizontal/vertical flipping, scaled blits with pre-built scale tables, region packing, and viewport clipping. Scale tables (`g_nXScaleSteps`, `g_nYScaleRows`, `g_nSizeScaleTables`) are allocated once from an 820 KB pool on first use and cached by width. The one un-reversed function (`PutLine_RLE_Flip` at the end of the range) handles a combined RLE+flip path.

**Key globals:** `g_nClipX/Y`, `g_nXScaleSteps[640]`, `g_nYScaleRows[480]`, `g_nSizeScaleTables[640]`, `g_nSizeScalePoolBase`, `g_nTempScaleRow[640]`, `g_nCachedScaleWidth`  
**Depends on:** GI.cpp (framebuffer pointer), Memalloc.cpp (scale pool alloc)  
**Used by:** CURSORS.cpp, Graninv.cpp (drag preview), RESCALE.cpp, probably all rendering modules

---

## ⬜ ADVENT.cpp — Adventure / scripting engine
**Address range:** `0x0040e3aa – 0x004132a0` (est.)  
**Functions:** 47/47 unreversed

Likely the game's script interpreter and adventure-game verb dispatcher. The name "ADVENT" is classic Sierra/LucasArts convention for the adventure engine layer. Probably handles verb+noun command resolution, script bytecode execution, room transitions, and object-interaction dispatch. Expected to be a heavy consumer of INVMANG, PLAYER, SCHED, and THEMES.

---

## ⬜ Advanim.cpp — Advanced / skeletal animation
**Address range:** `0x00403955 – 0x0040e3a9` (est.)  
**Functions:** 97 — the largest game file

97 functions suggests this is either a full skeletal or sprite-sequence animation system, or a combined animation + scene composition layer. Given the "Adv" prefix shared with ADVENT, it may be the animated cutscene/character-animation engine. The large function count suggests clip management, frame blending, and callback scheduling.

---

## ✅ ANI32.cpp — Scaled-sprite blit + AREAS spatial-index bridge
**Address range:** `0x00413530 – 0x00413bd0`  
**Functions:** 2/2  
**Output:** `src/ANI32.cpp`, `src/ANI32.h`

NOT thin animation wrappers — these two functions bridge the sprite render path and
the AREAS hit-test system:
- **`Ani32_DrawScaledRLE`** (`0x00413530`) — decode + scale an RLE paletted sprite (int16 w/h header, transparent/opaque run tokens), centre + bottom-anchor, blit column-by-column to the 8bpp surface, AND publish its on-screen bbox to the AREAS active-bbox globals so the drawn character is cursor-hittable. Honours `[Mouse] HalfHero` INI flag.
- **`Ani32_BuildAreaLookup`** (`0x00413bd0`) — full rebuild of the AREAS Y-bucket spatial index (debug name `area_lookup_init`); the full-rebuild counterpart to AREAS.cpp's per-node `Area_AddNodeToYBuckets`. Serialised on `g_nAreaCritSec`.

**⚠ Overlap note:** `Ani32_BuildAreaLookup` is conceptually AREAS.cpp code (shares all AREAS globals); kept in ANI32 per the binary's function layout.

**Depends on:** AREAS.cpp (node table, Y-buckets, bbox globals), Img/GI blit  
**Used by:** the character render path

---

## ✅ AREAS.cpp — Sprite-area hit-testing and node management
**Address range:** `0x00413f10 – 0x00414ed0`  
**Functions:** 15/15 (+1 boundary: `Bani_PutBlock` at `0x00414f90` is the first BANI.cpp function)  
**Output:** `src/AREAS.cpp`, `src/AREAS.h`

AREAS.cpp is the **spatial hit-testing and node-management layer**, not a room-geometry loader (that's THEMES/READRES). It owns:

**Y-bucket spatial index** — 120 rows × 200 int slots (`g_anAreaYBuckets[120][200]`). Row = `y/4`. Each row is a -1-terminated list of node indices. `Area_FindAt(x,y)` indexes directly to row `y/4` instead of scanning all nodes — O(bucket) hit detection.

**Node record** (pointed to by `g_pAreaNodeTable[i]`, ≥0x98 bytes):
```
[+0x00] int x, [+0x04] int y, [+0x08] int x2, [+0x0C] int y2  — bounding box
[+0x10] int flags  — bit check decides walkable/hittable
[+0x14] int tag    — unique ID, searched by Area_FindNodeByTag
[+0x90] int z      — depth for hit-priority comparison
[+0x94] int type   — 2 = excluded from hit-test
```

**Sprite-area list** — up to 8 dynamic sprite entries (stride 0x20). Used for clickable objects overlaid on the background. `Area_RemoveSprite` and `Area_RemoveSpriteAt` compact the list on removal.

**Selection list** — a simple resizable int list with cursor navigation (`Area_RewindList`, `Area_ListNext`, `Area_ListPrev`, `Area_ListGet/Set/Append`). Used to accumulate area query results.

**⚠ Global collision with MOVEMENT.cpp:** `g_pAreaNodeTable` at `0x0070ded8` is the same address `OTF_AllocSlot` (MOVEMENT.cpp boundary) writes to — on-the-fly nodes are area nodes inserted at runtime. `g_nAreaNodeCount` at `0x007127e8` is what MOVEMENT.cpp's boundary function increments.

**Key globals:** `g_anAreaYBuckets[120][200]`, `g_pAreaNodeTable`, `g_nAreaNodeCount`, `g_nAreaLastHit`, `g_nAreaSpriteCount`, `g_nAreaActiveNode`, `g_nAreaActiveBBox*`  
**Depends on:** READRES.cpp (node data loaded from resources), DDRAWI.cpp (CS)  
**Used by:** MOVEMENT.cpp (walk nodes), ADVENT.cpp (click dispatch), CURSORS.cpp (cursor shape)

---

## ✅ BANI.cpp — Block-animation image codec
**Address range:** `0x00414f90 – 0x00417df0`  
**Functions:** 19/19 (+2 CURSORS boundary functions at `0x00417e80`/`0x00418210`)  
**Output:** `src/BANI.cpp`, `src/BANI.h`

A **block-based RLE image codec**, not (only) looping room animations. A BANI image =
9-byte header (tag, int16 width/height/blockW/blockH) + a row-major grid of compressed
blocks. Each block is a bytecode stream decoded in **boustrophedon (serpentine)** order —
rows alternate L→R / R→L, the step flips at each row boundary.

- `Bani_PutBlock`/`Bani_PutIndi` — per-block draw-op dispatchers (by leading byte)
- Per-index-width RLE decoders: `Bani_DecodeRle6`/`Rle4`/`Rle3`/`RleNibble` + `*Remap` variants (palette-remap), plus `Bani_BlitRaw`/`BlitRawRemap` and continuation-substream decode/skip (`Bani_DecodeRleCont`/`SkipRleCont`)
- `Bani_DrawBlocks`/`Bani_DrawBlocksIndi` — walk the block grid
- `Bani_InitNoLoopList`/`Bani_IsNoLoopId` — table of play-once resource IDs

**⚠ Boundary:** `0x00417e80`/`0x00418210` = `Curs_Init`/`Curs_LoadCursor` (CURSORS.cpp), not BANI — confirmed by `CURSO` debug strings.

**Key globals:** `g_pBaniPalette`, `g_nBaniPaletteCount`, `g_anBaniNoLoopIds[100]`, `g_nBaniNoLoopCount`  
**Depends on:** GI.cpp blit, READRES (resource load)  
**Used by:** room rendering, animation system

---

## ⬜ DDRAWI.cpp — DirectDraw wrapper / init
**Address range:** `0x0041a89c – 0x0041f025` (est.)  
**Functions:** 37

DirectDraw initialisation, surface management, and page-flip. The suffix "RAWI" is almost certainly `RAWI = RAW Interface` — a thin wrapper over the Win95 DirectDraw API. Probably creates the primary + back surfaces, handles lost-surface recovery, and owns the `DirectDrawCreate` call seen in the binary's imports. May include the game's main blit-to-screen path.

---

## ✅ ERRORS.cpp — Error handling, assert system, and call-stack tracker
**Address range:** `0x0041f480 – 0x00420bd0`  
**Functions:** 31/31  
**Output:** `src/ERRORS.cpp`, `src/ERRORS.h`

Provides the game's full error and diagnostic infrastructure:

- **`Debug_Assert` (`0x0041f4b0`)** — 2-param public assert wrapper; calls inner handler with `g_pAssertMsg`
- **`Debug_TraceVal` (`0x0041fa60`)** — conditional MessageBox, only in debug builds (`g_nReleaseMode == 0`)
- **`Err_Fatal` (`0x00420bd0`)** — writes ERROR_LOG, shows fatal MessageBox, offers "Restart" via `CreateProcess`
- **`Err_Dispatch` (`0x0041fb70`)** — 42-case switch that routes error codes to message strings → `Err_Fatal`
- **`Err_RestartGame` (`0x0041f960`)** — relaunches CRUX.EXE via `CreateProcess` then `ExitProcess`
- **`Err_LoadStrings` (`0x0041f160`)** — reads `ERRORS.TXT` into `g_pErrStrings[0..23]`
- **`Err_ReadDebugLevel` (`0x0041f120`)** — reads `[General] DebugLevel` from `CRUX.INI` → `g_nDebugFlags`
- Call-stack tracker: `Err_PushStack`/`Err_ClearStack` + `g_apCallStack[50]`/`g_nCallStackDepth`
- 13 typed fatal-error helpers: `Err_BadResFile`, `Err_MissingRes`, `Err_OutOfMemory`, `Err_BadSound`, `Err_DriveCheck`, etc.

**⚠ Correction:** `thunk_FUN_0041f680` resolves to `Err_BadResEntry` (fatal: corrupt archive entry), NOT `Debug_Assert`. The previous MODULES.md speculation was wrong.

**Key globals:** `g_nDebugFlags`, `g_pErrStrings[24]`, `g_nCallStackDepth`, `g_apCallStack[50]`, `g_nHwndMain`, `g_nReleaseMode`, `g_pCurrentRoom`  
**Depends on:** Win32 MessageBox, CreateProcess, CRUX.INI  
**Used by:** Every module (Debug_Assert at every assert site)

---

## ✅ Except.cpp — does not exist as a distinct module (→ FILES.cpp)
**Address range:** `0x00420e50 – 0x00422240` (the "Except.cpp" anchor estimate)  
**Functions:** 0 of its own — all 6 are FILES.cpp front-matter  
**Output:** `src/Except.cpp` (documents the misattribution)

The 6 functions the anchor table assigned to "Except.cpp" actually carry
`C:\DevStudio\Projects\Crux\FILES.cpp` debug strings and are the front of FILES.cpp:
`Files_SetCurrentRoom`, `Files_SetErrSource` (the `Err_PushFrame` error-location helper used
engine-wide), `Files_GetTime`, **`Files_LoadScn`** (the compiled `.SCN` scene-file loader —
string tables + 0xB0-byte area-node array + cache records + slot table + on-the-fly node lists),
`Files_DoRead`, `Files_FreadString`. The `unaff_FS_OFFSET` "SEH frame" that suggested exception
code is just MSVC's standard `__try` prologue emitted around any function reaching the fatal-error
path. There is no separate C++ exception module. These are counted under FILES.cpp.

---

## ⬜ FILES.cpp — File I/O and resource file access
**Address range:** `0x00422298 – 0x0042a83b` (est.)  
**Functions:** 42

File system abstraction — open/read/write/close over the game's resource files (probably a `.DAT` or `.RES` archive). May implement the tagged resource lookup system (the `thunk_FUN_0040b0c0` / `LoadResourceByTag` pattern). Given Crux targets Win95, likely uses `CreateFile`/`ReadFile`.

---

## ✅ FRMTIMER.cpp — Frame-rate limiter
**Address range:** `0x0042a7f0 – 0x0042abf0`  
**Functions:** 5/5 (+1 FX boundary: `Fx_PlayAnyChar` at `0x0042ac80`)  
**Output:** `src/FRMTIMER.cpp`, `src/FRMTIMER.h`

A multimedia-timer frame limiter with sub-frame resolution. `FrmTimer_Init` creates an
auto-reset event and starts a `timeSetEvent` timer at `fps*3` ms (3× oversample).
`FrmTimer_OnTick` accumulates real wall-clock delta; when it crosses `threshold = 930/fps`,
it subtracts one true `period = 1000/fps` (carrying remainder so cadence doesn't drift),
signals `g_hFrmEvent`, and advances `Res_TickFrameCounter` + `SndMem_AdvanceLipsync`. The
render loop blocks on `g_hFrmEvent` — that wait is the actual cap. The 930-vs-1000 split
biases toward emitting slightly early rather than dropping a frame. `FrmTimer_SetFps`
changes rate at runtime; `FrmTimer_GetFps` queries.

**⚠ Boundary:** `0x0042ac80` = `Fx_PlayAnyChar` (FX.cpp) — finds a free SFX channel (slots 4-6) and calls `Mixer_PlayChannel`.

**Key globals:** `g_dwFrmTimerId`, `g_dwFrmAccumMs`, `g_nFrmFps`, `g_dwFrmThreshold`, `g_dwFrmPeriodMs`, `g_hFrmEvent`  
**Depends on:** THEMES timer wrappers (`timeSetEvent`), READRES (frame counter), SOUNDMEM (lipsync clock)  
**Used by:** Winmain execution thread / render loop

---

## ✅ FX.cpp — Looping sound-effect layer
**Address range:** `0x0042ac80 – 0x0042b6d0`  
**Functions:** 11/11 (+1 GI boundary: `GI_InitSurfaces` at `0x0042b760`)  
**Output:** `src/FX.cpp`, `src/FX.h`

**Sound effects, NOT visual** (every routine drives `SndMem_ReadSound` + `Mixer_PlayChannel`;
debug strings are `fx_play_*`). FX layers on MIXER/SOUNDMEM to manage **one looping background
effect on channel 3**, implemented by re-arming the mixer's channel-done callback so the sound
replays when it finishes. `Fx_PlayAnyChar` (`0x0042ac80`) handles one-shot effects on the
channel 4-6 pool.

- `Fx_Play` (public entry: lowercase name, store, set loop, start) → `Fx_PlayChar` → `Fx_LoopCallback` (re-arm)
- `Fx_Stop`/`Fx_StopLoop`/`Fx_ClearCallback`/`Fx_Resume`/`Fx_SetVolume`/`Fx_WaitChannel3`

**⚠ Boundary:** `0x0042b760` = `GI_InitSurfaces` (`gi_init`, GI.cpp) — creates the DirectDraw offscreen/back/overlay/dev surfaces, sets `g_nGIReady`.

**Key globals:** `g_nFxLoop`, `g_szFxName[256]`, `g_nFxVolume`  
**Depends on:** MIXER.cpp, SOUNDMEM.cpp (`SndMem_ReadSound`)  
**Used by:** ADVENT/RUNPROG sound opcodes

---

## ⬜ GI.cpp — Game Interface / DirectDraw operations
**Address range:** `0x0042b7ca – 0x00430f5a` (est.)  
**Functions:** 66

The core hardware abstraction layer between game logic and DirectDraw. 66 functions makes this one of the largest files. Based on what Graninv.cpp calls through thunks: `GI.cpp` provides surface lock/unlock, off-screen surface creation (`DDI_CreateOffscreenSurf`), blit-rect operations, page flip, palette setting, and the GI window geometry system (`ResizeGI`, `CloseGI`). Also likely owns `thunk_FUN_0042c370` (ResizeGI) and `thunk_FUN_0042c470` (CloseGI) seen in Graninv.

**High priority to reverse** — many Graninv.cpp speculative calls will resolve against this file.

---

## ✅ HELPSTK.cpp — Help-queue save/load + shared RLE line primitives
**Address range:** `0x004391b0 – 0x00439f70`  
**Functions:** 9/9  
**Output:** `src/HELPSTK.cpp`, `src/HELPSTK.h`

Two unrelated groups share this object file:
- **Help-queue persistence (2 fns):** `Help_SaveHelpStack`/`Help_LoadHelpStack` serialize the
  60-slot help-ID queue (`g_anGranHelpQueue[60]` + owner + count) to/from a save stream. The
  push/pop/shift logic itself lives in Graninv.cpp (`Gran_*Help`, whose asserts cite HELPSTK.cpp);
  HELPSTK only owns the buffer definition + save/load.
- **RLE sprite-line primitives (7 fns):** `Help_PutLineScaled`, `Help_LineCopyRun`,
  `Help_LineDecodeRLE`/`SkipRLE`/`DecodeRLEOffset`, `Help_BlitImage`, `Help_PutLine` —
  per-scanline opcode decoders. **⚠ These assert against `Img.c`, not HELPSTK** — they're
  shared rendering primitives in this TU as a compiler artifact. `Help_BlitImage` (`0x00439d80`)
  is the target of GI's `thunk_FUN_00439d80` and delegates opcode 0x04 to `Bani_DrawBlocks`.

**Key globals:** `g_anGranHelpQueue[60]` (0x006d6770); reuses `g_nGranHelpOwner`/`g_nGranHelpCount`  
**Depends on:** save-stream I/O (FILES), Bani_DrawBlocks, Img scale table  
**Used by:** Graninv.cpp help queue, GI blit path

---

## ✅ INVMANG.cpp — Inventory item icon atlas manager
**Address range:** `0x0043c910 – 0x0043d5b0`  
**Functions:** 5/6 (sixth `0x0043d690` is `Magwrit_InitPenNet` — first magwrit.cpp function)  
**Output:** `src/INVMANG.cpp`, `src/INVMANG.h`

INVMANG manages a **texture atlas** for inventory item icons, not a flat item array. It packs item icon pixel data into a fixed memory pool and maps item indices to atlas slots:

- **`Inv_GetResource(itemIdx, &w, &h)`** — lazy-loads item icon into atlas; returns pool pointer + dimensions; called by Graninv's `GV_LoadDragGraphics`
- **`Inv_AllocSlot(nBytes)`** — finds/compacts atlas slots to fit N bytes by sliding pixel data; returns slot index
- **`Inv_FreeSlot(itemIdx)`** — clears atlas slot, resets `g_anItemSlot[itemIdx]` to -1
- **`Inv_GetByTag(tag)`** — finds item index by integer tag (field at `obj+0x00`); resolves Graninv's `GetItemByTag`
- **`Inv_GetByName(name)`** — finds item index by name string (field at `obj+0xd0`)

**Resolved Graninv.cpp stubs:** `GetItemByTag` → `Inv_GetByTag`; `GetItemResource` → `Inv_GetResource`  
**Not here:** `g_anItemFlags[]`, `g_anGVItemList[]`, `g_nGVItemCount` (live in Graninv/GI); `PickUpItem` (in GI.cpp or ADVENT.cpp)

**Key globals:** `g_anItemSlot[]`, `g_abInvSlots[50×9]`, `g_nItemCount`, `g_apItems[]`, `g_pTexPool`  
**Depends on:** READRES.cpp (resource loading), Memalloc.cpp (pool)  
**Used by:** Graninv.cpp (GV inventory window drag-and-drop)

---

## ✅ magwrit.cpp — Wacom/PenNet tablet input driver (dev tool)
**Address range:** `0x0043d6cc – 0x0043f280`  
**Functions:** 25/25  
**Output:** `src/magwrit.cpp`, `src/magwrit.h`

A **Wacom tablet ("PenNet") input driver** — a dev/authoring-tool input path, loaded from an
external KWT DLL via function pointers. Manages ink "tasks" (pen strokes), tablet buttons, a
message pump, and a status display.

- **Tasks:** 30-slot table (0x70-byte records): `{taskId, managerHandle, callbackId, rangeLow,
  rangeHigh, 20-entry ink-point ring buffer, count, eventMask, state}`. `Magwrit_AddTask`/`EndTask`/
  `DetachTask`, range setters, `GetTaskPointCount`/`PopTaskPoint`, `DrawTaskInk`/`DrawInkSegment`.
- **Buttons:** 10-slot live binding table `{buttonId, callbackId}` + a 100-entry rect map loaded
  from the `[BUTTONS_MAP]` INI section; `Magwrit_OnButtonClicked` fires the bound callback once via
  `Timer_AddSyncProg` then detaches. `Magwrit_AddButton`/`DetachButton`/`DetachAllButtons`.
- **Messages:** DLL posts type 1 (pen point) / 2 (task-manager) / 3 (task-proc); routed by
  `Magwrit_DispatchPenNetMessage` → `OnTaskProcMessage`/`OnTaskManagerMessage`, queued in a
  200-entry FIFO (`EnqueueTaskMessage`/`DequeueMessage`).
- Lifecycle: `Magwrit_InitPenNet`/`TabletInit`/`LoadKwtDll`/`PostInitCallback`/`CleanExit`.

Low gameplay relevance (content-authoring input), reversed for completeness.

**Depends on:** external KWT/PenNet DLL (fn-ptr globals), TIMERS (`Timer_AddSyncProg`), GI (ink draw)

---

## ✅ Memalloc.cpp — sound-memory-pool partitioning
**Address range:** `0x0043f550 – 0x0043fd10` (genuine functions; the `0x00440050+` block that the anchor estimate covered is actually MIXER.cpp)  
**Functions:** 4/4  
**Output:** `src/Memalloc.cpp`, `src/Memalloc.h`

NOT a general heap allocator (raw alloc/free are MSVC CRT, e.g. `FUN_0048ac60`). These 4 functions
partition one large heap block into theme / secondary / music(sound) sub-pools:
- **`Mem_InitFromIni`** (`0x0043f550`) — reads requested theme/music/speech sizes from CRUX.INI, allocates the backing block, calls the partitioners
- **`Mem_InitPools`** — boots the inventory/heap manager (`InitInvMang(1)`)
- **`Mem_PickPoolLayout`** — scores up to 5 candidate split layouts, returns the best fit (1-5) or 0
- **`Mem_PartitionPool`** — commits the chosen layout into concrete base offsets + sizes

**⚠ Attribution:** the anchor estimate (37 fns to 0x00447ce7) was wrong — that range is MIXER.cpp.
Genuine Memalloc is just these 4.

**Key globals:** `g_nMemPoolBase/TotalSize`, `g_nMemPoolSecondaryBase/Size`, `g_nThemeMemRegionBase/Size`, `g_nSndMemPoolBase/Size`, `g_nMemPoolLayoutOverflow`  
**Used by:** THEMES (music pool), SOUNDMEM (sample pool), startup

---

## ✅ MIXER.cpp — DirectSound mixer
**Address range:** `0x00440050 – 0x004513f0` (true range; anchor estimate `0x00447ce8` was wrong)  
**Functions:** 42/42  
**Output:** `src/MIXER.cpp`, `src/MIXER.h`

Low-level DirectSound mixing engine. A background thread (`Mixer_ThreadProc`) fills
the DirectSound buffer from a sorted list of active channels; 8 format-specialised
PCM fill kernels (`Mixer_FillBuf_*` for U8/S16 × mono/stereo × rate) do the actual
sample conversion. `Mixer_Init` reads `[Sound]` params and creates the DirectSound
objects; `Mixer_PlayChannel` queues a sample with vol/pan/rate; channel management
(`Mixer_AddChannel`/`RemoveChannel`/`StopChannel`), volume + ducking, and master
volume round it out.

**⚠ Attribution fix:** the original anchor-based table mislabelled this whole block
as **Memalloc.cpp** (34 functions at `0x00440050–0x00447c80`) plus a separate "MIXER"
section that was actually 8 fill kernels + 19 CURSORS/MOVEMENT functions. Verified via
`s_..._Crux_MIXER_*` `__FILE__` strings. The true Memalloc.cpp is only the 3 functions
at `0x0043fa70–0x0043fd10` (sound-pool partitioning).

**Per-channel state** (`DAT_006dc278`, stride 0x28): start/end ptr, volume, saved
volume, looping, flags, pan, done-callback. **Active list** (`DAT_006dc098`, stride
0x30) sorted by samples-left.

**Key globals:** `g_pDSound`, `g_pDSBuffer`, `g_nMixerActiveChans`, `g_pfnMixerFillBuf`, `g_pMixerVolTable`  
**Depends on:** DirectSound, ERRORS.cpp  
**Used by:** SETPAL.cpp `Snd_*` sound API (see below), THEMES.cpp (music), SOUNDMEM.cpp (speech)

---

## ✅ MOVEMENT.cpp — Character walk animation manager
**Address range:** `0x004545d0 – 0x004574d0`  
**Functions:** 25/25 (+1 boundary function `OTF_AllocSlot` at `0x004576a0` belongs to ONTHEFLY.cpp)  
**Output:** `src/MOVEMENT.cpp`, `src/MOVEMENT.h`

This is NOT a room pathfinding system — it is a **direction-based walk animation manager**. Room-level pathfinding (waypoint selection) lives in AREAS.cpp/ADVENT.cpp. MOVEMENT.cpp takes a pre-computed node path and drives the character sprite through it frame by frame.

**Core concepts:**
- Characters have 8 directional walk animations (N, NE, E, SE, S, SW, W, NW) and an 8×8 turn-transition matrix. Resource names are `<charname><dircode>` and `<charname><from><to>` respectively.
- A walk path is up to 8 pre-computed room-node indices (`g_anMovPath[]`). Each node has `(x, y, z, area)` world-screen coordinates in `g_pMovNodes[]`.
- `Mov_Update()` runs once per frame: interpolates position linearly between nodes, uses `atan2` for direction angle, plays the appropriate directional anim, and fires turn sequences on direction changes.
- Carry anims: when `g_nMovCarryHint > 0` a second set of directional sprites (`<charname>_c<dir>`) overlays the walk, resolved by `Mov_SelectCarryAnim`.
- Turn pathfinding: `Mov_FindPath` does DFS over the 8×8 transition graph to find the shortest sequence of turn animations (e.g., NE→SW needs NE→E→SE→S→SW). `Mov_TurnAround` and `Mov_TurnOnTheFly` build the queued anim sequence.
- One follower NPC is supported via `Mov_SetFollower`; its animation sequence advances in sync with the player.

**Key globals:** `g_nMovDestNode`, `g_nMovPathSteps`, `g_nMovCurDir`, `g_nMovAnimStep/Dir/Frames`, `g_anMovDirAnim[8]`, `g_anMovCarryAnim[8]`, `g_anMovTransAnim[8][8]`, `g_nMovTurnSteps`, `g_anMovTurnSeq[8]`, `g_nMovCarryHint`, `g_nMovFollower`  
**Depends on:** AREAS.cpp (node table `g_pMovNodes`), Advanim.cpp (sprite blit + anim tick), MIXER.cpp (footstep sounds), READRES.cpp (load/free anim handles), ERRORS.cpp (Debug_TraceVal)  
**Used by:** ADVENT.cpp (walk commands), PLAYER.cpp (character movement), SCHED.cpp (arrival callbacks)

**⚠ Boundary note:** `0x004576a0` (`OTF_AllocSlot`) has an assert string referencing `ONTHEFLY.cpp` despite falling before the estimated ONTHEFLY boundary (`0x004576dd`). It is listed in FUNCTIONS.md under MOVEMENT.cpp but logically belongs to ONTHEFLY.cpp. Reclassify when ONTHEFLY.cpp is reversed.

---

## ⬜ ONTHEFLY.cpp — On-the-fly resource decompression
**Address range:** `0x004576dd – 0x0045c689` (est.)  
**Functions:** 22

Real-time resource decompression — probably LZ or RLE streaming into cache buffers so rooms/animations can be loaded without stalling the frame loop.

---

## ✅ PLAYER.cpp — FMV cutscene streaming player
**Address range:** `0x0045c7d0 – 0x0045d3c0`  
**Functions:** 12/13 (`0x0045d4e0` = `Res_FindByNumChar`, READRES boundary)  
**Output:** `src/PLAYER.cpp`, `src/PLAYER.h`

**"PLAYER" = video player, not character controller.** This module owns FMV cutscene playback via a background streaming thread and rate-paced CD reads.

- **`Player_Init`** — fills bunch-target table, resets voice/stream state
- **`Player_BunchRead`** — rate-paced chunked CD read synchronised to per-frame events
- **`Player_StreamSyncCallback`** — per-frame sync callback; increments missed-frame counter, signals thread
- **`Player_FlushVoices`** — flush 3 audio voice channels between FMV frames
- **`Player_RemapPalette`** — remap 8-bit FMV pixel rows through `g_abPlayerPalLUT[256]`
- **`Player_IsAbortPressed`** — checks ESC/skip key for cutscene abort
- **`Player_SetCoverSprite`** / **`Player_DrawCoverSprite`** — fullscreen cover-sprite overlay before each FMV frame

**`PickUpItem` is NOT here** — likely in GI.cpp or ADVENT.cpp (the item/inventory dispatch layer).  
`0x0045d4e0` = `Res_FindByNumChar` (READRES.cpp internal bidirectional index search helper).

**Key globals:** `g_abPlayerPalLUT[256]`, `g_anPlayerBunchTargets[4000]`, `g_nPlayerCurrentFrame`, `g_nPlayerMissedFrames`, `g_nPlayerCoverSpriteIdx`, `g_hPlayerThread`, `g_hPlayerSyncEvent`, `g_hPlayerFrameEvent`  
**Depends on:** READRES.cpp (bunch streaming), DDRAWI.cpp (back-buffer blit), audio voices  
**Used by:** ADVENT.cpp (FMV trigger), RUNPROG.cpp (cutscene loop)

---

## ✅ READRES.cpp — Multi-threaded CD-ROM "bunch" streaming engine + RESCALE subsystem
**Address range:** `0x0045d810 – 0x00461b30`  
**Functions:** 41/41  
**Output:** `src/READRES.cpp`, `src/READRES.h`

READRES.cpp implements the **"bunch" async streaming system** for multi-disc CD-ROM reading, plus an embedded sprite-rescale subsystem.

**Resource format:**
- `ADVENT.IDX` — flat index: 1-byte name-length prefix, N-byte name, then 12-byte record `{file_offset, byte_size, disk_number, ...}`
- `ADVENT.RES` — raw concatenated resource data, opened with Win32 `mmioOpen`
- Multi-CD: reads `[General] MultiRes` from `CRUX.INI`; per-disc paths from `ResPath_N` keys

**Async pipeline:**
1. `Res_BunchInit` — creates 3 `CRITICAL_SECTION`s, 4 events, spawns `Res_BunchReadingThread`
2. Callers post via `Res_BunchFreadLoadPtr` (single) or `Res_BunchFreadStreamLoadPtr` (streaming, 50k-byte chunks with per-frame deadlines)
3. Background thread: `Res_BunchSortTasks` picks most-urgent entry, reads with `mmioSeek`/`mmioRead` under `g_nFileLock`, signals `g_hEvtReadDone`
4. `Res_BunchFreadNow` — synchronous wrapper (enqueue + block)

**Known thunk resolutions:**
- `thunk_FUN_0040b0c0` → `Res_GetDirectByNumChar` (`0x0045d810`) — main public entry; tag→index resolved by a function outside this range
- `thunk_FUN_00405810` (`FreeResource`) is **not** in READRES — it is a heap-free wrapper elsewhere

**Embedded RESCALE subsystem** (`0x004612a0–0x00461b30`, 7 functions): `Rescale_CalcZoomTable`, `Rescale_CalcForBike`, `Rescale_CalcForRoom`, `Rescale_Reset`, `Rescale_DrawScaledSprite`, `Rescale_GetCount`, `Rescale_StartBike` — pre-computes perspective-scale tables for the bike sequences and room backgrounds. Distinct from RESCALE.cpp proper (`0x00461c10+`).

**Key globals:** `g_nBunchInitDone`, `g_nMultiResMode`, `g_nCurrentDiskNum`, `g_nMmioResFile`, `g_nFrameCounter`, `g_nBunchQueueCount`, `g_hEvtWorkReady`, `g_hEvtReadDone`, `g_hBunchThread`, `g_nRescaleTable`, `g_szResPath`  
**Depends on:** FILES.cpp (path handling), Win32 `mmio*` API  
**Used by:** Every module that loads resources

---

## ✅ RESCALE.cpp — Bike-scroll rescale effect
**Address range:** `0x00461c10 – 0x00461d60`  
**Functions:** 2/2 (+1 RUNPROG boundary: `Runprog_LoadEntryNames` at `0x00461ea0`)  
**Output:** `src/RESCALE.cpp`, `src/RESCALE.h`

The genuine RESCALE.cpp (distinct from the 7 embedded `Rescale_*` functions in READRES.cpp).
- **`Rescale_DrawBikeScroll`** (`0x00461c10`) — one frame of the bike-scroll effect: advances scroll-Y by 0xd (wrapping 0x27f→0x1fa), rebuilds the room zoom table via `Rescale_CalcForRoom`, blits each zoom level.
- **`Rescale_DrawByIndexChecked`** (`0x00461d60`) — blit one scaled sprite from a zoom-table entry with width bounds-check (confirmed `RESCALE.cpp` debug string).

**⚠ Boundary:** `0x00461ea0` = `Runprog_LoadEntryNames` (RUNPROG.cpp) — loads the "entry" resource name table (`rp_read_names`); physically in the RESCALE object but logically RUNPROG.

**Key globals:** `g_nRescaleRoomIdx`, `g_nRescaleBikeScrollY` (shares zoom-table `g_nRescaleTable` from READRES)  
**Depends on:** READRES.cpp (zoom-table builders), GI.cpp (blit)

---

## ✅ RUNPROG.cpp — Script bytecode virtual machine
**Address range:** `0x00462290 – 0x0046bc40`  
**Functions:** 9/11 (the other 2 are SafeHeap_Alloc/Free, already attributed to SAFEHEAP.cpp)  
**Output:** `src/RUNPROG.cpp`, `src/RUNPROG.h`, **`RUNPROG_OPCODES.md`** (full opcode spec)

**"RunProg" is the game's script interpreter, not a shell-out.** `RunProg_Exec`
(`0x00462560`) is a single ~3,232-line function — a `switch` over ~400 opcodes
that drives **every** verb interaction, dialogue line, cutscene, room transition,
inventory action, and puzzle in the game.

**VM model:**
- A program is `{ int count; Instruction insns[count]; }`, instruction stride 0x10 bytes
- Decoded fields: `opcode`, `arg0` (entity/var/item), `arg1`, `arg2`
- **`g_anSpeechPlayed[]`** (1500 ints, in SPEECH.cpp) doubles as the script
  **variable/register file** — read/written as `var[arg0]`, persisted in saves
- Control flow: IF/ELSE/ENDIF (`0x09–0x11` forward-scan), GOSUB (`0x065`/`0x168`),
  RPN evaluation stack (`0x173/0x174` push/pop, `0x182–0x184` add/sub/mul)
- Cutscene skip FSM: `DAT_00629f50` + `local_130`/`local_148`; right-click aborts

**Opcode families** (full table in `RUNPROG_OPCODES.md`):
flow/vars `0x00–0x1f` · audio/UI/system `0x20–0x3f` · dialog/scene/char `0x40–0x78` ·
speech variants + area lock `0xc7–0xff` · anim brackets + slot/item lists `0x100–0x14f` ·
registers/stack/anim/palette/save `0x150–0x1ff` · cursor/audio-channels/math `0x200–0x2c5` ·
speech vars/text `0x84d–0x858` · stack-gfx/save-dialogs/Gran mini-games `0x8fd–0x91c` ·
inventory `0x960–0x96c` · focus/slider/game-init `0x974–0xc1c` · object-props/theme/display `0x1839–0x5b23`

**Helpers:** `RunProg_WaitMoveDone` (movement-wait + right-click abort),
`RunProg_SelectAreaContext` (primary/secondary area-bank switch),
`RunProg_PlayScmWithPaletteGuard`, tracked-sound list (`Track`/`Clear`/`StopAndClear`),
`RunProg_RestorePaletteSnapshot`.

**⚠ Boundary:** the RUNPROG range estimate (`0x00461f1a`) overlapped RESCALE/READRES;
the true RUNPROG functions start at `0x00462290`. `0x0046bcc0`/`0x0046bd80` in this
range are SafeHeap_Alloc/Free (SAFEHEAP.cpp).

**ScummVM porting note:** re-implement as a per-opcode handler table from the
spec in `RUNPROG_OPCODES.md` rather than copying the monolithic switch.

**Depends on:** essentially every other module (it is the orchestration layer)  
**Used by:** ADVENT.cpp (`Adv_FindVerbHandler` → script handle → RunProg_Exec via SCHED dispatch), Winmain.cpp (execution thread)

---

## ✅ SAFEHEAP.cpp — Safe heap allocator with magic-tag integrity checking
**Address range:** `0x0046bcc0 – 0x0046bd80` (true range; `0x0046bf40` is SCHED misattribution)  
**Functions:** 2 true functions (unlisted in FUNCTIONS.md) + 1 boundary misattribution  
**Output:** `src/SAFEHEAP.cpp`, `src/SAFEHEAP.h`

Wraps malloc/free with a 4-byte magic-tag header for double-free/wild-pointer detection:
- **`SafeHeap_Alloc(file, line, size)`** (`0x0046bcc0`) — allocates `size+4` bytes, writes magic tag at `[ptr-4]`, returns user pointer
- **`SafeHeap_Free(file, line, ptr)`** (`0x0046bd80`) — `strcmp(ptr-4, tag)` assert, zeroes tag, calls `free(ptr-4)`

**⚠ Boundary:** `0x0046bf40` (listed in FUNCTIONS.md as SAFEHEAP) is actually `Sched_SetAllNormal` — distributes thread priorities across 5 game threads (main, bunch, theme, + 2 others) after leaving a high-priority window. Has SCHED.cpp debug strings. The two true SafeHeap functions (`0x0046bcc0`, `0x0046bd80`) were not in the original FUNCTIONS.MD table.

**Used by:** THEMES.cpp (`Theme_Init` pool allocation), likely Memalloc.cpp as the safe variant

---

## ✅ SCHED.cpp — Win32 process priority + raw palette buffer
**Address range:** `0x0046c120 – 0x0046ce10`  
**Functions:** 16/16 (originally attributed 33; 17 are actually SETPAL.cpp — see below)  
**Output:** `src/SCHED.cpp`, `src/SCHED.h`

**"SCHED" = CPU scheduling, not game event scheduling.** This module manages Win32 process priority (boosting to `HIGH_PRIORITY_CLASS` around palette-flip work) and owns the raw 6-bit palette buffer layer:

- `Sched_BeginHighPriority` / `Sched_EndHighPriority` — boost/restore process priority (suppressed when `g_nSchedDebugMode != 0`)
- Three 768-byte palette buffers in 6-bit-per-channel format (range 0–63): `g_abActivePal`, `g_abTargetPal`, `g_abAdjustedPal`
- `Sched_UpdatePalette` — compare target vs active, zero unchanged entries, apply border strip, call `SetPal_PreChange`
- `g_nPalBorderMode` — 0 = 3-entry border strip, non-zero = 30-entry border strip (affects palette fade quality)
- Two stubs (`Sched_Stub1/2`) are empty SEH-frame placeholders

The game event scheduler expected here is likely in **ADVENT.cpp** or **TIMERS.cpp** — those are still unreversed.

**Key globals:** `g_abActivePal[768]`, `g_abTargetPal[768]`, `g_abAdjustedPal[768]`, `g_nPalBorderMode`, `g_nPalGamma`, `g_nPalGeneration`, `g_nSchedDebugMode`  
**Depends on:** SETPAL.cpp (`SetPal_PreChange`), GI.cpp (page flip)  
**Used by:** GI.cpp, DDRAWI.cpp (palette flip pipeline)

---

## ✅ SETPAL.cpp — GDI palette + system-colours + fades + high-level sound API
**Address range:** `0x0046cf10 – 0x00470780`  
**Functions:** 53/53  
**Output:** `src/SETPAL.cpp`, `src/SETPAL.h`

**⚠ Boundary:** SETPAL.cpp starts at `0x0046cf10` (not `0x0046e4e8` as estimated) —
the first 17 functions were physically linked inside what FUNCTIONS.md attributed to
SCHED.cpp. The last function in the range, `Slider_Add` (`0x00470780`), is actually the
first **SLIDER.cpp** function (confirmed by `..._Crux_SLIDE_...` `__FILE__` string).

**⚠ Snd_ sound API lives here.** Beyond the palette code, SETPAL.cpp's translation unit
also contains the game's **high-level sound API** (`0x0046f3c0–0x004705e0`): `Sound_Init`
(reads `[Sound]` INI, calls `Mixer_Init`), `Snd_Play`/`Snd_PlayPanned`/`Snd_PlayCentered`/
`Snd_PlayCore`, `Snd_Stop`, speech/SFX volume + pan, and a 20-slot channel table
(`g_nSndChannelTable`, stride 0x30) — note this same table is shared with the SLIDER UI
widget. These functions keep the `Snd_` prefix (functionally correct) even though they sit
in the SETPAL.cpp TU; they are the layer the rest of the engine calls, sitting on top of
MIXER.cpp. `Snd_PlayCentered` (`0x0046f7f0`) is what MOVEMENT.cpp calls for walk sounds and
SOUNDMEM.cpp for centred speech.

**Palette subsystems:**
- `SetPal_PreChange` — alloc `LOGPALETTE`, fill 6-bit→8-bit (optional gamma), Create/Select/RealizePalette
- 6 fade variants (FadeOut/QuickFadeToBlack/SmoothFadeToBlack/FadeToTarget/FadeInFromBlack/FadeInSnapshot) — 64 brightness steps, 20 ms `Sleep`, optional per-frame callback
- `SetPal_Init` — snapshot 256 PALETTEENTRY, save 19 Win32 UI colours, create `g_hPalEvent`
- `SetPal_ClearSysColors`/`RestoreSysColors`, `SetPal_SetPalette` (high-level install)

**Key globals:** `g_abSnapshotPal[768]`, `g_abSysPalEntries[1024]`, `g_hPalEvent`, `g_nMainDC`, `g_nFullscreen`, `g_nSndSpeechVol`, `g_nSndSfxVol`, `g_nSndChannelTable[20×0x30]`, `g_nSndSubtitleOnly`  
**Depends on:** SCHED.cpp (raw palette buffers), GI.cpp (page flip), MIXER.cpp (Snd_ → Mixer_), Win32 GDI  
**Used by:** Graninv.cpp (`Gran_ConvPal`), DDRAWI.cpp, MOVEMENT.cpp + SOUNDMEM.cpp (`Snd_PlayCentered`)

**Reversed subsystems:**
- `SetPal_PreChange` — allocate `LOGPALETTE`, fill via `SetPal_FillLogPalette` (6-bit→8-bit with optional gamma), call `CreatePalette`/`SelectPalette`/`RealizePalette`
- 6 fade variants: `SetPal_FadeOut`, `SetPal_QuickFadeToBlack`, `SetPal_SmoothFadeToBlack`, `SetPal_FadeToTarget`, `SetPal_FadeInFromBlack`, `SetPal_FadeInSnapshot` — all step 64 brightness levels with 20 ms `Sleep`; optional per-frame callback via `g_nPalCallback`
- `SetPal_Init` — snapshot 256 `PALETTEENTRY` values from system palette, save 19 Win32 UI colours, create manual-reset event `g_hPalEvent`
- `SetPal_ClearSysColors` / `SetPal_RestoreSysColors` — zero/restore 19 Win32 UI colours; no-op in fullscreen
- `SetPal_SetPalette` — high-level install: signal event, copy to `g_abTargetPal`, drive full pipeline

**Key globals:** `g_abSnapshotPal[768]`, `g_abSysPalEntries[1024]`, `g_hPalEvent`, `g_nPalCallbackEnabled`, `g_nPalCallback`, `g_nMainDC`, `g_nFullscreen`  
**Depends on:** SCHED.cpp (raw palette buffers), GI.cpp (page flip after SetPalette), Win32 GDI  
**Used by:** Graninv.cpp (`Gran_ConvPal`), DDRAWI.cpp

---

## ✅ SLIDER.cpp — Animated slider widgets
**Address range:** `0x00470780 – 0x00471fe0`  
**Functions:** 10/10 (`Slider_Add` boundary at `0x00470780` + 1 SOUNDMEM boundary at `0x00472340`)  
**Output:** `src/SLIDER.cpp`, `src/SLIDER.h`

Animation-driven slider widgets. Each slider overlays a 0x30-byte `SliderEntry` on one of
the 20 shared channel records in `g_nSndChannelTable` (the same table the `Snd_*` API uses).
A slider's **thumb position is an animation slot's frame**: both the rendered frame and the
reported integer value are independent linear remaps of the thumb's pixel travel
(`pixelLo..pixelHi`):
- frame = `(pos - pixelLo) * frameCount / span`
- value = `(valueHi - valueLo + 1) * (pos - pixelLo) / span + valueLo`

Functions: `Slider_Add` (alloc slot), `Slider_Remove`, `Slider_SetCurrent` (default id),
`Slider_SetPosition`, `Slider_SetValueRange`/`SetPixelRange`/`SetMaxStep`,
`Slider_TrackClicked` (click-to-step), `Slider_Drag` (hold + drag thumb), `Slider_SetValue`
(place from integer). Flag bit0 = in-use, bit1 = vertical.

**⚠ Boundary:** `0x00472340` = `SndMem_ReadSound` (SOUNDMEM.cpp `_read_sound` cache loader), physically at the end of the SLIDER object file.

**Key globals:** `g_nSliderCurrent`, `g_nSliderRefX/Y`, `g_nSndChannelTable` (shared, 20×0x30)  
**Depends on:** Advanim.cpp (`Anim_SetCurrentFrame`/`SetPosition`/`GetFrameTopLeft`, slot X/Y)  
**Used by:** settings/dialog UI; `Gran_*Slider` in Graninv.cpp is a separate item-specific variant

---

## ⬜ SOUNDMEM.cpp — Sound memory / sample cache
**Address range:** `0x004725bc – 0x00474b68` (est.)  
**Functions:** 19

Sound sample caching — loads `.WAV` or raw PCM into DirectSound secondary buffers, manages a sample pool, and handles sample eviction under memory pressure.

---

## ✅ SPEECH.cpp — Subtitle / sentence bookkeeping engine
**Address range:** `0x00474cf0 – 0x004750d0`  
**Functions:** 7/7  
**Output:** `src/SPEECH.cpp`, `src/SPEECH.h`

Despite the name, SPEECH.cpp is the **subtitle and sentence-state bookkeeping** layer, not the audio driver. Audio output lives in MIXER/SOUNDMEM. SPEECH.cpp owns:
- `SENTENCE.BIN` — binary file mapping sentence IDs to subtitle text + durations
- A played/skipped state table per channel (`g_anSpeechPlayed[]`) recording which sentences the player has heard
- A pending sentence queue (`g_nSpeechPendingId`, `g_nSpeechChannel`)
- An offscreen 640×480 DirectDraw surface for subtitle rendering
- GDI font configuration read from an INI file

`Speech_Play` sets `g_nSpeechInterrupted=1` and calls `Speech_Commit`, which fires a timer event (TIMERS.cpp). `Gran_DiaryPlay` calls `Speech_Play`.

**⚠ MOVEMENT.cpp correction:** `thunk_FUN_00474d80` in `Mov_Update` resolves to `Speech_GetSentence` (returns `g_nSpeechSentence`), NOT `Advanim_HasOverride`. The carry-anim branch in `Mov_Update` checks whether a speech sentence handle is active.

**Key globals:** `g_nSpeechPendingId`, `g_nSpeechChannel`, `g_nSpeechInterrupted`, `g_anSpeechPlayed`, `g_pSpeechTexts`, `g_pSpeechDurations`, `g_nSpeechSurface`  
**Depends on:** TIMERS.cpp (timer event), DDRAWI.cpp (offscreen surface), GDI (font)  
**Used by:** Graninv.cpp (`Gran_DiaryPlay`), ADVENT.cpp

---

## ✅ TEXT.cpp — GDI text rendering + room-script table loader
**Address range:** `0x00475970 – 0x00479810`  
**Functions:** 29/29 (21 Txt_* text functions + 8 Thm_* room-script functions)  
**Output:** `src/TEXT.cpp`, `src/TEXT.h`

Two distinct subsystems share this file:

### Txt — Win32 GDI text display (21 functions)
Text rendering is **entirely GDI-based** — not bitmap blitting. A single bold `HFONT` (`CreateFontA`, weight 700) is selected into the game's shared HDC. Text is laid out via `DrawTextA` and written via `TextOutA`. Features: word-wrap, multi-page scrolling with per-tick character reveal (speed = length/duration), left/center/right alignment, right-to-left support, three-colour highlight range, and dirty-rect background save/restore via a DirectDraw offscreen surface.

`Txt_LookupString(id)` — searches a parallel key/value array by integer ID, trims whitespace, and sets `g_pTxtCurStr`. This is `thunk_FUN_00475f90` seen in Graninv.cpp — it sets up display text by item-description ID, NOT "GetItemName".

### Thm — Room-script table loader (8 functions)
A separate room-script data loader (`Thm_LoadTheme`, `Thm_ReadTable`, `Thm_Play`, etc.) that reads per-room script tables from READRES and feeds them to the THEMES.cpp music event system. Distinct from the audio THEMES.cpp module.

**Key globals:** `g_nTxtEnabled`, `g_nTxtFont`, `g_nTxtColorGDI`, `g_nTxtBoxLeft/Top/Right/Bottom`, `g_nTxtScrollPos`, `g_nTxtLinesPerPage`, `g_nTxtTotalLines`, `g_nThmIndex`, `g_nThmSegmentCount`, `g_nThmCommandCount`  
**Depends on:** GDI (font/text), DDRAWI.cpp (offscreen surface for dirty-rect), READRES.cpp (room script data)  
**Used by:** Graninv.cpp (`Txt_LookupString`), ADVENT.cpp (dialogue display)

---

## ✅ THEMES.cpp — Room music streaming engine
**Address range:** `0x004798e0 – 0x0047d5b0`  
**Functions:** 43/43  
**Output:** `src/THEMES.cpp`, `src/THEMES.h`

**THEMES.cpp has no graphics code.** "Theme" = musical theme. It is the entire background music streaming and mixing system.

**Architecture:**
- A dedicated background thread (`Theme_ThreadProc`) owns all playback, synchronised via two Win32 events (`g_hThemeFinishedEvent`, `g_hThemeExecuteEvent`)
- PCM ring buffer: `g_pThemeMemPool` (allocated once at init, filled with silence `0x80`)
- Segment-op stack (max 51 entries) of typed cues: play/silence/event/stop/crossfade/loop/set-fade
- `Theme_SetRoom(id)` queues a room change; the thread handles crossfade/cut per `g_nThemeTransitionMode` (7 modes)
- `Theme_MusicEvent(name)` maps event names to segment-table entries, allowing scripts to trigger stings
- Volume 0..64 (`g_nThemeVolume`). Fade-out driven by multimedia timer firing `Theme_FadeOutHandler`
- Timer subsystem: `Theme_SetTimer` wraps `timeSetEvent` with a 10-slot table; `Theme_RegisterAsyncProg` registers up to 100 async function pointers

**⚠ Function stubs:** Complex functions (Theme_Init, Theme_ThreadProc, Theme_MusicEvent, etc.) have their bodies written as pseudocode comments because MIXER.cpp and ONTHEFLY.cpp are not yet reversed. Globals and control flow are correct; extern call sites will be filled in when those modules are reversed.

**Key globals:** `g_pThemeMemPool`, `g_nThemePoolSize`, `g_hThemeThread`, `g_nThemePendingCmd`, `g_nThemeVolume`, `g_nThemeFadeTimerId`, `g_nThemeCurrentRoom`, `g_nThemeTransitionMode`, `g_nThemeTimerIds[10]`, `g_nThemeAsyncProgFuncs[100]`  
**Depends on:** MIXER.cpp (playback), ONTHEFLY.cpp (PCM decode), READRES.cpp (file open), SAFEHEAP.cpp (pool alloc), Win32 multimedia timer  
**Used by:** ADVENT.cpp (room transitions), TEXT.cpp (`Thm_*` room-script loader)

---

## ✅ TIMERS.cpp — Game timer system with async/sync prog queues
**Address range:** `0x0047d6a0 – 0x0047ecf0`  
**Functions:** 21/21  
**Output:** `src/TIMERS.cpp`, `src/TIMERS.h`

Two-layer timer system: one-shot countdown timers + a "prog queue" for deferred callbacks.

**Timer table** (`g_aTimers[30]`, stride 20 bytes):
```
+0x00 int callback   — function pointer to fire
+0x04 int frames     — countdown in game ticks
+0x08 int pauseCount — suspend depth (Tick/Untick)
+0x0C int flags      — 0=async, 1=sync
+0x10 int resetVal   — non-zero = self-resetting
```

**Key functions:**
- **`Timer_AddAsync` (`0x0047e3a0`)** = `FireTimer` called from Graninv.cpp — adds one-shot async countdown timer
- **`Timer_Tick` / `Timer_Untick`** — registered game-loop hooks; increment/decrement all `pauseCount` fields
- **`Timer_TickCallback`** — per-frame countdown engine; fires expired timers via async/sync dispatch
- **`Timer_TriggerInit` (`0x0047ecf0`)** — `__thiscall`; initialises named trigger object with hash + alloc (links to ADVENT.cpp script trigger system)
- Prog queues: `Timer_AddAsyncProg`/`Timer_AddSyncProg` enqueue deferred fn-ptr calls; `Timer_DispatchProg` routes ready entries to `RunSync`/`DispatchAsync`

**FireTimer resolution:** `Timer_AddAsync` is the `FireTimer(handle)` seen in Graninv.cpp `GV_SetInitHandler`.  
**SPEECH correction:** `Timers_FireSpeech` from SPEECH.cpp calls `thunk_FUN_0047d5b0` = `Theme_RegisterAsyncProg` directly (fires immediately, bypasses countdown).

**Key globals:** `g_nTimerCount`, `g_aTimers[30]`, `g_nThemeAsyncProgCount`, `g_aAsyncProgFn[]`, `g_aAsyncProgData[]`, `g_aSyncProgFn[]`  
**Depends on:** game loop hook registration (RUNPROG.cpp or SCHED.cpp)  
**Used by:** Graninv.cpp (`FireTimer`), SPEECH.cpp, ADVENT.cpp (script triggers)

---

## ✅ Tushtush.cpp — Scripted animation-trigger / spawner system
**Address range:** `0x0047eed0 – 0x004821f0` (+ ctor `Timer_TriggerInit` at `0x0047ed5e`, attributed to TIMERS)  
**Functions:** 47/47  
**Output:** `src/Tushtush.cpp`, `src/Tushtush.h`

"Tushtush" is the internal codename for the **scripted animation-trigger object** system — the
glue between the ADVENT script engine, the TIMERS async-program scheduler, and the
animation/sprite-rescale renderer. Two cooperating object types in doubly-linked lists:

- **`tt_obj`** (definition/spawner template, ~0x100 bytes): a named animation + up to 10 spawn
  POS/RANGE rectangles, three ADVENT script-callback ids (init / periodic / collision), a spawn
  probability fraction, and a periodic-fire frame.
- **`tt_sobj`** (live spawned instance, 0x24 bytes): picks a random spot in a parent rect,
  advances its anim frame, draws via `Rescale_DrawScaledSprite`, fires the parent's callbacks
  through `Timer_AddAsyncProg`.

`Tt_Init` registers `Tt_Handler` as a per-frame Anim tick callback: each frame it shows/advances/
collision-tests/reaps live sobjs, then rolls each definition's spawn dice (`Tt_CheckProb`) to
birth new ones. A single player-driven "character" sobj (`g_pTtCharSobj`) is collision-tested
against the rest. ADVENT verbs (`Tt_ObjAdd`/`SetCur`/`SetInitScript`/`SetCollisionScript`/
`SetPeriodicScript`/`SetPos`/`SetRange`/`SetPers`/`Rem`) build and configure definitions.

`Timer_TriggerInit` (`0x0047ed5e`, attributed to TIMERS) is the `tt_obj` constructor.

**Key globals:** `g_pTtObjList`, `g_pTtSobjList`, `g_pTtObjCursor`, `g_pTtCharSobj`, `g_nTtInitialized`  
**Depends on:** ADVENT (script callbacks), TIMERS (async-prog queue, ctor), Advanim + RESCALE (draw), FILES (CdFind/AdventDir)  
**Used by:** ADVENT.cpp (the trigger/spawn verbs in scripts)

---

## ✅ WINRES.cpp — sprite→GDI icon/cursor/bitmap builder + registry init
**Address range:** `0x00486fe0 – 0x00487bb0`  
**Functions:** 10/10 (+1 WIZARDS boundary: `Wiz_FindVar` at `0x00487cb0`)  
**Output:** `src/WINRES.cpp`, `src/WINRES.h`

Builds Win32 GDI handles from the game's own "SMA" sprite resources (not from the EXE resource
section as the name suggested):
- `WinRes_Sma2Icon`/`Sma2IconCore` — 32×32 colour HICON; `WinRes_Sma2Cursor`/`Sma2CursorMono` —
  HCURSOR; `WinRes_Sma2Bitmap`/`Sma2BitmapCore` — HBITMAP; `WinRes_Sma2IconMonoCore` — monochrome
  variant; `WinRes_BuildAndMask` — packs 8bpp→1bpp AND-plane; `WinRes_MakeEmptyIcon`
  (`0x004874a0`, the blank window icon used by Graninv `GV_InitWindow`).
- `WinRes_RegInitKey` — opens a parent registry key, creates a destination sub-key.

**⚠ Boundary:** `0x00487cb0` = `Wiz_FindVar` (WIZARDS.cpp — linear scan of `g_abWizVarTable`).

**Depends on:** Win32 GDI (CreateBitmap/CreateIconIndirect), SMA sprite format, registry  
**Used by:** Winmain/Graninv (window icon), CURSORS

---

## ✅ WIZARDS.cpp — "Wizard" script-file interpreter (dev tool)
**Address range:** `0x00487db0 – 0x00488a10` (the 7 interpreter functions at `0x00488110+` were misattributed to `(msvc-crt)`)  
**Functions:** 9/9  
**Output:** `src/WIZARDS.cpp`, `src/WIZARDS.h`

A complete **mini scripting-language interpreter** for "wizard" text scripts located in
`<SaveGameDir>\wizards\`. `Wiz_Run(name)` opens the file, then loops: read line →
tokenize → match command → dispatch. Dev-build content-authoring/automation tool.

**Pipeline:**
- `Wiz_Run` (`0x00488110`) — build path, open file ("Unable to open wizard"), reset, run loop
- `Wiz_GetNextLine` (`0x004888d0`) — fgets + trim, skip blanks and `//` comments
- `Wiz_TrimStr` (`0x00488a10`) — collapse whitespace runs (orig `trim_str`)
- `Wiz_SplitStrings` (`0x004883b0`) — quote-aware tokenizer → `g_szWizArgs` (stride 0x1e); errors on "Odd number of quotation marks"
- `Wiz_VerifyString` (`0x00488710`) — non-blank token check
- `Wiz_GetCommand` (`0x004887e0`) — keyword dispatch ("Unknown Command")
- `Wiz_ResetStructs` (`0x00488300`) — clear the 21-entry variable table
- Handlers: `Wiz_AddVar` (`0x00487db0`, ADDVAR), `Wiz_AskString` (`0x00487f50`, ASKSTRING)

**Complete command vocabulary** (statically recovered from `Wiz_GetCommand` — a 2-branch
strcmp chain, no large table): **`ADDVAR`** and **`ASKSTRING`**. Anything else → error.

**⚠ Variable names are NOT in the EXE** — they're created at runtime by `ADDVAR <name>`
lines inside the external `.wiz` script files (game data in `wizards\`), not compile-time
constants. Recovering actual variable names requires the shipped script files.

**Key globals:** `g_nWizParamCount`, `g_nWizVarCount`, `g_szWizArgs[stride 0x1e]`, `g_abWizVarTable[21×0x88]`  
**Used by:** dev-build wizard menu (`WizardListDLG` dialog, "List of wizards" / "Execute Wizard" — the dialog proc lives outside this module)

---

## ⬜ Winmain.cpp — WinMain entry point
**Address range:** `0x00482242 – 0x00486e77` (est.)  
**Functions:** 32

`WinMain`, window class registration, the top-level message loop, and possibly the splash screen. The `g_hMainWnd` and `g_hInstance` globals originate here. Should be reversed early as it ties all modules together and confirms calling conventions.

---

## ⬜ (startup/crt) zone — Import thunks
**Address range:** `0x00401000 – 0x00403954`  
**Functions:** 499

VC6-generated import-table thunks. Each is a 6-byte `JMP [IAT_entry]`. Not worth reversing individually; Ghidra's FLIRT signatures have already named most of them.

---

## ⬜ (msvc-crt) zone — Statically linked MSVC runtime
**Address range:** `0x0048c000+`  
**Functions:** 362 (42 named by FLIRT)

Statically linked MSVC 6.0 CRT (`_malloc`, `_memset`, `_strcmp`, etc.). Ghidra FLIRT has named 42; the remainder are internal CRT helpers. Skip unless a specific helper is needed for call-graph tracing.

---

## Dependency map (partial)

```
Winmain.cpp
  └─ RUNPROG.cpp (main loop)
       ├─ SCHED.cpp (event/timer dispatch)
       │    └─ TIMERS.cpp
       ├─ ADVENT.cpp (script VM)
       │    ├─ INVMANG.cpp
       │    ├─ PLAYER.cpp
       │    └─ THEMES.cpp
       ├─ GI.cpp (hardware blit layer)
       │    ├─ DDRAWI.cpp (DirectDraw init)
       │    ├─ SETPAL.cpp
       │    └─ Img.cpp
       ├─ MIXER.cpp + SOUNDMEM.cpp + SPEECH.cpp
       ├─ MOVEMENT.cpp
       │    └─ AREAS.cpp
       ├─ CURSORS.cpp ──────────────── uses GI.cpp, Img.cpp
       ├─ Graninv.cpp ──────────────── uses GI.cpp, INVMANG.cpp,
       │                                    PLAYER.cpp, SCHED.cpp,
       │                                    SPEECH.cpp, SETPAL.cpp,
       │                                    READRES.cpp  [speculative]
       ├─ Advanim.cpp / ANI32.cpp / BANI.cpp  (animation layers)
       ├─ READRES.cpp / FILES.cpp / ONTHEFLY.cpp  (I/O)
       └─ Memalloc.cpp / SAFEHEAP.cpp
```

---

## Reversal priority order

Based on dependency depth (reverse leaves first):

1. **ERRORS.cpp** — resolves `Debug_Assert`/`Debug_Trace` seen everywhere
2. **GI.cpp** — resolves ~20 Graninv.cpp speculative calls; unblocks Img.cpp
3. **READRES.cpp** — resolves resource load/free calls
4. **SCHED.cpp** — resolves `FireTimer`/`RegisterForUpdate`/event loop
5. **INVMANG.cpp** — resolves inventory slot/flag calls from Graninv.cpp
6. **DDRAWI.cpp** — resolves DirectDraw surface creation
7. **SETPAL.cpp** — resolves palette ops from Graninv.cpp's Gran_ConvPal
8. **PLAYER.cpp** — resolves `PickUpItem` and cursor-mode transitions
9. **THEMES.cpp** — large, high value, unlocks room system
10. **ADVENT.cpp** / **Tushtush.cpp** — large, likely game logic
