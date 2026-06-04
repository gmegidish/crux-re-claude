#pragma once
// MOVEMENT.cpp — Character walk animation manager
// Owns 8-directional walk anims, turn transitions, node-based path
// interpolation, follower characters, and carry-anim selection.

// --- Globals ---
extern int  g_nMovDestNode;       // 006dd5d8  current dest node (-1 = idle)
extern int  g_nMovPathSteps;      // 006dc6fc  remaining waypoint steps
extern int  g_nMovTurnSteps;      // 006e86d0  queued turn animation count
extern int  g_nMovTurning;        // 006e86e0  non-zero while turning
extern int  g_nMovForcedDir;      // 004d5280  direction override (-1=auto)
extern int  g_nMovCarryHint;      // 00629dc4  carry type (0=none, 1-8)
extern int  g_nMovAnimStep;       // 006dc6e0  frame step in active anim
extern int  g_nMovAnimDir;        // 006dc6e4  direction of active anim
extern int  g_nMovAnimFrames;     // 007d66ac  frame count of active anim
extern int  g_anMovDirAnim[8];    // 006dd5e0  walk anim IDs by direction
extern int  g_anMovCarryAnim[8];  // 006dd550  carry-walk anim IDs by dir
extern int  g_anMovPath[101];     // 006dc550  path waypoint node indices / graph route
extern int  g_nMovInterpStep;     // 006dc708  interpolation step countdown
extern int  g_nMovInterpTotal;    // 006dd614  total interp ticks per segment
extern int  g_nMovDeltaX;         // 006dd5d0  X delta to next node
extern int  g_nMovDeltaY;         // 006dd5d4  Y delta to next node
extern int  g_nMovCurZ;           // 006dd5c4  current interpolated depth/Z
extern int  g_nMovDone;           // 006dd600  set to 1 on arrival
extern int  g_nMovFollower;       // 004d5268  follower NPC index (-1=none)
extern int  g_nMovSoundsEnabled;  // 006e86dc  footstep sounds active
extern int  g_nMovCarrySlot;      // 004d527c  resolved carry anim slot
extern int  g_nMovCurDir;         // 004d5284  current facing (0-7)
extern int  g_anMovTransAnim[64]; // 006dd618  8x8 turn-transition anim matrix
extern int  g_anMovDirNode[8];    // 006dc720  direction-to-node mapping
extern int  g_anMovTurnSeq[8];    // 006dea00  queued turn anim IDs
extern int  g_nMovPathHead;       // 006dd5dc  path head node for walking
extern int  g_nMovSavedNode;      // 006dd570  frozen node (FreezePos/RestorePos)
extern int  g_nMovIdleDir;        // 006dd5e8  idle animation direction
extern int  g_nMovDefaultStand;   // 006dd558  default stand anim (carry=0)
extern int  g_nMovInitialized;    // 006e86a0  set to 1 by Mov_InitChar
extern int  g_nMovZOffset;        // 006e86d4  pending Z offset adjustment
extern int  g_nMovDirTier;        // 006e86d8  carry direction tier (0-2)
extern int  g_anMovDirOrder[8];   // 006dd5a0  direction traversal order
extern int  g_nMovPathSrc;        // 006dd5c0  source dir for FindPath
extern char g_abMovVisited[8];    // 006dc700  pathfinding visited flags
extern char g_abMovPathBuf[8];    // 006dd5c8  pathfinding path scratch buffer
extern int  g_nMovPathLen;        // 006dd598  path length from FindPath
extern int  g_anMovWaypointAnims[8];   // 006de9e0  per-dir waypoint anim IDs
extern int  g_anMovSecondaryAnims[8];  // 006dd578  per-dir secondary anim IDs

// --- Navigation-graph globals (per active zone) ---
extern int  g_anMovDistLUT[10000]; // 006dea60  int(sqrt(i)) distance LUT
extern int  g_nMovZoneCount;       //           number of navigation zones
extern int  g_nMovNodeCount;       //           active zone waypoint count
extern int  g_nMovEdgeCount;       //           active zone edge count
extern int* g_pMovEdges;           //           active zone edge table {a,b,w}*N
extern int  g_nMovActiveZone;      //           currently selected zone index
extern int  g_nMovSurfOffsetX;     //           surface lock X offset
extern int  g_nMovSurfOffsetY;     //           surface lock Y offset
extern int  g_nMovResBase;         // 004d5388  resource id base for errors

// --- Functions ---
void Mov_BuildDistLUT(void);
void Mov_SelectZone(int nZone);
void Mov_LockSurfaceAt(int nX, int nY, int a3, int a4, int a5);
int  Mov_FindNearestNode(int nX, int nY);
int  Mov_FindNearestNodeInBox(int nX, int nY, int nLeft, int nTop, int nRight, int nBottom);
int  Mov_FindNeighborByDir(int nDir);
int  Mov_CompareEdgeDir(int nX, int nY, int tx, int ty, int ax, int ay, int bx, int by);
void Mov_PathfindTo(int nTarget);

void Mov_Update(void);
void Mov_StartPath(void);
void Mov_Reset(void);
void Mov_FreezePos(void);
void Mov_RestorePos(void);
void Mov_SetFollower(int nCharIdx);
int  Mov_IsMoveDone(void);
void Mov_TracePos(void);
void Mov_EnableSounds(void);
void Mov_DisableSounds(void);
void Mov_SetDir(char nDir);
void Mov_ClearDir(void);
void Mov_GetPathHead(void);
void Mov_WalkTo(void);
void Mov_InitChar(int nNodeIdx, int nCharIdx);
int  Mov_FlipDir(int nDir);
int  Mov_WrapDir(int nDir);
void Mov_AddDirSuffix(char* pszName, int nDir);
void Mov_RestoreMoves(int nCharName);
void Mov_FindPath(int nTargetDir);
void Mov_FreeMoves(void);
void Mov_SelectCarryAnim(void);
void Mov_TurnAround(int nDir);
void Mov_TurnOnTheFly(int nDir);
void Mov_FacePoint(int nX, int nY);
