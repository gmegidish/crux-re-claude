// RunProg_Exec opcode handlers 0x150-0x17f — #included into RunProg_Exec switch
        // 0x150 ANIM_ADD_WALKTABLE
        case 0x150:
          local_a68 = Anim_AddByNum(local_140,1,0);
          if (_DAT_00574bec != 0) {
            Anim_AddToGroup(iStack_150);
          }
          Anim_SetWalkTableBase(local_a68,0);
          Anim_GetFrameTopLeft(local_a68,aiStack_5b4 + 6,&iStack_70c);
          Anim_SetPosition(local_a68,local_13c - iStack_59c,local_138 - iStack_70c);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x151 SET_DAT_004D6650
        case 0x151:
          DAT_004d6650 = local_13c;
          break;
        // 0x152 GI_GETPIXEL_TO_REG
        case 0x152:
          GI_SetDrawMode(0);
          local_714 = func_0x00401915(local_13c,local_138);
          local_714 = local_714 & 0xff;
          iVar1 = func_0x00401730(local_714);
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x153 SET_PIXEL_FROM_REG
        case 0x153:
          func_0x004012da(local_13c,(&g_anSpeechPlayed)[local_140]);
          SetPal_WaitOrRealizeIfNeeded();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x154 IF_NOT_AT_NODE_SKIP
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
        // 0x155 ANIM_OP_155
        case 0x155:
          GI_SetDrawMode(0);
          func_0x00401000(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x156 ANIM_GET_CURRENT_FRAME
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
        // 0x157 BANI_NOOP
        case 0x157:
          Bani_Noop(*(undefined4 *)(_DAT_0070d558 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
        // 0x158 NOOP
        case 0x158:
        case 0x1fd:
          break;
        // 0x159 ANIM_SET_COMPLETION_CB
        case 0x159:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d803c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          Anim_SetCompletionCallback(local_a68,iRam007c4998,local_13c,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x15a ANIM_CLEAR_COMPLETION_CB
        case 0x15a:
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d804c);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          Anim_SetCompletionCallback(local_a68,0xffffffff,0xffffffff,0xffffffff);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x15b REG_ADD_IMM
        case 0x15b:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x15c REG_SUB_IMM
        case 0x15c:
          (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] - local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x15d ENTER_CURSOR_SELECT
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
        // 0x15e OP_15E
        case 0x15e:
          func_0x00401f7d();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x15f OP_15F
        case 0x15f:
          func_0x00401163();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x160 OP_160
        case 0x160:
          func_0x00401032();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x161 OP_161_GET_TO_REG
        case 0x161:
          iVar1 = func_0x0040100a();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x162 OP_162_GET_TO_REG
        case 0x162:
          iVar1 = func_0x00402081();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x163 OP_163_GET_TO_REG
        case 0x163:
          iVar1 = func_0x004012df();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x164 OP_164_FROM_REG
        case 0x164:
          func_0x004017ee((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x165 OP_165
        case 0x165:
          func_0x00401f5f();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x166 ANIM_SHOW_FRAME
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
        // 0x167 ANIM_SET_TRIGGER_FRAME
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
        // 0x168 CALL_PROG
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
        // 0x169 LINKFULL_AREA (also 0x2c3 LINK)
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
        // 0x16a EXIT_CURSOR_SELECT
        case 0x16a:
          DAT_00629f70 = 0xffffffff;
          DAT_00629c08 = -1;
          _DAT_007d5a80 = 0xffffffff;
          local_a60 = '\0';
          g_nCursorMode = 3;
          Win_UpdateCursor();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x16b OP_16B
        case 0x16b:
          func_0x00401262();
          DAT_007d5f30 = 0;
          iVar1 = g_nScriptNextOp;
          break;
        // 0x16c THEME_MUSIC_EVENT
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
        // 0x16d PLAY_SOUND_COND
        case 0x16d:
          if (local_130 == 0) {
            Adv_TickFramesNoAsync(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        // 0x16e OP_16E_SOUND_COND
        case 0x16e:
          if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
            func_0x00401037(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            iVar1 = g_nScriptNextOp;
          }
          break;
        // 0x16f FX_STOP_LOOP
        case 0x16f:
          Fx_StopLoop();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x170 OP_170_FROM_REG
        case 0x170:
          func_0x004010dc((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x171 OP_171_FROM_REG
        case 0x171:
          func_0x00401843((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x172 OP_172_FROM_REG
        case 0x172:
          func_0x004019c4((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x173 STACK_PUSH
        case 0x173:
          if (99 < local_708 + 1) {
            thunk_FUN_00420e60(DAT_004d7b60 + 0x99e,s_C__DevStudio_Projects_Crux_RUNPR_004d8134);
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
        // 0x174 STACK_POP
        case 0x174:
          if (local_708 < 1) {
            thunk_FUN_00420e60(DAT_004d7b60 + 0x9a6,s_C__DevStudio_Projects_Crux_RUNPR_004d815c);
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
        // 0x175 ANIM_SET_X
        case 0x175:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x9bd,s_C__DevStudio_Projects_Crux_RUNPR_004d819c,
                            s_No_stani_to_OFFSETANIX_004d8184);
          }
          *(int *)(&g_anAnimSlotX + DAT_007c4108 * 0x58) = (&g_anSpeechPlayed)[local_140];
          iVar1 = g_nScriptNextOp;
          break;
        // 0x176 ANIM_SET_Y
        case 0x176:
          if (DAT_007c4108 < 0) {
            Err_BadResEntry(DAT_004d7b60 + 0x9c5,s_C__DevStudio_Projects_Crux_RUNPR_004d81dc,
                            s_No_stani_to_OFFSETANIY_004d81c4);
          }
          *(int *)(&g_anAnimSlotY + DAT_007c4108 * 0x58) = (&g_anSpeechPlayed)[local_140];
          iVar1 = g_nScriptNextOp;
          break;
        // 0x177 OP_177_COND
        case 0x177:
          if (local_130 == 0) {
            func_0x004016fe(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        // 0x178 OP_178
        case 0x178:
          func_0x00401bc2(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x179 PUSH_EVENT
        case 0x179:
          if (0xe < DAT_00629f68) {
            thunk_FUN_00420e60(DAT_004d7b60 + 0x9df,s_C__DevStudio_Projects_Crux_RUNPR_004d8204);
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
        // 0x17a STOP_LIPSYNC_TXT
        case 0x17a:
          Snd_Stop(1);
          SndMem_StopLipsync();
          Txt_Reset();
          iVar1 = g_nScriptNextOp;
          break;
        // 0x17b ANIM_SELECT_SLOT
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
        // 0x17c ANIM_DUMP_OR_FREE
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
        // 0x17d IF_NOT_CURRENT_SCENE_SKIP
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
        // 0x17e OP_17E
        case 0x17e:
          func_0x004013b1(local_13c,local_140);
          iVar1 = g_nScriptNextOp;
          break;
        // 0x17f PALETTE_FADEIN
        case 0x17f:
          FUN_004896d0(g_abTargetPal,g_abSnapshotPal,0x300);
          SetPal_FadeInFromBlack();
          iVar1 = g_nScriptNextOp;
          break;
