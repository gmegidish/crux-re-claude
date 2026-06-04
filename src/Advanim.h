// Advanim.h — Animation-slot system (advanced animation manager)
//
// Manages a pool of 150 animation "slots". Each slot holds one named .ANI
// resource loaded from the resource archive.  The system is responsible for:
//   - allocating / loading / freeing slots
//   - advancing per-frame tick (freeze/unfreeze, sound triggers, callbacks)
//   - on-screen tracking (which animations are visible this frame)
//   - Z-sorted draw order build and sprite blitting
//   - palette management (global + per-animation individual palettes)
//   - synchronized animation groups
//   - animation caching (save/restore last freed animation)
//
// Original source: C:\DevStudio\Projects\Crux\Advanim.cpp

#pragma once

// ============================================================
//  AnimSlot structure layout (0x58 bytes per slot, 150 slots)
//  Base address 0x005b10b0 (g_anAnimSlotFlags[slot])
//  All fields are int-sized; offsets relative to slot*0x58:
//
//  +0x00  g_anAnimSlotFlags[slot]        — bit flags (see below)
//  +0x04  g_anAnimSlotStopFrame[slot]    — stop-at-frame (-1 = none)
//  +0x08  g_anAnimSlotGroupId[slot]      — group index or -1
//  +0x0C  (unnamed – g_nCharWalkTableBase column, used for Z-sort)
//  +0x10  g_anAnimSlotX[slot]            — screen X offset
//  +0x14  g_anAnimSlotY[slot]            — screen Y offset
//  +0x18  g_anAnimSlotVelX[slot]         — X velocity
//  +0x1C  g_anAnimSlotVelY[slot]         — Y velocity
//  +0x20  g_anAnimSlotCurFrame[slot]     — current frame index
//  +0x24  g_anAnimSlotPrevFrame[slot]    — previous frame index
//  +0x28  g_anAnimSlotLastFrame[slot]    — last-displayed frame
//  +0x2C  g_anAnimSlotTriggerFrame[slot] — one-shot trigger frame (-1=none)
//  +0x30  g_anAnimSlotStep[slot]         — advance step (1, -1, 0)
//  +0x34  g_anAnimSlotFreezeCount[slot]  — freeze ref count
//  +0x38  g_anAnimSlotCallback[slot]     — completion program handle
//  +0x3C  g_anAnimSlotCallbackDelay[slot]— countdown to callback fire
//  +0x40  g_anAnimSlotCallbackFrame[slot]— frame that fires callback
//  +0x44  (e0 field — freeze pending)
//  +0x48  g_anAnimSlotReserved[slot]     — padding
//  +0x4C  g_anAnimSlotLangId[slot]       — language ID (-1=none)
//  +0x50  g_anAnimSlotNum[slot]          — resource number (-1=by-name)
//  +0x54  g_anAnimSlotIndiPalIdx[slot]   — individual palette index (-1=none)
//
//  Flag bits in g_anAnimSlotFlags[slot]:
//    Bit 0  (0x001)  active / loaded
//    Bit 1  (0x002)  dump-pending (being removed)
//    Bit 2  (0x004)  marked for dump
//    Bit 3  (0x008)  has-walk-sound
//    Bit 4  (0x010)  looping (cycle)
//    Bit 5  (0x020)  bounce-loop (play once, stop)
//    Bit 6  (0x040)  ping-pong
//    Bit 7  (0x080)  has-mask / transparency flag
//    Bit 8  (0x100)  group-member (sliding window)
//    Bit 9  (0x200)  casts shadow
//    Bit 12 (0x1000) on-screen (visible)
//    Bit 13 (0x2000) has individual palette
//    Bit 19 (0x80000) area sprite
//    Bit 24 (0x1000000) freeze pending
//    Bit 25 (0x2000000) freeze held
//    Bit 26 (0x4000000) has async-prog
//    Bit 27 (0x8000000) reverse direction
//    Bit 28 (0x10000000) has completion callback
//    Bit 29 (0x20000000) skip advance
// ============================================================

// ============================================================
//  Resource entry layout (0x20 bytes per entry, 0x578=1400 max)
//  Base: g_anResEntryX[entry]  (0x0051e4f0)
//  +0x00  g_anResEntryX[entry]     — frame X on-screen position
//  +0x04  g_anResEntryY[entry]     — frame Y on-screen position
//  +0x08  g_anResEntryBudget[entry]— LRU budget value
//  +0x0C  g_anResEntrySize[entry]  — data size in bytes
//  +0x10  g_anResEntryPtr[entry]   — pointer to loaded frame pixel data
//  +0x14  g_anResEntryTask[entry]  — async load task handle
//  +0x18  g_anResEntryFlags[entry] — async status
//  +0x1C  g_anResEntryReady[entry] — 1 = fully loaded and ready
// ============================================================

#define ANIM_MAX_SLOTS          150     // maximum animation slots
#define ANIM_MAX_FRAMES         400     // maximum frames per animation
#define ANIM_MAX_RES_ENTRIES   1400     // 0x578 resource cache entries
#define ANIM_MAX_ONSCREEN        50     // 0x32 max on-screen anims
#define ANIM_MAX_SOUNDS         200     // max unique frame-sound names
#define ANIM_MAX_GROUPS         150     // max synchronized groups
#define ANIM_MAX_GROUP_MEMBERS   10     // max members per group
#define ANIM_MAX_INDI_PAL        10     // max individual palettes

// ============================================================
//  Initialization / shutdown
// ============================================================

// Anim_Init          — initialize all animation tables and resource pool
void Anim_Init(void);

// Anim_GameInit      — per-game-start reset (also inits mov/scheduler state)
void Anim_GameInit(void);

// ============================================================
//  Slot allocation
// ============================================================

// Anim_FindFreeSlot  — return index of a free slot (-1 if none)
int  Anim_FindFreeSlot(void);

// Anim_CheckFreeSlot — assert that a free slot exists (used for pre-checks)
void Anim_CheckFreeSlot(void);

// ============================================================
//  Loading animations into slots
// ============================================================

// Anim_AddByName     — load animation by resource name into a free slot;
//                      returns slot index
int  Anim_AddByName(const char *pszName, int nBudget);

// Anim_AddByNum      — load animation by resource number; set looping mode;
//                      returns slot index
int  Anim_AddByNum(int nNum, int nLoopMode, int nBudget);

// Anim_ExternalAddByName — load an external (.SMA) animation file; returns slot
int  Anim_ExternalAddByName(const char *pszName);

// Anim_LoadToMem     — core loader: read .ANI data from resource archive into
//                      slot nSlot; nBudget controls streaming threshold
void Anim_LoadToMem(const char *pszName, int nType, int nSlot, int nBudget);

// Anim_DevLoadToMem  — developer (file-system) version of Anim_LoadToMem;
//                      reads directly from disk file; returns slot or -1
int  Anim_DevLoadToMem(const char *pszFname);

// Anim_LoadByName    — load anim by name, set Z-walk-table column; returns slot
int  Anim_LoadByName(const char *pszName, int nZCol);

// Anim_LoadByNameGetCount — like Anim_LoadByName but also returns frame count
int  Anim_LoadByNameGetCount(const char *pszName, int nZCol);

// Anim_LoadAndWait   — load animation and spin until the last frame is ready
void Anim_LoadAndWait(const char *pszName, int nZCol);

// Anim_LoadMask      — load a mask animation (type=8) into a reserved slot
void Anim_LoadMask(const char *pszName);

// Anim_PrepareForRead — prepare anim-slot-table for bulk-read (called by
//                       the resource loading subsystem)
void Anim_PrepareForRead(const int *pData);

// ============================================================
//  Frame-header I/O helpers
// ============================================================

// Anim_ReadFrameHeader   — read one 8-byte frame header from bunch-file
void Anim_ReadFrameHeader(void *pBunchHandle, int nSlot, int nFrame);

// Anim_DevReadFrameHeader — read one 8-byte frame header from open FILE*
void Anim_DevReadFrameHeader(void *pFile, int nSlot, int nFrame);

// Anim_SetFrameHeader    — parse an 8-byte header and register the frame
//                          into g_anAnimFrameTable / g_anResEntries
void Anim_SetFrameHeader(short *pHdr, int nSlot, int nFrame);

// ============================================================
//  Freeing / dumping animations
// ============================================================

// Anim_Free            — free animation slot, optionally saving frame cache
void Anim_Free(int nSlot);

// Anim_MarkForDump     — mark a loaded slot for deferred freeing
void Anim_MarkForDump(int nSlot);

// Anim_MarkForDumpByName — mark slot (looked up by name) for deferred freeing
int  Anim_MarkForDumpByName(const char *pszName);

// Anim_ProcessDumpQueue — free all slots in the dump queue
void Anim_ProcessDumpQueue(void);

// ============================================================
//  Save/restore animation cache
// ============================================================

// Anim_ClearSavedAnim  — invalidate saved animation cache
void Anim_ClearSavedAnim(void);

// Anim_TryRestoreSaved — if pszName matches saved cache, restore it; returns 1
int  Anim_TryRestoreSaved(const char *pszName, int nSlot);

// ============================================================
//  On-screen tracking
// ============================================================

// Anim_AddOnscreen      — add slot's anim ID to the on-screen list
void Anim_AddOnscreen(int nSlot);

// Anim_AddOnscreenByNum — add by resource number
void Anim_AddOnscreenByNum(int nNum);

// Anim_RemoveOnscreenByName — remove from on-screen list by name/lang pair
void Anim_RemoveOnscreenByName(int nAnimId);

// Anim_FindOnscreenByName — find index in on-screen list by name/lang; -1 if not found
int  Anim_FindOnscreenByName(int nAnimId);

// Anim_ClearOnscreenList — reset on-screen count to 0
void Anim_ClearOnscreenList(void);

// Anim_ReloadOnscreenAnims — reload all on-screen animations after a room change
void Anim_ReloadOnscreenAnims(void);

// Anim_PutLastByName   — show last frame of named animation (used for transitions)
void Anim_PutLastByName(const char *pszName);

// Anim_PutLastByNum    — show last frame of numbered animation
void Anim_PutLastByNum(int nNum);

// ============================================================
//  Name / slot lookup helpers
// ============================================================

// Anim_FindSlotByName  — find slot by name+lang; returns slot or -1
int  Anim_FindSlotByName(int nAnimId, int nLangId);

// Anim_NamesMatch      — compare two (animId, langId) pairs by name; returns bool
int  Anim_NamesMatch(int nId1, int nLang1, int nId2, int nLang2);

// Anim_GetName         — return pointer to 20-byte name string for slot
char *Anim_GetName(int nSlot);

// ============================================================
//  Per-slot state accessors
// ============================================================

// Anim_SetPosition     — set (x, y) screen position offset for slot
void Anim_SetPosition(int nSlot, int nX, int nY);

// Anim_SetVelocity     — set (vx, vy) pixel-per-step velocity for slot
void Anim_SetVelocity(int nSlot, int nVX, int nVY);

// Anim_SetCurrentFrame — set current frame index (only if slot is active)
void Anim_SetCurrentFrame(int nSlot, int nFrame);

// Anim_GetCurrentFrame — return current frame index (-1 if frozen/inactive)
int  Anim_GetCurrentFrame(int nSlot);

// Anim_GetNextFrame    — copy next-frame slot info from the frame-table
void Anim_GetNextFrame(int *pSlot);

// Anim_AdvanceFrame    — advance frame using the current frame-table entry
void Anim_AdvanceFrame(int *pSlot);

// Anim_SetFrameStep    — set per-slot frame advance step
void Anim_SetFrameStep(int nSlot, int nStep);

// Anim_SetGlobalFrameStep — set global frame step override (affects all slots)
void Anim_SetGlobalFrameStep(int nStep);

// Anim_SetStopFrame    — set the frame at which the animation should stop
int  Anim_SetStopFrame(int nSlot, int nFrame);

// Anim_SetStopAtLastFrame — set stop to last frame of animation
int  Anim_SetStopAtLastFrame(const char *pszName);

// Anim_IsAtStopFrame   — return 1 if current frame equals the stop frame
int  Anim_IsAtStopFrame(int nSlot);

// Anim_SetLoopingFlags — set looping bit flags (0x400 | 0x8) for slot
void Anim_SetLoopingFlags(int nSlot);

// ============================================================
//  Freeze / unfreeze
// ============================================================

// Anim_Freeze          — increment freeze count for slot
void Anim_Freeze(int nSlot);

// Anim_Unfreeze        — decrement freeze count (down to 0)
void Anim_Unfreeze(int nSlot);

// Anim_ResetFreeze     — force-reset freeze count to 0
void Anim_ResetFreeze(int nSlot);

// Anim_FreezeAll       — freeze all active slots + call tick-callback(0)
void Anim_FreezeAll(void);

// Anim_UnfreezeAll     — unfreeze all active slots + call tick-callback(1)
void Anim_UnfreezeAll(void);

// ============================================================
//  Completion callbacks
// ============================================================

// Anim_SetCompletionCallback — register a program-handle callback for slot
void Anim_SetCompletionCallback(int nSlot, int nProgHandle, int nDelay, int nFrame);

// Anim_ClearAllCompletionCallbacks — clear callbacks on all 150 slots
void Anim_ClearAllCompletionCallbacks(void);

// Anim_HasPendingCallback  — return 1 if any slot has a pending callback
int  Anim_HasPendingCallback(void);

// Anim_ResetPendingCallbacks — set all pending callback delays to 1 (immediate)
void Anim_ResetPendingCallbacks(void);

// ============================================================
//  Tick callbacks (registered functions called each game tick)
// ============================================================

// Anim_RegisterTickCallback   — add a (funcPtr, tickFunc, key) triple
void Anim_RegisterTickCallback(int nFuncPtr, int nTickFunc, int nKey);

// Anim_UnregisterTickCallback — remove callbacks for a given key
void Anim_UnregisterTickCallback(int nKey);

// Anim_FireTickCallbacks      — call all callbacks whose key matches nKey
void Anim_FireTickCallbacks(int nKey);

// Anim_SetTickMode     — queue a tick-mode change for next frame
void Anim_SetTickMode(int nMode);

// ============================================================
//  Per-frame tick
// ============================================================

// Anim_HandleFrameTick  — advance one slot's frame, fire sounds + callbacks
void Anim_HandleFrameTick(int nSlot);

// ============================================================
//  Frame position / size queries
// ============================================================

// Anim_ShowFrame         — blit frame nFrame of slot nSlot at (x+dx, y+dy)
void Anim_ShowFrame(int nSlot, int nFrame, int nDX, int nDY);

// Anim_ShowFrameScaled   — blit with uniform scale
void Anim_ShowFrameScaled(int nSlot, int nFrame, int nDX, int nDY, int nScale);

// Anim_ShowFrameRescale  — blit with rescale (same internal call as scaled)
void Anim_ShowFrameRescale(int nSlot, int nFrame, int nDX, int nDY, int nScale);

// Anim_ShowFrameRotated  — blit with rotation angle (degrees * 10)
void Anim_ShowFrameRotated(int nSlot, int nFrame, int nDX, int nDY,
                            int nScale, int nUnk, int nAngleTenths);

// Anim_GetFrameTopLeft   — get minimum (x, y) across all frames in slot
void Anim_GetFrameTopLeft(int nSlot, int *pnX, int *pnY);

// Anim_GetFrameBottomRight — get maximum (x+w, y) across all frames in slot
void Anim_GetFrameBottomRight(int nSlot, int *pnX, int *pnY);

// Anim_GetCurrentFramePos — get (x, y) of current-display frame + position offset
void Anim_GetCurrentFramePos(int nSlot, int *pnX, int *pnY);

// Anim_GetCurrentFrameRect — get (x, y, w, h) of current frame if visible
void Anim_GetCurrentFrameRect(int nSlot, int *pnX, int *pnY,
                               unsigned *puW, unsigned *puH);

// Anim_GetPrevFrameRect   — get (x, y, w, h) from last-displayed frame
void Anim_GetPrevFrameRect(int nSlot, int *pnX, int *pnY,
                            unsigned *puW, unsigned *puH);

// Anim_GetFramePos        — get (x, y) for explicit frame nFrame in slot
void Anim_GetFramePos(int nSlot, int nFrame, int *pnX, int *pnY);

// Anim_GetFramePosAndSize — get (x, y, w, h) for explicit frame; clips to 479y
void Anim_GetFramePosAndSize(int nSlot, int nFrame,
                              int *pnX, int *pnY, unsigned *puW, unsigned *puH);

// Anim_FindTopAtXY        — find topmost (highest Z) animation covering (x, y)
void Anim_FindTopAtXY(int nX, int nY);

// ============================================================
//  Z-order and drawing
// ============================================================

// Anim_BuildDrawOrder  — build sorted draw-order list for this frame
void Anim_BuildDrawOrder(void);

// Anim_CompareByZ      — qsort comparator: compare two slots by Z value
int  Anim_CompareByZ(int *pA, int *pB);

// Anim_SetMainCharAnim — called on room entry: re-run all on-screen anims
//                        and re-initialize the main character animation
void Anim_SetMainCharAnim(const char *pszName);

// Anim_SetWalkTableBase — set Z-order value for a slot's walk-table column
void Anim_SetWalkTableBase(int nSlot, int nZ);

// Anim_EnableDraw      — set g_nDrawEnabled=1
void Anim_EnableDraw(void);

// Anim_EnableDraw2     — set g_nDrawEnabled2=1
void Anim_EnableDraw2(void);

// Anim_BeginNormalDraw    — set draw mode 0 and flush draw list
void Anim_BeginNormalDraw(void);

// Anim_BeginAdditiveDraw  — set draw mode 2 and flush draw list
void Anim_BeginAdditiveDraw(void);

// Anim_FlushDraw          — flush draw list with mode 0
void Anim_FlushDraw(void);

// Anim_CompactFrameTable  — compact resource entry table, return new slot base
int  Anim_CompactFrameTable(void);

// ============================================================
//  Frame-sound management
// ============================================================

// Anim_SetFrameSound   — bind sound name + flags to a specific frame
void Anim_SetFrameSound(int nSlot, int nFrame, const char *pszSound, int nFlags);

// Anim_ReleaseSoundRef — decrement reference count of sound entry nIdx
void Anim_ReleaseSoundRef(int nIdx);

// Anim_StopSound       — stop sound on any channel currently playing for slot
void Anim_StopSound(int nSlot);

// ============================================================
//  Individual palettes
// ============================================================

// Anim_SetIndiPal      — load and assign per-animation palette from file
void Anim_SetIndiPal(int nSlot, const char *pszPalFile);

// Anim_DevSetIndiPal   — developer (disk) version of Anim_SetIndiPal
void Anim_DevSetIndiPal(int nSlot, const char *pszPalFile);

// Anim_ReleaseIndiPal  — release individual palette assigned to slot
void Anim_ReleaseIndiPal(int nSlot);

// ============================================================
//  Global / resource palette
// ============================================================

// Anim_SetPalCallback  — set function pointer called after each palette load
void Anim_SetPalCallback(int nCallback);

// Anim_LoadPalette     — load .PAL from resource archive and apply
void Anim_LoadPalette(const char *pszName);

// Anim_DevLoadPalette  — load .PAL from disk and apply
void Anim_DevLoadPalette(const char *pszName);

// Anim_UpdatePalettes  — tick palette + update all individual-palette transforms
void Anim_UpdatePalettes(void);

// Anim_TickPalette     — state-machine palette tick (load/fade/apply)
void Anim_TickPalette(void);

// ============================================================
//  Synchronized animation groups
// ============================================================

// Anim_StartGroup      — begin defining a new animation group (nSize members,
//                        nTriggerPct % chance per tick to start one member)
void Anim_StartGroup(int nSize, int nTriggerPct);

// Anim_AddToGroup      — add slot nSlot to the current group being defined
void Anim_AddToGroup(int nSlot);

// Anim_IsInGroup       — return 1 if slot nSlot is a member of any group
int  Anim_IsInGroup(int nSlot);

// ============================================================
//  Shadow flag
// ============================================================

// Anim_SetShadowFlag   — set/clear shadow-cast flag (bit 9) for slot
void Anim_SetShadowFlag(int nSlot, int nEnable);

// ============================================================
//  Globals (extern declarations for cross-module access)
// ============================================================

// Animation slot state arrays (0x58-byte stride, 150 entries each)
extern int  g_anAnimSlotFlags[150];        // 0x005b10b0
extern int  g_anAnimSlotStopFrame[150];    // 0x005b10b4
extern int  g_anAnimSlotGroupId[150];      // 0x005b10b8
extern int  g_anAnimSlotX[150];            // 0x005b10c0
extern int  g_anAnimSlotY[150];            // 0x005b10c4
extern int  g_anAnimSlotVelX[150];         // 0x005b10c8
extern int  g_anAnimSlotVelY[150];         // 0x005b10cc
extern int  g_anAnimSlotCurFrame[150];     // 0x005b10d0
extern int  g_anAnimSlotPrevFrame[150];    // 0x005b10d4
extern int  g_anAnimSlotLastFrame[150];    // 0x005b10d8
extern int  g_anAnimSlotTriggerFrame[150]; // 0x005b10dc
extern int  g_anAnimSlotStep[150];         // 0x005b10e0
extern int  g_anAnimSlotFreezeCount[150];  // 0x005b10e4
extern int  g_anAnimSlotCallback[150];     // 0x005b10e8
extern int  g_anAnimSlotCallbackDelay[150];// 0x005b10ec
extern int  g_anAnimSlotCallbackFrame[150];// 0x005b10f0
extern int  g_anAnimSlotReserved[150];     // 0x005b10f8
extern int  g_anAnimSlotLangId[150];       // 0x005b10fc
extern int  g_anAnimSlotNum[150];          // 0x005b1100
extern int  g_anAnimSlotIndiPalIdx[150];   // 0x005b1104

// Frame / resource tables
extern int  g_anAnimFrameCount[150];       // 0x00574990  — frames per slot
extern int  g_anAnimFrameTable[60000];     // 0x004e3b58  — slot*400+frame → entry
extern int  g_anAnimFrameTablePrev[60000]; // 0x004e3b54  — sliding-window prev
extern int  g_anAnimSoundIndex[60000];     // 0x00575a18  — frame sound ref
extern int  g_anAnimSoundFlags[60000];     // 0x005b7c90  — frame sound flags
extern int  g_nAnimGlobalStep;             // 0x004e3b38  — global frame step

// Resource entry cache (1400 * 0x20 bytes)
extern int  g_anResEntryX[1400];           // 0x0051e4f0
extern int  g_anResEntryY[1400];           // 0x0051e4f4
extern int  g_anResEntryBudget[1400];      // 0x0051e4f8
extern int  g_anResEntrySize[1400];        // 0x0051e4fc
extern int  g_anResEntryPtr[1400];         // 0x0051e500
extern int  g_anResEntryReady[1400];       // 0x0051e50c
extern int  g_anGroupTriggerPct[150];      // 0x0051e4d8

// On-screen tracking
extern int  g_nOnscreenAnimCount;          // 0x005296f0
extern int  g_anOnscreenAnimIds[100];      // 0x005755f0  — pairs [animNum, langId]

// Slot name strings
extern char g_abAnimSlotNames[3000];       // 0x005b4b88  — 150 * 20 bytes

// Dump queue
extern int  g_nDumpQueueCount;             // 0x005b7c8c
extern int  g_anDumpQueue[50];             // 0x005f26b0

// Sound name registry
extern int  g_nSoundNameCount;             // 0x005b4440
extern char g_abSoundNames[1800];          // 0x005b4480  — 200 * 9 bytes
extern int  g_anSoundRefCount[200];        // 0x005b03c8
extern int  g_anSoundChannelSlot[8];       // 0x005b1090
extern int  g_anSoundChannelActive[8];     // 0x005b03a8

// Animation groups
extern int  g_nGroupCount;                 // 0x005b4470
extern int  g_anGroupSize[150];            // 0x00575a00
extern char g_abGroupMembers[1500];        // 0x00574958  — 150 * 10 bytes
extern int  g_anGroupActiveSlot[150];      // 0x004e3b40
extern int  g_nGroupMemberTemp;            // 0x005b07ec

// Draw order
extern int  g_anDrawOrderList[200];        // 0x005b07f0
extern int  g_nDrawEnabled;                // 0x005f3330
extern int  g_nDrawEnabled2;               // 0x005f3334

// Tick callbacks
extern int  g_nTickCallbackCount;          // 0x005f3338
extern int  g_anTickCallbackFunc[200];     // 0x005f2610  — 50 * 0x10-byte records
extern int  g_nTickCallbackSeq;            // 0x005f333c
extern int  g_nTickModeNext;               // 0x005f3344
extern int  g_nTickModeCur;                // 0x005f3340

// Frame-cache save/restore
extern int  g_nSavedAnimFrameCount;        // 0x005b4474  — -1 = nothing saved
extern int  g_anSavedFrameTable[400];      // 0x005b7540
extern int  g_anSavedFrameXTable[400];     // 0x005b0a50
extern char g_abSavedAnimName[20];         // 0x005b7b88
extern int  g_nSavedAnimSlot;              // 0x00574be8

// Individual palettes
extern int  g_nIndiPalCount;               // 0x005b0a48
extern int  g_anIndiPalRefCount[10];       // 0x005b4448
extern char g_abIndiPalNames[2560];        // 0x00574bf0
extern char g_abIndiPalData[7680];         // 0x005b5740

// Misc
extern int  g_nMaskSlot;                   // 0x005b0398
extern int  g_nAnimMemBase;                // 0x005b7b80
extern int  g_nMaxAnimUsage;               // 0x005f3350
extern int  g_nSavedAnimFrameCount;        // 0x005b4474
