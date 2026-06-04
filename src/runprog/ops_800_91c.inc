// RunProg_Exec opcode handlers 0x800-0x91c — #included into RunProg_Exec switch

        case 0x84d:  // 0x84d
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
        case 0x84e:  // 0x84e
          func_0x0040123f((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x84f:  // 0x84f
          DAT_00629f54 = (uint)((&g_anSpeechPlayed)[local_140] == 0);
          break;
        case 0x850:  // 0x850
          iVar1 = func_0x0040183e();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x851:  // 0x851
          func_0x00401d70((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x852:  // 0x852 Txt_SetMode
          Txt_SetMode((&g_anSpeechPlayed)[local_140]);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x853:  // 0x853
          Debug_Assert(DAT_004d7b60 + 0xb53,s_C__DevStudio_Projects_Crux_RUNPR_004d82c4,
                       (&g_anSpeechPlayed)[local_140]);
          func_0x00401285((&g_anSpeechPlayed)[local_140]);
          SetPal_WaitOrRealizeIfNeeded();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x855:  // 0x855
          iVar1 = func_0x00401d11();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x856:  // 0x856 Txt_GetMode
          iVar1 = Txt_GetMode();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x857:  // 0x857
          (&g_anSpeechPlayed)[local_140] = (uint)(DAT_00629f54 == 0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x858:  // 0x858
          iVar1 = func_0x004013bb();
          (&g_anSpeechPlayed)[local_140] = iVar1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x8fe:  // 0x8fe
          func_0x00401edd();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x8ff:  // 0x8ff
          func_0x004018c5();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x900:  // 0x900 stack-based gfx draw-rect (8 args)
          if (local_708 < 8) {
            Err_BadResEntry(DAT_004d7b60 + -0x103,s_C__DevStudio_Projects_Crux_grani_004d8420,
                            s_Not_enough_data_on_the_stack_to_s_004d83f4);
          }
          func_0x00401046(aiStack_5b4[local_708 + 7],aiStack_5b4[local_708 + 6],
                          aiStack_5b4[local_708 + 5],aiStack_5b4[local_708 + 4],
                          aiStack_5b4[local_708 + 3],aiStack_5b4[local_708 + 2],
                          aiStack_5b4[local_708 + 1],aiStack_5b4[local_708],local_13c);
          local_708 = local_708 + -8;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x901:  // 0x901
          func_0x0040181b(local_140,0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x902:  // 0x902
          func_0x0040107d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x903:  // 0x903
          func_0x0040181b(0xffffffff,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x904:  // 0x904 save game dialog
          DAT_004dc35c = 0;
          local_714 = func_0x0040168b(auStack_11c,g_nHwndMain);
          if (local_714 != 0xffffffff) {
            Files_SaveGame(s_entry_004dc350);
            Files_SaveGameFull(auStack_11c);
          }
          DAT_004dc35c = 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x905:  // 0x905 load game dialog
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
        case 0x906:  // 0x906
        case 0x907:  // 0x907
        case 0x908:  // 0x908
        case 0x909:  // 0x909
        case 0x90a:  // 0x90a
        case 0x90b:  // 0x90b  Gran_PlayAnim/StartAnim type 1/2/3
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
        case 0x90c:  // 0x90c
          func_0x004015aa(local_140,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90d:  // 0x90d
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84bc);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401a46(local_a68,local_13c);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90e:  // 0x90e
          func_0x00401a4b(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x90f:  // 0x90f
          func_0x0040209a();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x910:  // 0x910
          func_0x0040125d();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x911:  // 0x911
          func_0x00401f55();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x912:  // 0x912 anim trigger type 1
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8448);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(1,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x913:  // 0x913 anim trigger type 2
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8450);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(2,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x914:  // 0x914 anim trigger type 3
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d8458);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x00401ae6(3,local_13c,local_138,local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x915:  // 0x915
          func_0x004011a9(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x916:  // 0x916
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84c4);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          func_0x004013f7(local_a68);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x917:  // 0x917
          iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d84cc);
          if (iVar1 != 0) {
            local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
          }
          iVar1 = func_0x00401f00(local_a68,DAT_007c4108);
          (&iStack_594)[local_708] = iVar1;
          local_708 = local_708 + 1;
          iVar1 = g_nScriptNextOp;
          break;
        case 0x918:  // 0x918 gfx flip on
          func_0x0040149c(1);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x919:  // 0x919 gfx flip off
          func_0x0040149c(0);
          iVar1 = g_nScriptNextOp;
          break;
        case 0x91a:  // 0x91a
          func_0x004010a0();
          iVar1 = g_nScriptNextOp;
          break;
        case 0x91c:  // 0x91c
          func_0x00401924();
          iVar1 = g_nScriptNextOp;
          break;
