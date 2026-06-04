// RunProg_Exec opcode handlers 0x960-0xc1c — #included into RunProg_Exec switch

case 0x960: // 0x960 GV_CLOSE_INVENTORY
  thunk_FUN_0043e100();
  iVar1 = g_nScriptNextOp;
  break;

case 0x961: // 0x961 INV_SET_ITEM_COUNT
  func_0x0040128a(local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x962: // 0x962 INV_MOVE_ITEM
  func_0x004017d5(local_13c,local_140,local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x963: // 0x963 INV_SELECT_ITEM
  func_0x00401a96(local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x964: // 0x964 INV_GET_ITEM_COUNT
  iVar1 = func_0x00401ded(local_13c);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x965: // 0x965 INV_SET_ITEM_STATE
  func_0x00401901((&g_anSpeechPlayed)[local_140],local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x966: // 0x966 INV_SET_ITEM_FLAG
  func_0x004013c5((&g_anSpeechPlayed)[local_140],local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x967: // 0x967 INV_ENABLE_ITEM
  func_0x0040114f(local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x968: // 0x968 INV_GET_ITEM_STATE
  iVar1 = func_0x004011ae(local_13c);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x969: // 0x969 INV_IS_ITEM_ENABLED
  iVar1 = func_0x0040151e(local_13c);
  (&g_anSpeechPlayed)[local_140] = iVar1;
  iVar1 = g_nScriptNextOp;
  break;

case 0x96a: // 0x96a INV_SET_ITEM_OWNER
  func_0x004015f0(local_13c,local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x96b: // 0x96b GV_OPEN_INV_WITH_ANIM
  thunk_FUN_0043e7f0(local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x96c: // 0x96c INV_RESET_ALL
  func_0x004013ed();
  iVar1 = g_nScriptNextOp;
  break;

case 0x974: // 0x974 ADV_SET_FOCUS
  func_0x00401a73(local_13c);
  iVar1 = g_nScriptNextOp;
  break;

case 0x975: // 0x975 AREA_DRAW_NODE_RECT
  iStack_150 = func_0x0040116d(local_140);
  func_0x00402009(local_13c,*(&g_pAreaNodeTable)[iStack_150],
                  (&g_pAreaNodeTable)[iStack_150][1],(&g_pAreaNodeTable)[iStack_150][2],
                  (&g_pAreaNodeTable)[iStack_150][3],local_138);
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c4: // 0x9c4 GV_OPEN_INVENTORY
  GV_OpenInventory();
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c5: // 0x9c5 GRAN_SLIDER_POS
  func_0x00401622(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c6: // 0x9c6 GRAN_SLIDER_MAX
  func_0x00401b09(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c7: // 0x9c7 GRAN_SLIDER_RESET
  func_0x004018e3();
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c8: // 0x9c8 GRAN_SLIDER_TICK
  func_0x00401695();
  iVar1 = g_nScriptNextOp;
  break;

case 0x9c9: // 0x9c9 GRAN_SLIDER_GET
  func_0x00401c67();
  iVar1 = g_nScriptNextOp;
  break;

case 0xc02: // 0xc02 GRAN_INIT_SLIDER
  Gran_InitSlider(local_140);
  iVar1 = g_nScriptNextOp;
  break;

case 0xc1c: // 0xc1c MOV_INIT_GAME_MODE
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
  break;
