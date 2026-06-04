// RunProg_Exec opcode handlers 0x1c0-0x1ff — #included into RunProg_Exec switch
//
// Note: opcodes 0x1c0-0x1f4 have no case handlers in RunProg_Exec (gaps).
// The only real cases in this range are 0x1f5-0x1ff (0x1fd is a no-op
// stacked with 0x158 in the decompiler output and reproduced standalone here).

        // 0x1f5 TXT_SET_COLOR — Txt_SetColor(r,g,b) from packed arg1
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

        // 0x1f6 ANIM_GET_FRAME_COUNT — Frame count of stani -> var[reg]
        case 0x1f6:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0xa9a,s_C__DevStudio_Projects_Crux_RUNPR_004d829c,
                            s_No_stani_to_GETANINOF_004d8284);
          }
          (&g_anSpeechPlayed)[local_140] = *(int *)(&g_anAnimFrameCount + DAT_007c4108 * 4);
          iVar1 = g_nScriptNextOp;
          break;

        // 0x1f7 REGS_CLEAR_ALL — Zero entire g_anSpeechPlayed[] (1500 entries)
        case 0x1f7:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < 0x5dc;
              local_714 = local_714 + 1) {
            (&g_anSpeechPlayed)[local_714] = 0;
          }
          break;

        // 0x1f8 ANIM_SET_TRIGGER_LAST — Trigger on last frame
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

        // 0x1f9 IF_UNK_FALSE_SKIP — Skip block if condition == 0
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

        // 0x1fa SPEECH_PLAY_STORE — if !subtitle-only: play speech, store result in var[reg] (undocumented in spec)
        case 0x1fa:
          if (g_nSndSubtitleOnly == 0) {
            iVar1 = func_0x0040124e();
            (&g_anSpeechPlayed)[local_140] = iVar1;
            iVar1 = g_nScriptNextOp;
          }
          break;

        // 0x1fb SPEECH_STOP_STORED — if !subtitle-only: stop speech identified by var[reg] (undocumented in spec)
        case 0x1fb:
          if (g_nSndSubtitleOnly == 0) {
            func_0x00401640((&g_anSpeechPlayed)[local_140]);
            iVar1 = g_nScriptNextOp;
          }
          break;

        // 0x1fc SPEECH_UNK — func_0x0040141a() (undocumented in spec)
        case 0x1fc:
          func_0x0040141a();
          iVar1 = g_nScriptNextOp;
          break;

        // 0x1fd NOP — body lives in ops_150_17f.inc as `case 0x158: case 0x1fd:` (dedup removed)

        // 0x1fe ANIM_ADD_CENTERED — Add anim positioned centred on last frame size
        case 0x1fe:
          if (_DAT_00574bec != 0) {
            Anim_AddToGroup(local_a68);
            _DAT_00574bec = _DAT_00574bec - 1;
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

        // 0x1ff PUSH_HERO_XY — Push hero (x,y) onto RPN stack
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
