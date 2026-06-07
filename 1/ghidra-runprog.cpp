// ghidra-runprog.cpp — RAW Ghidra decompilation of RunProg_Exec (CRUX.EXE @0x00462560).
// Reference only — NOT compiled (not in the Makefile SRCS list). Symbols are Ghidra's
// (DAT_*, FUN_*, local_*, thunks); use it to look up opcode `case 0x...:` handlers when
// porting into RunProg.cpp. Regenerate via mcp__ghidra decompile_function 0x00462560.
// The giant switch dispatches on the instruction opcode (`in.op`).


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void RunProg_Exec(int param_1,undefined4 param_2)

{
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
    if (local_144 < 0x84d) {
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
        switch(local_144) {
        case 1:
          if (local_130 == 0) {
            local_a68 = Anim_AddByNum(local_140,0,0);
            Anim_SetWalkTableBase(local_a68,local_13c);
            iVar1 = g_nScriptNextOp;
          }
          else {
            GI_SetDrawMode(0);
            func_0x00401000(local_140);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 2:
          break;
        case 3:
          DAT_00712838 = 1;
          DAT_0070ded0 = *(undefined4 *)(_DAT_0070b5c4 + local_140 * 4);
          break;
        case 4:
          (&g_anSpeechPlayed)[local_140] = local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 5:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 6:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + -1;
          iVar1 = g_nScriptNextOp;
          break;
        case 7:
          for (local_714 = 0; (int)local_714 < g_nAreaNodeCount; local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][5] == local_140) &&
               (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 != 0)) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            (ushort)(byte)(&g_pAreaNodeTable)[local_714][4]);
              local_120 = 1;
            }
          }
          Win_UpdateCursor();
          iVar1 = g_nScriptNextOp;
          break;
        case 8:
          for (local_714 = 0; (int)local_714 < g_nAreaNodeCount; local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][5] == local_140) &&
               (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 != 1)) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            CONCAT11(1,(char)(&g_pAreaNodeTable)[local_714][4]));
              local_120 = 1;
            }
          }
          Win_UpdateCursor();
          iVar1 = g_nScriptNextOp;
          break;
        case 9:
          if ((&g_anSpeechPlayed)[local_140] <= (int)local_13c) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 10:
          if ((&g_anSpeechPlayed)[local_140] != local_13c) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0xb:
          if ((int)local_13c <= (&g_anSpeechPlayed)[local_140]) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0xc:
          iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                               s__current_004d7bd8);
          if (iVar1 == 0) {
            iStack_12c = DAT_007d67b4;
          }
          else {
            iStack_12c = local_140;
          }
          uVar5 = func_0x00401f46(iStack_12c);
          func_0x00401415(uVar5);
          func_0x00401884(9999,0);
          local_124 = '\x01';
          iVar1 = g_nScriptNextOp;
          break;
        case 0xd:
          iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                               s__current_004d7be4);
          if (iVar1 == 0) {
            iStack_12c = DAT_007d67b4;
          }
          else {
            iStack_12c = local_140;
          }
          if ((-1 < DAT_00629c08) &&
             (iVar1 = func_0x00401f46(iStack_12c),
             *(int *)(&DAT_0070e458 + DAT_00629c08 * 4) == iVar1)) {
            func_0x0040108c();
            DAT_00629c08 = -1;
            local_a60 = '\0';
            g_nCursorMode = 3;
            DAT_007d5a80 = 0xffffffff;
          }
          uVar5 = func_0x00401f46(iStack_12c);
          func_0x00401ea1(uVar5);
          local_124 = '\x01';
          iVar1 = g_nScriptNextOp;
          break;
        case 0xe:
          if ((&g_anSpeechPlayed)[local_140] == local_13c) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0xf:
          break;
        case 0x10:
          iStack_720 = 0;
          while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
            FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
            if (local_144 == 0xf) {
              iVar1 = g_nScriptNextOp;
              if (iStack_720 == 0) break;
              iStack_720 = iStack_720 + -1;
            }
            else {
              iVar1 = func_0x00401a91(local_144);
              if (iVar1 != 0) {
                iStack_720 = iStack_720 + 1;
              }
            }
          }
          break;
        case 0x11:
          iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                               s__current_004d7bf0);
          if (iVar1 == 0) {
            iStack_12c = DAT_007d67b4;
          }
          else {
            iStack_12c = local_140;
          }
          iVar3 = func_0x00401f46(iStack_12c);
          iVar1 = g_nScriptNextOp;
          if (iVar3 != DAT_007d67b4) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x12:
          break;
        case 0x13:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7bfc);
          if (iVar1 == 0) {
            func_0x004018fc(*(undefined4 *)(&g_anAnimSlotNum + local_a68 * 0x58));
          }
          else {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
            func_0x004018fc(local_140);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            if (local_130 == 0) {
              Anim_MarkForDump(local_a68);
              iVar1 = g_nScriptNextOp;
            }
            else {
              func_0x004014c9(local_a68);
              Anim_Free(local_a68);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x14:
          if ((DAT_00629f58 == 0) && (g_nSndSubtitleOnly == 0)) {
            FUN_004895e0(local_700,*(undefined4 *)(_DAT_0070dec0 + local_140 * 4));
            FUN_0049def0(local_700);
            iVar3 = _strcmp(local_700,&DAT_00629880);
            iVar1 = g_nScriptNextOp;
            if (iVar3 != 0) {
              FUN_004895e0(&DAT_00629880,local_700);
              Thm_Play(local_700,0);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x15:
          if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
            Debug_Trace(DAT_004d7b60 + 0x1ee,s_C__DevStudio_Projects_Crux_RUNPR_004d7c14,
                        s_SOUND_command_004d7c04);
            Fx_PlayChar(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(3);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x16:
          break;
        case 0x17:
          break;
        case 0x18:
          if (local_130 != 0) {
            Mov_FreezePos();
          }
          local_714 = func_0x00401f6e(local_13c,local_138);
          Mov_PathfindTo(local_714);
          iVar1 = g_nScriptNextOp;
          if (local_a64 + 1 < local_718) {
            func_0x0040191a(local_714,local_148,&local_130);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x19:
          local_a68 = Anim_AddByNum(local_140,1,0);
          if (DAT_00574bec != 0) {
            Anim_AddToGroup(local_a68);
            DAT_00574bec = DAT_00574bec - 1;
          }
          Anim_SetWalkTableBase(local_a68,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a:
          if (g_nSndSubtitleOnly == 0) {
            Theme_StopMusic();
            FUN_004895e0(&DAT_00629880,0x7c49bc);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1b:
          break;
        case 0x1c:
          iVar1 = FUN_00489cf0();
          DVar4 = timeGetTime();
          (&g_anSpeechPlayed)[local_140] = (DVar4 * iVar1) % local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1d:
          Mov_FreezePos();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1e:
          func_0x0040105a();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1f:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7c7c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            if (local_130 == 0) {
              Anim_SetStopFrame(local_a68,*(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1);
              iVar1 = g_nScriptNextOp;
              if (g_nAdvTickSuppressed == 0) {
                do {
                  iVar3 = Anim_IsAtStopFrame(local_a68);
                  iVar1 = g_nScriptNextOp;
                  if (iVar3 != 0) goto LAB_00468d36;
                  Adv_Tick();
                  Timer_DispatchAsyncProg();
                  iVar1 = Adv_CheckRightClick();
                } while ((iVar1 == 0) || (local_148 == 0));
                Adv_TickFramesNoAsync(1);
                local_130 = 1;
                g_nAdvTickSuppressed = 4;
                iVar1 = g_nScriptNextOp;
              }
            }
            else {
              Bani_Noop(s_QuickRun_004d7c84);
              Anim_SetCurrentFrame(local_a68,*(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x20:
          if (local_130 == 0) {
            cStack_128 = (char)g_nCursorMode;
            g_nCursorMode = 2;
            do {
              while( true ) {
                Adv_CursorHandler(auStack_3ec,aiStack_400,auStack_8bc,auStack_724);
                iVar1 = func_0x00401258();
                if (iVar1 != 0) {
                  Adv_GetVerb(auStack_3ec,aiStack_400,auStack_8bc);
                }
                if (aiStack_400[0] != 8) break;
                Timer_DispatchAsyncProg();
              }
            } while (((aiStack_400[0] != 0) && (aiStack_400[0] != 1)) && (aiStack_400[0] != 2));
            Win_UpdateCursor();
            DAT_007d5b8c = g_nMouseButtons;
            g_nCursorMode = (int)cStack_128;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x21:
          break;
        case 0x22:
          break;
        case 0x23:
          break;
        case 0x24:
          break;
        case 0x25:
          break;
        case 0x26:
          if (g_nSndSubtitleOnly == 0) {
            func_0x00402072();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x27:
          if (g_nSndSubtitleOnly == 0) {
            func_0x00401d7f();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x28:
          break;
        case 0x29:
          break;
        case 0x2a:
          break;
        case 0x2b:
          if (g_nSndSubtitleOnly == 0) {
            func_0x0040178a();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x2c:
          if (g_nSndSubtitleOnly == 0) {
            func_0x004010b9();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x2d:
          if ((local_130 == 0) && (g_nAdvTickSuppressed == 0)) {
            do {
              iVar3 = func_0x00401875(3);
              iVar1 = g_nScriptNextOp;
              if (iVar3 == 0) goto LAB_00468d36;
              Adv_Tick();
              Timer_DispatchAsyncProg();
              iVar1 = Adv_CheckRightClick();
            } while ((iVar1 == 0) || (local_148 == 0));
            Adv_TickFramesNoAsync(1);
            local_130 = 1;
            g_nAdvTickSuppressed = 4;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x2e:
          g_nMovCarryHint = 7;
          break;
        case 0x2f:
          break;
        case 0x30:
          g_nMovCarryHint = func_0x004020a4(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x31:
          g_nMovCarryHint = 0;
          break;
        case 0x32:
          FUN_004895e0(&DAT_00629b08,s_entry_004dc350);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x33:
          DAT_00712838 = 2;
          break;
        case 0x34:
          func_0x00401451();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x35:
          func_0x00401663();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x36:
          Adv_CompactInvList(0);
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        case 0x39:
          Snd_Stop(2);
          Snd_Stop(3);
          Snd_Stop(1);
          Snd_Stop(0);
          SndMem_Init();
          FUN_004895e0(&DAT_00629880,0x7c49c0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x3a:
          if (local_13c == 1000) {
            DAT_00629da0 = -1;
          }
          local_714 = 0;
          while (((int)local_714 < g_nAreaNodeCount &&
                 ((&g_pAreaNodeTable)[local_714][0x25] != local_13c + 0x1e))) {
            local_714 = local_714 + 1;
          }
          if ((int)local_714 < g_nAreaNodeCount) {
            DAT_00629da0 = *(&g_pAreaNodeTable)[local_714];
            DAT_00629da8 = (&g_pAreaNodeTable)[local_714][1];
            DAT_00629da4 = (&g_pAreaNodeTable)[local_714][2];
            DAT_00629dac = (&g_pAreaNodeTable)[local_714][3];
          }
          break;
        case 0x3b:
          if (local_124 != '\0') {
            Adv_CompactInvList(0);
          }
          param_1 = local_140;
          goto LAB_00462616;
        case 0x3c:
          iRam007c4998 = local_140;
          break;
        case 0x3d:
          local_a68 = Anim_AddByNum(local_140,0,0);
          Anim_SetWalkTableBase(local_a68,local_13c);
          Anim_SetCompletionCallback(local_a68,iRam007c4998,1,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x3e:
          if (local_130 == 0) {
            local_a68 = Anim_AddByNum(local_140,1,0);
            Anim_SetWalkTableBase(local_a68,local_138);
            Anim_SetCompletionCallback(local_a68,iRam007c4998,local_13c,0xffffffff);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x3f:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7c90);
          if (iVar1 == 0) {
            DAT_007c4108 = local_a68;
          }
          else {
            DAT_007c4108 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (DAT_007c4108 == -1) {
            Debug_Trace(DAT_004d7b60 + 0x341,s_C__DevStudio_Projects_Crux_RUNPR_004d7cb4,
                        s_STANI_couldn_t_find_ani___s_004d7c98,
                        *(undefined4 *)(DAT_0070c24c + local_140 * 4));
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x40:
        case 0x2c2:
          if (0x13 < g_nAreaSpriteCount) {
            Err_BadResEntry(DAT_004d7b60 + 0x35f,s_C__DevStudio_Projects_Crux_RUNPR_004d7cf4,
                            s_Too_many_moving_areas_004d7cdc);
          }
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x363,s_C__DevStudio_Projects_Crux_RUNPR_004d7d30,
                            s_No_stani_to_LINK_to_004d7d1c);
          }
          iStack_150 = func_0x0040116d(local_140);
          (&g_anAreaSpriteList_nodeId)[g_nAreaSpriteCount * 8] = DAT_007c4108;
          (&g_anAreaSpriteList_areaId)[g_nAreaSpriteCount * 8] = iStack_150;
          if (local_144 == 0x40) {
            (&g_anAreaSpriteList_flags)[g_nAreaSpriteCount * 8] = 0;
          }
          else {
            (&g_anAreaSpriteList_flags)[g_nAreaSpriteCount * 8] = 1;
          }
          local_714 = g_anGroupTriggerPct[g_anAnimFrameTablePrev[DAT_007c4108 * 400 + 1] * 8 + 6];
          if (((int)local_714 < 0) || (0x27f < (int)local_714)) {
            local_714 = 0;
          }
          (&g_anAreaSpriteList_offX)[g_nAreaSpriteCount * 8] =
               *(&g_pAreaNodeTable)[iStack_150] - local_714;
          local_714 = g_anGroupTriggerPct[g_anAnimFrameTablePrev[DAT_007c4108 * 400 + 1] * 8 + 7];
          if (((int)local_714 < 0) || (0x1df < (int)local_714)) {
            local_714 = 0;
          }
          (&g_anAreaSpriteList_offY)[g_nAreaSpriteCount * 8] =
               (&g_pAreaNodeTable)[iStack_150][1] - local_714;
          *(INT *)(&g_anAreaSpriteList + g_nAreaSpriteCount * 0x20) =
               (&g_pAreaNodeTable)[iStack_150][2] - *(&g_pAreaNodeTable)[iStack_150];
          (&g_anAreaSpriteList_y)[g_nAreaSpriteCount * 8] =
               (&g_pAreaNodeTable)[iStack_150][3] - (&g_pAreaNodeTable)[iStack_150][1];
          *(uint *)(&g_anAnimSlotFlags + DAT_007c4108 * 0x58) =
               *(uint *)(&g_anAnimSlotFlags + DAT_007c4108 * 0x58) | 0x1000;
          g_nAreaSpriteCount = g_nAreaSpriteCount + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x41:
          iStack_150 = func_0x0040116d(local_140);
          for (local_714 = 0;
              ((int)local_714 < g_nAreaSpriteCount &&
              ((&g_anAreaSpriteList_areaId)[local_714 * 8] != iStack_150));
              local_714 = local_714 + 1) {
          }
          iVar1 = g_nScriptNextOp;
          if (local_714 != g_nAreaSpriteCount) {
            func_0x00401c17(local_714);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x42:
          DAT_00629f70 = 0xffffffff;
          DAT_007d67b4 = 0xffffffff;
          DAT_007d5f30 = 1;
          DAT_00629c08 = 0xffffffff;
          local_a60 = 0;
          g_nCursorMode = 3;
          func_0x0040108c();
          Win_UpdateCursor();
          DAT_007d67b4 = local_140;
          DAT_00629f70 = 1;
          DAT_007d5f30 = 0;
          DAT_00629c08 = DAT_00629db4;
          func_0x004012ad();
          func_0x00401087();
          local_a60 = '\x01';
          g_nCursorMode = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x43:
          Debug_Assert(DAT_004d7b60 + 0x39f,s_C__DevStudio_Projects_Crux_RUNPR_004d7d58,0xffffffff);
          DAT_00629f70 = 0xffffffff;
          DAT_007d67b4 = -1;
          DAT_007d5f30 = 1;
          DAT_00629c08 = -1;
          local_a60 = '\0';
          g_nCursorMode = 3;
          func_0x0040108c();
          Win_UpdateCursor();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x44:
          if (local_130 == 0) {
            local_a68 = Anim_AddByName(*(undefined4 *)(DAT_0070c24c + local_140 * 4),0xffffffff);
            _DAT_0069f2a0 = g_anGroupTriggerPct[g_anAnimFrameTablePrev[local_a68 * 400 + 1] * 8 + 6]
            ;
            _DAT_0069f2a4 = g_anGroupTriggerPct[g_anAnimFrameTablePrev[local_a68 * 400 + 1] * 8 + 7]
            ;
            DAT_0069f2ac = (byte *)g_anGroupTriggerPct
                                   [g_anAnimFrameTablePrev[local_a68 * 400 + 1] * 8 + 10];
            iRam005b03a4 = (*DAT_0069f2ac + 1) / 2 + _DAT_0069f2a0;
            iRam005b03a0 = (DAT_0069f2ac[1] + 1) / 2 + _DAT_0069f2a4;
            if ((int)local_13c < 2) {
              uStack_ab8 = 1;
            }
            else {
              uStack_ab8 = local_13c;
            }
            DAT_004ca69c = uStack_ab8;
            DAT_004ca6a0 = -1;
            while (DAT_004ca6a0 < (int)DAT_004ca69c) {
              iVar1 = Adv_CheckRightClick();
              if ((iVar1 != 0) && (local_148 != 0)) {
                Adv_TickFramesNoAsync(1);
                local_130 = 1;
                g_nAdvTickSuppressed = 4;
                break;
              }
              Adv_Tick();
            }
            Adv_TickFramesNoAsync(2);
            DAT_0069f2ac = (byte *)0x0;
            DAT_004ca6a0 = 0xffffffff;
            Anim_MarkForDump(local_a68);
            iVar1 = g_nScriptNextOp;
          }
          else {
            GI_SetDrawMode(0);
            func_0x00401000(local_140);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x45:
          DAT_004c4c48 = 1;
          break;
        case 0x46:
          FUN_004895e0(0x629af8,*(undefined4 *)(DAT_0070c24c + local_140 * 4));
          DAT_004c4c48 = 2;
          uRam005b039c = local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x47:
          DAT_00629f4c = 1;
          break;
        case 0x48:
          DAT_00629f4c = 0;
          break;
        case 0x49:
          if (local_130 == 0) {
            Adv_TickFramesNoAsync(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x4a:
          DAT_004c4c48 = 3;
          break;
        case 0x4b:
          break;
        case 0x4c:
          break;
        case 0x4d:
          if (g_nSndSubtitleOnly == 0) {
            if ((int)local_13c < 1) {
              func_0x004011a4(3000);
            }
            else {
              func_0x004011a4(local_13c);
            }
            FUN_004895e0(&DAT_00629880,0x7c49c4);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x4f:
          local_a68 = Anim_AddByNum(local_140,3,0);
          Anim_SetWalkTableBase(local_a68,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x50:
          if (local_130 == 0) {
            if ((g_nAdvTickSuppressed != 0) && (local_704 == -1)) {
              local_704 = Txt_GetMaxLines();
              Txt_SetMaxLines(0xffffffff);
            }
            local_714 = func_0x004017a3(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(1);
            iVar1 = g_nScriptNextOp;
            if ((local_714 != 0) && (local_148 != 0)) {
              Adv_TickFramesNoAsync(1);
              local_130 = 1;
              g_nAdvTickSuppressed = 4;
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x51:
          break;
        case 0x52:
          break;
        case 0x54:
          Anim_SetTickMode(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x55:
          if (local_124 != '\0') {
            Adv_CompactInvList(0);
          }
          DAT_0070e130 = 0;
          switch(local_13c) {
          case 0:
            param_1 = DAT_007114e4;
            break;
          case 1:
            param_1 = DAT_007114f8;
            break;
          case 2:
            param_1 = DAT_007114fc;
            break;
          case 3:
            param_1 = DAT_00711500;
            break;
          case 4:
            param_1 = DAT_00711504;
            break;
          default:
            Err_BadResEntry(DAT_004d7b60 + 0x474,s_C__DevStudio_Projects_Crux_RUNPR_004d7d9c,
                            s_Invalid_INVCHAIN_parameter_004d7d80);
          }
          if (param_1 == -1) {
            Err_BadResEntry(DAT_004d7b60 + 0x478,s_C__DevStudio_Projects_Crux_RUNPR_004d7dec,
                            s_Couldn_t_find_INVCHAIN_target_sc_004d7dc4);
          }
          goto LAB_00462616;
        case 0x56:
          break;
        case 0x59:
          break;
        case 0x5a:
          break;
        case 0x5b:
          break;
        case 0x5c:
          break;
        case 0x5d:
          break;
        case 0x5e:
          break;
        case 0x5f:
          break;
        case 0x60:
          break;
        case 0x61:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x49b,s_C__DevStudio_Projects_Crux_RUNPR_004d7e2c,
                            s_No_stani_to_PLACEANI_004d7e14);
          }
          local_a68 = Anim_AddByNum(local_140,1,0);
          Anim_SetWalkTableBase(local_a68,local_13c);
          func_0x00401ee7(DAT_007c4108,aiStack_5b4 + 7,&iStack_a50);
          func_0x00401ee7(local_a68,aiStack_5b4 + 6,&iStack_70c);
          Anim_SetPosition(local_a68,iStack_598 - iStack_59c,iStack_a50 - iStack_70c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x62:
          break;
        case 99:
          break;
        case 100:
          break;
        case 0x65:
          if (local_124 != '\0') {
            Adv_CompactInvList(0);
          }
          aiStack_8b8[local_a5c] = local_a64;
          aiStack_3e8[local_a5c] = param_1;
          aiStack_a4c[local_a5c] = local_710;
          local_710 = 0;
          local_a5c = local_a5c + 1;
          param_1 = local_140;
          goto LAB_00462616;
        case 0x66:
          break;
        case 0x67:
          if (local_130 == 0) {
            if (g_nAdvTickSuppressed == 0) {
              func_0x004014a6();
              func_0x0040191a(local_714,local_148,&local_130);
            }
            func_0x004016a4(1);
            func_0x004017a3(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x68:
          if (local_130 == 0) {
            func_0x0040154b(local_140,local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x69:
          break;
        case 0x6a:
          break;
        case 0x6b:
          DAT_00574bec = local_13c;
          func_0x0040130c(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x6c:
        case 0x71:
          if (local_130 == 0) {
            DAT_00629dc0 = 0;
            if (local_13c == 1000) {
              DAT_00629f6c = 1;
              Adv_TickFramesNoAsync(1);
              Debug_Trace(DAT_004d7b60 + 0x502,s_C__DevStudio_Projects_Crux_RUNPR_004d7e7c,
                          *(undefined4 *)(_DAT_0070dec8 + local_140 * 4));
              RunProg_PlayScmWithPaletteGuard(*(undefined4 *)(_DAT_0070dec8 + local_140 * 4),0,0,0);
              DAT_00629f6c = 0;
            }
            else {
              RunProg_PlayScmWithPaletteGuard
                        (*(undefined4 *)(_DAT_0070dec8 + local_140 * 4),local_13c,0,local_138);
            }
            iVar1 = g_nScriptNextOp;
            if ((DAT_00629dc0 != 0) && (local_148 != 0)) {
              local_130 = 1;
              g_nAdvTickSuppressed = 4;
            }
          }
          else {
            Player_ScmInit();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x6d:
          break;
        case 0x6e:
          break;
        case 0x6f:
          for (local_714 = 0;
              ((int)local_714 < (int)DAT_0070e5e8 &&
              (*(int *)(&DAT_0070e458 + local_714 * 4) != local_140)); local_714 = local_714 + 1) {
          }
          if (local_714 == DAT_0070e5e8) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x70:
          local_718 = local_a64;
          break;
        case 0x72:
          if ((DAT_00629f58 == 0) && (g_nSndSubtitleOnly == 0)) {
            FUN_004895e0(local_700,*(undefined4 *)(_DAT_0070dec0 + local_140 * 4));
            FUN_0049def0(local_700);
            iVar3 = _strcmp(local_700,&DAT_00629880);
            iVar1 = g_nScriptNextOp;
            if (iVar3 != 0) {
              FUN_004895e0(&DAT_00629880,local_700);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x73:
          (&g_anSpeechPlayed)[local_140] = DAT_00629dc0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x74:
          break;
        case 0x75:
          break;
        case 0x76:
          if (local_130 == 0) {
            if (g_nAdvTickSuppressed == 0) {
              func_0x004014a6();
              func_0x0040191a(local_714,local_148,&local_130);
            }
            func_0x004016a4(1);
            SndMem_StartSpeech(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x77:
        case 0x78:
          Debug_Trace(DAT_004d7b60 + 0x4f2,s_C__DevStudio_Projects_Crux_RUNPR_004d7e54,
                      *(undefined4 *)(_DAT_0070dec8 + local_140 * 4));
          Player_ScmAddChar(*(undefined4 *)(_DAT_0070dec8 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 199:
          func_0x00401960(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 200:
          if ((int)local_13c < 0) {
            func_0x00401078(-local_13c);
            iVar1 = g_nScriptNextOp;
          }
          else {
            func_0x00401a7d(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0xc9:
          break;
        case 0xca:
          if (local_138 == 0) {
            local_138 = 1;
          }
          local_710 = local_138 + 1;
          local_714 = local_a64 + ((&g_anSpeechPlayed)[local_140] - local_13c) * local_138;
          if ((int)local_714 < local_a64) {
            for (; local_a64 < local_718; local_a64 = local_a64 + 1) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              if (local_144 == 0xcb) {
                local_710 = 0;
                break;
              }
            }
          }
          do {
            if ((local_718 <= local_a64) || ((int)local_714 < local_a64)) goto code_r0x00464ec1;
            FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
            if (local_144 == 0xcb) goto code_r0x00464ea4;
            local_a64 = local_a64 + 1;
          } while( true );
        case 0xcb:
          local_710 = 0;
          break;
        case 0xcc:
          break;
        case 0xcd:
          if (local_130 == 0) {
            if ((g_nAdvTickSuppressed != 0) && (local_704 == -1)) {
              local_704 = Txt_GetMaxLines();
              Txt_SetMaxLines(0xffffffff);
            }
            SndMem_StartSpeech(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0xce:
          if (local_130 == 0) {
            local_714 = SndMem_WaitSpeech(0);
            iVar1 = g_nScriptNextOp;
            if ((local_714 != 0) && (local_148 != 0)) {
              Adv_TickFramesNoAsync(1);
              local_130 = 1;
              g_nAdvTickSuppressed = 4;
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0xcf:
          if (local_130 == 0) {
            Mov_TurnAround(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          else if (local_13c == 0) {
            g_nMovCarryHint = 3;
          }
          else {
            g_nMovCarryHint = local_13c;
          }
          break;
        case 0xd0:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 == 0) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            CONCAT11(2,(char)(&g_pAreaNodeTable)[local_714][4]));
            }
          }
          break;
        case 0xd1:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 == 2) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            (ushort)(byte)(&g_pAreaNodeTable)[local_714][4]);
            }
          }
          break;
        case 0xff:
        case 0x100:
          break;
        case 0x125:
          (&g_anSpeechPlayed)[local_140] = local_404;
          iVar1 = g_nScriptNextOp;
          break;
        case 300:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x5f5,s_C__DevStudio_Projects_Crux_RUNPR_004d7eb8,
                            s_No_stani_to_SOUNDFX_004d7ea4);
          }
          Anim_SetFrameSound(DAT_007c4108,local_13c,*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),2
                            );
          SndMem_Load(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),auStack_3ec,local_13c - 1,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x12d:
          if ((local_130 == 0) &&
             (Adv_TickFramesNoAsync(1), iVar1 = g_nScriptNextOp, g_nAdvTickSuppressed == 0)) {
            g_nAdvTickSuppressed = 2;
          }
          break;
        case 0x12e:
          if (g_nAdvTickSuppressed == 2) {
            g_nAdvTickSuppressed = 0;
          }
          if (local_704 != -1) {
            Txt_SetMaxLines(local_704);
            local_704 = -1;
          }
          iVar1 = g_nScriptNextOp;
          if (local_a64 != local_718 + -1) {
            g_nCursorMode = 3;
          }
          break;
        case 0x12f:
          Sched_SavePaletteSnapshot();
          FUN_004896d0(&DAT_007c4110,g_abSnapshotPal,0x300);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x130:
          FUN_004896d0(g_abTargetPal,&DAT_007c4110,0x300);
          Anim_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x131:
          local_148 = 1;
          RunProg_ClearTrackedSounds();
          local_404 = 0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x132:
          if (local_130 != 0) {
            RunProg_StopAndClearTrackedSounds();
            thunk_FUN_00403670();
            thunk_FUN_00403770();
            local_130 = 0;
            local_404 = 1;
            g_nAdvTickSuppressed = 0;
            if (local_704 != -1) {
              Txt_SetMaxLines(local_704);
              local_704 = -1;
            }
          }
          local_148 = 0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x133:
          local_a68 = Anim_AddByNum(local_140,1,0xffffffff);
          *(uint *)(&g_anAnimSlotFlags + local_a68 * 0x58) =
               *(uint *)(&g_anAnimSlotFlags + local_a68 * 0x58) & 0xfffffff7;
          Mov_SetFollower(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x134:
          if ((local_130 == 0) && (g_nAdvTickSuppressed == 0)) {
            do {
              iVar3 = func_0x004013d9();
              iVar1 = g_nScriptNextOp;
              if (iVar3 != 0) goto LAB_00468d36;
              Adv_Tick();
              Timer_DispatchAsyncProg();
              iVar1 = Adv_CheckRightClick();
            } while ((iVar1 == 0) || (local_148 == 0));
            Adv_TickFramesNoAsync(1);
            local_130 = 1;
            g_nAdvTickSuppressed = 4;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x135:
          Mov_SetFollower(0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x136:
          local_a68 = Anim_AddByNum(local_140,1,0);
          if (DAT_00574bec != 0) {
            Anim_AddToGroup(iStack_150);
          }
          Anim_SetWalkTableBase(local_a68,0);
          Anim_SetPosition(local_a68,local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x137:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7ee0);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetWalkTableBase(local_a68,local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x13b:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7ee8);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if ((local_130 == 0) && (local_a68 != -1)) {
            if ((int)local_13c < *(int *)(&g_anAnimFrameCount + local_a68 * 4)) {
              local_714 = Anim_SetStopFrame(local_a68,local_13c);
              iVar1 = g_nScriptNextOp;
              if ((local_714 != 0xffffffff) && (g_nAdvTickSuppressed == 0)) {
                do {
                  iVar3 = Anim_IsAtStopFrame(local_714);
                  iVar1 = g_nScriptNextOp;
                  if (iVar3 != 0) goto LAB_00468d36;
                  Adv_Tick();
                  Timer_DispatchAsyncProg();
                  iVar1 = Adv_CheckRightClick();
                } while ((iVar1 == 0) || (local_148 == 0));
                Adv_TickFramesNoAsync(1);
                local_130 = 1;
                g_nAdvTickSuppressed = 4;
                iVar1 = g_nScriptNextOp;
              }
            }
            else {
              Bani_Noop(s_Frame__d_out_of_range____004d7ef0,local_13c);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x13c:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7f0c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            if (*(int *)(&g_anAnimFrameCount + local_a68 * 4) <= (int)local_13c) {
              uVar5 = func_0x00401bdb(local_a68,local_13c);
              FUN_0048a060(local_258,s_Frame_too_high_in_FREEZEANI__s___004d7f14,uVar5);
              Err_BadResEntry(DAT_004d7b60 + 0x6b1,s_C__DevStudio_Projects_Crux_RUNPR_004d7f38,
                              local_258);
            }
            Anim_SetFrameStep(local_a68,0);
            Anim_SetCurrentFrame(local_a68,local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x13d:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7f60);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetFrameStep(local_a68,1);
            *(undefined4 *)(&g_anAnimSlotTriggerFrame + local_a68 * 0x58) = 0xffffffff;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x13e:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x6d4,s_C__DevStudio_Projects_Crux_RUNPR_004d7f88,
                            s_No_stani_to_SETANIFRM_004d7f70);
          }
          iVar1 = g_nScriptNextOp;
          if (DAT_007c4108 != -1) {
            if (*(int *)(&g_anAnimFrameCount + DAT_007c4108 * 4) <= (&g_anSpeechPlayed)[local_140])
            {
              uVar5 = func_0x00401bdb(DAT_007c4108,(&g_anSpeechPlayed)[local_140]);
              FUN_0048a060(local_258,s_Frame_too_high_in_SETANIFRM__s___004d7fb0,uVar5);
              Err_BadResEntry(DAT_004d7b60 + 0x6db,s_C__DevStudio_Projects_Crux_RUNPR_004d7fd4,
                              local_258);
            }
            Anim_SetCurrentFrame(DAT_007c4108,(&g_anSpeechPlayed)[local_140]);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x13f:
          iStack_150 = g_nScriptNextOp;
          iVar1 = local_13c + 1;
          if (g_nScriptNextOp < iVar1) {
            local_714 = g_nScriptNextOp;
            g_nScriptNextOp = iVar1;
            for (; iVar1 = g_nScriptNextOp, (int)local_714 < g_nScriptNextOp;
                local_714 = local_714 + 1) {
              func_0x00401893(local_714);
            }
          }
          break;
        case 0x140:
          func_0x004019ab(local_13c,local_138,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x141:
          Adv_SetInvSlotDirect(local_13c,local_138,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x142:
          func_0x004010c3(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x143:
          func_0x00401d34(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x144:
          *(undefined4 *)(&DAT_0070e65c + local_13c * 0x230) = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x145:
          iStack_150 = 0;
          for (local_714 = 0; (int)local_714 < (int)(&DAT_0070e5e8)[local_13c * 0x8c];
              local_714 = local_714 + 1) {
            if (-1 < *(int *)(&DAT_0070e458 + local_714 * 4 + local_13c * 0x230)) {
              iStack_150 = iStack_150 + 1;
            }
          }
          (&g_anSpeechPlayed)[local_140] = iStack_150;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x146:
          iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                               s__current_004d7ffc);
          if (iVar1 == 0) {
            iStack_12c = DAT_007d67b4;
          }
          else {
            iStack_12c = local_140;
          }
          func_0x0040206d(local_13c,iStack_12c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x147:
          (&g_anSpeechPlayed)[local_140] = iRam0070decc;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x148:
          iRam0070decc = (&g_anSpeechPlayed)[local_140];
          break;
        case 0x149:
          for (local_714 = 0; (int)local_714 < 10; local_714 = local_714 + 1) {
            if (*(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) == -1) {
              *(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) = local_140;
              iVar1 = g_nScriptNextOp;
              break;
            }
          }
          break;
        case 0x14a:
          for (local_714 = 0; (int)local_714 < 10; local_714 = local_714 + 1) {
            if (*(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) == local_140) {
              *(undefined4 *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) = 0xffffffff;
              iVar1 = g_nScriptNextOp;
              break;
            }
          }
          break;
        case 0x14b:
          iStack_150 = 0;
          for (local_714 = 0; (int)local_714 < (int)(&DAT_0070e5e8)[local_13c * 0x8c];
              local_714 = local_714 + 1) {
            iVar1 = func_0x004011d6(local_13c,
                                    *(undefined4 *)
                                     (&DAT_0070e458 + local_714 * 4 + local_13c * 0x230));
            if (iVar1 != 0) {
              iStack_150 = iStack_150 + 1;
            }
          }
          (&g_anSpeechPlayed)[local_140] = iStack_150;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x14c:
          iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                               s__current_004d8008);
          if (iVar1 == 0) {
            iStack_12c = DAT_007d67b4;
          }
          else {
            iStack_12c = local_140;
          }
          iVar3 = func_0x004011d6(local_13c,iStack_12c);
          iVar1 = g_nScriptNextOp;
          if (iVar3 == 0) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x14d:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8014);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetFrameStep(local_a68,0);
            Anim_SetCurrentFrame(local_a68,*(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x14e:
          if (local_130 != 0) {
            Mov_FreezePos();
          }
          local_714 = func_0x00401712(local_13c);
          Mov_PathfindTo(local_714);
          iVar1 = g_nScriptNextOp;
          if (local_a64 + 1 < local_718) {
            func_0x0040191a(local_714,local_148,&local_130);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x14f:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d801c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x004013ac(local_a68,local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x150:
          local_a68 = Anim_AddByNum(local_140,1,0);
          if (DAT_00574bec != 0) {
            Anim_AddToGroup(iStack_150);
          }
          Anim_SetWalkTableBase(local_a68,0);
          Anim_GetFrameTopLeft(local_a68,aiStack_5b4 + 6,&iStack_70c);
          Anim_SetPosition(local_a68,local_13c - iStack_59c,local_138 - iStack_70c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x151:
          DAT_004d6650 = local_13c;
          break;
        case 0x152:
          GI_SetDrawMode(0);
          local_714 = func_0x00401915(local_13c,local_138);
          local_714 = local_714 & 0xff;
          iVar1 = func_0x00401730(local_714);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x153:
          func_0x004012da(local_13c,(&g_anSpeechPlayed)[local_140]);
          SetPal_WaitOrRealizeIfNeeded();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x154:
          iVar3 = func_0x00401f6e(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          if (iVar3 != g_nMovDestNode) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x155:
          GI_SetDrawMode(0);
          func_0x00401000(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x156:
          if (DAT_007c4108 < 0) {
            (&g_anSpeechPlayed)[local_140] = -1;
            iVar1 = g_nScriptNextOp;
          }
          else {
            iVar1 = Anim_GetCurrentFrame(DAT_007c4108);
            (&g_anSpeechPlayed)[local_140] = iVar1;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x157:
          Bani_Noop(*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x158:
        case 0x1fd:
          break;
        case 0x159:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d803c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          Anim_SetCompletionCallback(local_a68,iRam007c4998,local_13c,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15a:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d804c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          Anim_SetCompletionCallback(local_a68,0xffffffff,0xffffffff,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15b:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15c:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] - local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15d:
          Debug_Assert(DAT_004d7b60 + 0x88a,s_C__DevStudio_Projects_Crux_RUNPR_004d8054,1);
          DAT_00629f70 = 2;
          DAT_007d67b4 = local_140;
          DAT_007d5f30 = 0;
          func_0x004012ad();
          func_0x00401087();
          local_a60 = '\x01';
          g_nCursorMode = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15e:
          func_0x00401f7d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x15f:
          func_0x00401163();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x160:
          func_0x00401032();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x161:
          iVar1 = func_0x0040100a();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x162:
          iVar1 = func_0x00402081();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x163:
          iVar1 = func_0x004012df();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x164:
          func_0x004017ee((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x165:
          func_0x00401f5f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x166:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d807c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            GI_SetDrawMode(0);
            Anim_ShowFrame(local_a68,local_13c,*(undefined4 *)(&g_anAnimSlotX + local_a68 * 0x58),
                           *(undefined4 *)(&g_anAnimSlotY + local_a68 * 0x58));
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x167:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8094);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            *(uint *)(&g_anAnimSlotTriggerFrame + local_a68 * 0x58) = local_13c;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x168:
          if (local_124 != '\0') {
            Adv_CompactInvList(0);
          }
          DAT_0070e130 = 0;
          aiStack_8b8[local_a5c] = local_a64;
          aiStack_3e8[local_a5c] = param_1;
          aiStack_a4c[local_a5c] = local_710;
          local_710 = 0;
          local_a5c = local_a5c + 1;
          param_1 = iRam007c4998;
          goto LAB_00462616;
        case 0x169:
        case 0x2c3:
          if (0x13 < g_nAreaSpriteCount) {
            Err_BadResEntry(DAT_004d7b60 + 0x90d,s_C__DevStudio_Projects_Crux_RUNPR_004d80bc,
                            s_Too_many_moving_areas_004d80a4);
          }
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x911,s_C__DevStudio_Projects_Crux_RUNPR_004d80fc,
                            s_No_stani_to_LINKFULL_to_004d80e4);
          }
          iStack_150 = func_0x0040116d(local_140);
          (&g_anAreaSpriteList_nodeId)[g_nAreaSpriteCount * 8] = DAT_007c4108;
          (&g_anAreaSpriteList_areaId)[g_nAreaSpriteCount * 8] = iStack_150;
          if (local_144 == 0x169) {
            (&g_anAreaSpriteList_flags)[g_nAreaSpriteCount * 8] = 0;
          }
          else {
            (&g_anAreaSpriteList_flags)[g_nAreaSpriteCount * 8] = 1;
          }
          (&g_anAreaSpriteList_offX)[g_nAreaSpriteCount * 8] = -1;
          (&g_anAreaSpriteList_offY)[g_nAreaSpriteCount * 8] = -1;
          *(undefined4 *)(&g_anAreaSpriteList + g_nAreaSpriteCount * 0x20) = 0xffffffff;
          (&g_anAreaSpriteList_y)[g_nAreaSpriteCount * 8] = -1;
          *(uint *)(&g_anAnimSlotFlags + DAT_007c4108 * 0x58) =
               *(uint *)(&g_anAnimSlotFlags + DAT_007c4108 * 0x58) | 0x1000;
          g_nAreaSpriteCount = g_nAreaSpriteCount + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x16a:
          DAT_00629f70 = 0xffffffff;
          DAT_00629c08 = -1;
          DAT_007d5a80 = 0xffffffff;
          local_a60 = '\0';
          g_nCursorMode = 3;
          Win_UpdateCursor();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x16b:
          func_0x00401262();
          DAT_007d5f30 = 0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x16c:
          if ((DAT_00629f58 == 0) && (g_nSndSubtitleOnly == 0)) {
            FUN_004895e0(local_700,*(undefined4 *)(_DAT_0070dec0 + local_140 * 4));
            FUN_0049def0(local_700);
            iVar3 = _strcmp(local_700,&DAT_00629880);
            iVar1 = g_nScriptNextOp;
            if (iVar3 != 0) {
              FUN_004895e0(&DAT_00629880,local_700);
              Theme_MusicEvent(local_700);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x16d:
          if (local_130 == 0) {
            Adv_TickFramesNoAsync(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x16e:
          if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
            func_0x00401037(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x16f:
          Fx_StopLoop();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x170:
          func_0x004010dc((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x171:
          func_0x00401843((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x172:
          func_0x004019c4((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x173:
          if (99 < local_708 + 1) {
            Files_SetErrSource(DAT_004d7b60 + 0x99e,s_C__DevStudio_Projects_Crux_RUNPR_004d8134);
            puVar6 = (undefined4 *)Err_SetRecord3(0x28,0x7c49cc,0xffffffff);
            uStack_a74 = *puVar6;
            uStack_a70 = puVar6[1];
            uStack_a6c = puVar6[2];
            FUN_00489090(&uStack_a74,&DAT_004ab3f8);
          }
          (&iStack_594)[local_708] = (&g_anSpeechPlayed)[local_140];
          local_708 = local_708 + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x174:
          if (local_708 < 1) {
            Files_SetErrSource(DAT_004d7b60 + 0x9a6,s_C__DevStudio_Projects_Crux_RUNPR_004d815c);
            puVar6 = (undefined4 *)Err_SetRecord3(0x28,0x7c49d0,0xffffffff);
            uStack_a8c = *puVar6;
            uStack_a88 = puVar6[1];
            uStack_a84 = puVar6[2];
            FUN_00489090(&uStack_a8c,&DAT_004ab3f8);
          }
          local_708 = local_708 + -1;
          (&g_anSpeechPlayed)[local_140] = (&iStack_594)[local_708];
          iVar1 = g_nScriptNextOp;
          break;
        case 0x175:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x9bd,s_C__DevStudio_Projects_Crux_RUNPR_004d819c,
                            s_No_stani_to_OFFSETANIX_004d8184);
          }
          *(int *)(&g_anAnimSlotX + DAT_007c4108 * 0x58) = (&g_anSpeechPlayed)[local_140];
          iVar1 = g_nScriptNextOp;
          break;
        case 0x176:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x9c5,s_C__DevStudio_Projects_Crux_RUNPR_004d81dc,
                            s_No_stani_to_OFFSETANIY_004d81c4);
          }
          *(int *)(&g_anAnimSlotY + DAT_007c4108 * 0x58) = (&g_anSpeechPlayed)[local_140];
          iVar1 = g_nScriptNextOp;
          break;
        case 0x177:
          if (local_130 == 0) {
            func_0x004016fe(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x178:
          func_0x00401bc2(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x179:
          if (0xe < DAT_00629f68) {
            Files_SetErrSource(DAT_004d7b60 + 0x9df,s_C__DevStudio_Projects_Crux_RUNPR_004d8204);
            puVar6 = (undefined4 *)Err_SetRecord3(0x1d,0x4d822c,0xffffffff);
            uStack_aa4 = *puVar6;
            uStack_aa0 = puVar6[1];
            uStack_a9c = puVar6[2];
            FUN_00489090(&uStack_aa4,&DAT_004ab3f8);
          }
          *(uint *)(&DAT_00629ef0 + DAT_00629f68 * 4) = local_13c;
          *(int *)(&DAT_00629980 + DAT_00629f68 * 4) = local_140;
          DAT_00629f68 = DAT_00629f68 + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x17a:
          Snd_Stop(1);
          SndMem_StopLipsync();
          Txt_Reset();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x17b:
          iVar3 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8234);
          iVar1 = g_nScriptNextOp;
          if (iVar3 == 0) {
            _DAT_007c441c = *(int *)(&g_anAnimSlotNum + local_a68 * 0x58);
            _DAT_007c49a0 = *(undefined4 *)(&g_anAnimSlotLangId + local_a68 * 0x58);
          }
          else {
            _DAT_007c441c = local_140;
            _DAT_007c49a0 = DAT_0070e130;
          }
          break;
        case 0x17c:
          if (-1 < _DAT_007c441c) {
            local_714 = Anim_FindSlotByName(_DAT_007c441c,_DAT_007c49a0);
            iVar1 = g_nScriptNextOp;
            if (local_714 != 0xffffffff) {
              if (local_130 == 0) {
                Anim_MarkForDump(local_714);
                iVar1 = g_nScriptNextOp;
              }
              else {
                Anim_Free(local_714);
                iVar1 = g_nScriptNextOp;
              }
            }
          }
          break;
        case 0x17d:
          if (local_13c != g_nMouseBtnDownMask) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x17e:
          func_0x004013b1(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x17f:
          FUN_004896d0(g_abTargetPal,g_abSnapshotPal,0x300);
          SetPal_FadeInFromBlack();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x180:
          DAT_004c4c48 = 4;
          break;
        case 0x181:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d827c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_GetFrameTopLeft(local_a68,aiStack_5b4 + 6,&iStack_70c);
            Anim_SetPosition(local_a68,local_13c - iStack_59c,local_138 - iStack_70c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x182:
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] + aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x183:
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] - aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x184:
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] * aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x185:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8044);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          if (local_138 < -1) {
            Anim_SetCompletionCallback
                      (local_a68,iRam007c4998,local_13c,
                       *(int *)(&g_anAnimFrameCount + local_a68 * 4) + local_138);
            iVar1 = g_nScriptNextOp;
          }
          else {
            Anim_SetCompletionCallback(local_a68,iRam007c4998,local_13c,local_138);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x186:
          DAT_00712838 = 5;
          DAT_0070ded0 = *(undefined4 *)(_DAT_0070b5c4 + local_140 * 4);
          break;
        case 0x187:
          if (-1 < _DAT_007c441c) {
            local_714 = Anim_FindSlotByName(_DAT_007c441c,_DAT_007c49a0);
            iVar1 = g_nScriptNextOp;
            if (local_714 != 0xffffffff) {
              Anim_Freeze(local_714);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x188:
          if (-1 < _DAT_007c441c) {
            local_714 = Anim_FindSlotByName(_DAT_007c441c,_DAT_007c49a0);
            iVar1 = g_nScriptNextOp;
            if (local_714 != 0xffffffff) {
              Anim_ResetFreeze(local_714);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x189:
          Adv_SetDrawSuppressed(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x18a:
          Adv_SetDrawSuppressed(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x18b:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8084);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            GI_SetDrawMode(0);
            func_0x00401b8b(local_a68,local_13c,*(undefined4 *)(&g_anAnimSlotX + local_a68 * 0x58),
                            *(undefined4 *)(&g_anAnimSlotY + local_a68 * 0x58),local_138);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x18c:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d808c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            GI_SetDrawMode(0);
            iStack_728 = 100;
            iStack_a50 = 0;
            iStack_598 = 0;
            for (local_714 = 100; iVar1 = g_nScriptNextOp, 0 < (int)local_714;
                local_714 = local_714 - 1) {
              if ((int)local_714 <= iStack_728) {
                func_0x00401b8b(local_a68,local_13c,iStack_598,iStack_a50,local_714);
                iStack_598 = iStack_598 + 1 + (int)(local_714 * 0x20 + -1) / 100;
                iStack_a50 = iStack_a50 + 1 + (int)(local_714 * 0x18 + -1) / 100;
                iStack_728 = (int)(local_714 * 9) / 10;
              }
            }
          }
          break;
        case 0x18d:
          iStack_154 = local_140;
          break;
        case 0x18e:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if ((&g_pAreaNodeTable)[local_714][5] == local_140) {
              func_0x004020d6(local_714,local_13c,iStack_154);
            }
          }
          break;
        case 399:
          func_0x0040144c(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 400:
          func_0x00401d52(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x191:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8024);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_Freeze(local_a68);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x192:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d802c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_Unfreeze(local_a68);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x193:
          func_0x004012ee();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x194:
          func_0x00401320();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x195:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8034);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_ResetFreeze(local_a68);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x196:
          func_0x00401636(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x197:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8124);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetFrameStep(local_a68,0);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x198:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d812c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetPosition(local_a68,local_13c,local_138);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x199:
          thunk_FUN_004037c0();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19a:
          Files_LoadPal(*(undefined4 *)(DAT_0070c250 + local_140 * 4),g_abTargetPal,1);
          Anim_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19b:
          func_0x004014ec();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19c:
          func_0x00401b6d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19d:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0xa61,s_C__DevStudio_Projects_Crux_RUNPR_004d8254,
                            s_No_stani_to_SLIDER_ADD_004d823c);
          }
          iVar1 = func_0x00401370(local_a68,local_13c);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19e:
          func_0x00401212((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19f:
          func_0x00401096((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a0:
          func_0x00401866((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a1:
          func_0x00401c0d((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a2:
          func_0x00401ba9((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a3:
          iVar1 = func_0x00401627(0xffffffff);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a4:
          iVar1 = func_0x004019d8(0xffffffff);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a5:
          func_0x004017b2((&g_anSpeechPlayed)[local_140],local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a6:
          func_0x0040142e(0xffffffff,(&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a7:
          if (g_nSndSubtitleOnly == 0) {
            while (iVar3 = func_0x004016e0(), iVar1 = g_nScriptNextOp, iVar3 != 0) {
              Adv_Tick();
              Timer_DispatchAsyncProg();
            }
          }
          break;
        case 0x1a8:
          if (g_nSndSubtitleOnly == 0) {
            func_0x004011a4(local_13c);
            FUN_004895e0(&DAT_00629880,0x7c49c8);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1a9:
          func_0x0040155f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1aa:
          func_0x0040119f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ab:
          func_0x0040103c(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ac:
          DAT_00629ee0 = 0xffffffff;
          FUN_004895e0(0x629dd8,*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          func_0x0040119f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ad:
          Files_SaveGame(s_entry_004dc350);
          Files_SaveGameFull(*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 500:
          func_0x00401d52(g_nSliderRefX,g_nSliderRefY);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1f5:
          uVar2 = (int)local_13c >> 0x1f;
          iVar1 = local_13c + (uVar2 & 0xffff);
          uVar7 = iVar1 >> 0x1f;
          iVar3 = local_13c + (uVar2 & 0xff);
          uVar8 = iVar3 >> 0x1f;
          Txt_SetColor(((local_13c ^ uVar2) - uVar2 & 0xff ^ uVar2) - uVar2,
                       ((iVar3 >> 8 ^ uVar8) - uVar8 & 0xff ^ uVar8) - uVar8,
                       ((iVar1 >> 0x10 ^ uVar7) - uVar7 & 0xff ^ uVar7) - uVar7);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1f6:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0xa9a,s_C__DevStudio_Projects_Crux_RUNPR_004d829c,
                            s_No_stani_to_GETANINOF_004d8284);
          }
          (&g_anSpeechPlayed)[local_140] = *(int *)(&g_anAnimFrameCount + DAT_007c4108 * 4);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1f7:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < 0x5dc;
              local_714 = local_714 + 1) {
            (&g_anSpeechPlayed)[local_714] = 0;
          }
          break;
        case 0x1f8:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d809c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            *(int *)(&g_anAnimSlotTriggerFrame + local_a68 * 0x58) =
                 *(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1f9:
          iVar3 = func_0x004013d4();
          iVar1 = g_nScriptNextOp;
          if (iVar3 == 0) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x1fa:
          if (g_nSndSubtitleOnly == 0) {
            iVar1 = func_0x0040124e();
            (&g_anSpeechPlayed)[local_140] = iVar1;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1fb:
          if (g_nSndSubtitleOnly == 0) {
            func_0x00401640((&g_anSpeechPlayed)[local_140]);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1fc:
          func_0x0040141a();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1fe:
          if (DAT_00574bec != 0) {
            Anim_AddToGroup(local_a68);
            DAT_00574bec = DAT_00574bec - 1;
          }
          local_a68 = Anim_AddByNum(local_140,1,0xffffffff);
          Anim_SetWalkTableBase(local_a68,local_13c);
          Anim_GetFramePosAndSize
                    (local_a68,*(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1,aiStack_5b4 + 6,
                     &iStack_70c,&iStack_14c,&iStack_a58);
          Anim_GetFrameTopLeft(local_a68,aiStack_5b4 + 6,&iStack_70c);
          if ((g_nSliderRefX - iStack_59c) - iStack_14c / 2 < -iStack_59c) {
            iStack_ac0 = -iStack_59c;
          }
          else {
            iStack_ac0 = (g_nSliderRefX - iStack_59c) - iStack_14c / 2;
          }
          iStack_598 = iStack_ac0;
          if ((g_nSliderRefY - iStack_70c) - iStack_a58 / 2 < -iStack_70c) {
            iStack_ac4 = -iStack_70c;
          }
          else {
            iStack_ac4 = (g_nSliderRefY - iStack_70c) - iStack_a58 / 2;
          }
          iStack_a50 = iStack_ac4;
          if (iStack_ac4 < 0x1df - iStack_a58) {
            iStack_ac8 = iStack_ac4;
          }
          else {
            iStack_ac8 = 0x1df - iStack_a58;
          }
          if (iStack_ac0 < 0x27f - iStack_14c) {
            iStack_acc = iStack_ac0;
          }
          else {
            iStack_acc = 0x27f - iStack_14c;
          }
          Anim_SetPosition(local_a68,iStack_acc,iStack_ac8);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ff:
          if (99 < local_708 + 2) {
            Err_BadResEntry(DAT_004d7b60 + -0x124,s_C__DevStudio_Projects_Crux_grani_004d836c,
                            s_Not_enough_place_on_the_stack_fo_004d833c);
          }
          func_0x00401672(g_nSliderRefX,g_nSliderRefY,auStack_590 + local_708 * 4,
                          &iStack_594 + local_708);
          local_708 = local_708 + 2;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x200:
          if (((&g_anSpeechPlayed)[local_140] < (int)local_13c) ||
             (local_138 < (&g_anSpeechPlayed)[local_140])) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x201:
          Curs_DisableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x202:
          Curs_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x204:
          if (local_130 == 0) {
            func_0x00401820(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x205:
          if (local_130 == 0) {
            Adv_WaitForMouseNoAsync();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x206:
          if (local_130 == 0) {
            iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7c74);
            if (iVar1 != 0) {
              local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
            }
            iVar1 = g_nScriptNextOp;
            if (local_a68 != -1) {
              Anim_SetStopFrame(local_a68,*(int *)(&g_anAnimFrameCount + local_a68 * 4) + -1);
              do {
                uVar2 = func_0x00401e4c();
                iVar1 = g_nScriptNextOp;
                if ((uVar2 & 1) != 0) break;
                iVar3 = Anim_IsAtStopFrame(local_a68);
                iVar1 = g_nScriptNextOp;
              } while (iVar3 == 0);
            }
          }
          break;
        case 0x207:
          if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
            Debug_Trace(DAT_004d7b60 + 0x1f6,s_C__DevStudio_Projects_Crux_RUNPR_004d7c4c,
                        s_SOUND_command_004d7c3c);
            func_0x00401dc5(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x208:
          func_0x00401942(local_13c,*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                          (int)*(short *)((int)(&g_apItems)[local_140] + 0xcc),
                          (int)*(short *)((int)(&g_apItems)[local_140] + 0xce));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x209:
          func_0x004018d9(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20a:
          func_0x004018d9(0xffffffff,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20b:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7f68);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = g_nScriptNextOp;
          if (local_a68 != -1) {
            Anim_SetFrameStep(local_a68,0xffffffff);
            *(undefined4 *)(&g_anAnimSlotTriggerFrame + local_a68 * 0x58) = 0xffffffff;
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x20c:
          if (local_130 == 0) {
            Adv_TickFrames((&g_anSpeechPlayed)[local_140]);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x20d:
          Speech_SetTag(iRam007c4998,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20e:
          if (local_130 == 0) {
            Adv_TickFrames(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x20f:
          func_0x004011c7(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x210:
          func_0x004011c7(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x211:
          func_0x0040118b(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 600:
          func_0x00401884(local_13c,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x259:
          InitImg();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x25a:
          local_a68 = Anim_AddByNum(local_140,1,0);
          *(undefined4 *)(&g_anAnimSlotFreezeCount + local_a68 * 0x58) = 1;
          func_0x00401ff0(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x262:
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(1,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x264:
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(3,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x265:
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(2,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x266:
          func_0x00401992(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x267:
          func_0x004013e3(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x268:
          func_0x0040109b(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x269:
          func_0x004010d7(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26a:
          local_718 = local_a64;
          local_a5c = 0;
          break;
        case 0x26b:
          func_0x00401b4a(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26c:
          func_0x004014dd();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26d:
          func_0x004018d4();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26f:
          func_0x00401406(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 700:
          if (local_130 == 0) {
            local_714 = SndMem_WaitSpeech(1);
            iVar1 = g_nScriptNextOp;
            if ((local_714 != 0) && (local_148 != 0)) {
              Adv_TickFramesNoAsync(1);
              local_130 = 1;
              g_nAdvTickSuppressed = 4;
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x2bd:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] / (int)local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2be:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] * local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2bf:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] % (int)local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2c0:
          DAT_004c4c48 = 5;
          break;
        case 0x2c1:
          func_0x00401e79();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2c4:
          if (g_nCursorMode != 1) {
            iStack_720 = 0;
            while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              iVar1 = g_nScriptNextOp;
              if (local_144 == 0xf) {
                if (iStack_720 == 0) break;
                iStack_720 = iStack_720 + -1;
              }
              else {
                if ((local_144 == 0x10) && (iStack_720 == 0)) break;
                iVar1 = func_0x00401a91(local_144);
                if (iVar1 != 0) {
                  iStack_720 = iStack_720 + 1;
                }
              }
            }
          }
          break;
        case 0x2c5:
          func_0x00401ece(local_13c);
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 < 0x8fe) {
      if (local_144 == 0x8fd) {
        if (local_708 < 3) {
          Err_BadResEntry(DAT_004d7b60 + -0x119,s_C__DevStudio_Projects_Crux_grani_004d83c0,
                          s_Not_enough_data_on_the_stack_to_t_004d8394);
        }
        FUN_0048a060(local_700,s__s_d_d_d_004d83e8,s_entry_004dc350,aiStack_5b4[local_708 + 5],
                     aiStack_5b4[local_708 + 6],aiStack_5b4[local_708 + 7]);
        local_708 = local_708 + -3;
        local_a68 = Anim_LoadByName(local_700,local_13c);
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x84d:
          if ((&g_anSpeechPlayed)[local_140] < 1) {
            iStack_ad0 = 0;
          }
          else {
            iStack_ad0 = (&g_anSpeechPlayed)[local_140];
          }
          if (iStack_ad0 < 10) {
            if ((&g_anSpeechPlayed)[local_140] < 1) {
              iStack_ad4 = 0;
            }
            else {
              iStack_ad4 = (&g_anSpeechPlayed)[local_140];
            }
            iStack_ad8 = iStack_ad4;
          }
          else {
            iStack_ad8 = 10;
          }
          (&g_anSpeechPlayed)[local_140] = iStack_ad8;
          if ((&g_anSpeechPlayed)[local_140] == 0) {
            func_0x004017e4(0xffffd8f0);
            iVar1 = g_nScriptNextOp;
          }
          else {
            func_0x004017e4(((&g_anSpeechPlayed)[local_140] + -10) * 300);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x84e:
          func_0x0040123f((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x84f:
          DAT_00629f54 = (uint)((&g_anSpeechPlayed)[local_140] == 0);
          break;
        case 0x850:
          iVar1 = func_0x0040183e();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x851:
          func_0x00401d70((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x852:
          Txt_SetMode((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x853:
          Debug_Assert(DAT_004d7b60 + 0xb53,s_C__DevStudio_Projects_Crux_RUNPR_004d82c4,
                       (&g_anSpeechPlayed)[local_140]);
          func_0x00401285((&g_anSpeechPlayed)[local_140]);
          SetPal_WaitOrRealizeIfNeeded();
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        case 0x855:
          iVar1 = func_0x00401d11();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x856:
          iVar1 = Txt_GetMode();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x857:
          (&g_anSpeechPlayed)[local_140] = (uint)(DAT_00629f54 == 0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x858:
          iVar1 = func_0x004013bb();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 < 0x961) {
      if (local_144 == 0x960) {
        Magwrit_PostInitCallback();
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x8fe:
          Gran_LoadItem();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x8ff:
          Gran_FreeCube();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x900:
          if (local_708 < 8) {
            Err_BadResEntry(DAT_004d7b60 + -0x103,s_C__DevStudio_Projects_Crux_grani_004d8420,
                            s_Not_enough_data_on_the_stack_to_s_004d83f4);
          }
          Gran_ShowCube(aiStack_5b4[local_708 + 7],aiStack_5b4[local_708 + 6],
                        aiStack_5b4[local_708 + 5],aiStack_5b4[local_708 + 4],
                        aiStack_5b4[local_708 + 3],aiStack_5b4[local_708 + 2],
                        aiStack_5b4[local_708 + 1],aiStack_5b4[local_708],local_13c);
          local_708 = local_708 + -8;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x901:
          func_0x0040181b(local_140,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x902:
          func_0x0040107d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x903:
          func_0x0040181b(0xffffffff,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x904:
          DAT_004dc35c = 0;
          local_714 = func_0x0040168b(auStack_11c,g_nHwndMain);
          if (local_714 != 0xffffffff) {
            Files_SaveGame(s_entry_004dc350);
            Files_SaveGameFull(auStack_11c);
          }
          DAT_004dc35c = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x905:
          DAT_004dc35c = 0;
          local_714 = func_0x004016b8(auStack_11c,g_nHwndMain);
          if (local_714 != 0xffffffff) {
            DAT_00629ee0 = 0xffffffff;
            FUN_004895e0(0x629dd8,auStack_11c);
            func_0x0040119f();
          }
          DAT_004dc35c = 1;
          Sleep(300);
          Adv_TickFramesNoAsync(2);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x906:
        case 0x907:
        case 0x908:
        case 0x909:
        case 0x90a:
        case 0x90b:
          if (local_708 < 1) {
            Err_BadResEntry(DAT_004d7b60 + -0xa0,s_C__DevStudio_Projects_Crux_grani_004d8494,
                            s_Not_enough_data_on_the_stack_to_p_004d8460);
          }
          switch(local_144) {
          case 0x906:
            local_708 = local_708 + -1;
            iStack_59c = Gran_PlayAnim(3,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
            break;
          case 0x907:
            local_708 = local_708 + -1;
            iStack_59c = Gran_StartAnim(3,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
            break;
          case 0x908:
            local_708 = local_708 + -1;
            iStack_59c = Gran_PlayAnim(1,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
            break;
          case 0x909:
            local_708 = local_708 + -1;
            iStack_59c = Gran_StartAnim(1,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
            break;
          case 0x90a:
            local_708 = local_708 + -1;
            iStack_59c = Gran_PlayAnim(2,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
            break;
          case 0x90b:
            local_708 = local_708 + -1;
            iStack_59c = Gran_StartAnim(2,(&g_anSpeechPlayed)[local_140],(&iStack_594)[local_708]);
          }
          iVar1 = g_nScriptNextOp;
          if (iStack_59c != -1) {
            local_a68 = iStack_59c;
          }
          break;
        case 0x90c:
          func_0x004015aa(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90d:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84bc);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401a46(local_a68,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90e:
          func_0x00401a4b(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90f:
          func_0x0040209a();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x910:
          func_0x0040125d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x911:
          func_0x00401f55();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x912:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8448);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(1,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x913:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8450);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(2,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x914:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8458);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(3,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x915:
          func_0x004011a9(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x916:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84c4);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x004013f7(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x917:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84cc);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = func_0x00401f00(local_a68,DAT_007c4108);
          (&iStack_594)[local_708] = iVar1;
          local_708 = local_708 + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x918:
          func_0x0040149c(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x919:
          func_0x0040149c(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x91a:
          func_0x004010a0();
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        case 0x91c:
          func_0x00401924();
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 < 0x9c5) {
      if (local_144 == 0x9c4) {
        GV_OpenInventory();
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x961:
          func_0x0040128a(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x962:
          func_0x004017d5(local_13c,local_140,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x963:
          func_0x00401a96(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x964:
          iVar1 = func_0x00401ded(local_13c);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x965:
          func_0x00401901((&g_anSpeechPlayed)[local_140],local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x966:
          func_0x004013c5((&g_anSpeechPlayed)[local_140],local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x967:
          func_0x0040114f(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x968:
          iVar1 = func_0x004011ae(local_13c);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x969:
          iVar1 = func_0x0040151e(local_13c);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x96a:
          func_0x004015f0(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x96b:
          Magwrit_DetachButton(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x96c:
          func_0x004013ed();
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        case 0x974:
          func_0x00401a73(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x975:
          iStack_150 = func_0x0040116d(local_140);
          func_0x00402009(local_13c,*(&g_pAreaNodeTable)[iStack_150],
                          (&g_pAreaNodeTable)[iStack_150][1],(&g_pAreaNodeTable)[iStack_150][2],
                          (&g_pAreaNodeTable)[iStack_150][3],local_138);
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 < 0xc03) {
      if (local_144 == 0xc02) {
        Gran_InitSlider(local_140);
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x9c5:
          func_0x00401622(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x9c6:
          func_0x00401b09(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x9c7:
          func_0x004018e3();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x9c8:
          func_0x00401695();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x9c9:
          func_0x00401c67();
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        }
      }
    }
    else if (local_144 < 0xc1d) {
      if (local_144 == 0xc1c) {
        g_nAni32ClipTop = 0;
        g_nAni32ClipBottom = 0x27f;
        Mov_InitChar(0x20,&DAT_004d84d4);
        Curs_LoadCursor(1,s_csdef_004d84d8,1,1,1);
        Curs_LoadCursor(0,s_csobject_004d84e0,1,5,1);
        Curs_LoadCursor(3,s_csobject_004d84ec,1,5,1);
        Curs_LoadCursor(2,s_csexit_004d84f8,1,9,4);
        Curs_LoadCursor(100,s_csleft_004d8500,2,9,4);
        Curs_LoadCursor(0x65,s_csright_004d8508,2,9,4);
        Curs_LoadCursor(0x66,&DAT_004d8510,2,9,4);
        Curs_LoadCursor(0x67,s_csdown_004d8518,2,9,4);
        Sched_SetBorderMode(0);
        iVar1 = g_nScriptNextOp;
      }
      else if (local_144 == 0xc03) {
        Gran_SetSliderRange(local_140,local_13c,local_138);
        iVar1 = g_nScriptNextOp;
      }
      else {
        if (local_144 != 0xc04) goto LAB_00468d22;
        Gran_StopSlider();
        iVar1 = g_nScriptNextOp;
      }
    }
    else if (local_144 < 0x13bb) {
      if (local_144 == 0x13ba) {
        local_a68 = Anim_AddByNum(local_140,1,0);
        if (DAT_00574bec != 0) {
          Anim_AddToGroup(local_a68);
          DAT_00574bec = DAT_00574bec - 1;
        }
        Anim_SetWalkTableBase(local_a68,local_13c);
        Anim_SetFrameStep(local_a68,0);
        Anim_SetCurrentFrame(local_a68,0);
        Anim_Freeze(local_a68);
        iVar1 = g_nScriptNextOp;
      }
      else {
        if (local_144 != 0x1004) goto LAB_00468d22;
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
      }
    }
    else if (local_144 < 0x1839) {
      if (local_144 == 0x1838) {
        Gran_InitTape();
        iVar1 = g_nScriptNextOp;
      }
      else {
        if (local_144 != 0x17d4) goto LAB_00468d22;
        Anim_SetIndiPal(DAT_007c4108,*(undefined4 *)(DAT_0070c250 + local_140 * 4));
        iVar1 = g_nScriptNextOp;
      }
    }
    else if (local_144 < 0x1b59) {
      if (local_144 == 7000) {
        Tt_Init(local_13c,local_138);
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x1839:
          func_0x00401cd5(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x183a:
          func_0x00402063(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        default:
          goto LAB_00468d22;
        case 0x183c:
          func_0x004010be();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1840:
          func_0x00401a00(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1842:
          func_0x00401b63(local_140);
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 < 0x1b77) {
      if (local_144 == 0x1b76) {
        Tt_CharSet(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
        iVar1 = g_nScriptNextOp;
      }
      else {
        switch(local_144) {
        case 0x1b59:
          func_0x0040138e(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5a:
          func_0x00401ebf(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5b:
          func_0x00401933(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5c:
          func_0x00401910(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5d:
          func_0x00401afa(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5e:
          func_0x00401708(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b5f:
          func_0x00401b81(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b60:
          func_0x00401dac(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b61:
          func_0x00401bd1(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        default:
LAB_00468d22:
          Debug_TraceVal(s_Bad_commad___d_004d8520,local_144);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b6c:
          iVar1 = func_0x00401fe1();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b6d:
          func_0x0040156e((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b6e:
          iVar1 = func_0x00401cf3(param_2);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b6f:
          func_0x004019b5((&g_anSpeechPlayed)[local_140],param_2,local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b70:
          iVar1 = func_0x00402027((&g_anSpeechPlayed)[local_140]);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1b71:
          iVar1 = func_0x00402031((&g_anSpeechPlayed)[local_140]);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
        }
      }
    }
    else if (local_144 == 0x1b80) {
      Tt_CharRemove();
      iVar1 = g_nScriptNextOp;
    }
    else {
      if (local_144 != 0x5b23) goto LAB_00468d22;
      Speech_Init();
      iVar1 = g_nScriptNextOp;
    }
  }
  else {
    local_71c = 0;
    iVar1 = g_nScriptNextOp;
  }
  goto LAB_00468d36;
LAB_00468dd9:
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


