// RunProg_Exec opcode handlers 0x200-0x2ff — #included into RunProg_Exec switch

        case 0x200: // 0x200 IF_SPEECH_NOT_IN_RANGE
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
        case 0x201: // 0x201 CURS_DISABLE_DRAW
          Curs_DisableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x202: // 0x202 CURS_ENABLE_DRAW
          Curs_EnableDraw();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x204: // 0x204 SCHED_SET_MODE
          if (local_130 == 0) {
            func_0x00401820(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x205: // 0x205 SCHED_TICK
          if (local_130 == 0) {
            Adv_WaitForMouseNoAsync();
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x206: // 0x206 ANIM_WAIT_LAST_FRAME
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
        case 0x207: // 0x207 SND_PLAY_ON_OBJ
          if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
            Debug_Trace(DAT_004d7b60 + 0x1f6,s_C__DevStudio_Projects_Crux_RUNPR_004d7c4c,
                        s_SOUND_command_004d7c3c);
            func_0x00401dc5(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x208: // 0x208 INV_PLAY_SOUND_AT_ITEM
          func_0x00401942(local_13c,*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                          (int)*(short *)((int)(&g_apItems)[local_140] + 0xcc),
                          (int)*(short *)((int)(&g_apItems)[local_140] + 0xce));
          iVar1 = g_nScriptNextOp;
          break;
        case 0x209: // 0x209 SND_PLAY_ON_NODE
          func_0x004018d9(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20a: // 0x20a SND_STOP_ALL_NODE
          func_0x004018d9(0xffffffff,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20b: // 0x20b ANIM_SET_REVERSE
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
        case 0x20c: // 0x20c SND_PLAY_SPEECH_BY_VAR
          if (local_130 == 0) {
            Adv_TickFrames((&g_anSpeechPlayed)[local_140]);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x20d: // 0x20d SPEECH_SET_TAG_TO_VAR
          Speech_SetTag(iRam007c4998,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x20e: // 0x20e SND_PLAY_SPEECH_BY_ARG
          if (local_130 == 0) {
            Adv_TickFrames(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x20f: // 0x20f SCHED_INTERRUPT_ENABLE
          func_0x004011c7(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x210: // 0x210 SCHED_INTERRUPT_DISABLE
          func_0x004011c7(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x211: // 0x211 SCHED_INTERRUPT_SET
          func_0x0040118b(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 600: // 0x258 SCHED_SET_MODE2 (decompiler emitted decimal 600)
          func_0x00401884(local_13c,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x259: // 0x259 GFX_INIT_IMG
          InitImg();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x25a: // 0x25a ANIM_ADD_FROZEN
          local_a68 = Anim_AddByNum(local_140,1,0);
          *(undefined4 *)(&g_anAnimSlotFreezeCount + local_a68 * 0x58) = 1;
          func_0x00401ff0(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x262: // 0x262 SND_SET_PAN_L
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(1,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x264: // 0x264 SND_SET_PAN_R
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(3,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x265: // 0x265 SND_SET_PAN_C
          uVar5 = GI_PercentOfWidth(local_13c,local_138);
          Snd_SetChannelPan(2,uVar5);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x266: // 0x266 SND_SET_VOLUME
          func_0x00401992(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x267: // 0x267 SND_SET_PITCH
          func_0x004013e3(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x268: // 0x268 SND_SET_MUSIC_VOL
          func_0x0040109b(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x269: // 0x269 SND_PLAY_MUSIC
          func_0x004010d7(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4),local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26a: // 0x26a BREAK_LOOP
          local_718 = local_a64;
          local_a5c = 0;
          break;
        case 0x26b: // 0x26b SND_FADE_MUSIC
          func_0x00401b4a(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26c: // 0x26c SND_STOP_MUSIC
          func_0x004014dd();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26d: // 0x26d SND_STOP_ALL
          func_0x004018d4();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x26f: // 0x26f SND_STOP_ON_OBJ
          func_0x00401406(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        case 700: // 0x2bc WAIT_SPEECH_OR_SKIP (decompiler emitted decimal 700)
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
        case 0x2bd: // 0x2bd DIV_VAR
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] / (int)local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2be: // 0x2be MUL_VAR
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] * local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2bf: // 0x2bf MOD_VAR
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] % (int)local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x2c0: // 0x2c0 SET_EXIT_CODE_5
          DAT_004c4c48 = 5;
          break;
        case 0x2c1: // 0x2c1 SCHED_YIELD_FRAME
          func_0x00401e79();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x2c2 AREA_LINK_SPRITE — body lives in ops_40_7f.inc as `case 0x40: case 0x2c2:` (dedup removed)
        // 0x2c3 AREA_LINK_SPRITE_FULL — body lives in ops_150_17f.inc as `case 0x169: case 0x2c3:` (dedup removed)
        case 0x2c4: // 0x2c4 IF_NOT_GAME_MODE
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
        case 0x2c5: // 0x2c5 SCHED_SET_SUBMODE
          func_0x00401ece(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
