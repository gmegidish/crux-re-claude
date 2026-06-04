// RunProg_Exec opcode handlers 0x1839-0x5b23 — #included into RunProg_Exec switch

case 7000: // 0x1b58 TT_INIT
  Tt_Init(local_13c,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1839: // 0x1839 ADV_SET_OBJ_PROPERTY
  func_0x00401cd5(local_140,local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x183a: // 0x183a ADV_TRIGGER_OBJ_EVENT
  func_0x00402063(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x183c: // 0x183c ADV_RESET_INPUT
  func_0x004010be();
  iVar1 = g_nScriptNextOp;
  break;

case 0x1840: // 0x1840 ADV_SET_OBJ_POS
  func_0x00401a00(local_140,local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1842: // 0x1842 ADV_ATTACH_OBJ_TO_NODE
  func_0x00401b63(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b59: // 0x1b59 THEME_SET_THEME
  func_0x0040138e(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5a: // 0x1b5a THEME_FADE
  func_0x00401ebf(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5b: // 0x1b5b THEME_BG
  func_0x00401933(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5c: // 0x1b5c THEME_LAYER
  func_0x00401910(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5d: // 0x1b5d THEME_FADE_RANGE
  func_0x00401afa(local_140,local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5e: // 0x1b5e THEME_FADE_STEP
  func_0x00401708(local_13c,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b5f: // 0x1b5f THEME_APPLY_PALETTE
  func_0x00401b81(local_13c,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b60: // 0x1b60 THEME_CROSSFADE
  func_0x00401dac(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b61: // 0x1b61 THEME_OP
  func_0x00401bd1(local_13c,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b6c: // 0x1b6c GI_GET_SCREEN_MODE
  iVar1 = func_0x00401fe1();
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b6d: // 0x1b6d GI_SET_SCREEN_MODE
  func_0x0040156e((&g_anSpeechPlayed)[local_140]);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b6e: // 0x1b6e GI_GET_PROG_PARAM
  iVar1 = func_0x00401cf3(param_2);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b6f: // 0x1b6f GI_OP
  func_0x004019b5((&g_anSpeechPlayed)[local_140],param_2,local_13c,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b70: // 0x1b70 GI_GET_WIDTH
  iVar1 = func_0x00402027((&g_anSpeechPlayed)[local_140]);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b71: // 0x1b71 GI_GET_HEIGHT
  iVar1 = func_0x00402031((&g_anSpeechPlayed)[local_140]);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b76: // 0x1b76 GFX_FLUSH_BITMAP
  Tt_CharSet(*(undefined4 *)(DAT_0070c24c + local_140 * 4));
  iVar1 = g_nScriptNextOp;
  break;

case 0x1b80: // 0x1b80 GFX_RELOAD_PALETTE
  Tt_CharRemove();
  iVar1 = g_nScriptNextOp;
  break;

case 0x5b23: // 0x5b23 SPEECH_INIT
  Speech_Init();
  iVar1 = g_nScriptNextOp;
  break;
