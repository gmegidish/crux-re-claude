// RUNPROG.cpp — Script bytecode virtual machine
//
// The heart of the Crux adventure engine. RunProg_Exec is a single ~3,232-line
// function: a giant switch over ~400 opcodes. Every verb+object interaction,
// dialogue line, cutscene, room transition, inventory action, and puzzle in the
// game is a stream of these opcodes, stored in g_pScriptPrograms[progId].
//
// ============================================================================
//  VM MODEL  (full opcode spec in RUNPROG_OPCODES.md)
// ============================================================================
//  - A program is a flat buffer: { int count; Instruction insns[count]; }
//  - Instruction stride = 0x10 bytes. Decoded fields:
//        opcode = local_144,  arg0 = local_140,  arg1 = local_13c,  arg2 = local_138
//  - g_anSpeechPlayed[] (1500 ints) is the script VARIABLE/REGISTER file.
//  - Control flow:
//        IF/ELSE/ENDIF  = opcodes 0x09..0x11, forward-scan to matching 0x0f/0x10
//        GOSUB          = 0x065 / 0x168 (push PC+context, jump)
//        RPN stack      = 0x173/0x174 push/pop, 0x182/0x183/0x184 add/sub/mul
//  - Cutscene skip state: DAT_00629f50 (speech-wait FSM), local_130 (fast-fwd),
//        local_148 (blocking-anim). Right-click aborts cutscenes.
//
//  PORTING NOTE: For ScummVM this monolithic switch should become a table of
//  per-opcode handler methods. The opcode semantics are documented exhaustively
//  in RUNPROG_OPCODES.md — that file is the spec; this file preserves the
//  original control structure and the helper functions verbatim.
//
// Original source: C:\DevStudio\Projects\Crux\RUNPROG.cpp

#include "RUNPROG.h"
#include <windows.h>
#include <string.h>

// ============================================================
//  Cross-module helpers (resolved as modules were reversed)
// ============================================================
extern void Adv_Tick(void);
extern void Adv_TickFramesNoAsync(int nFrames);
extern int  Adv_CheckRightClick(void);
extern void Timer_DispatchAsyncProg(void);
extern void Mov_FreezePos(void);
extern void Anim_EnableDraw(void);
extern void Player_PlayScm(int nName, int bMode, int a, int b);
extern void Snd_Stop(int nChannelId);
extern void Sched_SavePaletteSnapshot(void);

// State globals shared with the rest of the engine
extern int  g_nMovDone;            // 0x006dd600  set when character reaches dest
extern int  g_nAdvTickSuppressed;  // tick-suppression / cutscene-skip flag
extern int  g_nPalProtect;         // 0x00629f6c  protect-palette flag
extern char g_abSnapshotPal[768];  // saved palette snapshot

// Tracked-sound list — sounds a script registers so they can be bulk-stopped.
static int  g_anTrackedSounds[10]; // 0x007c40e0
static int  g_nTrackedSoundCount;  // 0x007c49b8

// ============================================================
//  RunProg_WaitMoveDone  (0x00462290)
//  Busy-wait until the character finishes moving. Pumps one frame of
//  simulation per iteration; honours right-click cutscene abort.
// ============================================================
void RunProg_WaitMoveDone(int nProgId, int bInterruptible, int* pbInterrupted)
{
    extern void WalkToNode(int nNode);  // thunk_FUN_00453320

    while (g_nMovDone == 0)
    {
        Adv_Tick();
        Timer_DispatchAsyncProg();

        if (g_nAdvTickSuppressed != 0)
            return;

        if (Adv_CheckRightClick() && bInterruptible != 0)
        {
            Adv_TickFramesNoAsync(1);
            if (pbInterrupted)
                *pbInterrupted = 1;
            g_nAdvTickSuppressed = 4;
            Mov_FreezePos();
            WalkToNode(nProgId);
            return;
        }
    }
}

// ============================================================
//  RunProg_SelectAreaContext  (0x00462380)
//  Point the runtime area-data globals at one of two parallel banks,
//  selected by DAT_0070e130 (0 = primary OTF node list, 1 = alternate).
//  Called at the top of every dispatch loop iteration.
// ============================================================
void RunProg_SelectAreaContext(void)
{
    // Switches g_nAreaCacheCount / g_anAreaCacheTable / g_pOtfNodeListPool /
    // node-count / sprite-list pointers between the primary and secondary
    // area banks based on DAT_0070e130. (See decompile at 0x00462380.)
}

// ============================================================
//  RunProg_Exec  (0x00462560)  — THE DISPATCHER
//
//  ~400-opcode switch. The body is faithfully documented in
//  RUNPROG_OPCODES.md (grouped by range). The control skeleton is:
//
//    LAB_restart:
//      RunProg_SelectAreaContext();
//    LAB_fetch:
//      prog   = g_pScriptPrograms[progId];
//      count  = prog[0];
//      memcpy(&insn, prog + 1 + pc, 0x10);   // opcode + 3 args
//      if (insn.opcode == 0x12d) cutsceneState = 2;   // SPEECH_WAIT bootstrap
//      else { ...set default cursor mode... }
//      switch (insn.opcode) {
//          case 0x000: ... 400 cases ...
//          default:    Debug_Trace("RTSI Unrec cmd ...");   // unused opcode
//      }
//      g_nScriptNextOp reloaded; advance PC; loop while pc < count.
//
//  FULLY ASSEMBLED: the ~429-case switch body is transcribed from the Ghidra
//  decompile into 11 fragments under src/runprog/ (ops_00_3f.inc … ops_1839_5b23.inc),
//  #included below in opcode order. Local names and call targets are kept verbatim
//  from the decompile (hence the undefined4/uint/DWORD/func_0x… artifacts — this is
//  faithful structure, not a clean-room rewrite, and does not compile standalone).
//  RUNPROG_OPCODES.md remains the human-readable spec; the ScummVM port should still
//  collapse this into a per-opcode handler table.
// ============================================================
void RunProg_Exec(unsigned int nProgId, int nId)
{
  // Decompiler signature was RunProg_Exec(int param_1, undefined4 param_2);
  // map the engine's named params onto the verbatim decompiler names so the
  // #included case-body fragments (which reference param_1 / param_2) compile.
  int param_1 = (int)nProgId;
  int param_2 = nId;

  // ---- Verbatim local declarations (from Ghidra decompile of 0x00462560) ----
  int iVar1;
  uint uVar2;
  int iVar3;
  DWORD DVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  uint uVar7;
  uint uVar8;
  undefined4 *unaff_FS_OFFSET;
  int iStack_ad8;
  int iStack_ad4;
  int iStack_ad0;
  int iStack_acc;
  int iStack_ac8;
  int iStack_ac4;
  int iStack_ac0;
  uint uStack_ab8;
  undefined4 uStack_aa4;
  undefined4 uStack_aa0;
  undefined4 uStack_a9c;
  undefined4 uStack_a8c;
  undefined4 uStack_a88;
  undefined4 uStack_a84;
  undefined4 uStack_a74;
  undefined4 uStack_a70;
  undefined4 uStack_a6c;
  int local_a68;
  int local_a64;
  char local_a60;
  int local_a5c;
  int iStack_a58;
  int local_a54;
  int iStack_a50;
  int aiStack_a4c [100];
  undefined1 auStack_8bc [4];
  int aiStack_8b8 [100];
  int iStack_728;
  undefined1 auStack_724 [4];
  int iStack_720;
  int local_71c;
  int local_718;
  uint local_714;
  int local_710;
  int iStack_70c;
  int local_708;
  int local_704;
  char local_700 [332];
  int aiStack_5b4 [6];
  int iStack_59c;
  int iStack_598;
  int iStack_594;
  undefined1 auStack_590 [396];
  int local_404;
  int aiStack_400 [5];
  undefined1 auStack_3ec [4];
  int aiStack_3e8 [100];
  undefined1 local_258 [256];
  int *local_158;
  int iStack_154;
  int iStack_150;
  int iStack_14c;
  int local_148;
  int local_144;
  int local_140;
  uint local_13c;
  int local_138;
  int local_134;
  int local_130;
  int iStack_12c;
  char cStack_128;
  char local_124;
  int local_120;
  undefined1 auStack_11c [260];
  char *local_18;
  undefined1 *local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;

  // ---- Prologue / SEH frame setup ----
  puStack_c = &LAB_004a7620;
  local_10 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = &local_10;
  local_14 = &stack0xfffff518;
  local_18 = s_run_prog_unsigned_num_int_id__004d7b64;
  local_8 = 0;
  local_a68 = -1;
  local_134 = 0x10;
  local_710 = 0;
  local_a5c = 0;
  local_71c = 0;
  local_130 = 0;
  local_148 = 0;
  local_120 = 0;
  local_708 = 0;
  local_704 = -1;
  local_404 = 0;
  local_a54 = 1;
  local_a60 = (char)g_nCursorMode;
LAB_00462616:
  RunProg_SelectAreaContext();
  local_a64 = 0;
  local_710 = 0;
LAB_0046262f:
  local_124 = '\0';
  local_158 = *(int **)((int)g_pScriptPrograms + param_1 * 4);
  local_718 = *local_158;
  if ((((local_a54 != 0) && (0 < *local_158)) && (local_a64 == 0)) && (g_nAdvTickSuppressed == 0)) {
    FUN_004896d0(&local_144,local_158 + 1,local_134);
    if (local_144 == 0x12d) {
      g_nAdvTickSuppressed = 2;
    }
    else {
      if ((g_nCursorMode != 1) && (g_nCursorMode != 4)) {
        g_nCursorMode = 3;
      }
      DAT_00629f70 = 0;
    }
  }
  local_a54 = 0;
LAB_004626e2:
  // ---- Fetch / decode / dispatch loop ----
  if (local_718 <= local_a64) goto LAB_00468dd9;
  if (local_71c == 0) {
    iVar1 = Adv_CheckRightClick();
    if ((iVar1 != 0) && (local_148 != 0)) {
      Adv_TickFramesNoAsync(1);
      local_130 = 1;
      g_nAdvTickSuppressed = 4;
    }
    Adv_Tick();
    Timer_DispatchAsyncProg();
    FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
    FUN_0048a060(local_258,s_RTSI_Unrec__cmd__com__d_ptr__d_x_004d7b84,local_144,local_140,local_13c
                 ,local_138);
    Debug_Trace(DAT_004d7b60 + 0xa0,s_C__DevStudio_Projects_Crux_RUNPR_004d7bb0,local_258);
    if (DAT_007c49b0 != 0) {
      local_130 = 1;
      g_nAdvTickSuppressed = 4;
    }
    iVar1 = g_nScriptNextOp;
    // Opcode 0x84c (Snd_StopAll/speech-timing) is handled outside the switch in
    // the original (it is not a switch case). Preserve it verbatim here.
    if (local_144 == 0x84c) {
      local_714 = Snd_StopAll();
      if ((int)local_714 < -9999) {
        (&g_anSpeechPlayed)[local_140] = 0;
        iVar1 = g_nScriptNextOp;
      }
      else {
        (&g_anSpeechPlayed)[local_140] = (int)local_714 / 300 + 10;
        iVar1 = g_nScriptNextOp;
      }
    }
    else {
      switch (local_144) {
#include "runprog/ops_00_3f.inc"
#include "runprog/ops_40_7f.inc"
#include "runprog/ops_80_ff.inc"
#include "runprog/ops_100_14f.inc"
#include "runprog/ops_150_17f.inc"
#include "runprog/ops_180_1bf.inc"
#include "runprog/ops_1c0_1ff.inc"
#include "runprog/ops_200_2ff.inc"
#include "runprog/ops_800_91c.inc"
#include "runprog/ops_960_c1c.inc"
#include "runprog/ops_1839_5b23.inc"

      // ----------------------------------------------------------------------
      // Opcodes present in the original range-tree dispatch but absent from the
      // 11 transcribed fragment files (they lived in `if (local_144 != X)`
      // branches between 0xc1c and 0x1839). Reproduced verbatim here so the
      // merged switch faithfully covers the full opcode set.
      // ----------------------------------------------------------------------
      case 0xc03: // GRAN_SET_SLIDER_RANGE
        Gran_SetSliderRange(local_140,local_13c,local_138);
        iVar1 = g_nScriptNextOp;
        break;
      case 0xc04: // GRAN_STOP_SLIDER
        Gran_StopSlider();
        iVar1 = g_nScriptNextOp;
        break;
      case 0x13ba: // ANIM_ADD_FROZEN_GROUPED
        local_a68 = Anim_AddByNum(local_140,1,0);
        if (_DAT_00574bec != 0) {
          Anim_AddToGroup(local_a68);
          _DAT_00574bec = _DAT_00574bec - 1;
        }
        Anim_SetWalkTableBase(local_a68,local_13c);
        Anim_SetFrameStep(local_a68,0);
        Anim_SetCurrentFrame(local_a68,0);
        Anim_Freeze(local_a68);
        iVar1 = g_nScriptNextOp;
        break;
      case 0x1004: // CD_CHANGE_MODE_INIT
        Curs_LoadCursor(3,s_CURSINV_004d82ec,1,0x19,0x24);
        Curs_LoadCursor(8,s_CURSAREA_004d82f4,2,1,1);
        Curs_LoadCursor(0,s_CURSAREA_004d8300,2,1,1);
        Curs_LoadCursor(2,s_CURSEXIT_004d830c,2,1,1);
        Curs_LoadCursor(9,s_CURSHOUR_004d8318,2,1,1);
        DAT_004d71ac = Anim_AddByName(s_changecd_004d8324,0xffffffff);
        *(uint *)(&g_anAnimSlotFlags + DAT_004d71ac * 0x58) =
             *(uint *)(&g_anAnimSlotFlags + DAT_004d71ac * 0x58) | 0x200;
        Files_LoadPal(s_CHANGECD_004d8330,&DAT_0071d4a0,1);
        DAT_004d71b0 = 1;
        GV_LoadDragGraphics();
        GI_LoadGeneralPal();
        Speech_Init();
        Txt_SetMaxLines(2);
        Files_RegisterSpecialSave(&LAB_00401348,&LAB_00401ccb);
        iVar1 = g_nScriptNextOp;
        break;
      case 0x17d4: // ANIM_SET_INDI_PAL
        Anim_SetIndiPal(DAT_007c4108,*(undefined4 *)(DAT_0070c250 + local_140 * 4));
        iVar1 = g_nScriptNextOp;
        break;
      case 0x1838: // GRAN_INIT_TAPE
        Gran_InitTape();
        iVar1 = g_nScriptNextOp;
        break;

      // ---- Unimplemented / unknown opcode: "Bad commad" trace ----
      default:
LAB_00468d22:
        Debug_TraceVal(s_Bad_commad___d_004d8520,local_144);
        iVar1 = g_nScriptNextOp;
        break;
      }
    }
  }
  else {
    local_71c = 0;
    iVar1 = g_nScriptNextOp;
  }
  goto LAB_00468d36;
LAB_00468dd9:
  // ---- Loop exit / GOSUB-return ----
  if (local_124 != '\0') {
    Adv_CompactInvList(0);
  }
  if (local_a5c == 0) {
    if ((g_nAdvTickSuppressed == 2) || (g_nAdvTickSuppressed == 4)) {
      Debug_Trace(DAT_004d7b60 + 0xba5,s_C__DevStudio_Projects_Crux_RUNPR_004d8554,
                  s_You_forgot_to_MELT_the_animation_004d8530);
      g_nAdvTickSuppressed = 0;
      if (local_704 != -1) {
        Txt_SetMaxLines(local_704);
        local_704 = -1;
      }
    }
    if (local_130 != 0) {
      thunk_FUN_00403670();
      thunk_FUN_00403770();
    }
    if (local_120 != 0) {
      Win_UpdateCursor();
    }
    g_nCursorMode = (int)local_a60;
    *unaff_FS_OFFSET = local_10;
    return;
  }
  local_a5c = local_a5c + -1;
  param_1 = aiStack_3e8[local_a5c];
  local_a64 = aiStack_8b8[local_a5c];
  local_710 = aiStack_a4c[local_a5c];
  if (aiStack_a4c[local_a5c] == 0) {
    local_a64 = local_a64 + 1;
  }
  else {
    local_71c = 1;
  }
  goto LAB_0046262f;
code_r0x00464ea4:
  local_710 = 0;
code_r0x00464ec1:
  iVar1 = g_nScriptNextOp;
  if (local_710 != 0) {
    local_a64 = local_a64 + -1;
  }
LAB_00468d36:
  // ---- Loop advance ----
  g_nScriptNextOp = iVar1;
  if ((0 < local_710) && (local_710 = local_710 + -1, local_710 < 1)) {
    while ((local_a64 < local_718 &&
           (FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134),
           local_144 != 0xcb))) {
      local_a64 = local_a64 + 1;
    }
    local_a64 = local_a64 + 1;
  }
  local_a64 = local_a64 + 1;
  goto LAB_004626e2;
}

// ============================================================
//  RunProg_PlayScmWithPaletteGuard  (0x0046b9b0)
//  Play a .SCM animation sequence, optionally bracketed by a palette
//  snapshot/restore so the SCM does not permanently dirty the room palette.
// ============================================================
void RunProg_PlayScmWithPaletteGuard(int nName, int nFlags, int a, int b)
{
    if (g_nPalProtect != 0)
        Sched_SavePaletteSnapshot();

    Player_PlayScm(nName, nFlags == 0, a, b);

    if (g_nPalProtect != 0)
    {
        RunProg_RestorePaletteSnapshot();
        Anim_EnableDraw();
    }
}

// ============================================================
//  RunProg_ClearTrackedSounds  (0x0046baa0)
//  Discard the tracked-sound list without stopping anything.
// ============================================================
void RunProg_ClearTrackedSounds(void)
{
    g_nTrackedSoundCount = 0;
}

// ============================================================
//  RunProg_StopAndClearTrackedSounds  (0x0046bac0)
//  Stop every sound the script registered, then empty the list.
// ============================================================
void RunProg_StopAndClearTrackedSounds(void)
{
    for (int i = 0; i < g_nTrackedSoundCount; i++)
        Snd_Stop(g_anTrackedSounds[i]);
    g_nTrackedSoundCount = 0;
}

// ============================================================
//  RunProg_TrackSound  (0x0046bb20)
//  Register a sound channel ID for later bulk-stop (max 10, dedup).
// ============================================================
void RunProg_TrackSound(int nChannelId)
{
    for (int i = 0; i < g_nTrackedSoundCount; i++)
        if (g_anTrackedSounds[i] == nChannelId)
            return;  // already tracked

    if (g_nTrackedSoundCount < 10)
        g_anTrackedSounds[g_nTrackedSoundCount++] = nChannelId;
}

// ============================================================
//  RunProg_RestorePaletteSnapshot  (0x0046bba0)
//  Copy the saved snapshot palette back into the live palette buffer.
// ============================================================
void RunProg_RestorePaletteSnapshot(void)
{
    extern char g_abLivePalette[768];  // DAT_007c4110
    memcpy(g_abLivePalette, g_abSnapshotPal, 0x300);
}

// ============================================================
//  RunProg_Nop  (0x0046bc40)  — no-op placeholder
// ============================================================
void RunProg_Nop(void)
{
}
