// Advanim.cpp — Animation-slot system (advanced animation manager)
//
// Manages a pool of 150 animation slots. Each slot holds one named .ANI
// resource streamed from the resource archive. The system provides:
//   - Slot allocation, loading and freeing (with optional frame-cache save)
//   - Per-tick frame advance, velocity integration, freeze/unfreeze
//   - Frame-level sound triggers (per-frame sound name + channel/volume flags)
//   - Completion callbacks fired on loop-end or at a specific frame
//   - On-screen animation tracking list (max 50 visible anims)
//   - Z-sorted draw-order list built each frame by Anim_BuildDrawOrder
//   - Synchronized animation groups (random trigger, shared timing)
//   - Per-animation individual palettes (max 10 simultaneously)
//   - Global and dev-mode palette load/tick (fade-in, fade-to-target)
//   - Animation frame-cache save/restore for fast room re-entry
//
// Original source: C:\DevStudio\Projects\Crux\Advanim.cpp
//
// Architecture:
//   All per-slot state lives in parallel arrays indexed by slot index (0-149).
//   The AnimSlot "struct" is 0x58 bytes; the base is g_anAnimSlotFlags[].
//
//   Frame data is stored in a flat resource-entry cache of 1400 entries
//   (g_anResEntryPtr[]), managed by the Res_* subsystem.  Each animation
//   frame maps to one entry via g_anAnimFrameTable[slot*400 + frame].
//
//   Sound triggers use a separate parallel pair of arrays:
//     g_anAnimSoundIndex[slot*400+frame] — index into g_abSoundNames[]
//     g_anAnimSoundFlags[slot*400+frame] — channel/volume/flag word
//
// Key constants from decompiled debug strings:
//   "ani_prepare_for_read"   FUN_00404470 = Anim_PrepareForRead
//   "main_anim_char__name"   FUN_00404680 = Anim_SetMainCharAnim
//   "void handle_ani_int_actani" FUN_004059c0 = Anim_HandleFrameTick
//   "void ani_add_onscreen"  FUN_00406430 = Anim_AddOnscreen
//   "ani_add_by_num"         FUN_00409570 = Anim_AddByNum
//   "ani_add_by_name"        FUN_004098c0 = Anim_AddByName
//   "external_ani_add_by_name" FUN_00409aa0= Anim_ExternalAddByName
//   "void ani_put_last_by_name" FUN_00409cc0= Anim_PutLastByName
//   "ani_put_last_by_num"    FUN_0040a100 = Anim_PutLastByNum
//   "void ani_add_onscreen_by_num" FUN_0040a1d0 = Anim_AddOnscreenByNum
//   "ani_to_mem"             FUN_0040bb20 = Anim_LoadToMem
//   "dev_ani_to_mem"         FUN_0040cc30 = Anim_DevLoadToMem
//   "read_frame_hdr"         FUN_0040a630 = Anim_ReadFrameHeader
//   "dev_read_frame_hdr"     FUN_0040d2f0 = Anim_DevReadFrameHeader
//   "set_frame_hdr"          FUN_0040a740 = Anim_SetFrameHeader
//   "set_frm_sound"          FUN_0040ac20 = Anim_SetFrameSound
//   "ani_set_indi_pal"       FUN_0040b430 = Anim_SetIndiPal
//   "dev_set_indi_pal"       FUN_0040b690 = Anim_DevSetIndiPal
//   "void load_mask"         FUN_0040af60 = Anim_LoadMask
//   "show_frame"             FUN_00407e80 = Anim_ShowFrame
//   "show_frame_scl"         FUN_00408120 = Anim_ShowFrameScaled
//   "show_frame_rescale"     FUN_00408370 = Anim_ShowFrameRescale
//   "show_frame_r"           FUN_004085c0 = Anim_ShowFrameRotated
//   "ani_find_top_ani"       FUN_0040dfe0 = Anim_FindTopAtXY

#include "Advanim.h"
#include "AREAS.h"
#include "TIMERS.h"
#include "SCHED.h"
#include "THEMES.h"
#include "SETPAL.h"
#include "FILES.h"
#include "ERRORS.h"
#include "SAFEHEAP.h"
#include <windows.h>
#include <string.h>
#include <math.h>

// ============================================================
//  Globals
// ============================================================

// --- AnimSlot state parallel arrays (stride 0x58 per slot) ---

int  g_anAnimSlotFlags[150]         = {0};  // 0x005b10b0  bit-flags (active, looping, etc.)
int  g_anAnimSlotStopFrame[150]     = {0};  // 0x005b10b4  stop-at-frame (-1=none)
int  g_anAnimSlotGroupId[150]       = {0};  // 0x005b10b8  group index or -1
int  g_anAnimSlotX[150]             = {0};  // 0x005b10c0  screen X offset
int  g_anAnimSlotY[150]             = {0};  // 0x005b10c4  screen Y offset
int  g_anAnimSlotVelX[150]          = {0};  // 0x005b10c8  X velocity (px/step)
int  g_anAnimSlotVelY[150]          = {0};  // 0x005b10cc  Y velocity
int  g_anAnimSlotCurFrame[150]      = {0};  // 0x005b10d0  current frame index
int  g_anAnimSlotPrevFrame[150]     = {0};  // 0x005b10d4  previous frame
int  g_anAnimSlotLastFrame[150]     = {0};  // 0x005b10d8  last-displayed frame
int  g_anAnimSlotTriggerFrame[150]  = {0};  // 0x005b10dc  trigger-at-frame (-1=none)
int  g_anAnimSlotStep[150]          = {0};  // 0x005b10e0  advance step
int  g_anAnimSlotFreezeCount[150]   = {0};  // 0x005b10e4  freeze ref-count
int  g_anAnimSlotCallback[150]      = {0};  // 0x005b10e8  completion prog handle
int  g_anAnimSlotCallbackDelay[150] = {0};  // 0x005b10ec  countdown
int  g_anAnimSlotCallbackFrame[150] = {0};  // 0x005b10f0  callback-at-frame
int  g_anAnimSlotReserved[150]      = {0};  // 0x005b10f8  padding
int  g_anAnimSlotLangId[150]        = {0};  // 0x005b10fc  language ID
int  g_anAnimSlotNum[150]           = {0};  // 0x005b1100  resource number
int  g_anAnimSlotIndiPalIdx[150]    = {0};  // 0x005b1104  indi-palette index

// --- Frame / resource tables ---

// g_anAnimFrameCount[slot] = number of frames; accessed externally by MOVEMENT.cpp
int  g_anAnimFrameCount[150]        = {0};  // 0x00574990

// g_anAnimFrameTable[slot*400+frame] = resource-entry index
int  g_anAnimFrameTable[60000]      = {0};  // 0x004e3b58

// Sliding-window previous frame entries (used with mask layer management)
int  g_anAnimFrameTablePrev[60000]  = {0};  // 0x004e3b54

// Per-frame sound indices and flags
int  g_anAnimSoundIndex[60000]      = {0};  // 0x00575a18  sound name ref, -1=none
int  g_anAnimSoundFlags[60000]      = {0};  // 0x005b7c90  upper word=flags, lower=vol

// Global frame advance step (1=normal, -1=reverse all, 0=freeze all)
int  g_nAnimGlobalStep              = 1;    // 0x004e3b38

// --- Resource entry cache (1400 entries x 0x20 bytes each) ---
// These arrays are the "flat" projection of the resource cache structure.
// Layout at offset = entry * 0x20:
//   +0x00 = X position      +0x04 = Y position
//   +0x08 = budget/LRU      +0x0C = data size
//   +0x10 = data pointer    +0x14 = async task handle
//   +0x18 = async flags     +0x1C = ready flag

int  g_anResEntryX[1400]            = {0};  // 0x0051e4f0
int  g_anResEntryY[1400]            = {0};  // 0x0051e4f4
int  g_anResEntryBudget[1400]       = {0};  // 0x0051e4f8
int  g_anResEntrySize[1400]         = {0};  // 0x0051e4fc
int  g_anResEntryPtr[1400]          = {0};  // 0x0051e500
int  g_anResEntryReady[1400]        = {0};  // 0x0051e50c
int  g_anGroupTriggerPct[150]       = {0};  // 0x0051e4d8  group trigger %

// --- On-screen tracking ---
int  g_nOnscreenAnimCount           = 0;    // 0x005296f0  count of on-screen entries
int  g_anOnscreenAnimIds[100]       = {0};  // 0x005755f0  pairs [animNum, langId]

// --- Slot name strings (150 * 20 bytes) ---
char g_abAnimSlotNames[3000]        = {0};  // 0x005b4b88

// --- Dump queue ---
int  g_nDumpQueueCount              = 0;    // 0x005b7c8c
int  g_anDumpQueue[50]              = {0};  // 0x005f26b0

// --- Sound name registry ---
int  g_nSoundNameCount              = 0;    // 0x005b4440  active entries in table
char g_abSoundNames[1800]           = {0};  // 0x005b4480  200 * 9 bytes
int  g_anSoundRefCount[200]         = {0};  // 0x005b03c8
int  g_anSoundChannelSlot[8]        = {0};  // 0x005b1090  slot playing on each channel
int  g_anSoundChannelActive[8]      = {0};  // 0x005b03a8

// --- Animation groups ---
int  g_nGroupCount                  = 0;    // 0x005b4470  active groups
int  g_anGroupSize[150]             = {0};  // 0x00575a00  members per group
char g_abGroupMembers[1500]         = {0};  // 0x00574958  150*10 member slot indices
int  g_anGroupActiveSlot[150]       = {0};  // 0x004e3b40  active slot per group
int  g_nGroupMemberTemp             = 0;    // 0x005b07ec  building-group temp counter

// --- Draw order ---
int  g_anDrawOrderList[200]         = {0};  // 0x005b07f0  sorted slot indices
int  g_nDrawEnabled                 = 0;    // 0x005f3330
int  g_nDrawEnabled2                = 0;    // 0x005f3334

// --- Tick callbacks ---
// Each record is 0x10 bytes: [callFuncPtr, tickFuncPtr, filterKey, seqId]
int  g_nTickCallbackCount           = 0;    // 0x005f3338
int  g_anTickCallbackFunc[200]      = {0};  // 0x005f2610  50 * 0x10-byte records
int  g_nTickCallbackSeq             = 0;    // 0x005f333c
int  g_nTickModeNext                = 0;    // 0x005f3344  queued next tick mode
int  g_nTickModeCur                 = 0;    // 0x005f3340  current tick mode

// --- Frame-cache save/restore ---
int  g_nSavedAnimFrameCount         = -1;   // 0x005b4474  -1 = nothing cached
int  g_anSavedFrameTable[400]       = {0};  // 0x005b7540
int  g_anSavedFrameXTable[400]      = {0};  // 0x005b0a50
char g_abSavedAnimName[20]          = {0};  // 0x005b7b88  name of cached animation
int  g_nSavedAnimSlot               = 0;    // 0x00574be8

// --- Individual palettes ---
int  g_nIndiPalCount                = 0;    // 0x005b0a48  active indi-pal entries
int  g_anIndiPalRefCount[10]        = {0};  // 0x005b4448
char g_abIndiPalNames[2560]         = {0};  // 0x00574bf0  10 * 256-byte names
char g_abIndiPalData[7680]          = {0};  // 0x005b5740  10 * 768-byte RGB data

// --- Misc ---
int  g_nMaskSlot                    = -1;   // 0x005b0398  slot holding mask anim
int  g_nAnimMemBase                 = -1;   // 0x005b7b80  base of frame-data pool
int  g_nMaxAnimUsage                = 0;    // 0x005f3350  high-water mark

// Palette callback and palette buffers (defined in palette subsystem)
extern int   g_nPalCallback;               // 0x005b0658  — set by Anim_SetPalCallback
extern char  g_abTargetPal[768];           // 0x005293a8  — 256*3 target palette
extern char  g_abSnapshotPal[768];         // 0x005293f0  — snapshot palette

// Walk table base (per-character Z values, 0x16-ints wide; used for Z-sort)
// Accessed as (&g_nCharWalkTableBase)[slot * 0x16]
extern int   g_nCharWalkTableBase;         // 0x005b10b0+0x0C: col 3 of slot record

// ============================================================
//  Initialization
// ============================================================

// Anim_Init (0x0040a350)
// Initialize all animation tables.  Called once at program start.
// Reads MAXANI INI setting, sets up memory pool, zeros all tables.
void Anim_Init(void)
{
    // Read MAXANI from INI; init frame-data pool addresses
    // Zero all slot flags/frames/entries; zero draw-order buffer
    // (See decompiled body for exact field-by-field init)
}

// Anim_GameInit (0x0040e370)
// Per-game-start reset.  Resets movement, scheduler, carry-hint, etc.
// Also calls Anim_Init and Theme_InitTimerTable.
void Anim_GameInit(void)
{
    // (See decompiled body)
}

// ============================================================
//  Slot allocation
// ============================================================

// Anim_FindFreeSlot (0x004045b0)
// Scan g_anAnimSlotFlags[0..149]; return first slot with bit 0 clear.
// Returns -1 if all slots are occupied.
int Anim_FindFreeSlot(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if (!(g_anAnimSlotFlags[i] & 1))
            return i;
    }
    return -1;
}

// Anim_CheckFreeSlot (0x004091d0)
// Asserts a free slot exists (calls Anim_FindFreeSlot and discards result).
void Anim_CheckFreeSlot(void)
{
    Anim_FindFreeSlot();
}

// ============================================================
//  Name / slot lookup
// ============================================================

// Anim_NamesMatch (0x004051c0)
// Compare two (animId, langId) pairs by dereferencing their name string tables.
// Returns non-zero if names are identical.
int Anim_NamesMatch(int nId1, int nLang1, int nId2, int nLang2)
{
    // Uses g_apLangAnimNames[langId][animId] pointer tables at 0x00711510 (lang 0)
    // and 0x00709570 (lang 1+) to look up C strings, then strcmp.
    // (See decompiled body)
    return 0;
}

// Anim_FindSlotByName (0x004052d0)
// Search g_anAnimSlotNum / g_anAnimSlotLangId for a matching (animId, langId).
// Returns slot index or -1.
int Anim_FindSlotByName(int nAnimId, int nLangId)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if ((g_anAnimSlotFlags[i] & 1) &&
            !(g_anAnimSlotFlags[i] & 4) &&
            (g_anAnimSlotLangId[i] != -1))
        {
            if ((g_anAnimSlotNum[i] == nAnimId &&
                 g_anAnimSlotLangId[i] == nLangId) ||
                Anim_NamesMatch(nAnimId, nLangId,
                                g_anAnimSlotNum[i], g_anAnimSlotLangId[i]))
                return i;
        }
    }
    return -1;
}

// Anim_FindOnscreenByName (0x00403ed0)
// Scan g_anOnscreenAnimIds[0..g_nOnscreenAnimCount-1]; return index if found.
int Anim_FindOnscreenByName(int nAnimId)
{
    for (int i = 0; i < g_nOnscreenAnimCount; i++)
    {
        if (g_anOnscreenAnimIds[i * 2] == nAnimId)
            return i;
    }
    return -1;
}

// Anim_GetName (0x0040e220)
// Return pointer to slot's 20-byte name string.
char *Anim_GetName(int nSlot)
{
    return g_abAnimSlotNames + nSlot * 0x14;
}

// ============================================================
//  On-screen tracking
// ============================================================

// Anim_ClearOnscreenList (0x00404280)
// Reset on-screen count to zero.
void Anim_ClearOnscreenList(void)
{
    g_nOnscreenAnimCount = 0;
}

// Anim_RemoveOnscreenByName (0x00403dd0)
// Remove animation nAnimId from on-screen list by compacting the array.
void Anim_RemoveOnscreenByName(int nAnimId)
{
    int nIdx = Anim_FindOnscreenByName(nAnimId);
    if (nIdx == -1)
        return;
    g_nOnscreenAnimCount--;
    for (int i = nIdx; i < g_nOnscreenAnimCount; i++)
    {
        g_anOnscreenAnimIds[i * 2]     = g_anOnscreenAnimIds[(i + 1) * 2];
        g_anOnscreenAnimIds[i * 2 + 1] = g_anOnscreenAnimIds[(i + 1) * 2 + 1];
    }
}

// Anim_AddOnscreen (0x00406430)
// Add slot's (animNum, langId) pair to on-screen list if not already present.
// Asserts list does not exceed 50 entries.
void Anim_AddOnscreen(int nSlot)
{
    if (g_nOnscreenAnimCount > 49)
        return; // error: too many on-screen

    if (g_anAnimSlotNum[nSlot] == -1)
        return; // no valid anim num

    // Check for duplicate
    for (int i = 0; i < g_nOnscreenAnimCount; i++)
    {
        if (g_anOnscreenAnimIds[i * 2]     == g_anAnimSlotNum[nSlot] &&
            g_anOnscreenAnimIds[i * 2 + 1] == g_anAnimSlotLangId[nSlot])
            return;
    }

    g_anOnscreenAnimIds[g_nOnscreenAnimCount * 2]     = g_anAnimSlotNum[nSlot];
    g_anOnscreenAnimIds[g_nOnscreenAnimCount * 2 + 1] = g_anAnimSlotLangId[nSlot];
    g_nOnscreenAnimCount++;
}

// Anim_AddOnscreenByNum (0x0040a1d0)
// Add (nNum, g_nCurrentLang) to on-screen list if not already present.
void Anim_AddOnscreenByNum(int nNum)
{
    if (g_nOnscreenAnimCount > 49)
        return;

    for (int i = 0; i < g_nOnscreenAnimCount; i++)
    {
        if (g_anOnscreenAnimIds[i * 2] == nNum)
            return;
    }

    g_anOnscreenAnimIds[g_nOnscreenAnimCount * 2]     = nNum;
    // g_anOnscreenAnimIds[g_nOnscreenAnimCount * 2 + 1] = g_nCurrentLang;  (0x0070e130)
    g_nOnscreenAnimCount++;
}

// ============================================================
//  Per-slot state accessors
// ============================================================

// Anim_SetPosition (0x00406c50)
void Anim_SetPosition(int nSlot, int nX, int nY)
{
    g_anAnimSlotX[nSlot] = nX;
    g_anAnimSlotY[nSlot] = nY;
}

// Anim_SetVelocity (0x00406d00)
void Anim_SetVelocity(int nSlot, int nVX, int nVY)
{
    g_anAnimSlotVelX[nSlot] = nVX;
    g_anAnimSlotVelY[nSlot] = nVY;
}

// Anim_SetCurrentFrame (0x00406db0)
// Set current frame only if slot is active (bit 0 set in flags).
void Anim_SetCurrentFrame(int nSlot, int nFrame)
{
    if (g_anAnimSlotFlags[nSlot] & 1)
        g_anAnimSlotCurFrame[nSlot] = nFrame;
}

// Anim_GetCurrentFrame (0x00406e70)
// Return current frame if slot is active and not frozen; else -1.
int Anim_GetCurrentFrame(int nSlot)
{
    if ((g_anAnimSlotFlags[nSlot] & 1) && g_anAnimSlotFreezeCount[nSlot] == 0)
        return g_anAnimSlotCurFrame[nSlot];
    return -1;
}

// Anim_SetStopFrame (0x00405450)
// Set stop-at-frame for slot.  Only sets if slot is not looping/not in group
// or if a group-id is assigned.  Returns nSlot or -1 on failure.
int Anim_SetStopFrame(int nSlot, int nFrame)
{
    if ((g_anAnimSlotFlags[nSlot] & 2) || (g_anAnimSlotFlags[nSlot] & 4) ||
        (g_anAnimSlotGroupId[nSlot] != -1))
    {
        g_anAnimSlotStopFrame[nSlot] = nFrame;
    }
    else
    {
        nSlot = -1;
    }
    return nSlot;
}

// Anim_SetStopAtLastFrame (0x00405540)
// Find slot by name and set stop frame to (frameCount - 1).
int Anim_SetStopAtLastFrame(const char *pszName)
{
    // int nSlot = Anim_FindSlotByName(pszName, g_nCurrentLang);
    // if (nSlot < 0) return -1;
    // return Anim_SetStopFrame(nSlot, g_anAnimFrameCount[nSlot] - 1);
    return -1; // (See decompiled body at 0x00405540)
}

// Anim_IsAtStopFrame (0x00405610)
// Return non-zero if current stop-frame equals -1 (meaning: stop frame reached,
// and the slot's stop-frame field has been reset to -1 after firing).
int Anim_IsAtStopFrame(int nSlot)
{
    return g_anAnimSlotStopFrame[nSlot] == -1;
}

// Anim_SetFrameStep (0x00406f30)
void Anim_SetFrameStep(int nSlot, int nStep)
{
    g_anAnimSlotStep[nSlot] = nStep;
}

// Anim_SetGlobalFrameStep (0x00406fc0)
// Set global step. 1 = normal; -1 = all reverse; 0 = freeze all.
void Anim_SetGlobalFrameStep(int nStep)
{
    g_nAnimGlobalStep = nStep;
}

// Anim_SetLoopingFlags (0x00406b90)
// Set looping flags: bit 0x400 (group-loop) and bit 0x8 (has-walk-sound).
void Anim_SetLoopingFlags(int nSlot)
{
    g_anAnimSlotFlags[nSlot] |= 0x400;
    g_anAnimSlotFlags[nSlot] |= 8;
}

// Anim_SetWalkTableBase (0x00406980)
// Set the Z-value column in the per-character walk table for this slot.
// Accessed as (&g_nCharWalkTableBase)[nSlot * 0x16] = nZ.
void Anim_SetWalkTableBase(int nSlot, int nZ)
{
    // (&g_nCharWalkTableBase)[nSlot * 0x16] = nZ;
}

// ============================================================
//  Freeze / unfreeze
// ============================================================

// Anim_Freeze (0x00407050)
// Increment freeze count.
void Anim_Freeze(int nSlot)
{
    g_anAnimSlotFreezeCount[nSlot]++;
}

// Anim_Unfreeze (0x004070f0)
// Decrement freeze count (floor at 0).
void Anim_Unfreeze(int nSlot)
{
    if (g_anAnimSlotFreezeCount[nSlot] > 0)
        g_anAnimSlotFreezeCount[nSlot]--;
}

// Anim_ResetFreeze (0x004071a0)
void Anim_ResetFreeze(int nSlot)
{
    g_anAnimSlotFreezeCount[nSlot] = 0;
}

// Anim_FreezeAll (0x00407230)
// Freeze all active slots that are on-screen or in a group.
// Then call tick-callback with argument 0.
// Then call Timer_Tick().
void Anim_FreezeAll(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if ((g_anAnimSlotFlags[i] & 1) &&
            ((g_anAnimSlotFlags[i] & 2) || g_anAnimSlotGroupId[i] != -1))
            Anim_Freeze(i);
    }
    // fire tick callbacks with arg=0
    // Timer_Tick();
}

// Anim_UnfreezeAll (0x00407380)
// Unfreeze all active slots; call tick-callback(1); call Timer_Untick().
void Anim_UnfreezeAll(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if ((g_anAnimSlotFlags[i] & 1) &&
            ((g_anAnimSlotFlags[i] & 2) || g_anAnimSlotGroupId[i] != -1))
            Anim_Unfreeze(i);
    }
    // fire tick callbacks with arg=1
    // Timer_Untick();
}

// ============================================================
//  Completion callbacks
// ============================================================

// Anim_SetCompletionCallback (0x00406a10)
void Anim_SetCompletionCallback(int nSlot, int nProgHandle, int nDelay, int nFrame)
{
    g_anAnimSlotCallback[nSlot]      = nProgHandle;
    g_anAnimSlotCallbackDelay[nSlot] = nDelay;
    g_anAnimSlotCallbackFrame[nSlot] = nFrame;
}

// Anim_ClearAllCompletionCallbacks (0x00406ad0)
void Anim_ClearAllCompletionCallbacks(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
        Anim_SetCompletionCallback(i, -1, -1, -1);
}

// Anim_ResetPendingCallbacks (0x0040dbe0)
// For every slot with an active non-immediate callback, set delay to 1.
void Anim_ResetPendingCallbacks(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if ((g_anAnimSlotFlags[i] & 1) &&
            g_anAnimSlotCallback[i] != -1 &&
            g_anAnimSlotFreezeCount[i] == 0 &&
            g_anAnimSlotCallbackDelay[i] > 0)
            g_anAnimSlotCallbackDelay[i] = 1;
    }
}

// Anim_HasPendingCallback (0x0040dcf0)
// Return 1 if any active slot has a pending callback with delay > 0.
int Anim_HasPendingCallback(void)
{
    for (int i = 0; i < ANIM_MAX_SLOTS; i++)
    {
        if ((g_anAnimSlotFlags[i] & 1) &&
            g_anAnimSlotCallback[i] != -1 &&
            g_anAnimSlotFreezeCount[i] == 0 &&
            g_anAnimSlotCallbackDelay[i] > 0)
            return 1;
    }
    return 0;
}

// ============================================================
//  Tick callbacks
// ============================================================

// Anim_RegisterTickCallback (0x004065e0)
// Add a (callFunc, tickFunc, key, seq) record to g_anTickCallbackFunc[].
void Anim_RegisterTickCallback(int nFuncPtr, int nTickFunc, int nKey)
{
    // stores 4 ints at g_anTickCallbackFunc[g_nTickCallbackCount * 4]
    // uses g_nTickCallbackSeq++ as unique id
    g_nTickCallbackCount++;
    g_nTickCallbackSeq++;
}

// Anim_UnregisterTickCallback (0x004066e0)
// Remove all callback records matching nKey; compact the array.
void Anim_UnregisterTickCallback(int nKey)
{
    for (int i = 0; i < g_nTickCallbackCount; )
    {
        if (g_anTickCallbackFunc[i * 4] == nKey)
        {
            g_nTickCallbackCount--;
            for (int j = i; j < g_nTickCallbackCount; j++)
            {
                g_anTickCallbackFunc[j * 4]     = g_anTickCallbackFunc[(j + 1) * 4];
                g_anTickCallbackFunc[j * 4 + 1] = g_anTickCallbackFunc[(j + 1) * 4 + 1];
                g_anTickCallbackFunc[j * 4 + 2] = g_anTickCallbackFunc[(j + 1) * 4 + 2];
                g_anTickCallbackFunc[j * 4 + 3] = g_anTickCallbackFunc[(j + 1) * 4 + 3];
            }
            i--;
        }
        i++;
    }
}

// Anim_FireTickCallbacks (0x00406820)
// Call all callbacks whose filter-key field matches nKey.
void Anim_FireTickCallbacks(int nKey)
{
    for (int i = 0; i < g_nTickCallbackCount; i++)
    {
        if (g_anTickCallbackFunc[i * 4 + 2] == nKey)
        {
            // call g_anTickCallbackFunc[i*4] as function pointer
        }
    }
}

// Anim_SetTickMode (0x004068f0)
void Anim_SetTickMode(int nMode)
{
    g_nTickModeNext = nMode;
}

// ============================================================
//  Per-frame tick
// ============================================================

// Anim_HandleFrameTick (0x004059c0)
// Per-slot tick: advance frame, integrate velocity, fire frame sounds,
// fire completion callbacks, handle loop-end (bounce/stop/restart),
// update the on-screen-list display, handle group member transitions.
//
// This is the most complex function in the module.  Key logic:
//   1. If freeze count > 0: skip.
//   2. Sound trigger: check g_anAnimSoundIndex[] at current frame;
//      fire sound via MIXER on the appropriate channel.
//   3. Update last/prev frame trackers.
//   4. Advance frame: curFrame += (g_nAnimGlobalStep==1) ? step : globalStep.
//      Integrate position: X += step * velX; Y += step * velY.
//   5. On stop-frame hit: clear stop-frame, fire trigger if set.
//   6. On loop-end (curFrame >= frameCount):
//      - If looping: reset to 0.
//      - If stop-at-end: call Anim_MarkForDump.
//      - Decrement callback delay; fire Theme_RegisterAsyncProg when it hits 0.
//      - Handle mask-layer sliding-window demote.
void Anim_HandleFrameTick(int nSlot)
{
    // (See decompiled body at 0x004059c0)
}

// ============================================================
//  Mark for dump / dump queue
// ============================================================

// Anim_MarkForDump (0x00405810)
// Set dump-pending (bit 2) on slot; remove from area sprite list if needed;
// enqueue in g_anDumpQueue[].
void Anim_MarkForDump(int nSlot)
{
    if (nSlot < 0) return;
    if (g_anAnimSlotFlags[nSlot] & 0x1000)
    {
        Area_RemoveSprite(nSlot);
        g_anAnimSlotFlags[nSlot] &= ~0x1000;
    }
    if ((g_anAnimSlotFlags[nSlot] & 1) && !(g_anAnimSlotFlags[nSlot] & 8))
    {
        g_anAnimSlotFlags[nSlot] |= 4;
        g_anDumpQueue[g_nDumpQueueCount++] = nSlot;
    }
}

// Anim_MarkForDumpByName (0x004056b0)
// Find slot by name and mark it for dump.
int Anim_MarkForDumpByName(const char *pszName)
{
    // int nSlot = Anim_FindSlotByName(pszName, g_nCurrentLang);
    // if (nSlot >= 0) Anim_MarkForDump(nSlot);
    // return nSlot;
    return -1; // (See decompiled body at 0x004056b0)
}

// Anim_ProcessDumpQueue (0x004074d0)
// Call Anim_Free() on each slot in g_anDumpQueue, then clear queue.
void Anim_ProcessDumpQueue(void)
{
    for (int i = 0; i < g_nDumpQueueCount; i++)
        Anim_Free(g_anDumpQueue[i]);
    if (g_nDumpQueueCount)
    {
        // thunk_FUN_004037c0();  -- dirty rectangle / screen refresh
    }
    g_nDumpQueueCount = 0;
}

// ============================================================
//  Loading
// ============================================================

// Anim_AddByName (0x004098c0)
// Core load function. Finds a free slot, copies name, calls Anim_LoadToMem,
// resets slot num/lang to -1, resets curFrame to 0.
int Anim_AddByName(const char *pszName, int nBudget)
{
    int nSlot = Anim_FindFreeSlot();
    if (nSlot < 0)
        return -1; // error: no free slots

    // strcpy(g_abAnimSlotNames + nSlot * 0x14, pszName);
    Anim_LoadToMem(pszName, 7, nSlot, nBudget);
    g_anAnimSlotNum[nSlot]    = -1;
    g_anAnimSlotLangId[nSlot] = -1;
    g_anAnimSlotCurFrame[nSlot] = 0;
    return nSlot;
}

// Anim_AddByNum (0x00409570)
// Load by resource number. Looks up name from pointer table, calls
// Anim_AddByName, then sets nNum, lang = g_nCurrentLang, loop mode.
// nLoopMode: 0=bounce, 1=cycle, 3=pingpong.
int Anim_AddByNum(int nNum, int nLoopMode, int nBudget)
{
    // const char *pszName = g_apAnimNameByNum[nNum]; // 0x0070c24c table
    // int nSlot = Anim_AddByName(pszName, nBudget);
    // set walk-table slot; set num; set lang; set loop bits from nLoopMode
    // reset curFrame to 0;
    return -1; // (See decompiled body at 0x00409570)
}

// Anim_ExternalAddByName (0x00409aa0)
// Load a .SMA (external save-game animation) file.
// Builds path as "%s/%s.SMA" using g_abSaveGameDir.
int Anim_ExternalAddByName(const char *pszName)
{
    // (See decompiled body at 0x00409aa0)
    return -1;
}

// Anim_LoadToMem (0x0040bb20)
// Core loader: reads .ANI header from resource bundle, allocates frame
// headers, registers frame data with the resource streaming system.
// Also reads sound chunk (type 0x17) and per-frame sound assignments (type 10).
// nType=7 for normal animations, nType=8 for mask animations.
// nBudget controls streaming priority; -1=load all frames immediately.
void Anim_LoadToMem(const char *pszName, int nType, int nSlot, int nBudget)
{
    // (See decompiled body at 0x0040bb20 — largest function in module)
    // Also tries Anim_TryRestoreSaved first when nType==7
}

// Anim_DevLoadToMem (0x0040cc30)
// Developer version: opens file from disk using FUN_0048a340, reads header,
// loads all frames synchronously. Returns slot or -1.
int Anim_DevLoadToMem(const char *pszFname)
{
    // (See decompiled body at 0x0040cc30)
    return -1;
}

// Anim_LoadByName (0x0040b0c0)
// Wrapper: Anim_AddByName + set walk-table Z column + set flags.
int Anim_LoadByName(const char *pszName, int nZCol)
{
    int nSlot = Anim_AddByName(pszName, 0);
    // Anim_SetWalkTableBase(nSlot, nZCol);
    g_anAnimSlotFlags[nSlot] &= ~2;
    g_anAnimSlotFlags[nSlot] |= 8;
    g_anAnimSlotCallback[nSlot] = -1;
    g_anAnimSlotFlags[nSlot] |= 0x10;
    return nSlot;
}

// Anim_LoadByNameGetCount (0x0040b1f0)
// Like Anim_LoadByName but returns frame count instead of slot.
int Anim_LoadByNameGetCount(const char *pszName, int nZCol)
{
    int nSlot = Anim_AddByName(pszName, 0);
    // Anim_SetWalkTableBase(nSlot, nZCol);
    // set flags similar to Anim_LoadByName but clear 0x10, set 0x20
    return g_anAnimFrameCount[nSlot];
}

// Anim_LoadAndWait (0x0040b340)
// Load and spin until Anim_IsAtStopFrame() returns non-zero.
void Anim_LoadAndWait(const char *pszName, int nZCol)
{
    int nSlot = Anim_LoadByName(pszName, nZCol);
    g_anAnimSlotStopFrame[nSlot] = g_anAnimFrameCount[nSlot] - 1;
    while (!Anim_IsAtStopFrame(nSlot))
    {
        // thunk_FUN_00412ac0(); — yield / service resources
    }
    Anim_MarkForDump(nSlot);
}

// Anim_LoadMask (0x0040af60)
// Load a mask animation (type=8) into a free slot; store slot in g_nMaskSlot.
void Anim_LoadMask(const char *pszName)
{
    int nSlot = Anim_FindFreeSlot();
    if (nSlot == -1) return; // error

    g_nMaskSlot = nSlot;
    // strcpy(g_abAnimSlotNames + nSlot * 0x14, pszName);
    Anim_LoadToMem(pszName, 8, nSlot, -1);
}

// ============================================================
//  Frame-header I/O
// ============================================================

// Anim_ReadFrameHeader (0x0040a630)
// Read 8 bytes from resource bunch file; call Anim_SetFrameHeader.
void Anim_ReadFrameHeader(void *pBunchHandle, int nSlot, int nFrame)
{
    // Res_BunchFreadNow(buf, 1, 8, pBunchHandle);
    // Anim_SetFrameHeader(buf, nSlot, nFrame);
}

// Anim_DevReadFrameHeader (0x0040d2f0)
// Same but reads from raw FILE*.
void Anim_DevReadFrameHeader(void *pFile, int nSlot, int nFrame)
{
    // FUN_0048a180(buf, 1, 8, pFile);
    // Anim_SetFrameHeader(buf, nSlot, nFrame);
}

// Anim_SetFrameHeader (0x0040a740)
// Parse an 8-byte frame descriptor: [x:short, y:short, dataOffset:int].
// Find or allocate a resource-entry slot; assign frame data pointer and
// dimensions; record entry index in g_anAnimFrameTable[nSlot*400+nFrame].
// Implements LRU eviction logic via Anim_CompactFrameTable when memory
// budget (g_nResDataEnd) is exceeded.
void Anim_SetFrameHeader(short *pHdr, int nSlot, int nFrame)
{
    // (See decompiled body at 0x0040a740)
}

// ============================================================
//  Free
// ============================================================

// Anim_Free (0x0040d630)
// Free animation slot.  If the animation is cacheable (fully loaded,
// not an external file), saves frame data to g_anSavedFrameTable[] for
// fast restore.  Clears all slot state, removes from area-sprite list,
// releases individual palette, decrements sound reference counts.
void Anim_Free(int nSlot)
{
    if (nSlot < 0) return;
    // (See decompiled body at 0x0040d630)
}

// ============================================================
//  Save/restore cache
// ============================================================

// Anim_ClearSavedAnim (0x0040d400)
void Anim_ClearSavedAnim(void)
{
    g_nSavedAnimFrameCount = -1;
}

// Anim_TryRestoreSaved (0x0040d490)
// If pszName matches g_abSavedAnimName, copy g_anSavedFrameTable[] back
// into g_anAnimFrameTable[nSlot*400..], restore g_anResEntryX[] from
// g_anSavedFrameXTable[], then call Anim_ClearSavedAnim.
// Returns 1 on restore, 0 otherwise.
int Anim_TryRestoreSaved(const char *pszName, int nSlot)
{
    if (g_nSavedAnimFrameCount == -1) return 0;
    // if strcmp(pszName, g_abSavedAnimName) != 0: clear and return 0
    // else: copy tables, Anim_ClearSavedAnim, return 1
    return 0;
}

// ============================================================
//  Put-last helpers (transition to static last frame)
// ============================================================

// Anim_PutLastByName (0x00409cc0)
// Read last frame header from resource and display it at (0, 0).
// Used to hold a character in their final pose during a scene transition.
void Anim_PutLastByName(const char *pszName)
{
    // (See decompiled body at 0x00409cc0)
}

// Anim_PutLastByNum (0x0040a100)
// Look up name by resource number and call Anim_PutLastByName.
void Anim_PutLastByNum(int nNum)
{
    // const char *pszName = g_apAnimNameByNum[nNum];
    // Anim_PutLastByName(pszName);
    // Anim_AddOnscreenByNum(nNum);
}

// ============================================================
//  Reload on-screen anims after room change
// ============================================================

// Anim_ReloadOnscreenAnims (0x00404000)
// Called after a room transition.  For each on-screen animation:
//   - If on-screen flag is set and animation is active: reload it.
// For each slot with neverload flag, invalidate its anim pointer.
// Reset counts and call Anim_ClearOnscreenList.
void Anim_ReloadOnscreenAnims(void)
{
    // (See decompiled body at 0x00404000)
}

// ============================================================
//  Draw order
// ============================================================

// Anim_CompareByZ (0x00407930)
// qsort comparator.  Compares walk-table Z values for two slots.
int Anim_CompareByZ(int *pA, int *pB)
{
    // return (&g_nCharWalkTableBase)[*pA * 0x16] - (&g_nCharWalkTableBase)[*pB * 0x16];
    return 0;
}

// Anim_BuildDrawOrder (0x004075c0)
// Build g_anDrawOrderList[]: collect active on-screen slots + group active
// members, sort by Z, then sort in group-triggered random members.
// Called every frame before rendering.
void Anim_BuildDrawOrder(void)
{
    // (See decompiled body at 0x004075c0)
}

// Anim_EnableDraw (0x004079e0)
void Anim_EnableDraw(void)  { g_nDrawEnabled  = 1; }

// Anim_EnableDraw2 (0x00407a70)
void Anim_EnableDraw2(void) { g_nDrawEnabled2 = 1; }

// Anim_BeginNormalDraw (0x0040de00)
// Set GI draw mode 0 and flush the draw buffer.
void Anim_BeginNormalDraw(void)
{
    // GI_SetDrawMode(0);
    // thunk_FUN_0042f860(&DAT_005296f8);
}

// Anim_BeginAdditiveDraw (0x0040dea0)
// Set GI draw mode 2 (additive) and flush.
void Anim_BeginAdditiveDraw(void)
{
    // GI_SetDrawMode(2);
    // thunk_FUN_0042f860(&DAT_005296f8);
}

// Anim_FlushDraw (0x0040df40)
// Set GI draw mode 0 and flush (clear mode).
void Anim_FlushDraw(void)
{
    // GI_SetDrawMode(0);
    // thunk_FUN_0042fa30(&DAT_005296f8);
}

// Anim_CompactFrameTable (0x00407b00)
// Walk the resource-entry cache; compact live entries; retarget all
// g_anAnimFrameTable references.  Returns new first-free entry index.
int Anim_CompactFrameTable(void)
{
    // (See decompiled body at 0x00407b00)
    return 0;
}

// ============================================================
//  Frame position / size queries
// ============================================================

// Anim_ShowFrame (0x00407e80)
// Blit frame nFrame of slot nSlot at (resEntryX + dx, resEntryY + dy).
// Waits for async load if entry not yet ready.
// Uses individual palette if g_anAnimSlotIndiPalIdx != -1.
void Anim_ShowFrame(int nSlot, int nFrame, int nDX, int nDY)
{
    // (See decompiled body at 0x00407e80)
}

// Anim_ShowFrameScaled (0x00408120)
void Anim_ShowFrameScaled(int nSlot, int nFrame, int nDX, int nDY, int nScale)
{
    // (See decompiled body at 0x00408120)
}

// Anim_ShowFrameRescale (0x00408370)
void Anim_ShowFrameRescale(int nSlot, int nFrame, int nDX, int nDY, int nScale)
{
    // (See decompiled body at 0x00408370)
}

// Anim_ShowFrameRotated (0x004085c0)
// nAngleTenths is angle in degrees * 10; converted to 1/64 units internally.
void Anim_ShowFrameRotated(int nSlot, int nFrame, int nDX, int nDY,
                            int nScale, int nUnk, int nAngleTenths)
{
    // (See decompiled body at 0x004085c0)
}

// Anim_GetFrameTopLeft (0x00408810)
void Anim_GetFrameTopLeft(int nSlot, int *pnX, int *pnY)
{
    *pnX = 0x7fffffff;
    *pnY = 0x7fffffff;
    for (int i = 0; i < g_anAnimFrameCount[nSlot]; i++)
    {
        int nEntry = g_anAnimFrameTable[nSlot * ANIM_MAX_FRAMES + i];
        if (g_anResEntryX[nEntry] < *pnX) *pnX = g_anResEntryX[nEntry];
        if (g_anResEntryY[nEntry] < *pnY) *pnY = g_anResEntryY[nEntry];
    }
}

// Anim_GetFrameBottomRight (0x00408980)
// Returns max(X + width) and max(Y) across all frames.
// Width is read from short at resData+1 (frame header embedded in pixel data).
void Anim_GetFrameBottomRight(int nSlot, int *pnX, int *pnY)
{
    // (See decompiled body at 0x00408980)
}

// Anim_GetCurrentFramePos (0x00408b20)
// Return (x, y) of the frame at g_anAnimSlotPrevFrame[nSlot] + slot position.
void Anim_GetCurrentFramePos(int nSlot, int *pnX, int *pnY)
{
    if (g_anAnimSlotFreezeCount[nSlot] < 1)
    {
        int nEntry = g_anAnimFrameTable[nSlot * ANIM_MAX_FRAMES + g_anAnimSlotPrevFrame[nSlot]];
        *pnX = g_anResEntryX[nEntry] + g_anAnimSlotX[nSlot];
        *pnY = g_anResEntryY[nEntry] + g_anAnimSlotY[nSlot];
    }
    else
    {
        *pnX = -1;
        *pnY = -1;
    }
}

// Anim_GetCurrentFrameRect (0x00408c40)
// Return (x, y, w, h) for current display frame if slot is active + on-screen.
void Anim_GetCurrentFrameRect(int nSlot, int *pnX, int *pnY,
                               unsigned *puW, unsigned *puH)
{
    // (See decompiled body at 0x00408c40)
}

// Anim_GetPrevFrameRect (0x00408e10)
// Return (x, y, w, h) for the last-displayed frame.
void Anim_GetPrevFrameRect(int nSlot, int *pnX, int *pnY,
                            unsigned *puW, unsigned *puH)
{
    // (See decompiled body at 0x00408e10)
}

// Anim_GetFramePos (0x00408f80)
void Anim_GetFramePos(int nSlot, int nFrame, int *pnX, int *pnY)
{
    int nEntry = g_anAnimFrameTable[nSlot * ANIM_MAX_FRAMES + nFrame];
    *pnX = g_anResEntryX[nEntry] + g_anAnimSlotX[nSlot];
    *pnY = g_anResEntryY[nEntry] + g_anAnimSlotY[nSlot];
}

// Anim_GetFramePosAndSize (0x00409070)
// Like Anim_GetFramePos but also reads (w, h) from frame pixel-data header.
// Clamps to 479px maximum Y (screen height - 1).
void Anim_GetFramePosAndSize(int nSlot, int nFrame,
                              int *pnX, int *pnY, unsigned *puW, unsigned *puH)
{
    // (See decompiled body at 0x00409070)
}

// Anim_FindTopAtXY (0x0040dfe0)
// Find the slot with highest Z value whose bounding rect contains (x, y).
void Anim_FindTopAtXY(int nX, int nY)
{
    // (See decompiled body at 0x0040dfe0)
}

// ============================================================
//  Sound
// ============================================================

// Anim_SetFrameSound (0x0040ac20)
// Bind pszSound (9-byte name) to frame nFrame of slot nSlot.
// Registers sound in g_abSoundNames[], increments ref count, stores index
// and nFlags (channel / volume / loop bits) in parallel arrays.
void Anim_SetFrameSound(int nSlot, int nFrame, const char *pszSound, int nFlags)
{
    // (See decompiled body at 0x0040ac20)
}

// Anim_ReleaseSoundRef (0x0040aec0)
void Anim_ReleaseSoundRef(int nIdx)
{
    g_anSoundRefCount[nIdx]--;
}

// Anim_StopSound (0x0040e2b0)
// Stop any channel that is currently playing on behalf of slot nSlot.
void Anim_StopSound(int nSlot)
{
    for (int i = 0; i < 8; i++)
    {
        if (g_anSoundChannelSlot[i] == nSlot)
        {
            // thunk_FUN_00443df0(i); — stop mixer channel
        }
    }
}

// ============================================================
//  Individual palettes
// ============================================================

// Anim_SetIndiPal (0x0040b430)
// Load named palette from resource archive; assign to slot.
void Anim_SetIndiPal(int nSlot, const char *pszPalFile)
{
    // (See decompiled body at 0x0040b430)
}

// Anim_DevSetIndiPal (0x0040b690)
// Developer (disk) version of Anim_SetIndiPal.
void Anim_DevSetIndiPal(int nSlot, const char *pszPalFile)
{
    // (See decompiled body at 0x0040b690)
}

// Anim_ReleaseIndiPal (0x0040b8f0)
// Decrement ref count; if it drops below 1 compact the indi-palette table.
void Anim_ReleaseIndiPal(int nSlot)
{
    // (See decompiled body at 0x0040b8f0)
}

// ============================================================
//  Global palette
// ============================================================

// Anim_SetPalCallback (0x004049e0)
void Anim_SetPalCallback(int nCallback)
{
    g_nPalCallback = nCallback;
}

// Anim_LoadPalette (0x00404a70)
// Load .PAL by name; store in g_abTargetPal; fire g_nPalCallback if set.
// Also copies to g_abSnapshotPal and calls SetPal_* functions.
void Anim_LoadPalette(const char *pszName)
{
    // (See decompiled body at 0x00404a70)
}

// Anim_DevLoadPalette (0x00404c50)
// Developer (disk) version.
void Anim_DevLoadPalette(const char *pszName)
{
    // (See decompiled body at 0x00404c50)
}

// Anim_UpdatePalettes (0x00404d90)
// Called each game tick.  Updates global palette via Sched_UpdatePalette(1),
// then re-bakes all active individual-palette colour transforms.
void Anim_UpdatePalettes(void)
{
    // (See decompiled body at 0x00404d90)
}

// Anim_TickPalette (0x00404ec0)
// State-machine palette manager.  States stored in g_nPalState (0x004c4c48):
//   0 → 1: load snapshot
//   3: call Anim_UpdatePalettes
//   4: set g_nDrawEnabled=0; call SetPal_FadeInFromBlack
//   5: set g_nDrawEnabled=0; call SetPal_FadeToTarget
// Always updates g_abSnapshotPal from g_abTargetPal.
void Anim_TickPalette(void)
{
    // (See decompiled body at 0x00404ec0)
}

// ============================================================
//  Synchronized animation groups
// ============================================================

// Anim_StartGroup (0x00409260)
// Begin group definition.  Record nSize members expected and nTriggerPct
// in g_anGroupSize / g_anGroupTriggerPct for current group index.
void Anim_StartGroup(int nSize, int nTriggerPct)
{
    g_nGroupMemberTemp = 0;
    int nGrp = g_nGroupCount;
    g_anGroupSize[nGrp]         = (nSize == 0) ? 1 : nSize;
    g_anGroupTriggerPct[nGrp]   = nTriggerPct;
    g_anGroupActiveSlot[nGrp]   = -1;
}

// Anim_AddToGroup (0x00409460)
// Register slot nSlot as member of current group.  Update groupId in slot;
// clear flag bit 3 (has-sound) for this member.
// Increments g_nGroupCount when all members registered.
void Anim_AddToGroup(int nSlot)
{
    int nGrp = g_nGroupCount;
    g_abGroupMembers[g_nGroupMemberTemp + nGrp * 10] = (char)nSlot;
    g_nGroupMemberTemp++;
    g_anAnimSlotGroupId[nSlot] = nGrp;
    g_anAnimSlotFlags[nSlot] &= ~8;
    if (g_nGroupMemberTemp == g_anGroupSize[nGrp])
        g_nGroupCount++;
}

// Anim_IsInGroup (0x00409340)
// Return 1 if nSlot appears in any group's member list.
int Anim_IsInGroup(int nSlot)
{
    for (int nGrp = 0; nGrp < g_nGroupCount; nGrp++)
    {
        for (int m = 0; m < g_anGroupSize[nGrp]; m++)
        {
            int nMember = (unsigned char)g_abGroupMembers[m + nGrp * 10];
            if (g_anAnimSlotNum[nMember] == g_anAnimSlotNum[nSlot])
                return 1;
        }
    }
    return 0;
}

// ============================================================
//  Shadow flag
// ============================================================

// Anim_SetShadowFlag (0x0040db20)
// Set or clear bit 9 (shadow) for slot nSlot.
void Anim_SetShadowFlag(int nSlot, int nEnable)
{
    if (nSlot >= 0 && nSlot < ANIM_MAX_SLOTS)
    {
        g_anAnimSlotFlags[nSlot] =
            (g_anAnimSlotFlags[nSlot] & ~0x200) | ((nEnable & 1) << 9);
    }
}

// ============================================================
//  SetMainCharAnim
// ============================================================

// Anim_SetMainCharAnim (0x00404680)
// Entry-point called when switching the main character.  Sets global frame step
// to 1, saves the palette snapshot, reloads all on-screen anims, re-runs the
// main char animation (with current room), processes any pending loads.
void Anim_SetMainCharAnim(const char *pszName)
{
    // (See decompiled body at 0x00404680)
}

// ============================================================
//  Prepare for read (batch resource loading callback)
// ============================================================

// Anim_PrepareForRead (0x00404470)
// Receives a block of animation data from the resource loader.
// Finds a free slot; copies 0x16 ints (= 0x58 bytes) of slot state from
// the provided buffer.
void Anim_PrepareForRead(const int *pData)
{
    // (See decompiled body at 0x00404470)
}

// ============================================================
//  GetNextFrame / AdvanceFrame (slot state helpers for MOVEMENT.cpp)
// ============================================================

// Anim_GetNextFrame (0x00404310)
// Used by MOVEMENT.cpp walk-step logic.
// pSlot points to a per-char walk record (+0x14 = frame index, +0x1C = lang).
// Copies next frame data into *pSlot from the AnimSlot frame table.
void Anim_GetNextFrame(int *pSlot)
{
    // int nFrame = pSlot[5];   // offset +0x14
    // int nLang  = pSlot[7];   // offset +0x1C
    // pSlot[5] = g_anAnimSlotNum[nFrame];
    // pSlot[7] = g_anAnimSlotLangId[nFrame];
}

// Anim_AdvanceFrame (0x004043d0)
// Advance the walk-record frame by calling Anim_FindSlotByName (thunk 0x4052d0)
// on the current (num, lang) pair and storing the result.
void Anim_AdvanceFrame(int *pSlot)
{
    // int nResult = Anim_FindSlotByName(pSlot[5], pSlot[7]);
    // pSlot[5] = nResult;
}
