// ADVENT.cpp  --  Adventure engine core
//
// Two primary responsibilities:
//   1. Scene loop (Adv_RunScene): room entry, cursor/verb input collection,
//      script dispatch, inventory display, and scene exit.
//   2. Inventory display panel: multi-layer slot array management, icon
//      blit, scroll, compact, and Win32 drag-cursor construction.
//
// Original source: C:\DevStudio\Projects\Crux\ADVENT.cpp
// Address range:   0x0040e550 -- 0x004133c0  (47 functions)
//
// ---- Module layout ----------------------------------------------------------
//
//  0x0040e550  Adv_Init                    engine-global reset (speech table, timers)
//  0x0040e650  Adv_LoadScreenshot          screenshot save (file dialog + BMP write)
//  0x0040e960  Adv_PushCursorState         save cursor / drag state to one-slot snapshot
//  0x0040ea20  Adv_PopCursorState          restore cursor state from snapshot
//  0x0040eae0  Adv_ClearCursorState        discard cursor state snapshot
//  0x0040eb70  Adv_UpdateHotspot           update current hovered area node
//  0x0040ecb0  Adv_PostVerb                post pending verb triple + SetEvent
//  0x0040ed70  Adv_GetVerb                 read stored verb triple (no consume)
//  0x0040ee20  Adv_TestAndClearVerb        consume pending-verb flag
//  0x0040eec0  Adv_CursorHandler           main input wait loop (debug: "curs_handler")
//  0x0040f700  Adv_CheckRightClick         detect fresh right-button press
//  0x0040f7c0  Adv_InitAreaSlots           parse area node table → slot structures
//  0x0040fc40  Adv_RefreshCursor           build drag-cursor sprite from held item
//  0x004100d0  Adv_FlushAnimCallbacks      drain pending anim callbacks
//  0x00410170  Adv_SetCDCheck              flag "check CD at next scene entry"
//  0x00410190  Adv_AutoSaveRescue          write RESCUE.SAV if flag set
//  0x004101f0  Adv_RunScene                main scene loop (debug: "run_scn")
//  0x00411570  Adv_FindVerbHandler         look up script handle for verb on node/item
//  0x004116b0  Adv_RectsOverlap            AABB overlap test
//  0x00411760  Adv_RegisterCleanup         push cleanup fn (debug: "add_cleanup")
//  0x00411870  Adv_UnregisterCleanup       remove cleanup fn from stack
//  0x00411980  Adv_RunCleanups             pop+call all cleanups LIFO
//  0x00411a30  Adv_ClampInvScroll          clamp scroll offset to valid range
//  0x00411b40  Adv_AddInvItem              append item to layer-0 list
//  0x00411c80  Adv_RemoveInvItem           remove item from layer-0 list
//  0x00411d70  Adv_ClearInvLayer           zero out one layer's slot state
//  0x00411eb0  Adv_SetInvSlot              assign node to (layer, slot, nodeId)
//  0x00412030  Adv_SetInvSlotDirect        write node to (layer, slot) directly
//  0x004120d0  Adv_FillFreeInvSlot         write node into first free (-1) slot
//  0x004121c0  Adv_ClearInvSlot            set (layer, slot) = -1
//  0x00412260  Adv_RemoveInvSlotByNode     clear all slots matching nodeId
//  0x00412350  Adv_IsNodeInSpecialSlot     check special-slot array for nodeId
//  0x00412430  Adv_CompactInvList          remove gaps, update scroll clamp
//  0x00412570  Adv_SetUpdateCallback       store GV update callback ptr (RegisterForUpdate)
//  0x00412600  Adv_ScrollInv               advance scroll offset, compact
//  0x00412710  Adv_UpdateInv               blit one inventory layer (debug: "update_inv")
//  0x004129e0  Adv_DrawAllInvLayers        redraw all active layers
//  0x00412ab0  Adv_SetDrawSuppressed       set g_nAdvDrawSuppressed
//  0x00412ac0  Adv_Tick                    pump one display frame
//  0x00412be0  Adv_TickFrames              pump N frames with async dispatch
//  0x00412cc0  Adv_TickFramesNoAsync       pump N frames, no async dispatch
//  0x00412da0  Adv_WaitForMouse            block until mouse, async ok (debug: "wait_for_mouse")
//  0x00412ed0  Adv_WaitForMouseNoAsync     block until mouse, no async (debug: "wait_for_mouse_no_async")
//  0x00413000  Adv_WaitForFrameOrMouse     block frame/mouse, returns 1/2/3 (debug: "wait_for_frame_or_mouse")
//  0x00413190  Adv_WaitForFrameOrMouseNoAsync  same, no async (debug: "wait_for_frame_or_mouse_no_async")
//  0x00413320  Adv_ResetAnimSentinel       reset sentinel globals (mislabelled Advanim_Tick in MOVEMENT.cpp)
//  0x004133c0  Adv_LoadAnimByName          load anim resource → slot (debug: "add_a32")
//
// ---- Key questions answered -------------------------------------------------
//
//  PickUpItem(itemIdx, mode, flag):
//      NOT a 1:1 function in this module.  The "pick up" action is composed:
//        - Adv_AddInvItem(nNodeIdx)         adds the node to the inv list
//        - Adv_SetInvSlot/FillFreeInvSlot   places the node in a display slot
//        - Adv_CompactInvList               removes gaps
//      Graninv.cpp calls a thunk that resolves to an INVMANG.cpp function
//      (not in this address range).  The ADVENT.cpp side handles only the
//      display-panel bookkeeping.
//
//  RegisterForUpdate(pfnCallback):
//      = Adv_SetUpdateCallback (0x00412570).
//      Stores pfnCallback in g_nAdvUpdateCallback.  Adv_UpdateInv calls it
//      when layer==0 to notify the GV Win32 window that item counts changed.
//
//  0x00413320 (was "Advanim_Tick" in MOVEMENT.cpp):
//      = Adv_ResetAnimSentinel.
//      Resets two sentinel globals (g_nAdvAnimSentinelMax = 0x7fffffff,
//      g_nAdvAnimSentinelMin = 0) used to bracket animation frame IDs before
//      each verb dispatch.  No animation frames are advanced.  The name
//      "Advanim_Tick" in MOVEMENT.cpp was a mis-identification.
//
//  0x004133c0 (LoadAnimByName / thunk_FUN_004133c0):
//      = Adv_LoadAnimByName.
//      Confirmed as an animation resource loader: calls Anim_CheckFreeSlot,
//      Anim_LoadToMem, then copies the name string into the slot name table.
//      This is exactly what MOVEMENT.cpp's Mov_RestoreMoves calls to load
//      directional walk animations by name.
//
//  Script VM architecture:
//      There is no traditional bytecode interpreter in this module.
//      Scripts are pre-compiled to thunk/callback handles stored in the
//      area-node table (offsets +0x15..+0x23 = 15 verb-type entries,
//      each holding a script handle) and in the item table (25 entries
//      at +0x68..+0xc4).  Adv_FindVerbHandler looks up the handle for
//      the current (node/item, verbCode) pair.  Execution is performed by
//      thunk_FUN_00462560 (from SCHED.cpp / script engine), not by code here.
//      Adv_RunScene also forwards keyboard shortcuts stored in
//      DAT_00629ef0[] → DAT_00629980[] for special F-key commands.
//
// ---- Dependencies ----------------------------------------------------------
//
// Calls out to (resolved at link time via thunk table):
//   Area_FindAt, GI_SetDrawMode, GI_ClearSeenSurf, GI_LockActiveSurf,
//   GI_LockActiveSurf_v2, Anim_AddByName, Anim_GetFrameTopLeft,
//   Anim_GetFramePosAndSize, Anim_MarkForDump, Anim_HasPendingCallback,
//   Anim_ResetPendingCallbacks, Anim_SetMainCharAnim, Anim_TickPalette,
//   Anim_CheckFreeSlot, Anim_LoadToMem, Curs_SetCursorByMode, Curs_Tick,
//   Inv_GetResource, Speech_SetTag, Speech_ResetPos,
//   Res_GetCurrentDiskNum, Files_SelectFile, Files_ReplaceExtension,
//   Files_SaveGameFull, Sched_SavePaletteSnapshot, Sched_UpdatePalette,
//   Theme_InitTimerTable, Timer_DispatchAsyncProg, Timer_HasPendingAsyncProg,
//   Timer_HasPendingSyncProg, Timer_ResetCounters, Txt_SetMode, Txt_SetString,
//   Debug_Trace, Debug_Assert, Err_SetRecord3, SafeHeap_Alloc,
//   DestroyIcon, WaitForSingleObject, WaitForMultipleObjects, SetEvent.

#include "ADVENT.h"
#include <windows.h>
#include <string.h>
#include <math.h>

// ============================================================
//  Globals
// ============================================================

int  g_nAdvVerbPending      = 0;    // 0x00629f60
int  g_nAdvVerbArea         = 0;    // 0x00629ac4
int  g_nAdvVerbType         = 0;    // 0x00629f2c
int  g_nAdvVerbParam        = 0;    // 0x00629c0c
int  g_nAdvCursorStateSaved = 0;    // 0x00629f74
int  g_nAdvDrawSuppressed   = 0;    // 0x00629f90
int  g_nAdvCleanupCount     = 0;    // 0x00629f88
int  g_nAdvUpdateCallback   = 0;    // 0x00629f40  (fn ptr stored as int)
int  g_nAdvTickSuppressed   = 0;    // 0x00629f50
int  g_nAdvCDCheckPending   = 0;    // 0x00629f78
int  g_nAdvAnimSentinelMax  = 0;    // 0x004c7ca8
int  g_nAdvAnimSentinelMin  = 0;    // 0x0062a110

// Cleanup callback table (max 10, LIFO) — 0x00629ad0
static int g_apAdvCleanupFns[10];   // 0x00629ad0

// ============================================================
//  Adv_Init  (0x0040e550)
//  Reset engine globals for a new game session.
//  Clears the g_anSpeechPlayed[0x5dc] table, inits the timer
//  table, resets walk state, and zeroes DAT_00646748.
// ============================================================
void Adv_Init(void)
{
    // body: see Ghidra 0x0040e550
    // g_nMovCarryHint = 0; DAT_00646748 = 0;
    // memset(g_anSpeechPlayed, 0, 0x5dc * 4);
    // Theme_InitTimerTable();
    // DAT_00629f4c = 0; g_nMovDone = 1; g_nMovDestNode = -1;
}

// ============================================================
//  Adv_LoadScreenshot  (0x0040e650)
//  File-dialog screenshot loader.
//  Allocates a temp buffer, opens Files_SelectFile, reads a
//  BMP via FUN_0048a340, and writes it with FUN_0048a490.
//  Also saves/restores palette snapshot via Sched_*.
// ============================================================
void Adv_LoadScreenshot(void)
{
    // body: see Ghidra 0x0040e650
}

// ============================================================
//  Adv_PushCursorState  (0x0040e960)
//  Snapshot: g_nAdvCursorStateSaved=1,
//  DAT_00629dcc=DAT_00629f70, DAT_00629dbc=DAT_00629c08,
//  DAT_00629edc=DAT_007d67b4.
// ============================================================
void Adv_PushCursorState(void)
{
    // body: see Ghidra 0x0040e960
}

// ============================================================
//  Adv_PopCursorState  (0x0040ea20)
//  If g_nAdvCursorStateSaved != 0, restore from snapshot.
// ============================================================
void Adv_PopCursorState(void)
{
    // body: see Ghidra 0x0040ea20
}

// ============================================================
//  Adv_ClearCursorState  (0x0040eae0)
//  Set g_nAdvCursorStateSaved = 0.
// ============================================================
void Adv_ClearCursorState(void)
{
    // body: see Ghidra 0x0040eae0
}

// ============================================================
//  Adv_UpdateHotspot  (0x0040eb70)
//  Debug string: "curs_and_strings(int num, char...)"
//  If *pChanged == 0: call Area_FindAt (adjusting for tablet
//  mode if cursor type is 0x09) and update *pAreaIdx.
//  Set *pChanged = 1 if area changed.
// ============================================================
void Adv_UpdateHotspot(int *pAreaIdx, char *pChanged)
{
    // body: see Ghidra 0x0040eb70
}

// ============================================================
//  Adv_PostVerb  (0x0040ecb0)
//  Store (nArea, nVerbType, nParam) into the pending-verb
//  globals and call SetEvent(DAT_006dc52c).
// ============================================================
void Adv_PostVerb(int nArea, int nVerbType, int nParam)
{
    // body: see Ghidra 0x0040ecb0
    // g_nAdvVerbPending = 1;
    // g_nAdvVerbArea = nArea;
    // g_nAdvVerbType = nVerbType;
    // g_nAdvVerbParam = nParam;
    // SetEvent(DAT_006dc52c);
}

// ============================================================
//  Adv_GetVerb  (0x0040ed70)
//  Non-destructive read of the stored verb triple.
// ============================================================
void Adv_GetVerb(int *pArea, int *pVerbType, int *pParam)
{
    // body: see Ghidra 0x0040ed70
    // *pArea = g_nAdvVerbArea; *pVerbType = g_nAdvVerbType; *pParam = g_nAdvVerbParam;
}

// ============================================================
//  Adv_TestAndClearVerb  (0x0040ee20)
//  Returns old g_nAdvVerbPending and resets it to 0.
// ============================================================
int Adv_TestAndClearVerb(void)
{
    // body: see Ghidra 0x0040ee20
    return 0;
}

// ============================================================
//  Adv_CursorHandler  (0x0040eec0)
//  Debug string: "curs_handler(int num, int key_i...)"
//  Main input event loop.  Reads right-click config from INI
//  on first call, then WaitForSingleObject/WaitForMultipleObjects
//  until mouse or keyboard event.  Decodes to verb codes 0..11
//  stored in *pVerb; *pObj = area node; *pKey = keyboard handle.
//  Handles Konami-code cheat sequence via DAT_00629f44 state machine.
// ============================================================
void Adv_CursorHandler(int *pArea, int *pVerb, int *pObj, int *pKey)
{
    // body: see Ghidra 0x0040eec0
}

// ============================================================
//  Adv_CheckRightClick  (0x0040f700)
//  Returns 1 if DAT_006dc4f8 & 2 and the cached value differs.
//  Updates DAT_007d5b8c to suppress repeat.
// ============================================================
int Adv_CheckRightClick(void)
{
    // body: see Ghidra 0x0040f700
    return 0;
}

// ============================================================
//  Adv_InitAreaSlots  (0x0040f7c0)
//  Walk g_pAreaNodeTable[0..g_nAreaNodeCount-1], dispatch on
//  node-type field [0x25]:
//    type 1 = background rect   -> GI_LockActiveSurf_v2
//    type 2 = player start node -> init coords to -1
//    type 5..0x1d = inv slot    -> insert into DAT_0070e5f0 table
//    type 0x1e = scrollable inv area -> set DAT_00629da0 bounds
//  Then compact the slot table, adjusting all type values.
// ============================================================
void Adv_InitAreaSlots(void)
{
    // body: see Ghidra 0x0040f7c0
}

// ============================================================
//  Adv_RefreshCursor  (0x0040fc40)
//  Debug string: "refresh_curs()"
//  If DAT_00629f70 > 0 and the held-item has changed:
//    - Mark old anim slot for dump, load new item anim.
//    - Compute hotspot from item's cc/ce offsets.
//    - If sprite <= 32x32, create Win32 cursor via FUN_00487770.
//  Then call Curs_SetCursorByMode(Area_FindAt(mouseX, mouseY)).
// ============================================================
void Adv_RefreshCursor(void)
{
    // body: see Ghidra 0x0040fc40
}

// ============================================================
//  Adv_FlushAnimCallbacks  (0x004100d0)
//  Call Anim_ResetPendingCallbacks then loop:
//    while Anim_HasPendingCallback: Adv_Tick + Timer_DispatchAsyncProg
// ============================================================
void Adv_FlushAnimCallbacks(void)
{
    // body: see Ghidra 0x004100d0
}

// ============================================================
//  Adv_SetCDCheck  (0x00410170)
//  Sets g_nAdvCDCheckPending = 1.
// ============================================================
void Adv_SetCDCheck(void)
{
    // body: see Ghidra 0x00410170
}

// ============================================================
//  Adv_AutoSaveRescue  (0x00410190)
//  If g_nAdvRescueSaveFlag: build path from g_abSaveGameDir +
//  "RESCUE.SAV" then call Files_SaveGameFull.  Returns true on save.
// ============================================================
bool Adv_AutoSaveRescue(void)
{
    // body: see Ghidra 0x00410190
    return false;
}

// ============================================================
//  Adv_RunScene  (0x004101f0)
//  Debug string: "run_scn(char *scr_name)"
//  Centralised scene entry.  Sequence:
//    1. Optional CD-swap dialog (g_nAdvCDCheckPending).
//    2. Init: speech tag, cursor mode, drag item, timers.
//    3. Adv_InitAreaSlots, Speech_ResetPos.
//    4. Load scene resources via thunk_FUN_00462560.
//    5. Adv_RefreshCursor, Curs_SetCursorByMode.
//    6. Input loop calling Adv_CursorHandler.
//       Decode verb via Adv_FindVerbHandler → thunk_FUN_00462560.
//    7. Drag-cursor update (held item) inline.
//    8. Return exit code: 1 normal, -5..-8 special, -1 quit.
// ============================================================
int Adv_RunScene(int nCharAnim)
{
    // body: see Ghidra 0x004101f0
    return 0;
}

// ============================================================
//  Adv_FindVerbHandler  (0x00411570)
//  If bIsItem == 0: search node table entry param_1 at offsets
//    [0x15..0x23] for verbCode; return corresponding [6..0x14].
//  If bIsItem != 0: search item table at param_1 offsets
//    [0x68..0xc4] for verbCode; return corresponding [4..0x70].
//  Returns -1 if not found.
// ============================================================
int Adv_FindVerbHandler(int nAreaOrItem, int nVerb, int bIsItem)
{
    // body: see Ghidra 0x00411570
    return -1;
}

// ============================================================
//  Adv_RectsOverlap  (0x004116b0)
//  Returns 0 if either rect is degenerate or they don't intersect.
//  Condition: !(x3<x1 || y3<y2 || x1>x4 || y1>y4)  (approx)
// ============================================================
int Adv_RectsOverlap(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
    // body: see Ghidra 0x004116b0
    return 0;
}

// ============================================================
//  Adv_RegisterCleanup  (0x00411760)
//  Debug string: "add_cleanup(void *func_name, vo...)"
//  Push pfnCleanup onto g_apAdvCleanupFns[g_nAdvCleanupCount++].
//  Asserts on overflow (max 10).
//  Public alias: RegisterForUpdate(pfnCallback) from Graninv.cpp.
// ============================================================
void Adv_RegisterCleanup(int pfnCleanup)
{
    // body: see Ghidra 0x00411760
}

// ============================================================
//  Adv_UnregisterCleanup  (0x00411870)
//  Find pfnCleanup in the stack, shift remaining entries down,
//  decrement count.
// ============================================================
void Adv_UnregisterCleanup(int pfnCleanup)
{
    // body: see Ghidra 0x00411870
}

// ============================================================
//  Adv_RunCleanups  (0x00411980)
//  LIFO: while count > 0: count--, call g_apAdvCleanupFns[count]().
// ============================================================
void Adv_RunCleanups(void)
{
    // body: see Ghidra 0x00411980
}

// ============================================================
//  Adv_ClampInvScroll  (0x00411a30)
//  Ensure layer-0 scroll offset (DAT_0070e654) doesn't exceed
//  total - visible count.  If total < visible, set offset = 0.
// ============================================================
void Adv_ClampInvScroll(int nLayer)
{
    // body: see Ghidra 0x00411a30
}

// ============================================================
//  Adv_AddInvItem  (0x00411b40)
//  Append nNodeIdx to the layer-0 item-slot array.
//  If there's a gap (-1) reuse it, else grow array and clamp scroll.
// ============================================================
void Adv_AddInvItem(int nNodeIdx)
{
    // body: see Ghidra 0x00411b40
}

// ============================================================
//  Adv_RemoveInvItem  (0x00411c80)
//  Find nNodeIdx in layer-0 array and set that entry to -1.
// ============================================================
void Adv_RemoveInvItem(int nNodeIdx)
{
    // body: see Ghidra 0x00411c80
}

// ============================================================
//  Adv_ClearInvLayer  (0x00411d70)
//  memset slot array to -1, zero counts, scroll, flags.
// ============================================================
void Adv_ClearInvLayer(int nLayer)
{
    // body: see Ghidra 0x00411d70
}

// ============================================================
//  Adv_SetInvSlot  (0x00411eb0)
//  Locate area-node nNodeId in g_pAreaNodeTable by field [5].
//  Store its index in slot (nLayer, nSlotIdx).
//  Grow the visible count if needed.
//  If layer's fixed-flag is clear, set node type to 0x28.
// ============================================================
void Adv_SetInvSlot(int nLayer, int nSlotIdx, int nNodeId)
{
    // body: see Ghidra 0x00411eb0
}

// ============================================================
//  Adv_SetInvSlotDirect  (0x00412030)
//  Write nNodeId directly to DAT_0070e458[nLayer][nSlotIdx].
// ============================================================
void Adv_SetInvSlotDirect(int nLayer, int nSlotIdx, int nNodeId)
{
    // body: see Ghidra 0x00412030
}

// ============================================================
//  Adv_FillFreeInvSlot  (0x004120d0)
//  Find first slot in layer nLayer with value < 0 and write nNodeId.
// ============================================================
void Adv_FillFreeInvSlot(int nLayer, int nNodeId)
{
    // body: see Ghidra 0x004120d0
}

// ============================================================
//  Adv_ClearInvSlot  (0x004121c0)
//  Set DAT_0070e458[nLayer][nSlotIdx] = -1.
// ============================================================
void Adv_ClearInvSlot(int nLayer, int nSlotIdx)
{
    // body: see Ghidra 0x004121c0
}

// ============================================================
//  Adv_RemoveInvSlotByNode  (0x00412260)
//  Walk all slots of layer nLayer; set to -1 wherever == nNodeId.
// ============================================================
void Adv_RemoveInvSlotByNode(int nLayer, int nNodeId)
{
    // body: see Ghidra 0x00412260
}

// ============================================================
//  Adv_IsNodeInSpecialSlot  (0x00412350)
//  Check 10-entry special-slot array for layer nLayer.
//  Returns 1 if nNodeId found; 0 if not or nNodeId == -1.
// ============================================================
int Adv_IsNodeInSpecialSlot(int nLayer, int nNodeId)
{
    // body: see Ghidra 0x00412350
    return 0;
}

// ============================================================
//  Adv_CompactInvList  (0x00412430)
//  Shift all non-(-1) entries down, shrink count, clamp scroll.
//  Sets DAT_004c7394 = 1 (dirty flag for GV update).
// ============================================================
void Adv_CompactInvList(int nLayer)
{
    // body: see Ghidra 0x00412430
}

// ============================================================
//  Adv_SetUpdateCallback  (0x00412570)
//  Store pfnCallback in g_nAdvUpdateCallback.
//  Called as RegisterForUpdate(pfnCallback) from Graninv.cpp.
// ============================================================
void Adv_SetUpdateCallback(int pfnCallback)
{
    // body: see Ghidra 0x00412570
    // g_nAdvUpdateCallback = pfnCallback;
}

// ============================================================
//  Adv_ScrollInv  (0x00412600)
//  Compact layer nLayer, adjust scroll offset by nDelta,
//  clamp to 0, then compact again and tick N frames via
//  Adv_TickFramesNoAsync.
// ============================================================
void Adv_ScrollInv(int nDelta, int nLayer)
{
    // body: see Ghidra 0x00412600
}

// ============================================================
//  Adv_UpdateInv  (0x00412710)
//  Debug string: "update_inv(int set_num)"
//  If nLayer == 0: call g_nAdvUpdateCallback(DAT_004c7394).
//    If callback returns non-zero, clear the dirty flag.
//  Else: iterate visible slots, call Inv_GetResource for each
//    item, blit via GI_LockActiveSurf into the slot's node rect.
//    Skip the currently-dragged slot when in drag-cursor mode.
// ============================================================
void Adv_UpdateInv(int nLayer)
{
    // body: see Ghidra 0x00412710
}

// ============================================================
//  Adv_DrawAllInvLayers  (0x004129e0)
//  If g_nAdvDrawSuppressed == 0: call Adv_UpdateInv for each
//  layer 0..g_nAdvLayerCount-1.
// ============================================================
void Adv_DrawAllInvLayers(void)
{
    // body: see Ghidra 0x004129e0
}

// ============================================================
//  Adv_SetDrawSuppressed  (0x00412ab0)
//  g_nAdvDrawSuppressed = param_1.
// ============================================================
void Adv_SetDrawSuppressed(int bSuppressed)
{
    // body: see Ghidra 0x00412ab0
}

// ============================================================
//  Adv_Tick  (0x00412ac0)
//  Single display-frame pump.  If not suppressed:
//    - Flush Win32 messages (thunk_FUN_00403670).
//    - If no frame pending: flush + thunk_FUN_00403250.
//    - If frame pending and speech thread is waiting:
//        WaitForSingleObject/SetEvent handshake, then
//        flush + thunk_FUN_00403540 (render one frame).
// ============================================================
void Adv_Tick(void)
{
    // body: see Ghidra 0x00412ac0
}

// ============================================================
//  Adv_TickFrames  (0x00412be0)
//  Pump exactly nFrames complete frames with async dispatch.
// ============================================================
void Adv_TickFrames(int nFrames)
{
    // body: see Ghidra 0x00412be0
}

// ============================================================
//  Adv_TickFramesNoAsync  (0x00412cc0)
//  Pump exactly nFrames complete frames, no async dispatch.
// ============================================================
void Adv_TickFramesNoAsync(int nFrames)
{
    // body: see Ghidra 0x00412cc0
}

// ============================================================
//  Adv_WaitForMouse  (0x00412da0)
//  Debug string: "wait_for_mouse()"
//  Loop: Adv_Tick + Timer_DispatchAsyncProg + flush msgs +
//  WaitForSingleObject/WaitForMultipleObjects until mouse event.
// ============================================================
void Adv_WaitForMouse(void)
{
    // body: see Ghidra 0x00412da0
}

// ============================================================
//  Adv_WaitForMouseNoAsync  (0x00412ed0)
//  Debug string: "wait_for_mouse_no_async()"
//  Same as Adv_WaitForMouse but no Timer_DispatchAsyncProg.
// ============================================================
void Adv_WaitForMouseNoAsync(void)
{
    // body: see Ghidra 0x00412ed0
}

// ============================================================
//  Adv_WaitForFrameOrMouse  (0x00413000)
//  Debug string: "wait_for_frame_or_mouse()"
//  Returns: 1 = mouse event, 2 = frame event, 3 = both.
//  Dispatches async programs on frame events.
// ============================================================
int Adv_WaitForFrameOrMouse(void)
{
    // body: see Ghidra 0x00413000
    return 0;
}

// ============================================================
//  Adv_WaitForFrameOrMouseNoAsync  (0x00413190)
//  Debug string: "wait_for_frame_or_mouse_no_async()"
//  Same as above but no async dispatch.
// ============================================================
int Adv_WaitForFrameOrMouseNoAsync(void)
{
    // body: see Ghidra 0x00413190
    return 0;
}

// ============================================================
//  Adv_ResetAnimSentinel  (0x00413320)
//  Called from MOVEMENT.cpp as "Advanim_Tick" — mis-identification.
//  Actual purpose: reset sentinel globals used to bracket
//  animation frame comparisons before each verb dispatch:
//    g_nAdvAnimSentinelMax = 0x7fffffff
//    g_nAdvAnimSentinelMin = 0
//  No animation frames are advanced.
// ============================================================
void Adv_ResetAnimSentinel(void)
{
    // body: see Ghidra 0x00413320
    // g_nAdvAnimSentinelMax = 0x7fffffff;
    // g_nAdvAnimSentinelMin = 0;
}

// ============================================================
//  Adv_LoadAnimByName  (0x004133c0)
//  Debug string: "add_a32(char *name, int *ani_reg)"
//  1. Anim_CheckFreeSlot() -> *pHandle (assert on -1)
//  2. Anim_LoadToMem(pszName, 0xb, slot, 0xffffffff)
//  3. Copy pszName -> g_abAnimSlotNames[slot * 0x14]
//  This is what MOVEMENT.cpp calls as LoadAnimByName.
// ============================================================
void Adv_LoadAnimByName(const char *pszName, int *pHandle)
{
    // body: see Ghidra 0x004133c0
}
