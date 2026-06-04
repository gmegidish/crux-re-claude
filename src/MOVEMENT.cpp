// MOVEMENT.cpp — Character walk animation manager
//
// Manages 8-directional sprite walk animations, turn-transition sequences,
// linear node-to-node position interpolation, follower NPCs, and carry-anim
// selection for the player character (and any one follower).
//
// Architecture overview:
//   - Characters have 8 directional walk anims + an 8×8 turn-transition matrix.
//   - A walk path is a small array of pre-computed room-node indices.
//   - Each node has (x, y, z, area) world-screen coordinates.
//   - Each frame Mov_Update() linearly interpolates position between nodes,
//     selects the right directional anim via atan2, and fires turn sequences
//     when the direction changes.
//   - Carry anims overlay a separate set of directional walk sprites when the
//     character is carrying an item (g_nMovCarryHint 1-8).
//
// Original source: C:\DevStudio\Projects\Crux\MOVEMENT.cpp

#include "MOVEMENT.h"
#include <windows.h>
#include <string.h>
#include <math.h>

// ============================================================
//  Globals
// ============================================================

int  g_nMovDestNode      = -1;  // 006dd5d8
int  g_nMovPathSteps     = 0;   // 006dc6fc
int  g_nMovTurnSteps     = 0;   // 006e86d0
int  g_nMovTurning       = 0;   // 006e86e0
int  g_nMovForcedDir     = -1;  // 004d5280
int  g_nMovCarryHint     = 0;   // 00629dc4
int  g_nMovAnimStep      = 0;   // 006dc6e0
int  g_nMovAnimDir       = 0;   // 006dc6e4
int  g_nMovAnimFrames    = 0;   // 007d66ac
int  g_anMovDirAnim[8]   = {-1,-1,-1,-1,-1,-1,-1,-1};  // 006dd5e0
int  g_anMovCarryAnim[8] = {-1,-1,-1,-1,-1,-1,-1,-1};  // 006dd550
int  g_anMovPath[101]    = {-1,-1,-1,-1,-1,-1,-1,-1};  // 006dc550  graph-search route buffer
int  g_nMovInterpStep    = 0;   // 006dc708
int  g_nMovInterpTotal   = 0;   // 006dd614
int  g_nMovDeltaX        = 0;   // 006dd5d0
int  g_nMovDeltaY        = 0;   // 006dd5d4
int  g_nMovCurZ          = 0;   // 006dd5c4
int  g_nMovDone          = 0;   // 006dd600
int  g_nMovFollower      = -1;  // 004d5268
int  g_nMovSoundsEnabled = 0;   // 006e86dc
int  g_nMovCarrySlot     = 0;   // 004d527c
int  g_nMovCurDir        = 0;   // 004d5284
int  g_anMovTransAnim[64];      // 006dd618  [from_dir * 8 + to_dir]
int  g_anMovDirNode[8];         // 006dc720
int  g_anMovTurnSeq[8];         // 006dea00
int  g_nMovPathHead      = -1;  // 006dd5dc
int  g_nMovSavedNode     = -1;  // 006dd570
int  g_nMovIdleDir       = 0;   // 006dd5e8
int  g_nMovDefaultStand  = -1;  // 006dd558
int  g_nMovInitialized   = 0;   // 006e86a0
int  g_nMovZOffset       = 0;   // 006e86d4
int  g_nMovDirTier       = 0;   // 006e86d8
int  g_anMovDirOrder[8];        // 006dd5a0
int  g_nMovPathSrc       = 0;   // 006dd5c0
char g_abMovVisited[8];         // 006dc700
char g_abMovPathBuf[8];         // 006dd5c8
int  g_nMovPathLen       = 0;   // 006dd598
int  g_anMovWaypointAnims[8]  = {-1,-1,-1,-1,-1,-1,-1,-1};  // 006de9e0
int  g_anMovSecondaryAnims[8] = {-1,-1,-1,-1,-1,-1,-1,-1};  // 006dd578

// Per-node walk table: each entry is 0x10 bytes — {int x, int y, int z, int area}
// The pointer lives at 0x004d525c (owned by AREAS.cpp / game-init code)
extern int* g_pMovNodes;   // 004d525c

// Character walk-sequence tables (owner: Advanim.cpp or ADVENT.cpp)
// Each character has a 0x58-byte record; offsets +0 = frame step, +0x44 = done flag
// g_pCharWalkTable + charIdx * 0x58
extern char* g_pCharWalkTable;   // 005b10d0

// Animation frame index table for directional sprites (owner: Advanim.cpp)
// Indexed as [dir * 0x640/sizeof(int) + step]; value is offset into sprite sheet
extern int g_anAnimFrameTable[];  // 004e3b58

// Sprite/image base used to compute frame pointers from the frame index table
extern char g_pSpriteBase[];      // 0051e4f0

// Per-character animation-frame counts per direction (owner: Advanim.cpp)
extern int g_anAnimFrameCount[];  // 00574990  [dir] = frame count for that direction

// carry-anim slot table by hint index (9 entries: index 0-8)
extern int g_anMovCarryByHint[9]; // 006dd54c

// 8-direction char codes for resource name suffix building (owner: Mov_RestoreMoves data)
extern char g_achDirCode[8];  // 004d55b8..004d55d4  one char per direction

// ============================================================
//  External function stubs (resolved as dependent modules are reversed)
// ============================================================

// SPEECH.cpp — returns the active speech sentence handle (non-zero = sentence playing)
// NOTE: previously mislabelled Advanim_HasOverride; thunk_FUN_00474d80 → Speech_GetSentence
extern int  Speech_GetSentence(void);
// SOUNDMEM.cpp — returns current lipsync phoneme byte (-1 = not speaking)
// NOTE: thunk_FUN_004746a0 → SndMem_GetLipsyncByte, not Advanim_GetOverride
extern int  SndMem_GetLipsyncByte(void);
// Advanim.cpp — advances one frame for the active character animation
extern void Advanim_Tick(void);
// Advanim.cpp — sets on-screen character position + active animation sprites
extern void Advanim_SetPos(int nX, int nY, int nZ, int nAnimHi, int nAnimLo);
// SCHED.cpp or Advanim.cpp — fires the "arrived at node" event/callback
extern void Mov_ArrivalCallback(void);
// MIXER.cpp — plays the footstep sound associated with the given node
// SETPAL.cpp — centred sound play; thunk_FUN_0046f7f0 = Snd_PlayCentered
extern void Snd_PlayCentered(int nHandle, int nFrame);
// READRES.cpp — loads an animation resource by name, writes handle to *pHandle
extern int  LoadAnimByName(const char* pszName, int* pnHandle);
// READRES.cpp — releases an animation handle
extern void FreeAnimHandle(int nHandle, int bForce);
// READRES.cpp — builds a string from a format literal and an integer
extern void BuildString(char* pszOut, const void* pFmt);
extern void AppendString(char* pszOut, const void* pFmt);
// Debug logging (ERRORS.cpp)
extern void Debug_TraceVal(int a, int b, int c);
// Follower NPC animation update (Advanim.cpp or PLAYER.cpp)
extern void Advanim_UpdateFollower(void);

// ============================================================
//  Mov_Update — per-frame walk tick
//  Called once per game frame while a character has a destination set.
// ============================================================
void Mov_Update(void)
{
    if (g_nMovDestNode == -1)
        return;

    if (g_nMovPathSteps == 0 && g_nMovTurnSteps == 0 && g_nMovTurning != 0)
    {
        // --- At destination: play idle / arrival animation ---
        int nAnimDir;

        if (g_nMovForcedDir == -1 || g_nMovTurnSteps > 0 || g_nMovTurning == 1)
        {
            if (g_nMovCarryHint == -1)
            {
                // No-carry idle: force standing frame 2
                g_nMovAnimDir  = 2;
            }
            else
            {
                if (g_nMovTurnSteps > 0 && g_nMovTurning == 0)
                {
                    // Dequeue next turn-animation step
                    g_nMovTurning  = 1;
                    g_nMovTurnSteps--;
                    g_nMovAnimDir  = g_anMovTurnSeq[g_nMovTurnSteps];
                    g_nMovAnimStep = 2;
                    g_nMovAnimFrames = *(int*)(&g_anAnimFrameCount[0] + g_nMovAnimDir * 4);
                }

                if (g_nMovTurning == 0)
                {
                    // Select idle direction from carry hint or forced dir
                    int nSlot = 2;
                    if (g_nMovCarryHint > 0 && g_nMovCarryHint < 9)
                        nSlot = g_nMovCarryHint - 1;
                    g_nMovAnimDir  = g_anMovPath[nSlot];
                    g_nMovAnimStep = 2;
                    g_nMovCurDir   = (char)nSlot;
                }
            }
        }

        // Apply pending Z offset
        if (g_nMovZOffset != 0)
        {
            *(int*)(g_pMovNodes + g_nMovDestNode * 4 + 2) += g_nMovZOffset;
            g_nMovZOffset = 0;
        }

        // Clamp animation step
        if (g_nMovAnimFrames <= g_nMovAnimStep)
            g_nMovAnimStep = (g_nMovAnimFrames > 3) ? 3 : (int)g_nMovAnimFrames - 1;
        else
            g_nMovAnimStep = (g_nMovAnimStep < (int)g_nMovAnimFrames - 1) ? g_nMovAnimStep : (int)g_nMovAnimFrames - 1;

        // Retrieve current destination node position
        int* pNode = g_pMovNodes + g_nMovDestNode * 4;
        int nX     = pNode[0];
        int nY     = pNode[1];
        int nZ     = pNode[2];

        // Compute frame pointers for current and base anim directions
        int* pFrame    = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400 + g_nMovAnimStep] * 0x20);
        int* pBaseFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400] * 0x20);
        int* pIdleFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400] * 0x20);

        g_nMovAnimStep++;

        // Update follower if one is active
        Advanim_UpdateFollower();

        // Blit previous frame (ghost/shadow trail) if needed
        if (pBaseFrame)
        {
            int* pCurNode = g_pMovNodes + g_nMovDestNode * 4;
            Advanim_SetPos(pCurNode[0], pCurNode[1], pCurNode[2], pFrame[4], pIdleFrame[4]);
        }

        // Blit current character frame
        {
            int* pCurNode = g_pMovNodes + g_nMovDestNode * 4;
            Advanim_SetPos(pCurNode[0], pCurNode[1], pCurNode[2], pFrame[4], pIdleFrame[4]);
        }

        // Fire arrival callback if area changed
        if (g_nMovAnimDir != -1 && (int)g_pSpriteBase[0] != -1)
            Mov_ArrivalCallback();

        g_nMovDone = 1;
    }
    else
    {
        // --- Walking: linear interpolation between current and next node ---
        if (g_nMovInterpStep == 0)
        {
            // First tick for this path segment — compute direction and rate
            int nDestIdx = g_anMovPath[g_nMovPathSteps];
            int* pSrc = g_pMovNodes + g_nMovDestNode * 4;
            int* pDst = g_pMovNodes + nDestIdx       * 4;

            g_nMovDeltaX  = pDst[0] - pSrc[0];
            g_nMovDeltaY  = pDst[1] - pSrc[1];
            int  nSrcZ    = pSrc[2];
            int  nDstZ    = pDst[2];
            g_nMovAnimDir = *(int*)(g_pMovNodes + g_nMovDestNode * 4 + 3 * 4);  // area/room

            int nDeltaZ   = nDstZ - nSrcZ;

            // Determine the 8-direction from the angle of travel
            int nDir = 2;
            if (g_nMovDeltaX != 0 || g_nMovDeltaY != 0)
            {
                atan2((double)-g_nMovDeltaY, (double)g_nMovDeltaX);
                unsigned int uAngle = (unsigned int)__ftol() & 7;
                nDir = (int)uAngle;

                // Diagonal ambiguity: if odd quadrant and preferred diagonal is locked,
                // resolve to cardinal axis with larger displacement
                if ((uAngle & 1) && g_anMovDirNode[uAngle] != (int)uAngle)
                {
                    nDir = (g_nMovDeltaX < 1) ? 0 : 4;
                    int nA = abs(g_nMovDeltaY * 5);
                    int nB = abs(g_nMovDeltaX * 5);
                    if (nB / 2 < nA)
                    {
                        nDir = (g_nMovDeltaY < 0) ? 6 : 2;
                    }
                }
            }

            // If direction changed, start a turn sequence
            int nAnimIdx = g_anMovDirAnim[nDir];
            g_nMovCurDir = (char)nDir;
            if (nAnimIdx != g_nMovAnimDir)
                g_nMovAnimStep = 3;

            g_nMovAnimFrames = *(int*)(&g_anAnimFrameCount[0] + nAnimIdx * 4);
            g_nMovAnimStep = (g_nMovAnimStep < g_nMovAnimFrames - 1) ? g_nMovAnimStep : g_nMovAnimFrames - 1;
            g_nMovAnimDir  = nAnimIdx;

            // Compute interpolation rate (random footstep offset applies)
            {
                int nMinZ = (nDstZ < nSrcZ) ? nDstZ : nSrcZ;
                int nRand = abs(g_nMovDeltaX);  // placeholder
                g_nMovInterpTotal = (nRand * 100) / nMinZ;
                if (g_nMovInterpTotal == 0)
                    g_nMovInterpTotal = 1;
            }

            g_nMovDestNode   = nDestIdx;
            g_nMovInterpStep = g_nMovInterpTotal;
        }

        g_nMovInterpStep--;

        // Fire walk sound on eligible frames
        if (g_nMovCarryHint == 0 && g_nMovSoundsEnabled != 0)
        {
            if (g_anMovTransAnim[g_nMovAnimStep * 8 + g_nMovAnimDir] != -1)
            {
                Snd_PlayCentered(
                    g_anMovTransAnim[g_nMovAnimStep * 8 + g_nMovAnimDir],
                    g_anMovTransAnim[g_nMovAnimStep * 8 + g_nMovAnimDir] & 7);
            }
        }

        // Interpolated screen depth
        g_nMovCurZ = (int)(g_pMovNodes + g_nMovDestNode * 4)[2] -
                     (int)(g_nMovInterpStep * g_nMovDeltaY) / g_nMovInterpTotal;

        // Compute frame pointers
        int* pFrame    = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400 + g_nMovAnimStep] * 0x20);
        int* pBaseFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400] * 0x20);
        int* pIdleFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovAnimDir * 400] * 0x20);

        g_nMovAnimStep++;

        // Check for carry/on-the-fly override
        if (Speech_GetSentence() == 1)
        {
            int nOvr = SndMem_GetLipsyncByte();
            if (nOvr >= 0)
            {
                Mov_SelectCarryAnim();
                pFrame    = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[400 + nOvr * 400 + g_nMovCarrySlot * 400] * 0x20);
                pBaseFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovCarrySlot * 400] * 0x20);
                pIdleFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovCarrySlot * 400] * 0x20);
            }
        }
        else if (g_nMovFollower >= 0)
        {
            // Advance follower character animation sequence
            int nFollowStep = *(int*)(g_pCharWalkTable + g_nMovFollower * 0x58);
            if (g_anMovTransAnim[g_nMovAnimStep * 8 + g_nMovAnimDir] != -1)
            {
                Snd_PlayCentered(
                    g_anMovTransAnim[nFollowStep * 8 + g_nMovAnimDir],
                    2);
            }
            pFrame = (int*)(&g_pSpriteBase[0] +
                g_anAnimFrameTable[*(int*)(g_pCharWalkTable + g_nMovFollower * 0x58) * 4 + g_nMovFollower * 400] * 0x20);
            *(int*)(g_pCharWalkTable + g_nMovFollower * 0x58) += 1;
            if (*(int*)(g_pCharWalkTable + g_nMovFollower * 0x58) == g_anAnimFrameCount[g_nMovFollower])
            {
                *(int*)(g_pCharWalkTable + g_nMovFollower * 0x58)  = 2;
                *(int*)(g_pCharWalkTable + g_nMovFollower * 0x58 + 0x44) = 1;
            }
            pBaseFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovFollower * 400] * 0x20);
            pIdleFrame = (int*)(&g_pSpriteBase[0] + g_anAnimFrameTable[g_nMovFollower * 400] * 0x20);
        }

        Advanim_Tick();

        // Blit ghost/shadow trail
        if (pBaseFrame)
        {
            int nX = (int)(g_pMovNodes + g_nMovDestNode * 4)[0] -
                     (short)((g_nMovInterpStep * g_nMovDeltaX) / g_nMovInterpTotal);
            int nY = (int)(g_pMovNodes + g_nMovDestNode * 4)[1] -
                     (short)((g_nMovInterpStep * g_nMovDeltaY) / g_nMovInterpTotal);
            Advanim_SetPos(nX, nY, g_nMovCurZ, pBaseFrame[4], pIdleFrame[4]);
        }

        // Blit main character frame
        {
            int nX = (int)(g_pMovNodes + g_nMovDestNode * 4)[0] -
                     (short)((g_nMovInterpStep * g_nMovDeltaX) / g_nMovInterpTotal);
            int nY = (int)(g_pMovNodes + g_nMovDestNode * 4)[1] -
                     (short)((g_nMovInterpStep * g_nMovDeltaY) / g_nMovInterpTotal);
            Advanim_SetPos(nX, nY, g_nMovCurZ, pFrame[4], pIdleFrame[4]);
        }

        // Fire arrival callback when area changes
        if (g_nMovAnimDir != -1 && (int)g_pSpriteBase[0] != -1)
            Mov_ArrivalCallback();

        // Clamp animation step at end of cycle
        if (g_nMovAnimFrames <= g_nMovAnimStep)
            g_nMovAnimStep = 3;
        if (g_nMovAnimFrames - 1 < g_nMovAnimStep)
            g_nMovAnimStep = (char)g_nMovAnimFrames - 1;

        // Advance to next path segment when interpolation finishes
        if (g_nMovInterpStep == 0)
            g_nMovPathSteps--;
    }
}

// ============================================================
//  Mov_StartPath — begin advancing along the queued path
// ============================================================
void Mov_StartPath(void)
{
    if (g_nMovDone == 0)
        g_nMovPathSteps = 1;
}

// ============================================================
//  Mov_Reset — clear all movement state (dest + saved node)
// ============================================================
void Mov_Reset(void)
{
    g_nMovSavedNode = -1;
    g_nMovDestNode  = -1;
}

// ============================================================
//  Mov_FreezePos — suspend movement: save dest and clear it
// ============================================================
void Mov_FreezePos(void)
{
    if (g_nMovDestNode != -1)
    {
        g_nMovSavedNode = g_nMovDestNode;
        g_nMovDestNode  = -1;
    }
}

// ============================================================
//  Mov_RestorePos — resume movement after a freeze
// ============================================================
void Mov_RestorePos(void)
{
    if (g_nMovSavedNode != -1)
    {
        g_nMovDestNode  = g_nMovSavedNode;
        g_nMovSavedNode = -1;
    }
}

// ============================================================
//  Mov_SetFollower — attach a follower NPC to the walk system
// ============================================================
void Mov_SetFollower(int nCharIdx)
{
    g_nMovFollower = nCharIdx;
    if (nCharIdx >= 0)
        *(int*)(g_pCharWalkTable + nCharIdx * 0x58) = 2;
}

// ============================================================
//  Mov_IsMoveDone — returns 1 when the character has arrived
// ============================================================
int Mov_IsMoveDone(void)
{
    if (g_nMovFollower  == -1) return 1;
    if (g_nMovDestNode  == -1) return 1;
    if (*(int*)(g_pCharWalkTable + g_nMovFollower * 0x58 + 0x44) == 1) return 1;
    return 0;
}

// ============================================================
//  Mov_TracePos — debug: print current node position (x, y, z)
// ============================================================
void Mov_TracePos(void)
{
    int* pNode = g_pMovNodes + g_nMovDestNode * 4;
    Debug_TraceVal(pNode[0], pNode[1], pNode[2]);
}

// ============================================================
//  Mov_EnableSounds / Mov_DisableSounds — footstep sound toggle
// ============================================================
void Mov_EnableSounds(void)  { g_nMovSoundsEnabled = 1; }
void Mov_DisableSounds(void) { g_nMovSoundsEnabled = 0; }

// ============================================================
//  Mov_SetDir — set forced direction with carry-tier support
//
//  nDir 0-3   → normal walk directions, tier 0
//  nDir 4-7   → carry-low directions, tier 1
//  nDir 8-11  → carry-high directions, tier 2
// ============================================================
void Mov_SetDir(char nDir)
{
    if (nDir < 4)
    {
        g_nMovForcedDir = (int)nDir;
    }
    else if (nDir < 8)
    {
        g_nMovForcedDir = nDir - 4;
        g_nMovDirTier   = 1;
    }
    else if (nDir < 12)
    {
        g_nMovForcedDir = nDir - 7;
        g_nMovDirTier   = 2;
    }
}

// ============================================================
//  Mov_ClearDir — remove forced direction override
// ============================================================
void Mov_ClearDir(void)
{
    g_nMovForcedDir = -1;
}

// ============================================================
//  Mov_GetPathHead — read the node at the head of the walk path
// ============================================================
void Mov_GetPathHead(void)
{
    if (g_nMovPathSteps == 0)
        g_nMovPathHead = g_nMovDestNode;
    else
        g_nMovPathHead = g_anMovPath[g_nMovPathSteps];
}

// ============================================================
//  Mov_WalkTo — start walking to the previously set path head
// ============================================================
void Mov_WalkTo(void)
{
    // thunk_FUN_00453320 → MIXER or SCHED walk-to function
    extern void WalkToNode(int nNode);
    WalkToNode(g_nMovPathHead);
}

// ============================================================
//  Mov_InitChar — initialise movement state for a character
//
//  Sets the character's stand animation and clears the path.
//  param_1 is used as a char code (0x20 = space = stand still).
//  param_2 is passed to Mov_RestoreMoves to load walk data.
// ============================================================
void Mov_InitChar(int nNodeCode, int nCharName)
{
    // nNodeCode is set to 0x20 (space char) to indicate standing
    g_nMovInitialized = 1;
    Mov_RestoreMoves(nCharName);
    g_nMovCurDir     = 2;
    g_nMovAnimDir    = g_nMovIdleDir;
    g_nMovAnimStep   = 2;
    g_nMovAnimFrames = *(int*)(&g_anAnimFrameCount[0] + g_nMovIdleDir * 4);
    g_nMovSavedNode  = -1;
    g_nMovDestNode   = -1;
    g_nMovSoundsEnabled = 0;
    g_nMovFollower   = -1;
}

// ============================================================
//  Mov_FlipDir — horizontally mirror a cardinal direction
//
//  Maps: E(1)→W(5)  NE(2)→SW(1) unusual mapping preserved from original.
//  Used when a character sprite is mirrored across X.
// ============================================================
int Mov_FlipDir(int nDir)
{
    switch (nDir)
    {
    case 1: return 5;
    case 2: return 1;
    case 3: return 7;
    case 4: return 3;
    }
    return nDir;
}

// ============================================================
//  Mov_WrapDir — wrap a direction index into [0, 7]
// ============================================================
int Mov_WrapDir(int nDir)
{
    if (nDir < 0)      return nDir + 8;
    if (nDir >= 8)     return nDir - 8;
    return nDir & 7;
}

// ============================================================
//  Mov_AddDirSuffix — append a single direction character to a resource name
//
//  pszName  - base resource name, extended in-place
//  nDir     - direction index (0-7)
//
//  Each direction has a fixed single-byte code (from g_achDirCode[]).
//  The second byte of the 2-char suffix is always 0, making it a 1-char
//  NUL-terminated extension.
//
//  Original name: adddir_char_string___int_dir
// ============================================================
void Mov_AddDirSuffix(char* pszName, int nDir)
{
    size_t nLen = strlen(pszName);
    // Direction codes come from a table at 0x004d55b8 (8 × 4-byte entries)
    // BuildString copies one direction character into local_1c
    char chDir = g_achDirCode[nDir];
    pszName[nLen]     = chDir;
    pszName[nLen + 1] = 0;  // pad byte (game reads exactly 2 chars)
    pszName[nLen + 2] = '\0';
}

// ============================================================
//  Mov_RestoreMoves — load all walk animations for a character
//
//  Builds resource names of the form "<charname><dir>" and
//  "<charname>_c<dir>" (carry variants) for all 8 directions,
//  then loads the 8×8 turn-transition matrix and waypoint arrays.
//  Falls back by propagating the nearest valid direction when some
//  directions are not present in the resource set.
//
//  Original name: restore_moves_char__charname__
// ============================================================
void Mov_RestoreMoves(int nCharName)
{
    char szName[16];
    int  i, j;
    bool bAnyDirAnim, bAnyCarryAnim;

    // Reset all anim IDs
    for (i = 0; i < 8; i++)
    {
        g_anMovDirAnim[i]   = -1;
        g_anMovCarryAnim[i] = -1;
    }

    // Reset direction-to-node identity mapping
    for (i = 0; i < 8; i++)
    {
        g_anMovDirOrder[i] = i;
        g_anMovDirNode[i]  = i;
    }

    // Load base directional walk anims: "<charname><dircode>"
    for (i = 0; i < 8; i++)
    {
        BuildString(szName, (void*)nCharName);
        Mov_AddDirSuffix(szName, i);
        LoadAnimByName(szName, &g_anMovDirAnim[i]);
    }

    // Load carry-walk anims: "<charname>_c<dircode>"
    for (i = 0; i < 8; i++)
    {
        BuildString(szName, (void*)nCharName);
        AppendString(szName, "_c");
        Mov_AddDirSuffix(szName, i);
        LoadAnimByName(szName, &g_anMovCarryAnim[i]);
    }

    // Propagate missing carry anims from adjacent directions
    bAnyCarryAnim = false;
    for (i = 0; i < 8; i++)
    {
        if (g_anMovCarryAnim[i] >= 0) { bAnyCarryAnim = true; break; }
    }

    if (bAnyCarryAnim)
    {
        bool bDone = false;
        while (!bDone)
        {
            bDone = true;
            int aMap[8];
            for (i = 0; i < 8; i++)
            {
                aMap[i] = -1;
                if (g_anMovCarryAnim[i] == -1)
                {
                    bDone = false;
                    int nNext = Mov_WrapDir(i + 1);
                    int nPrev = Mov_WrapDir(i - 1);
                    if (g_anMovCarryAnim[nNext] == -1)
                    {
                        if (g_anMovCarryAnim[nPrev] >= 0)
                            aMap[i] = nPrev;
                    }
                    else
                    {
                        aMap[i] = nNext;
                    }
                }
            }
            for (i = 0; i < 8; i++)
            {
                if (aMap[i] >= 0)
                {
                    g_anMovCarryAnim[i] = g_anMovCarryAnim[aMap[i]];
                    g_anMovDirOrder[i]  = g_anMovDirOrder[aMap[i]];
                }
            }
        }
    }

    // Propagate missing base directional anims similarly
    bAnyDirAnim = false;
    for (i = 0; i < 8; i++)
    {
        if (g_anMovDirNode[i] >= 0) { bAnyDirAnim = true; break; }
    }

    if (bAnyDirAnim)
    {
        bool bDone = false;
        while (!bDone)
        {
            bDone = true;
            int aMap[8];
            for (i = 0; i < 8; i++)
            {
                aMap[i] = -1;
                if (g_anMovDirAnim[i] == -1)
                {
                    bDone = false;
                    int nNext = Mov_WrapDir(i + 1);
                    int nPrev = Mov_WrapDir(i - 1);
                    if (g_anMovDirAnim[nNext] == -1)
                    {
                        if (g_anMovDirAnim[nPrev] >= 0)
                            aMap[i] = nPrev;
                    }
                    else
                    {
                        aMap[i] = nNext;
                    }
                }
            }
            for (i = 0; i < 8; i++)
            {
                if (aMap[i] >= 0)
                {
                    g_anMovDirAnim[i] = g_anMovDirAnim[aMap[i]];
                    g_anMovDirNode[i] = g_anMovDirNode[aMap[i]];
                }
            }
        }
    }

    // Free stale handles before reloading
    for (i = 0; i < 8; i++)
    {
        FreeAnimHandle(g_anMovCarryAnim[i], 1);
        FreeAnimHandle(g_anMovDirAnim[i],   1);
    }
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
        {
            FreeAnimHandle(g_anMovTransAnim[i * 8 + j], 1);
            g_anMovTransAnim[i * 8 + j] = -1;
        }

    // Load 8×8 turn-transition matrix: "<charname><dir_from><dir_to>"
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
        {
            BuildString(szName, (void*)nCharName);
            Mov_AddDirSuffix(szName, i);
            Mov_AddDirSuffix(szName, j);
            LoadAnimByName(szName, &g_anMovTransAnim[j * 8 + i]);
            if (g_anMovTransAnim[j * 8 + i] >= 0)
                FreeAnimHandle(g_anMovTransAnim[j * 8 + i], 1);
        }

    // Load per-direction waypoint anims: "<charname>_wp<dir>"
    for (j = 0; j < 8; j++)
    {
        BuildString(szName, (void*)nCharName);
        AppendString(szName, "_wp");
        Mov_AddDirSuffix(szName, j);
        LoadAnimByName(szName, &g_anMovWaypointAnims[j]);
        if (g_anMovWaypointAnims[j] >= 0)
            FreeAnimHandle(g_anMovWaypointAnims[j], 1);
    }

    // Load per-direction secondary anims: "<charname>_sec<dir>"
    for (j = 0; j < 8; j++)
    {
        BuildString(szName, (void*)nCharName);
        AppendString(szName, "_sec");
        Mov_AddDirSuffix(szName, j);
        LoadAnimByName(szName, &g_anMovSecondaryAnims[j]);
        if (g_anMovSecondaryAnims[j] >= 0)
            FreeAnimHandle(g_anMovSecondaryAnims[j], 1);
    }
}

// ============================================================
//  Mov_FindPath — depth-first search over the 8-direction transition graph
//
//  Finds the shortest sequence of turn-transitions from the current
//  direction (g_nMovPathSrc) to nTargetDir.  Writes the shortest path
//  into g_abMovPathBuf and g_nMovPathLen.
//
//  Original name: find_path_int_j__
// ============================================================
void Mov_FindPath(int nTargetDir)
{
    char abSaved[8];
    int  nBestDir = -1;
    char chBestLen = 'c';  // 0x63 = unreachable sentinel

    if (nTargetDir == g_nMovPathSrc)
        return;

    // Save the visited set and try each valid neighbour
    memcpy(abSaved, g_abMovPathBuf, 8);
    g_abMovVisited[g_nMovPathSrc] = 1;

    for (int i = 0; i < 8; i++)
    {
        if (g_anMovTransAnim[i * 8 + g_nMovPathSrc] != -1 && g_abMovVisited[i] == 0)
        {
            int nSaved = g_nMovPathSrc;
            memcpy(g_abMovPathBuf, abSaved, 8);
            g_nMovPathLen = (char)(g_nMovPathLen - 'a' + 1 + 'a'); // g_nMovPathLen - saved
            g_nMovPathSrc = i;
            Mov_FindPath(nTargetDir);

            if (g_nMovPathLen < chBestLen)
            {
                memcpy(abSaved, g_abMovPathBuf, 8);
                chBestLen = g_nMovPathLen;
                nBestDir  = i;
            }

            g_abMovVisited[i] = 0;
            g_nMovPathSrc = nSaved;
        }
    }

    if (nBestDir < 0)
    {
        g_nMovPathLen = '\n';   // 0x0A = path not found
    }
    else
    {
        memcpy(g_abMovPathBuf, abSaved, 8);
        g_nMovPathLen = chBestLen;
        g_abMovPathBuf[chBestLen] = (char)nBestDir;
        g_nMovPathLen = (char)(g_nMovPathLen + 1);
    }
}

// ============================================================
//  Mov_FreeMoves — release all loaded walk animation handles
// ============================================================
void Mov_FreeMoves(void)
{
    for (int i = 0; i < 8; i++)
    {
        if (g_anMovDirAnim[i] >= 0)
        {
            FreeAnimHandle(g_anMovDirAnim[i], 0);
            g_anMovDirAnim[i] = -1;
        }
        if (g_anMovCarryAnim[i] >= 0)
        {
            FreeAnimHandle(g_anMovCarryAnim[i], 0);
            g_anMovCarryAnim[i] = -1;
        }
    }
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            if (g_anMovTransAnim[i * 8 + j] >= 0)
            {
                FreeAnimHandle(g_anMovTransAnim[i * 8 + j], 0);
                g_anMovTransAnim[i * 8 + j] = -1;
            }
        }
}

// ============================================================
//  Mov_SelectCarryAnim — resolve which carry-walk slot to use
//
//  g_nMovCarryHint determines the slot:
//    0       → use default stand (g_nMovDefaultStand)
//    1-8     → use the corresponding g_anMovCarryByHint[] entry
//    else    → use the carry anim for the current direction
// ============================================================
void Mov_SelectCarryAnim(void)
{
    if (g_nMovCarryHint >= 1 && g_nMovCarryHint <= 8)
        g_nMovCarrySlot = g_anMovCarryByHint[g_nMovCarryHint];
    else if (g_nMovCarryHint == 0)
        g_nMovCarrySlot = g_nMovDefaultStand;
    else
        g_nMovCarrySlot = g_anMovCarryAnim[g_nMovCurDir];
}

// ============================================================
//  Mov_TurnAround — queue a full turn from current dir to nDir
//
//  Runs Mov_FindPath then builds the g_anMovTurnSeq[] array of
//  intermediate transition anim IDs, walking the path in reverse.
//
//  Original name: turn_around_int_dir__
// ============================================================
void Mov_TurnAround(int nDir)
{
    char auTurnBuf[8];
    int  nSrcDir;

    g_nMovCarryHint = nDir;

    // "0" is a sentinel meaning "face backwards" → use step 3
    if (nDir == 0)
        nDir = 3;

    g_nMovPathSrc = g_anMovDirNode[g_nMovCurDir];
    int nTarget   = *(int*)(&g_anMovDirNode[0] + nDir * 4);

    // Local saved start for path reversal
    nSrcDir = g_nMovPathSrc;

    memset(g_abMovVisited, 0, 8);
    g_nMovPathLen = 0;
    Mov_FindPath(nTarget);

    // Initialise turn-step counters
    for (int i = 0; i < g_nMovPathLen; i++)
        g_nMovTurnSteps = 0;

    // Reverse the found path into the turn-sequence anim buffer
    if (g_nMovPathLen < 10)
    {
        for (int i = 0; i < g_nMovPathLen; i++)
        {
            int nStep = g_abMovPathBuf[g_nMovPathLen - i];  // walk backwards
            if (g_anMovTransAnim[nStep * 8 + nSrcDir] != -1)
            {
                auTurnBuf[g_nMovTurnSteps] = (char)g_anMovTransAnim[nStep * 8 + nSrcDir];
                g_nMovTurnSteps++;
                nSrcDir = nStep;
            }
        }
    }

    // Copy into the global turn-sequence output
    for (int i = 0; i < g_nMovTurnSteps; i++)
        g_anMovTurnSeq[i] = (int)(char)auTurnBuf[g_nMovTurnSteps - i - 1];
}

// ============================================================
//  Mov_TurnOnTheFly — queue a turn mid-walk (no carry-hint set)
//
//  Identical logic to Mov_TurnAround but does NOT set g_nMovCarryHint.
//
//  Original name: turnonthefly_int_dir__
// ============================================================
void Mov_TurnOnTheFly(int nDir)
{
    char auTurnBuf[8];
    int  nSrcDir;

    g_nMovPathSrc = g_anMovDirNode[g_nMovCurDir];
    int nTarget   = g_anMovDirNode[nDir];

    nSrcDir = g_nMovPathSrc;

    memset(g_abMovVisited, 0, 8);
    g_nMovPathLen = 0;
    Mov_FindPath(nTarget);

    for (int i = 0; i < g_nMovPathLen; i++)
        g_nMovTurnSteps = 0;

    if (g_nMovPathLen < 10)
    {
        for (int i = 0; i < g_nMovPathLen; i++)
        {
            int nStep = g_abMovPathBuf[g_nMovPathLen - i];
            if (g_anMovTransAnim[nStep * 8 + nSrcDir] != -1)
            {
                auTurnBuf[g_nMovTurnSteps] = (char)g_anMovTransAnim[nStep * 8 + nSrcDir];
                g_nMovTurnSteps++;
                nSrcDir = nStep;
            }
        }
    }

    for (int i = 0; i < g_nMovTurnSteps; i++)
        g_anMovTurnSeq[i] = (int)(char)auTurnBuf[g_nMovTurnSteps - i - 1];
}

// ============================================================
//  Mov_FacePoint — turn to face a given screen position
//
//  Computes the 8-direction from the current node toward (nX, nY)
//  using a slope-comparison table (integer approximation of atan2),
//  then calls Mov_TurnAround.
// ============================================================
void Mov_FacePoint(int nX, int nY)
{
    int nDir;

    // Subtract current position
    int* pNode = g_pMovNodes + g_nMovDestNode * 4;
    nX -= pNode[0];
    nY -= pNode[1];

    // Slope table: uses |dy*5| vs |dx*2| to classify into 8 octants
    if (nX < 0)
    {
        if (abs(nY * 5) < nX * -2)
        {
            nDir = 1;   // W
        }
        else if (nY * 2 + nX * 5 == 0 || nY * 2 < nX * -5)
        {
            nDir = (nY * 2 < nX * 5) ? 7 : ((nY < 1) ? 8 : 6);
        }
        else
        {
            nDir = 3;   // NW or SW
        }
    }
    else
    {
        if (abs(nY * 5) < nX * 2)
        {
            nDir = 5;   // E
        }
        else if (nY * 2 + nX * -5 == 0 || nY * 2 < nX * 5)
        {
            nDir = (nY * 2 < nX * -5) ? 7 : ((nY < 1) ? 6 : 8);
        }
        else
        {
            nDir = 3;   // NE or SE
        }
    }

    Mov_TurnAround(nDir);
}

// ============================================================
//  Navigation-graph subsystem (zones / waypoints / edges)
//
//  These functions implement a second, graph-based navigation layer used
//  for room-to-room walking. Unlike the simple direction tables above, the
//  active screen ("zone") owns:
//    - a waypoint node table (g_pMovNodes / 004d525c), 0x10 bytes per node:
//          { int x, int y, int z, int area }
//    - an edge table (g_pMovEdges), 3 INTs per edge: { nodeA, nodeB, weight }
//    - precomputed counts and surface metadata, all swapped in by
//      Mov_SelectZone() from per-zone arrays.
//  A precomputed square-root LUT (g_anMovDistLUT) maps a squared distance
//  index back to an integer Euclidean distance for edge weighting.
// ============================================================

// --- Navigation-graph globals (per-active-zone) ---
int  g_anMovDistLUT[10000];     // 006dea60  int(sqrt(i)) for i in [0,10000)
int  g_nMovZoneCount     = 0;   // zone count guard
int  g_nMovNodeCount     = 0;   // active zone's waypoint count
int  g_nMovEdgeCount     = 0;   // active zone's edge count
int* g_pMovEdges         = 0;   // active zone's edge table {a,b,weight}*N
int  g_nMovActiveZone    = 0;   // currently selected zone index
int  g_nMovSurfOffsetX   = 0;   // 006e86b? surface lock X offset
int  g_nMovSurfOffsetY   = 0;   //          surface lock Y offset
int  g_nMovResBase       = 0;   // 004d5388  resource id base for Err_BadResEntry

// Cross-module helpers (defined elsewhere in the codebase)
extern double FUN_0048b1c4(double d);  // sqrt helper -> double
extern void   GI_LockActiveSurf_v8(int x, int y, int a4, int a5, int a3);
extern void   Err_BadResEntry(int nCode, const char* pszFile, const char* pszMsg);

// Per-zone source data tables (owned by game-init / AREAS data segment)
extern int   g_aMovZoneNodeCount[];   // 006dd608  node count per zone
extern int   g_aMovZoneNodeData[];    // 006dd720  node tables, 0x640 per zone
extern int   g_aMovZoneEdgeCount[];   // 006e86a8  edge count per zone
extern int   g_aMovZoneEdgeData[];    // 006dc740  edge tables, 0x4b0 per zone
extern int   g_aMovZoneField1[];      // 006dc6f0
extern int   g_aMovZoneField2[];      // 006dea50
extern int   g_nMovZoneField1;        // 006dd718
extern int   g_nMovZoneField2;        // 004d5264

// ============================================================
//  Mov_BuildDistLUT — fill the sqrt distance lookup table
//
//  g_anMovDistLUT[i] = (int)sqrt((double)i) for i in [0, 10000).
//  Used to convert squared edge lengths into integer weights.
//
//  Original name: Mov_BuildDistLUT  (00452950)
// ============================================================
void Mov_BuildDistLUT(void)
{
    for (int i = 0; i < 10000; i++)
    {
        g_anMovDistLUT[i] = (int)FUN_0048b1c4((double)i);
    }
}

// ============================================================
//  Mov_SelectZone — make a navigation zone (screen) active
//
//  Swaps in the per-zone node table, edge table, counts and surface
//  metadata so the rest of the graph functions operate on this screen.
//
//  Original name: Mov_SelectZone  (00452a30)
// ============================================================
void Mov_SelectZone(int nZone)
{
    if (nZone < g_nMovZoneCount)
    {
        g_nMovNodeCount = g_aMovZoneNodeCount[nZone];
        g_pMovNodes     = g_aMovZoneNodeData + (nZone * 0x640) / sizeof(int);
        g_nMovEdgeCount = g_aMovZoneEdgeCount[nZone];
        g_pMovEdges     = g_aMovZoneEdgeData + (nZone * 0x4b0) / sizeof(int);
        g_nMovZoneField1 = g_aMovZoneField1[nZone];
        g_nMovZoneField2 = g_aMovZoneField2[nZone];
        g_nMovActiveZone = nZone;
    }
}

// ============================================================
//  Mov_LockSurfaceAt — lock the active drawing surface at an offset
//
//  Adds the global surface offsets to the requested position and forwards
//  to the graphics-interface surface-lock routine.
//
//  Original name: Mov_LockSurfaceAt  (00452b50)
// ============================================================
void Mov_LockSurfaceAt(int nX, int nY, int a3, int a4, int a5)
{
    GI_LockActiveSurf_v8(nX + g_nMovSurfOffsetX, nY + g_nMovSurfOffsetY, a4, a5, a3);
}

// ============================================================
//  Mov_FindNearestNode — nearest waypoint node to (nX, nY)
//
//  Linear scan of the active zone's node table; returns the index of the
//  node with the smallest squared distance, or -1 if there are no nodes.
//
//  Original name: Mov_FindNearestNode  (00452c00)
// ============================================================
int Mov_FindNearestNode(int nX, int nY)
{
    int nBest    = -1;
    int nBestSq  = 0xc7602;   // ~816258, effectively "infinity" here

    for (int i = 0; i < g_nMovNodeCount; i++)
    {
        int dx = g_pMovNodes[i * 4]     - nX;
        int dy = g_pMovNodes[i * 4 + 1] - nY;
        int sq = dx * dx + dy * dy;
        if (sq < nBestSq)
        {
            nBest   = i;
            nBestSq = sq;
        }
    }
    return nBest;
}

// ============================================================
//  Mov_FindNearestNodeInBox — nearest node inside a bounding box
//
//  Like Mov_FindNearestNode but only considers nodes whose coordinates
//  lie within [nLeft, nRight] x [nTop, nBottom].
//
//  Original name: Mov_FindNearestNodeInBox  (00452d30)
// ============================================================
int Mov_FindNearestNodeInBox(int nX, int nY, int nLeft, int nTop, int nRight, int nBottom)
{
    int nBest   = -1;
    int nBestSq = 0xc7602;

    for (int i = 0; i < g_nMovNodeCount; i++)
    {
        int x = g_pMovNodes[i * 4];
        int y = g_pMovNodes[i * 4 + 1];
        if (x >= nLeft && y >= nTop && x <= nRight && y <= nBottom)
        {
            int dx = x - nX;
            int dy = y - nY;
            int sq = dx * dx + dy * dy;
            if (sq < nBestSq)
            {
                nBest   = i;
                nBestSq = sq;
            }
        }
    }
    return nBest;
}

// ============================================================
//  Mov_FindNeighborByDir — best edge-neighbour of the dest node
//
//  Scans all edges touching g_nMovDestNode and picks the neighbour node
//  closest (in squared distance) to the dest node, biased by direction:
//    1 = prefer East (neighbour.x > dest.x, mostly horizontal)
//    2 = prefer West (neighbour.x < dest.x, mostly horizontal)
//    3 = prefer South (neighbour.y > dest.y, mostly vertical)
//    4 = prefer North (neighbour.y < dest.y, mostly vertical)
//    other = no directional bias.
//  Returns the chosen neighbour node index, or -1.
//
//  Original name: Mov_FindNeighborByDir  (00452ea0)
// ============================================================
int Mov_FindNeighborByDir(int nDir)
{
    int destX = g_pMovNodes[g_nMovDestNode * 4];
    int destY = g_pMovNodes[g_nMovDestNode * 4 + 1];
    int nBest   = -1;
    int nBestSq = 0xc7602;

    for (int e = 0; e < g_nMovEdgeCount; e++)
    {
        int nNeighbor;
        if (g_pMovEdges[e * 3] == g_nMovDestNode)
            nNeighbor = g_pMovEdges[e * 3 + 1];
        else if (g_pMovEdges[e * 3 + 1] == g_nMovDestNode)
            nNeighbor = g_pMovEdges[e * 3];
        else
            continue;

        int nx = g_pMovNodes[nNeighbor * 4];
        int ny = g_pMovNodes[nNeighbor * 4 + 1];
        int sqX = (nx - destX) * (nx - destX);
        int sqY = (ny - destY) * (ny - destY);

        bool bAccept = false;
        switch (nDir)
        {
        case 1:  // East, predominantly horizontal
            bAccept = (destX < nx) && (sqY <= sqX);
            break;
        case 2:  // West, predominantly horizontal
            bAccept = (nx < destX) && (sqY <= sqX);
            break;
        case 3:  // South, predominantly vertical
            bAccept = (ny < destY) && (sqX <= sqY);
            break;
        case 4:  // North, predominantly vertical
            bAccept = (destY < ny) && (sqX <= sqY);
            break;
        default:
            bAccept = true;
            break;
        }

        if (bAccept && (sqX + sqY < nBestSq))
        {
            nBest   = nNeighbor;
            nBestSq = sqX + sqY;
        }
    }
    return nBest;
}

// ============================================================
//  Mov_CompareEdgeDir — pick which of two edges aims closer to target
//
//  Given the origin (nX,nY), a target point (tx,ty) and two candidate
//  neighbour points (ax,ay) and (bx,by), normalises each direction to a
//  fixed-point unit vector scaled by 1000 and compares squared deviation
//  from the target direction. Returns 1 if candidate A (first) is closer,
//  0 if candidate B is closer-or-equal.
//
//  Original name: Mov_CompareEdgeDir  (00453130)
// ============================================================
int Mov_CompareEdgeDir(int nX, int nY, int tx, int ty,
                       int ax, int ay, int bx, int by)
{
    // Direction toward the target, fixed-point (scale 1000)
    int dtx = nX - tx, dty = nY - ty;
    int lenT = dtx * dtx + dty * dty;
    int utx = (dtx * 1000) / lenT;
    int uty = (dty * 1000) / lenT;

    // Direction toward candidate A
    int dax = nX - ax, day = nY - ay;
    int lenA = dax * dax + day * day;
    int uax = (dax * 1000) / lenA;
    int uay = (day * 1000) / lenA;

    // Direction toward candidate B
    int dbx = nX - bx, dby = nY - by;
    int lenB = dbx * dbx + dby * dby;
    int ubx = (dbx * 1000) / lenB;
    int uby = (dby * 1000) / lenB;

    int devA = (utx - ubx) * (utx - ubx) + (uty - uby) * (uty - uby);
    int devB = (utx - uax) * (utx - uax) + (uty - uay) * (uty - uay);

    return (devA < devB) ? 1 : 0;
}

// ============================================================
//  Mov_PathfindTo — bidirectional graph search to a target node
//
//  Walks the active zone's edge graph from g_nMovDestNode toward nTarget
//  and writes the resulting node sequence into g_anMovPath.
//
//  The search is bidirectional: two front-buffers (selected by the toggle
//  bit nSide, 0/1) grow alternately. When a front reaches nTarget the
//  search swaps to continue from the meeting front, copying the partial
//  path across so the two halves stitch into a single route. Edge weights
//  come from g_anMovDistLUT. A depth-first stack (anStack/anStackCnt)
//  backtracks when a node has no further usable edges. On completion the
//  route is reconstructed backwards from g_nMovDestNode into g_anMovPath
//  and g_nMovPathSteps is set; if no route is found, Err_BadResEntry is
//  raised. Before searching, edges touching the dest node are reordered by
//  Mov_CompareEdgeDir so the most target-aligned edge is tried first.
//
//  Original name: Mov_PathfindTo  (00453320)
// ============================================================
void Mov_PathfindTo(int nTarget)
{
    // Per-side scalars (index by nSide / 1-nSide):
    //   nCost[side]  - accumulated path cost on each front
    //   nDepth[side] - number of committed steps on each front
    //   nCur[side]   - current node on each front
    unsigned int nCost[2]  = { 0x7fffffff, 0x7fffffff };
    int          nDepth[2] = { 0, 0 };
    int          nCur[2]   = { 0, 0 };
    unsigned int anBestCost[100];          // best known cost to each node
    int          anEdgeStack[2][100];      // edge chosen at each depth, per side
    int          anVisited[100];           // visited-edge guard per depth
    int          anExpandCnt[103];         // expansion counter per depth
    int          nLastSavedDest = 0;       // aiStack_68c[3]
    char         nSide   = 0;
    char         nMeet   = 2;              // 2 => "no meeting yet" sentinel
    int          nStepCost = 0;            // cost of the edge at current depth

    memset(anBestCost, 0xff, sizeof(anBestCost));

    // --- Trivial cases ---
    if (nTarget == g_nMovDestNode)
    {
        if (g_nMovPathSteps == 0 || g_nMovInterpStep == 0)
        {
            g_nMovPathSteps = 0;
            g_nMovSavedNode = -1;
            g_nMovDone      = 1;
        }
        else
        {
            g_nMovPathSteps = 1;
        }
        return;
    }
    if (g_nMovDestNode == -1)
    {
        g_nMovPathSteps = 0;
        g_nMovDestNode  = nTarget;
        g_nMovSavedNode = -1;
        g_nMovDone      = 1;
        return;
    }

    g_nMovSavedNode = -1;
    if (g_nMovDone == 0)
        nLastSavedDest = g_nMovDestNode;

    int destX = g_pMovNodes[g_nMovDestNode * 4];
    int destY = g_pMovNodes[g_nMovDestNode * 4 + 1];
    int tgtX  = g_pMovNodes[nTarget * 4];
    int tgtY  = g_pMovNodes[nTarget * 4 + 1];

    // --- Reorder edges around the dest node so the most target-aligned
    //     neighbour is tried first (selection sort via Mov_CompareEdgeDir). ---
    for (int i = 0; i < g_nMovEdgeCount; i++)
    {
        if (g_pMovEdges[i * 3] != g_nMovDestNode && g_pMovEdges[i * 3 + 1] != g_nMovDestNode)
            continue;

        int curX, curY;
        if (g_pMovEdges[i * 3] == g_nMovDestNode)
        {
            curX = g_pMovNodes[g_pMovEdges[i * 3 + 1] * 4];
            curY = g_pMovNodes[g_pMovEdges[i * 3 + 1] * 4 + 1];
        }
        else
        {
            curX = g_pMovNodes[g_pMovEdges[i * 3] * 4];
            curY = g_pMovNodes[g_pMovEdges[i * 3] * 4 + 1];
        }

        for (int j = i + 1; j < g_nMovEdgeCount; j++)
        {
            if (g_pMovEdges[j * 3] != g_nMovDestNode && g_pMovEdges[j * 3 + 1] != g_nMovDestNode)
                continue;

            int candX, candY;
            if (g_pMovEdges[j * 3] == g_nMovDestNode)
            {
                candX = g_pMovNodes[g_pMovEdges[j * 3 + 1] * 4];
                candY = g_pMovNodes[g_pMovEdges[j * 3 + 1] * 4 + 1];
            }
            else
            {
                candX = g_pMovNodes[g_pMovEdges[j * 3] * 4];
                candY = g_pMovNodes[g_pMovEdges[j * 3] * 4 + 1];
            }

            if (Mov_CompareEdgeDir(destX, destY, tgtX, tgtY, curX, curY, candX, candY) != 0)
            {
                // candidate j aims better -> swap edge i and j (all 3 ints)
                for (int k = 0; k < 3; k++)
                {
                    INT t = g_pMovEdges[i * 3 + k];
                    g_pMovEdges[i * 3 + k] = g_pMovEdges[j * 3 + k];
                    g_pMovEdges[j * 3 + k] = t;
                }
                curX = candX;
                curY = candY;
            }
        }
    }

    // --- Bidirectional depth-first search ---
    int nDepthCur = 0;
    memset(anExpandCnt, 0, sizeof(int) * 100);
    nCost[nSide]  = 0;
    nDepth[nSide] = 0;
    nCur[nSide]   = g_nMovDestNode;
    nDepth[1]     = 0;   // aiStack_68c[2] in the decompile

    do
    {
        int nFound = 0;
        int e;

        // Find the next unused edge leaving the current front node.
        for (e = 0; e < g_nMovEdgeCount; e++)
        {
            bool bGuard = (nDepthCur == 0) || (anVisited[nDepthCur] != e);
            bool bTouches = (g_pMovEdges[e * 3] == nCur[nSide]) ||
                            (g_pMovEdges[e * 3 + 1] == nCur[nSide]);
            if (bGuard && bTouches)
            {
                nFound++;
                anEdgeStack[nSide][nDepthCur] = e;
                if (anExpandCnt[nDepthCur] < nFound)
                {
                    anExpandCnt[nDepthCur]++;
                    break;
                }
            }
        }

        if (e == g_nMovEdgeCount)
        {
            // No further edge at this depth: backtrack one step.
            nDepthCur--;
            if (nDepthCur < 0)
                break;
            nDepth[nSide]--;
            nCost[nSide] -= nStepCost;
            int be = anEdgeStack[nSide][nDepthCur];
            nCur[nSide] = (g_pMovEdges[be * 3] == nCur[nSide])
                        ? g_pMovEdges[be * 3 + 1]
                        : g_pMovEdges[be * 3];
        }
        else
        {
            int chosen = anEdgeStack[nSide][nDepthCur];
            nCost[nSide] += g_anMovDistLUT[g_pMovEdges[chosen * 3 + 2]];

            // Relax best-known cost for the edge's far endpoint.
            bool bWorse = true;
            if (g_pMovEdges[e * 3] == nCur[nSide])
            {
                int far = g_pMovEdges[e * 3 + 1];
                if (nCost[nSide] < anBestCost[far])
                {
                    anBestCost[far] = nCost[nSide];
                    bWorse = false;
                }
                bWorse = (nCost[nSide] >= anBestCost[far]);
            }
            else if (nCost[nSide] < anBestCost[g_pMovEdges[e * 3]])
            {
                anBestCost[g_pMovEdges[e * 3]] = nCost[nSide];
                bWorse = false;
            }

            // Reject if this edge already appears on the current path.
            bool bCycle = false;
            for (int s = 0; s < nDepthCur; s++)
            {
                if (anEdgeStack[nSide][nDepthCur] == anEdgeStack[nSide][s])
                {
                    bCycle = true;
                    break;
                }
            }

            if (bCycle || nCost[1 - nSide] < nCost[nSide] || bWorse)
            {
                // Prune: undo the cost we just added and try another edge.
                nCost[nSide] -= g_anMovDistLUT[g_pMovEdges[chosen * 3 + 2]];
            }
            else
            {
                nDepth[nSide]++;
                if (g_pMovEdges[chosen * 3] == nTarget || g_pMovEdges[chosen * 3 + 1] == nTarget)
                {
                    // Reached the target on this front: swap to the other
                    // front and seed it from this one so the halves stitch.
                    nMeet = nSide;
                    nSide = (char)(1 - nSide);
                    nDepth[nSide] = nDepth[1 - nSide] - 1;
                    nCost[nSide]  = nCost[1 - nSide];
                    nCost[nSide] -= g_anMovDistLUT
                        [g_pMovEdges[anEdgeStack[1 - nSide][nDepthCur] * 3 + 2]];
                    nCur[nSide] = nCur[1 - nSide];
                    for (int s = 0; s < nDepth[nSide]; s++)
                        anEdgeStack[nSide][s] = anEdgeStack[1 - nSide][s];
                }
                else
                {
                    // Advance current front across the chosen edge.
                    nCur[nSide] = (g_pMovEdges[chosen * 3] == nCur[nSide])
                                ? g_pMovEdges[chosen * 3 + 1]
                                : g_pMovEdges[chosen * 3];
                    nDepthCur++;
                    anExpandCnt[nDepthCur - 1] = 0;
                    anVisited[nDepthCur] = anEdgeStack[nSide][nDepthCur];
                }
            }
        }

        nStepCost = (nDepthCur == 0)
                  ? 0
                  : g_anMovDistLUT[g_pMovEdges[anEdgeStack[nSide][nDepthCur] * 3 + 2]];
    }
    while (nDepthCur >= 0);

    if (nMeet == 2)
    {
        Err_BadResEntry(g_nMovResBase + 0x109,
                        "C:\\DevStudio\\Projects\\Crux\\MOVEMENT.cpp",
                        "No route to target.");
    }
    else
    {
        nSide = nMeet;
    }

    // --- Reconstruct the route backwards from the dest node into g_anMovPath. ---
    int nWalk = g_nMovDestNode;
    for (int i = 0; i < nDepth[nSide]; i++)
    {
        int edge = anEdgeStack[nSide][i];
        int slot = nDepth[nSide] - i;
        g_anMovPath[slot] = (g_pMovEdges[edge * 3] == nWalk)
                          ? g_pMovEdges[edge * 3 + 1]
                          : g_pMovEdges[edge * 3];
        nWalk = g_anMovPath[slot];
    }

    g_nMovPathSteps = nDepth[nSide];
    if (g_nMovDone == 0 && g_nMovInterpStep != 0)
    {
        g_nMovDestNode = nLastSavedDest;
        g_nMovPathSteps++;
    }
    else
    {
        g_nMovInterpStep = 0;
        g_nMovDone       = 0;
    }
}
