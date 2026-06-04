// RunProg_Exec opcode handlers 0x100-0x14f — #included into RunProg_Exec switch

        // 0x100 (no-op) — body lives in ops_80_ff.inc as `case 0xff: case 0x100:` (dedup removed)
        case 0x125: // 0x125 SET_VAR_FROM_ACC
          (&g_anSpeechPlayed)[local_140] = local_404;
          iVar1 = g_nScriptNextOp;
          break;
        case 300: // 0x12c ANIM_SET_FRAME_SOUND
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x5f5,s_C__DevStudio_Projects_Crux_RUNPR_004d7eb8,
                            s_No_stani_to_SOUNDFX_004d7ea4);
          }
          Anim_SetFrameSound(DAT_007c4108,local_13c,*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),2
                            );
          SndMem_Load(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),auStack_3ec,local_13c - 1,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x12d: // 0x12d SPEECH_WAIT
          if ((local_130 == 0) &&
             (Adv_TickFramesNoAsync(1), iVar1 = g_nScriptNextOp, g_nAdvTickSuppressed == 0)) {
            g_nAdvTickSuppressed = 2;
          }
          break;
        case 0x12e: // 0x12e SPEECH_END
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
        case 0x12f: // 0x12f PAL_SNAPSHOT
          Sched_SavePaletteSnapshot();
          FUN_004896d0(&DAT_007c4110,g_abSnapshotPal,0x300);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x130: // 0x130 PAL_RESTORE
          FUN_004896d0(g_abTargetPal,&DAT_007c4110,0x300);
          Anim_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x131: // 0x131 ANIM_BEGIN
          local_148 = 1;
          RunProg_ClearTrackedSounds();
          local_404 = 0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x132: // 0x132 ANIM_END
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
        case 0x133: // 0x133 SET_FOLLOWER
          local_a68 = Anim_AddByNum(local_140,1,0xffffffff);
          *(uint *)(&g_anAnimSlotFlags + local_a68 * 0x58) =
               *(uint *)(&g_anAnimSlotFlags + local_a68 * 0x58) & 0xfffffff7;
          Mov_SetFollower(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x134: // 0x134 WAIT_ANIM
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
        case 0x135: // 0x135 CLEAR_FOLLOWER
          Mov_SetFollower(0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x136: // 0x136 ADD_ANIM
          local_a68 = Anim_AddByNum(local_140,1,0);
          if (_DAT_00574bec != 0) {
            Anim_AddToGroup(iStack_150);
          }
          Anim_SetWalkTableBase(local_a68,0);
          Anim_SetPosition(local_a68,local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x137: // 0x137 SET_WALKTABLE
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
        case 0x13b: // 0x13b WAIT_FRAME
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
        case 0x13c: // 0x13c FREEZE_ANIM
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
        case 0x13d: // 0x13d UNFREEZE_ANIM
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
        case 0x13e: // 0x13e SET_ANI_FRAME
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
        case 0x13f: // 0x13f ENSURE_SLOTS
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
        case 0x140: // 0x140 SET_ANIM_PARAM
          func_0x004019ab(local_13c,local_138,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x141: // 0x141 SET_ANIM_PARAM2
          Adv_SetInvSlotDirect(local_13c,local_138,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x142: // 0x142 SET_ANIM_2PARAM
          func_0x004010c3(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x143: // 0x143 SET_ANIM_ID
          func_0x00401d34(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x144: // 0x144 ENABLE_SLOT
          *(undefined4 *)(&DAT_0070e65c + local_13c * 0x230) = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x145: // 0x145 COUNT_VALID_ITEMS
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
        case 0x146: // 0x146 ADD_ITEM_TO_SLOT
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
        case 0x147: // 0x147 READ_SCRATCH
          (&g_anSpeechPlayed)[local_140] = iRam0070decc;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x148: // 0x148 WRITE_SCRATCH
          iRam0070decc = (&g_anSpeechPlayed)[local_140];
          break;
        case 0x149: // 0x149 SLOT_ADD_ITEM
          for (local_714 = 0; (int)local_714 < 10; local_714 = local_714 + 1) {
            if (*(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) == -1) {
              *(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) = local_140;
              iVar1 = g_nScriptNextOp;
              break;
            }
          }
          break;
        case 0x14a: // 0x14a SLOT_REMOVE_ITEM
          for (local_714 = 0; (int)local_714 < 10; local_714 = local_714 + 1) {
            if (*(int *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) == local_140) {
              *(undefined4 *)(&DAT_0070e660 + local_714 * 4 + local_13c * 0x230) = 0xffffffff;
              iVar1 = g_nScriptNextOp;
              break;
            }
          }
          break;
        case 0x14b: // 0x14b COUNT_ACTIVE_ITEMS
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
        case 0x14c: // 0x14c IF_ITEM_NOT_IN_SLOT
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
        case 0x14d: // 0x14d FREEZE_ANIM_LAST
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
        case 0x14e: // 0x14e SCENE_CHANGE
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
        case 0x14f: // 0x14f SET_ANIM_SUBFRAME
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d801c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x004013ac(local_a68,local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
