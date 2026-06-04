// RunProg_Exec opcode handlers 0x00-0x3f — #included into RunProg_Exec switch
//
// Transcribed verbatim from the Ghidra decompile of RunProg_Exec (0x00462560).
// Local variable names (local_144 opcode, local_140 arg0, local_13c arg1,
// local_138 arg2, local_158/local_a64/local_134/local_718 program-buffer cursor,
// local_148 blocking-anim flag, local_130 fast-forward flag, local_124 inv-dirty
// flag, local_a68 anim slot, iStack_720 IF/ELSE nesting counter, iStack_12c item
// alias, etc.) are kept exactly because this fragment is textually included inside
// RunProg_Exec, which declares all of them.
//
// NOTE: The decompiler emitted no `case 0:` label; opcode 0x00 (and unimplemented
// opcodes in this range) fall through to the dispatcher `default` (LAB_00468d22).
// Per RUNPROG_OPCODES.md 0x00-0x04 are the INVCHAIN_0..4 family — only 0x01..0x04
// appear as distinct cases here; 0x00 is not a separate label in this build.

case 1:  // 0x01 INVCHAIN_1 (load anim / quick-run variant)
  if (local_130 == 0) {
    local_a68 = Anim_AddByNum(local_140,0,0);
    Anim_SetWalkTableBase(local_a68,local_13c);
    iVar1 = g_nScriptNextOp;
  }
  else {
    GI_SetDrawMode(0);
    func_0x00401000(local_140); /* TODO resolve */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 2:  // 0x02 INVCHAIN_2
  break;
case 3:  // 0x03 INVCHAIN_3
  DAT_00712838 = 1;
  DAT_0070ded0 = *(undefined4 *)(_DAT_0070b5c4 + local_140 * 4);
  break;
case 4:  // 0x04 INVCHAIN_4 / set var
  (&g_anSpeechPlayed)[local_140] = local_13c;
  iVar1 = g_nScriptNextOp;
  break;
case 5:  // 0x05 INC_VAR  var[arg0]++
  (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + 1;
  iVar1 = g_nScriptNextOp;
  break;
case 6:  // 0x06 DEC_VAR  var[arg0]--
  (&g_anSpeechPlayed)[local_140] = (&g_anSpeechPlayed)[local_140] + -1;
  iVar1 = g_nScriptNextOp;
  break;
case 7:  // 0x07 AREA_NODE_DISABLE
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
case 8:  // 0x08 AREA_NODE_ENABLE
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
case 9:  // 0x09 IF_VAR_LE  skip block unless var[arg0] <= arg1
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
        iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
        if (iVar1 != 0) {
          iStack_720 = iStack_720 + 1;
        }
      }
    }
  }
  break;
case 10:  // 0x0a IF_VAR_NE  skip block unless var[arg0] != arg1
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
        iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
        if (iVar1 != 0) {
          iStack_720 = iStack_720 + 1;
        }
      }
    }
  }
  break;
case 0xb:  // 0x0b IF_VAR_GE  skip block unless var[arg0] >= arg1
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
        iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
        if (iVar1 != 0) {
          iStack_720 = iStack_720 + 1;
        }
      }
    }
  }
  break;
case 0xc:  // 0x0c INV_ADD_ITEM  add item arg0 to inventory
  iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                       s__current_004d7bd8);
  if (iVar1 == 0) {
    iStack_12c = DAT_007d67b4;
  }
  else {
    iStack_12c = local_140;
  }
  uVar5 = func_0x00401f46(iStack_12c); /* TODO resolve */
  func_0x00401415(uVar5);              /* TODO resolve */
  func_0x00401884(9999,0);             /* TODO resolve */
  local_124 = '\x01';
  iVar1 = g_nScriptNextOp;
  break;
case 0xd:  // 0x0d INV_REMOVE_ITEM  remove item arg0; clear drag slot; cursor->talk
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
    func_0x0040108c(); /* TODO resolve */
    DAT_00629c08 = -1;
    local_a60 = '\0';
    g_nCursorMode = 3;
    _DAT_007d5a80 = 0xffffffff;
  }
  uVar5 = func_0x00401f46(iStack_12c); /* TODO resolve */
  func_0x00401ea1(uVar5);              /* TODO resolve */
  local_124 = '\x01';
  iVar1 = g_nScriptNextOp;
  break;
case 0xe:  // 0x0e IF_VAR_EQ  skip block unless var[arg0] == arg1
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
        iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
        if (iVar1 != 0) {
          iStack_720 = iStack_720 + 1;
        }
      }
    }
  }
  break;
case 0xf:  // 0x0f ENDIF  end-of-IF marker (no-op on normal fall-through)
  break;
case 0x10:  // 0x10 ELSE  unconditional skip to matching ENDIF
  iStack_720 = 0;
  while (local_a64 = local_a64 + 1, iVar1 = g_nScriptNextOp, local_a64 < local_718) {
    FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
    if (local_144 == 0xf) {
      iVar1 = g_nScriptNextOp;
      if (iStack_720 == 0) break;
      iStack_720 = iStack_720 + -1;
    }
    else {
      iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
      if (iVar1 != 0) {
        iStack_720 = iStack_720 + 1;
      }
    }
  }
  break;
case 0x11:  // 0x11 IF_INV_HAS  skip block unless inventory contains item arg0
  iVar1 = FUN_0049a830(*(undefined4 *)((int)(&g_apItems)[local_140] + 0xd0),
                       s__current_004d7bf0);
  if (iVar1 == 0) {
    iStack_12c = DAT_007d67b4;
  }
  else {
    iStack_12c = local_140;
  }
  iVar3 = func_0x00401f46(iStack_12c); /* TODO resolve */
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
        iVar1 = func_0x00401a91(local_144); /* TODO resolve: is-IF-opener predicate */
        if (iVar1 != 0) {
          iStack_720 = iStack_720 + 1;
        }
      }
    }
  }
  break;
case 0x12:  // 0x12 (unimplemented / reserved)
  break;
case 0x13:  // 0x13 FREE_ANIM  mark anim slot for dump, or free immediately in cutscene
  iVar1 = FUN_0049a830(*(undefined4 *)(DAT_0070c24c + local_140 * 4),s__this_004d7bfc);
  if (iVar1 == 0) {
    func_0x004018fc(*(undefined4 *)(&g_anAnimSlotNum + local_a68 * 0x58)); /* TODO resolve */
  }
  else {
    local_a68 = Anim_FindSlotByName(local_140,DAT_0070e130);
    func_0x004018fc(local_140); /* TODO resolve */
  }
  iVar1 = g_nScriptNextOp;
  if (local_a68 != -1) {
    if (local_130 == 0) {
      Anim_MarkForDump(local_a68);
      iVar1 = g_nScriptNextOp;
    }
    else {
      func_0x004014c9(local_a68); /* TODO resolve */
      Anim_Free(local_a68);
      iVar1 = g_nScriptNextOp;
    }
  }
  break;
case 0x14:  // 0x14 PLAY_MUSIC  Thm_Play track arg0 only if different (skipped in load)
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
case 0x15:  // 0x15 PLAY_SOUND  start SFX from resource arg0; track sound
  if ((g_nSndSubtitleOnly == 0) && (local_130 == 0)) {
    Debug_Trace(DAT_004d7b60 + 0x1ee,s_C__DevStudio_Projects_Crux_RUNPR_004d7c14,
                s_SOUND_command_004d7c04);
    Fx_PlayChar(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
    RunProg_TrackSound(3);
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x16:  // 0x16 (unimplemented / reserved)
  break;
case 0x17:  // 0x17 (unimplemented / reserved)
  break;
case 0x18:  // 0x18 WALK_TO  walk char to (arg1,arg2); freeze pos in cutscene; move-done cb
  if (local_130 != 0) {
    Mov_FreezePos();
  }
  local_714 = func_0x00401f6e(local_13c,local_138); /* TODO resolve: node lookup */
  Mov_PathfindTo(local_714);
  iVar1 = g_nScriptNextOp;
  if (local_a64 + 1 < local_718) {
    func_0x0040191a(local_714,local_148,&local_130); /* TODO resolve: wait-move-done */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x19:  // 0x19 LOAD_ANIM_LOOPING  Anim_AddByNum looping; walk-table base; add to group
  local_a68 = Anim_AddByNum(local_140,1,0);
  if (_DAT_00574bec != 0) {
    Anim_AddToGroup(local_a68);
    _DAT_00574bec = _DAT_00574bec - 1;
  }
  Anim_SetWalkTableBase(local_a68,local_13c);
  iVar1 = g_nScriptNextOp;
  break;
case 0x1a:  // 0x1a STOP_MUSIC  Theme_StopMusic + clear current-music name (skipped in load)
  if (g_nSndSubtitleOnly == 0) {
    Theme_StopMusic();
    FUN_004895e0(&DAT_00629880,0x7c49bc);
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x1b:  // 0x1b (unimplemented / reserved)
  break;
case 0x1c:  // 0x1c SET_VAR_RAND  var[arg0] = (timeGetTime() * rand()) % arg1
  iVar1 = FUN_00489cf0(); /* rand */
  DVar4 = timeGetTime();
  (&g_anSpeechPlayed)[local_140] = (DVar4 * iVar1) % local_13c;
  iVar1 = g_nScriptNextOp;
  break;
case 0x1d:  // 0x1d FREEZE_POS  Mov_FreezePos()
  Mov_FreezePos();
  iVar1 = g_nScriptNextOp;
  break;
case 0x1e:  // 0x1e RESTORE_POS  restore frozen movement position
  func_0x0040105a(); /* TODO resolve: Mov_RestorePos */
  iVar1 = g_nScriptNextOp;
  break;
case 0x1f:  // 0x1f WAIT_ANIM_END  set stop frame to last; spin-wait until reached (abort on skip)
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
case 0x20:  // 0x20 WAIT_MOUSE_CLICK  cursor->talk; pump events until mouse pressed; store area
  if (local_130 == 0) {
    cStack_128 = (char)g_nCursorMode;
    g_nCursorMode = 2;
    do {
      while( true ) {
        Adv_CursorHandler(auStack_3ec,aiStack_400,auStack_8bc,auStack_724);
        iVar1 = func_0x00401258(); /* TODO resolve */
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
case 0x21:  // 0x21 (unimplemented / reserved)
  break;
case 0x22:  // 0x22 (unimplemented / reserved)
  break;
case 0x23:  // 0x23 (unimplemented / reserved)
  break;
case 0x24:  // 0x24 (unimplemented / reserved)
  break;
case 0x25:  // 0x25 (unimplemented / reserved)
  break;
case 0x26:  // 0x26 HIDE_INVENTORY  (skipped in load)
  if (g_nSndSubtitleOnly == 0) {
    func_0x00402072(); /* TODO resolve: Inv_Hide */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x27:  // 0x27 SHOW_INVENTORY  (skipped in load)
  if (g_nSndSubtitleOnly == 0) {
    func_0x00401d7f(); /* TODO resolve: Inv_Show */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x28:  // 0x28 (unimplemented / reserved)
  break;
case 0x29:  // 0x29 (unimplemented / reserved)
  break;
case 0x2a:  // 0x2a (unimplemented / reserved)
  break;
case 0x2b:  // 0x2b HIDE_CURSOR  (skipped in load)
  if (g_nSndSubtitleOnly == 0) {
    func_0x0040178a(); /* TODO resolve: Cursor_Hide */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x2c:  // 0x2c SHOW_CURSOR  (skipped in load)
  if (g_nSndSubtitleOnly == 0) {
    func_0x004010b9(); /* TODO resolve: Cursor_Show */
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x2d:  // 0x2d WAIT_SOUND_END  spin-wait until sound channel 3 finishes (abort on skip)
  if ((local_130 == 0) && (g_nAdvTickSuppressed == 0)) {
    do {
      iVar3 = func_0x00401875(3); /* TODO resolve: Snd_IsPlaying(ch) */
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
case 0x2e:  // 0x2e SET_CARRY_HINT_7  g_nMovCarryHint = 7
  g_nMovCarryHint = 7;
  break;
case 0x2f:  // 0x2f (unimplemented / reserved)
  break;
case 0x30:  // 0x30 SET_CARRY_HINT  g_nMovCarryHint = lookup(arg1)
  g_nMovCarryHint = func_0x004020a4(local_13c); /* TODO resolve: carry-hint lookup */
  iVar1 = g_nScriptNextOp;
  break;
case 0x31:  // 0x31 CLEAR_CARRY_HINT  g_nMovCarryHint = 0
  g_nMovCarryHint = 0;
  break;
case 0x32:  // 0x32 SET_ENTRY_POINT  set scene entry-point name to "entry"
  FUN_004895e0(&DAT_00629b08,s_entry_004dc350);
  iVar1 = g_nScriptNextOp;
  break;
case 0x33:  // 0x33 SET_STATE_2  DAT_00712838 = 2
  DAT_00712838 = 2;
  break;
case 0x34:  // 0x34 (unimplemented-in-spec; transcribed as decompiled)
  func_0x00401451(); /* TODO resolve */
  iVar1 = g_nScriptNextOp;
  break;
case 0x35:  // 0x35 (unimplemented-in-spec; transcribed as decompiled)
  func_0x00401663(); /* TODO resolve */
  iVar1 = g_nScriptNextOp;
  break;
case 0x36:  // 0x36 RESET_GAME  Adv_* reset/restart game state
  Adv_CompactInvList(0);
  iVar1 = g_nScriptNextOp;
  break;
default:
  goto LAB_00468d22;
case 0x39:  // 0x39 RESET_AUDIO  tear down all 4 audio channels, SndMem_Init, clear music name
  Snd_Stop(2);
  Snd_Stop(3);
  Snd_Stop(1);
  Snd_Stop(0);
  SndMem_Init();
  FUN_004895e0(&DAT_00629880,0x7c49c0);
  iVar1 = g_nScriptNextOp;
  break;
case 0x3a:  // 0x3a SET_WALK_TARGET  store node (field[0x25]==arg1+0x1e) position as walk dest
  if (local_13c == 1000) {
    _DAT_00629da0 = -1;
  }
  local_714 = 0;
  while (((int)local_714 < g_nAreaNodeCount &&
         ((&g_pAreaNodeTable)[local_714][0x25] != local_13c + 0x1e))) {
    local_714 = local_714 + 1;
  }
  if ((int)local_714 < g_nAreaNodeCount) {
    _DAT_00629da0 = *(&g_pAreaNodeTable)[local_714];
    _DAT_00629da8 = (&g_pAreaNodeTable)[local_714][1];
    _DAT_00629da4 = (&g_pAreaNodeTable)[local_714][2];
    _DAT_00629dac = (&g_pAreaNodeTable)[local_714][3];
  }
  break;
case 0x3b:  // 0x3b RESTART_SCRIPT  reset if inventory dirty; param_1=arg0; restart interpreter
  if (local_124 != '\0') {
    Adv_CompactInvList(0);
  }
  param_1 = local_140;
  goto LAB_00462616;
case 0x3c:  // 0x3c SET_CALLBACK_ID  store arg0 as pending completion-callback script ID
  iRam007c4998 = local_140;
  break;
case 0x3d:  // 0x3d LOAD_ANIM_ONESHOT  Anim_AddByNum non-looping; walk-table base; completion cb
  local_a68 = Anim_AddByNum(local_140,0,0);
  Anim_SetWalkTableBase(local_a68,local_13c);
  Anim_SetCompletionCallback(local_a68,iRam007c4998,1,0xffffffff);
  iVar1 = g_nScriptNextOp;
  break;
case 0x3e:  // 0x3e LOAD_ANIM_CB  (non-cutscene) load looping anim arg0; cb with arg1 repeat count
  if (local_130 == 0) {
    local_a68 = Anim_AddByNum(local_140,1,0);
    Anim_SetWalkTableBase(local_a68,local_138);
    Anim_SetCompletionCallback(local_a68,iRam007c4998,local_13c,0xffffffff);
    iVar1 = g_nScriptNextOp;
  }
  break;
case 0x3f:  // 0x3f STORE_ANIM_SLOT  find anim slot by name arg0, store into DAT_007c4108 ("stani")
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
