#pragma once
// ---------------------------------------------------------------------------
// ADVENT.h  --  Adventure engine core: scene loop, verb dispatch, inventory
//               display, cursor refresh, and frame-sync primitives.
//
// Original: C:\DevStudio\Projects\Crux\ADVENT.cpp
// Address range: 0x0040e550 -- 0x004133c0  (47 functions)
//
// Architecture overview:
//
//   Scene loop (Adv_RunScene)
//   -------------------------------------------------
//   Called once per room entry.  Initialises cursor state, resets the area
//   node tables, then loops:
//     1. Calls Adv_Tick (one display-driver frame pump) and Curs_Tick.
//     2. Calls Adv_CursorHandler to collect the next mouse/keyboard event.
//     3. Dispatches the result via Adv_FindVerbHandler → script execution.
//     4. Returns a signed exit code:
//          1       next room (normal walk)
//          -5..-8  special transition codes
//          -1      quit
//
//   Verb pipeline
//   -------------------------------------------------
//   Mouse/keyboard events are normalised into (area, verbType, param) tuples.
//   Adv_CursorHandler loops on WaitForSingleObject / WaitForMultipleObjects,
//   calling Adv_UpdateHotspot each iteration to track cursor position, then
//   decodes button state into verb codes 0..11:
//     0 = left-click / look          6  = double-right-click (take)
//     1 = right-click (use/take)     7  = quit hotkey
//     2 = middle-click               8  = script callback
//     4 = F4 quit                    9  = scroll right-click
//    10 = scroll left-click         11  = keyboard shortcut
//   Adv_PostVerb stores a pending verb; Adv_TestAndClearVerb polls and clears.
//   Adv_FindVerbHandler looks up the matching script handle from the node or
//   item table (15 slots per node, 25 per item).
//
//   Inventory display layers
//   -------------------------------------------------
//   The engine supports multiple inventory display layers (0 = main).
//   Each layer holds an ordered list of area-node indices (g_aInvSlots[]).
//   Adv_DrawAllInvLayers iterates all layers and blits item icons via
//   Inv_GetResource / GI_LockActiveSurf.
//   Adv_AddInvItem / Adv_RemoveInvItem manage the flat layer-0 list.
//   Adv_SetInvSlot / Adv_ClearInvSlot / Adv_FillFreeInvSlot manage slots
//   by (layer, slotIndex) position.  Adv_CompactInvList removes gaps.
//   Adv_ScrollInv shifts the visible window (scroll offset) for a layer.
//
//   Cursor refresh (Adv_RefreshCursor)
//   -------------------------------------------------
//   Called once per scene tick.  If an item is being dragged (g_nAdvDragItem),
//   creates or updates a Win32 cursor from the item's animation sprite and
//   sets cursor mode 1.  Then calls Curs_SetCursorByMode to apply the result.
//
//   Cleanup hooks (Adv_RegisterCleanup / Adv_RunCleanups)
//   -------------------------------------------------
//   A LIFO stack (max 10) of void(*)(void) function pointers.  Adv_RunScene
//   calls Adv_RunCleanups on exit to tear down sub-systems registered during
//   the scene (timer resets, animation state, etc.).
//   Also exposed as the public RegisterForUpdate / UnregisterForUpdate API
//   used by Graninv.cpp.
//
//   Frame/mouse wait primitives
//   -------------------------------------------------
//   Four helpers sit above WaitForSingleObject/WaitForMultipleObjects:
//     Adv_WaitForMouse            -- loop until mouse event, dispatch async
//     Adv_WaitForMouseNoAsync     -- same, no async dispatch
//     Adv_WaitForFrameOrMouse     -- returns 1=mouse 2=frame 3=both
//     Adv_WaitForFrameOrMouseNoAsync -- same, no async dispatch
//   Adv_Tick is the single-frame pump; Adv_TickFrames / Adv_TickFramesNoAsync
//   advance N complete display frames.
//
//   Animation sentinel (Adv_ResetAnimSentinel)
//   -------------------------------------------------
//   Resets g_nAdvAnimSentinelMax = 0x7fffffff and g_nAdvAnimSentinelMin = 0
//   before each verb dispatch to prevent stale animation comparisons.
//   MOVEMENT.cpp exposed this as "Advanim_Tick" but it is purely a sentinel
//   reset with no animation side-effects.
//
//   Animation loader (Adv_LoadAnimByName)
//   -------------------------------------------------
//   Allocates a free anim slot, calls Anim_LoadToMem, then copies the name
//   into g_abAnimSlotNames[slot*0x14].  This is the function MOVEMENT.cpp
//   calls as LoadAnimByName(pszName, &handle).
// ---------------------------------------------------------------------------

// ---- Public API ------------------------------------------------------------

// Initialise engine globals: speech-played table, carry hint, timer table.
void Adv_Init(void);

// Load a screenshot from disk (file dialog), compress and save with palette.
void Adv_LoadScreenshot(void);

// Save current cursor/item-drag state to a one-slot snapshot.
void Adv_PushCursorState(void);

// Restore cursor/item-drag state from the snapshot (only if saved).
void Adv_PopCursorState(void);

// Discard the cursor state snapshot.
void Adv_ClearCursorState(void);

// Update *pAreaIdx if the cursor has moved to a different area node.
// Sets *pChanged to 1 on change. Handles tablet (cursor type 0x09) coords.
void Adv_UpdateHotspot(int *pAreaIdx, char *pChanged);

// Post a pending verb event: area, verbType, extra param.  Sets an event.
void Adv_PostVerb(int nArea, int nVerbType, int nParam);

// Retrieve the most-recently posted verb triple.
void Adv_GetVerb(int *pArea, int *pVerbType, int *pParam);

// Consume the pending-verb flag; returns old value and clears it.
int  Adv_TestAndClearVerb(void);

// Main input loop: waits for mouse/keyboard, decodes to (area, verb, obj).
// param_2 receives verb code on return; param_3 receives area; param_4 kbd id.
void Adv_CursorHandler(int *pArea, int *pVerb, int *pObj, int *pKey);

// Returns 1 if the right mouse button was just freshly pressed.
int  Adv_CheckRightClick(void);

// Parse g_pAreaNodeTable and build the area-slot display structures.
void Adv_InitAreaSlots(void);

// Refresh the drag-cursor sprite from the currently held item.
void Adv_RefreshCursor(void);

// Drain all pending Anim callbacks (run until Anim_HasPendingCallback == 0).
void Adv_FlushAnimCallbacks(void);

// Signal that a CD-disc-swap check is needed at next scene entry.
void Adv_SetCDCheck(void);

// If g_nAdvRescueSaveFlag is set, write RESCUE.SAV and return true.
bool Adv_AutoSaveRescue(void);

// Main scene loop. param_1 = main-character anim handle.
// Returns scene exit code (1 = normal walk, negative = special).
int  Adv_RunScene(int nCharAnim);

// Look up the script handle for (areaOrItem, verbCode, bIsItem).
// Returns -1 if not found.
int  Adv_FindVerbHandler(int nAreaOrItem, int nVerb, int bIsItem);

// Returns 1 if [x1,y1..x2,y2] and [x3,y3..x4,y4] overlap.
int  Adv_RectsOverlap(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

// Push a void(*)(void) cleanup function onto the shutdown stack (max 10).
// This is the public RegisterForUpdate(pfnCallback) entry point.
void Adv_RegisterCleanup(int pfnCleanup);

// Remove a specific cleanup function from the stack.
// This is the public UnregisterForUpdate(pfnCallback) entry point.
void Adv_UnregisterCleanup(int pfnCleanup);

// Run all registered cleanups LIFO and reset the stack.
void Adv_RunCleanups(void);

// Clamp the scroll offset for inventory layer param_1.
void Adv_ClampInvScroll(int nLayer);

// Append an item (area node index) to the flat layer-0 inventory list.
void Adv_AddInvItem(int nNodeIdx);

// Remove an item by area node index from the flat layer-0 list.
void Adv_RemoveInvItem(int nNodeIdx);

// Clear all slots, counts, and scroll state for one inventory layer.
void Adv_ClearInvLayer(int nLayer);

// Assign an area node to display slot (nLayer, nSlotIdx, nNodeId).
// Marks the node type as 0x28 (in-panel) if the layer's fixed flag is clear.
void Adv_SetInvSlot(int nLayer, int nSlotIdx, int nNodeId);

// Low-level: write nNodeId directly into slot (nLayer, nSlotIdx).
void Adv_SetInvSlotDirect(int nLayer, int nSlotIdx, int nNodeId);

// Write nNodeId into the first empty (-1) slot of layer nLayer.
void Adv_FillFreeInvSlot(int nLayer, int nNodeId);

// Set slot (nLayer, nSlotIdx) to -1.
void Adv_ClearInvSlot(int nLayer, int nSlotIdx);

// Clear all slots in layer nLayer that contain nNodeId.
void Adv_RemoveInvSlotByNode(int nLayer, int nNodeId);

// Return 1 if nNodeId appears in the special-slot array for layer nLayer.
int  Adv_IsNodeInSpecialSlot(int nLayer, int nNodeId);

// Remove gaps from layer nLayer's slot array and update scroll clamp.
void Adv_CompactInvList(int nLayer);

// Store pfnCallback as the inventory-update notification function.
// This is the public RegisterForUpdate(pfnCallback) companion for GV.
void Adv_SetUpdateCallback(int pfnCallback);

// Scroll layer nLayer by nDelta positions and compact.
void Adv_ScrollInv(int nDelta, int nLayer);

// Redraw inventory layer nLayer (0 = call update callback, else blit icons).
void Adv_UpdateInv(int nLayer);

// Call Adv_UpdateInv for all active layers (skipped if g_nAdvDrawSuppressed).
void Adv_DrawAllInvLayers(void);

// Set/clear the global draw-suppressed flag (non-zero = suppress inv draws).
void Adv_SetDrawSuppressed(int bSuppressed);

// Pump one display frame: flush Win32 messages, advance scene, render.
void Adv_Tick(void);

// Pump exactly nFrames complete display frames, dispatching async programs.
void Adv_TickFrames(int nFrames);

// Pump nFrames frames without dispatching async programs.
void Adv_TickFramesNoAsync(int nFrames);

// Block until a mouse event arrives, pumping async programs each iteration.
void Adv_WaitForMouse(void);

// Block until a mouse event arrives (no async program dispatch).
void Adv_WaitForMouseNoAsync(void);

// Block until a mouse event OR a full display frame; returns 1=mouse 2=frame 3=both.
int  Adv_WaitForFrameOrMouse(void);

// Block until a mouse event OR a full display frame (no async dispatch).
int  Adv_WaitForFrameOrMouseNoAsync(void);

// Reset animation sentinel globals (max=0x7fffffff, min=0).
// Called from MOVEMENT.cpp before each verb dispatch (was mislabelled Advanim_Tick).
void Adv_ResetAnimSentinel(void);

// Load animation resource pszName into a free slot; write slot index to *pHandle.
// This is what MOVEMENT.cpp calls as LoadAnimByName.
void Adv_LoadAnimByName(const char *pszName, int *pHandle);

// ---- Globals ---------------------------------------------------------------

extern int  g_nAdvVerbPending;      // 0x00629f60  non-zero = verb event queued
extern int  g_nAdvVerbArea;         // 0x00629ac4  area/node for pending verb
extern int  g_nAdvVerbType;         // 0x00629f2c  verb type code (0=look 1=use ...)
extern int  g_nAdvVerbParam;        // 0x00629c0c  extra verb parameter
extern int  g_nAdvCursorStateSaved; // 0x00629f74  push/pop cursor state valid flag
extern int  g_nAdvDrawSuppressed;   // 0x00629f90  non-zero = skip all inv draws
extern int  g_nAdvCleanupCount;     // 0x00629f88  number of registered cleanup fns
extern int  g_nAdvUpdateCallback;   // 0x00629f40  GV update callback fn pointer (int)
extern int  g_nAdvTickSuppressed;   // 0x00629f50  non-zero during scene load
extern int  g_nAdvCDCheckPending;   // 0x00629f78  set by Adv_SetCDCheck
extern int  g_nAdvAnimSentinelMax;  // 0x004c7ca8  reset to 0x7fffffff by Adv_ResetAnimSentinel
extern int  g_nAdvAnimSentinelMin;  // 0x0062a110  reset to 0 by Adv_ResetAnimSentinel
