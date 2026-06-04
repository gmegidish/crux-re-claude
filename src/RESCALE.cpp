// ---------------------------------------------------------------------------
// RESCALE.cpp  —  Sprite / image rescaling (zoom-table playback)
// Original: C:\DevStudio\Projects\Crux\RESCALE.cpp
// RE offsets: 0x00461c10 – 0x00461f??
//
// The zoom table itself (g_nRescaleTable / g_nRescaleCount / g_nRescaleIdx)
// and the Calc/Draw helpers live in the embedded copy inside READRES.cpp
// (0x004612a0 – 0x00461b30).  The two functions in this file are the genuine
// RESCALE.cpp translation unit: they consume that table to actually blit the
// scaled sprites and they carry the "...\Crux\RESCALE.cpp" error strings.
//
// The zoom table is an array of {int width, int x, int y} triples:
//     g_nRescaleTable[i*3 + 0] = width   (also &DAT_007c3b20 + i*0x0c)
//     g_nRescaleTable[i*3 + 1] = x        (    &DAT_007c3b24 + i*0x0c)
//     g_nRescaleTable[i*3 + 2] = y        (    &DAT_007c3b28 + i*0x0c)
// ---------------------------------------------------------------------------
#include "RESCALE.h"
#include "READRES.h"     // g_nRescaleTable, g_nRescaleCount, g_nRescaleIdx, Rescale_CalcForRoom
#include "Advanim.h"     // g_anAnimFrameTablePrev, g_anGroupTriggerPct
#include "GI.h"          // GI_LockActiveSurf_v7
#include "ERRORS.h"      // Err_BadResEntry
#include "SAFEHEAP.h"

// --- Globals ---
int g_nRescaleRoomIdx     = 0;   // 0x007c3fd4
int g_nRescaleBikeScrollY = 0;   // 0x004d77f8

// ---------------------------------------------------------------------------
// 0x00461c10  Rescale_DrawBikeScroll
//
// One frame of the "bike riding" rescale effect.  Looks up the sprite group
// for the current room, advances the vertical scroll position, rebuilds the
// zoom table for the room, then blits every level of the zoom table.
// ---------------------------------------------------------------------------
void Rescale_DrawBikeScroll(void)
{
    // Sprite-group id for the current room (row stride is 400 ints, field +1).
    int nGroup = g_anAnimFrameTablePrev[g_nRescaleRoomIdx * 400 + 1];

    // Scroll the scenery upward; wrap when it runs past the bottom.
    g_nRescaleBikeScrollY += 0xd;
    if (g_nRescaleBikeScrollY > 0x27f)
        g_nRescaleBikeScrollY = 0x1fa;

    // Rebuild the per-room zoom table (top/bottom extra = 0x32 each).
    Rescale_CalcForRoom(0x32, 0x32);

    // Draw the sprite at every zoom level currently in the table.
    for (int i = 0; i <= g_nRescaleIdx; i++)
    {
        GI_LockActiveSurf_v7(
            0,                                   // active surface
            0x1ae,                               // destination Y
            g_anGroupTriggerPct[nGroup * 8 + 10],// sprite id from the group row
            (&g_nRescaleTable)[i * 3 + 1],       // src x   (DAT_007c3b24 + i*0xc)
            (&g_nRescaleTable)[i * 3 + 2],       // src y   (DAT_007c3b28 + i*0xc)
            (&g_nRescaleTable)[i * 3 + 0]);      // src width
    }
}

// ---------------------------------------------------------------------------
// 0x00461d60  Rescale_DrawByIndexChecked
//
// Blit a single scaled sprite using the pre-computed zoom-table entry nIdx,
// validating the entry's width against the legal range [0 .. 0x280] first.
// ---------------------------------------------------------------------------
void Rescale_DrawByIndexChecked(int nDest, int nY, int nIdx, int nSprite)
{
    int nWidth = (&g_nRescaleTable)[nIdx * 3 + 0];

    if (nWidth > 0x280)
        Err_BadResEntry(__LINE__, "C:\\DevStudio\\Projects\\Crux\\RESCALE.cpp");
    if (nWidth < 0)
        Err_BadResEntry(__LINE__, "C:\\DevStudio\\Projects\\Crux\\RESCALE.cpp");

    GI_LockActiveSurf_v7(
        nDest,
        nY,
        nSprite,
        (&g_nRescaleTable)[nIdx * 3 + 1],   // src x
        (&g_nRescaleTable)[nIdx * 3 + 2],   // src y
        nWidth);                            // src width
}

// ===========================================================================
// 0x00461ea0  Runprog_LoadEntryNames     [logically RUNPROG.cpp]
//
// This function sits in the RESCALE.cpp address range but its error strings
// reference "C:\DevStudio\Projects\Crux\RUNPROG.cpp" and the source banner
// "rp_read_names" — it belongs to RUNPROG, not RESCALE.  Reverse-engineered
// here for completeness because it physically lives at 0x00461ea0; the real
// declaration belongs in RUNPROG.h (left untouched by this task).
//
//   - Frees any previously-loaded entry-name blocks.
//   - Opens the "entry" resource via Res_FindByNumChar(0x16, ...).
//   - For each on-the-fly node, reads a length-prefixed string, allocates it
//     with SafeHeap_Alloc, NUL-terminates it, and stores the pointer in
//     g_pRunprogEntryNames[].
//
// Globals: g_pRunprogEntryNames (0x007c4420), g_nRunprogEntryNameCount (0x007c49b4)
// ===========================================================================
//
// Sketch (kept as a comment so RESCALE.cpp stays the RESCALE translation unit):
//
//   void Runprog_LoadEntryNames(void)
//   {
//       for (int i = 0; i < g_nRunprogEntryNameCount; i++)
//           if (g_pRunprogEntryNames[i]) {
//               SafeHeap_Free(__FILE__, __LINE__, g_pRunprogEntryNames[i]);
//               g_pRunprogEntryNames[i] = NULL;
//           }
//       g_nRunprogEntryNameCount = 0;
//
//       if (Res_FindByNumChar(0x16, "entry", buf, 0, &handle, &len) == 0) {
//           g_nRunprogEntryNameCount = g_nOtfNodeListCount;
//           for (int i = 0; i < g_nRunprogEntryNameCount; i++) {
//               int n;
//               if (Res_BunchFreadNow((int)&n, 1, 4, &handle) != 0) Err_SetRecord3(...);
//               if (n != 0) {
//                   g_pRunprogEntryNames[i] = (char*)SafeHeap_Alloc(__FILE__, __LINE__, n + 1);
//                   if (!g_pRunprogEntryNames[i]) Err_SetRecord3(...);
//                   if (Res_BunchFreadNow((int)g_pRunprogEntryNames[i], 1, n, &handle) != 0)
//                       Err_SetRecord3(...);
//                   g_pRunprogEntryNames[i][n] = '\0';
//               }
//           }
//       }
//   }
