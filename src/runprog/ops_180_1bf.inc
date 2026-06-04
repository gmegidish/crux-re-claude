// RunProg_Exec opcode handlers 0x180-0x1bf — #included into RunProg_Exec switch
        case 0x180:  // 0x180 SET_RENDER_PASS
          DAT_004c4c48 = 4;
          break;
        case 0x181:  // 0x181 ANIM_SET_POS_TOPLEFT
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
        case 0x182:  // 0x182 STACK_ADD
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] + aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x183:  // 0x183 STACK_SUB
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] - aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x184:  // 0x184 STACK_MUL
          aiStack_5b4[local_708 + 6] = aiStack_5b4[local_708 + 6] * aiStack_5b4[local_708 + 7];
          local_708 = local_708 + -1;
          break;
        case 0x185:  // 0x185 ANIM_SET_COMPLETION_CB_FRAME
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
        case 0x186:  // 0x186 (DAT_00712838=5; DAT_0070ded0 from _DAT_0070b5c4[reg])
          DAT_00712838 = 5;
          DAT_0070ded0 = *(undefined4 *)(_DAT_0070b5c4 + local_140 * 4);
          break;
        case 0x187:  // 0x187 ANIM_FREEZE_SEL
          if (-1 < _DAT_007c441c) {
            local_714 = Anim_FindSlotByName(_DAT_007c441c,_DAT_007c49a0);
            iVar1 = g_nScriptNextOp;
            if (local_714 != 0xffffffff) {
              Anim_Freeze(local_714);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x188:  // 0x188 ANIM_UNFREEZE_SEL
          if (-1 < _DAT_007c441c) {
            local_714 = Anim_FindSlotByName(_DAT_007c441c,_DAT_007c49a0);
            iVar1 = g_nScriptNextOp;
            if (local_714 != 0xffffffff) {
              Anim_ResetFreeze(local_714);
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        case 0x189:  // 0x189 (Adv_SetDrawSuppressed(1))
          Adv_SetDrawSuppressed(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x18a:  // 0x18a (Adv_SetDrawSuppressed(0))
          Adv_SetDrawSuppressed(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x18b:  // 0x18b ANIM_SHOW_FRAME_Z
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
        case 0x18c:  // 0x18c ANIM_EXPLODE
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
        case 0x18d:  // 0x18d (iStack_154 = reg)
          iStack_154 = local_140;
          break;
        case 0x18e:  // 0x18e AREA_SET_NODE_PROPS
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if ((&g_pAreaNodeTable)[local_714][5] == local_140) {
              func_0x004020d6(local_714,local_13c,iStack_154);
            }
          }
          break;
        case 399:  // 0x18f (func_0x0040144c(reg))
          func_0x0040144c(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 400:  // 0x190 MOV_MOVE_TO
          func_0x00401d52(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x191:  // 0x191 ANIM_FREEZE
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
        case 0x192:  // 0x192 ANIM_UNFREEZE
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
        case 0x193:  // 0x193 (func_0x004012ee())
          func_0x004012ee();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x194:  // 0x194 (func_0x00401320())
          func_0x00401320();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x195:  // 0x195 ANIM_RESET_FREEZE
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
        case 0x196:  // 0x196 (func_0x00401636(arg1, reg))
          func_0x00401636(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x197:  // 0x197 ANIM_SET_FRAMESTEP_0
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
        case 0x198:  // 0x198 ANIM_SET_POSITION
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
        case 0x199:  // 0x199 (thunk_FUN_004037c0())
          thunk_FUN_004037c0();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19a:  // 0x19a FILES_LOAD_PAL
          Files_LoadPal(*(undefined4 *)(DAT_0070c250 + local_140 * 4),g_abTargetPal,1);
          Anim_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19b:  // 0x19b (func_0x004014ec())
          func_0x004014ec();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19c:  // 0x19c (func_0x00401b6d())
          func_0x00401b6d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19d:  // 0x19d SLIDER_ADD
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0xa61,s_C__DevStudio_Projects_Crux_RUNPR_004d8254,
                            s_No_stani_to_SLIDER_ADD_004d823c);
          }
          iVar1 = func_0x00401370(local_a68,local_13c);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19e:  // 0x19e (func_0x00401212(var[reg]))
          func_0x00401212((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x19f:  // 0x19f (func_0x00401096(var[reg]))
          func_0x00401096((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a0:  // 0x1a0 (func_0x00401866(var[reg], arg1, arg2))
          func_0x00401866((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a1:  // 0x1a1 (func_0x00401c0d(var[reg], arg1, arg2))
          func_0x00401c0d((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a2:  // 0x1a2 (func_0x00401ba9(var[reg], arg1, arg2))
          func_0x00401ba9((&g_anSpeechPlayed)[local_140],local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a3:  // 0x1a3 (func_0x00401627(-1) → var[reg])
          iVar1 = func_0x00401627(0xffffffff);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a4:  // 0x1a4 (func_0x004019d8(-1) → var[reg])
          iVar1 = func_0x004019d8(0xffffffff);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a5:  // 0x1a5 (func_0x004017b2(var[reg], arg1))
          func_0x004017b2((&g_anSpeechPlayed)[local_140],local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a6:  // 0x1a6 (func_0x0040142e(-1, var[reg]))
          func_0x0040142e(0xffffffff,(&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1a7:  // 0x1a7 WAIT_FOR_UNK
          if (g_nSndSubtitleOnly == 0) {
            while (iVar3 = func_0x004016e0(), iVar1 = g_nScriptNextOp, iVar3 != 0) {
              Adv_Tick();
              Timer_DispatchAsyncProg();
            }
          }
          break;
        case 0x1a8:  // 0x1a8 SET_AMBIENT_MUSIC
          if (g_nSndSubtitleOnly == 0) {
            func_0x004011a4(local_13c);
            FUN_004895e0(&DAT_00629880,0x7c49c8);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x1a9:  // 0x1a9 (func_0x0040155f())
          func_0x0040155f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1aa:  // 0x1aa (func_0x0040119f())
          func_0x0040119f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ab:  // 0x1ab (func_0x0040103c(arg1))
          func_0x0040103c(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ac:  // 0x1ac (set music name from _DAT_0070d558[reg]; stop music)
          DAT_00629ee0 = 0xffffffff;
          FUN_004895e0(0x629dd8,*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          func_0x0040119f();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x1ad:  // 0x1ad FILES_SAVE_GAME
          Files_SaveGame(s_entry_004dc350);
          Files_SaveGameFull(*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
