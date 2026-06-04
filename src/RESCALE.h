// ---------------------------------------------------------------------------
// RESCALE.h  —  Sprite / image rescaling (zoom-table playback)
// Original: C:\DevStudio\Projects\Crux\RESCALE.cpp
// RE offsets: 0x00461c10 – 0x00461f??  (real RESCALE.cpp tail)
//
// NOTE: READRES.cpp embeds a SEPARATE copy of the Rescale_* family
//       (Rescale_CalcZoomTable / Rescale_CalcForRoom / Rescale_DrawScaledSprite,
//        0x004612a0 – 0x00461b30).  The two functions below are the genuine
//       RESCALE.cpp translation unit (their error strings reference
//       "C:\DevStudio\Projects\Crux\RESCALE.cpp").  The zoom-table machinery
//       (g_nRescaleTable / g_nRescaleCount / g_nRescaleIdx) is shared and is
//       declared in READRES.h.
// ---------------------------------------------------------------------------
#pragma once

// --- Globals (RESCALE.cpp) ---

// 0x007c3fd4  Current room / walk-node index.  Selects a row in
//   g_anAnimFrameTablePrev (stride 400); row[+1] yields the sprite-group id
//   used by Rescale_DrawBikeScroll.
extern int g_nRescaleRoomIdx;

// 0x004d77f8  Bike scroll Y accumulator.  Advanced by 0xd each frame; wraps
//   back to 0x1fa once it exceeds 0x27f.
extern int g_nRescaleBikeScrollY;

// ---------------------------------------------------------------------------
// 0x00461c10  Rescale_DrawBikeScroll
//   Advances the bike scroll position, rebuilds the room zoom table via
//   Rescale_CalcForRoom(0x32, 0x32), then blits every entry of the zoom table
//   (g_nRescaleTable triples) with GI_LockActiveSurf_v7.
// ---------------------------------------------------------------------------
void Rescale_DrawBikeScroll(void);

// ---------------------------------------------------------------------------
// 0x00461d60  Rescale_DrawByIndexChecked
//   Bounds-checks the zoom-table width at nIdx (must be 0..0x280, else
//   Err_BadResEntry) and draws that scaled sprite with GI_LockActiveSurf_v7.
//     nDest   - destination surface / context
//     nY      - destination Y (raster line)
//     nSprite - sprite / source bitmap id
// ---------------------------------------------------------------------------
void Rescale_DrawByIndexChecked(int nDest, int nY, int nIdx, int nSprite);
