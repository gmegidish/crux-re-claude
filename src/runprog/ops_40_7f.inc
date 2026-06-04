// RunProg_Exec opcode handlers 0x40-0x7f — #included into RunProg_Exec switch

        case 0x40:        // 0x40 AREA_LINK_SPRITE (shared with 0x2c2 AREA_LINK_SPRITE; see 0x41 REMOVE_AREA_SPRITE counterpart)
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
        case 0x41:        // 0x41 REMOVE_AREA_SPRITE
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
        case 0x42:        // 0x42 BEGIN_DIALOG
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
        case 0x43:        // 0x43 END_DIALOG
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
        case 0x44:        // 0x44 PLAY_ANIM_WAIT
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
        case 0x45:        // 0x45 SET_INTERACT_MODE1
          DAT_004c4c48 = 1;
          break;
        case 0x46:        // 0x46 SET_INTERACT_MODE2
          FUN_004895e0(0x629af8,*(undefined4 *)(DAT_0070c24c + local_140 * 4));
          DAT_004c4c48 = 2;
          uRam005b039c = local_13c;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x47:        // 0x47 ENABLE_INPUT_FLAG
          DAT_00629f4c = 1;
          break;
        case 0x48:        // 0x48 DISABLE_INPUT_FLAG
          DAT_00629f4c = 0;
          break;
        case 0x49:        // 0x49 BREAK_IF_SKIP
          if (local_130 == 0) {
            Adv_TickFramesNoAsync(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x4a:        // 0x4a SET_INTERACT_MODE3
          DAT_004c4c48 = 3;
          break;
        case 0x4b:        // 0x4b (reserved / no-op)
          break;
        case 0x4c:        // 0x4c (reserved / no-op)
          break;
        case 0x4d:        // 0x4d FADE_TO_ROOM
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
        case 0x4f:        // 0x4f ADD_WALK_ANIM
          local_a68 = Anim_AddByNum(local_140,3,0);
          Anim_SetWalkTableBase(local_a68,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x50:        // 0x50 SHOW_TEXT_WAIT
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
        case 0x51:        // 0x51 (reserved / no-op)
          break;
        case 0x52:        // 0x52 (reserved / no-op)
          break;
        case 0x54:        // 0x54 SET_ANIM_TICK_MODE
          Anim_SetTickMode(local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x55:        // 0x55 SET_WALK_MODE
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
        case 0x56:        // 0x56 (reserved / no-op)
          break;
        case 0x59:        // 0x59 (reserved / no-op)
          break;
        case 0x5a:        // 0x5a (reserved / no-op)
          break;
        case 0x5b:        // 0x5b (reserved / no-op)
          break;
        case 0x5c:        // 0x5c (reserved / no-op)
          break;
        case 0x5d:        // 0x5d (reserved / no-op)
          break;
        case 0x5e:        // 0x5e (reserved / no-op)
          break;
        case 0x5f:        // 0x5f (reserved / no-op)
          break;
        case 0x60:        // 0x60 (reserved / no-op)
          break;
        case 0x61:        // 0x61 PLACE_ANIM
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
        case 0x62:        // 0x62 (reserved / no-op)
          break;
        case 99:          // 0x63 (reserved / no-op)
          break;
        case 100:         // 0x64 (reserved / no-op)
          break;
        case 0x65:        // 0x65 CALL_SCRIPT
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
        case 0x66:        // 0x66 (reserved / no-op)
          break;
        case 0x67:        // 0x67 SAY_TEXT
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
        case 0x68:        // 0x68 SET_CHAR_FLAG
          if (local_130 == 0) {
            func_0x0040154b(local_140,local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        case 0x69:        // 0x69 (reserved / no-op)
          break;
        case 0x6a:        // 0x6a (reserved / no-op)
          break;
        case 0x6b:        // 0x6b SET_TIMER
          _DAT_00574bec = local_13c;
          func_0x0040130c(local_13c,local_138);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x6c:        // 0x6c (shared with 0x71 RUN_SCENE)
        case 0x71:        // 0x71 RUN_SCENE
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
        case 0x6d:        // 0x6d (reserved / no-op)
          break;
        case 0x6e:        // 0x6e (reserved / no-op)
          break;
        case 0x6f:        // 0x6f IF_OBJECT_NOT_IN_LIST
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
        case 0x70:        // 0x70 END_SCRIPT
          local_718 = local_a64;
          break;
        case 0x72:        // 0x72 SET_ROOM
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
        case 0x73:        // 0x73 SET_SPEECH_PLAYED
          (&g_anSpeechPlayed)[local_140] = DAT_00629dc0;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x74:        // 0x74 (reserved / no-op)
          break;
        case 0x75:        // 0x75 (reserved / no-op)
          break;
        case 0x76:        // 0x76 SAY_SPEECH
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
        case 0x77:        // 0x77 (shared with 0x78 ADD_CHAR_TO_SCENE)
        case 0x78:        // 0x78 ADD_CHAR_TO_SCENE
          Debug_Trace(DAT_004d7b60 + 0x4f2,s_C__DevStudio_Projects_Crux_RUNPR_004d7e54,
                      *(undefined4 *)(_DAT_0070dec8 + local_140 * 4));
          Player_ScmAddChar(*(undefined4 *)(_DAT_0070dec8 + local_140 * 4));
          iVar1 = g_nScriptNextOp;
          break;
