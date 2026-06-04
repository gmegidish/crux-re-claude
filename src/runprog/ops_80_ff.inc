// RunProg_Exec opcode handlers 0x80-0xff — #included into RunProg_Exec switch
//
// Faithful transcription of the real `case` blocks decompiled from
// RunProg_Exec (0x00462560) for the opcode sub-range 0x80-0xff.
//
// UNUSED / FALL-THROUGH ranges (no case label — handled by `default`,
// emits the "RTSI Unrec cmd" debug trace):
//   0x80-0xc6  (entire range unused; previous real case is 0x78)
//   0xd2-0xfe  (unused)
// The ONLY real opcodes in 0x80-0xff are 0xc7-0xd1 and 0xff.
//
// Local names are kept EXACTLY as the decompiler emitted them — this file is
// textually included into RunProg_Exec, so all locals (local_140, local_13c,
// local_138, local_130, local_148, local_704, local_710, local_714, local_718,
// local_a64, local_144, local_158, local_134, iVar1, ...) and the goto targets
// code_r0x00464ea4 / code_r0x00464ec1 resolve to the enclosing function's scope.

        // 0xc7 SCM_ADD_CHAR — register character arg0 into SCM playlist (variant of 0x78)
        case 199:
          func_0x00401960(local_140);
          iVar1 = g_nScriptNextOp;
          break;
        // 0xc8 MUSIC_CTL — sign-dispatched: arg1<0 stop music; arg1>=0 play music channel
        case 200:
          if ((int)local_13c < 0) {
            func_0x00401078(-local_13c);
            iVar1 = g_nScriptNextOp;
          }
          else {
            func_0x00401a7d(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          break;
        // 0xc9 (no-op)
        case 0xc9:
          break;
        // 0xca SPEECH_SKIP — seek to correct speech variant by var[arg0] heard-count; scan forward to 0xcb
        case 0xca:
          if (local_138 == 0) {
            local_138 = 1;
          }
          local_710 = local_138 + 1;
          local_714 = local_a64 + ((&g_anSpeechPlayed)[local_140] - local_13c) * local_138;
          if ((int)local_714 < local_a64) {
            for (; local_a64 < local_718; local_a64 = local_a64 + 1) {
              FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
              if (local_144 == 0xcb) {
                local_710 = 0;
                break;
              }
            }
          }
          do {
            if ((local_718 <= local_a64) || ((int)local_714 < local_a64)) goto code_r0x00464ec1;
            FUN_004896d0(&local_144,(int)local_158 + local_a64 * local_134 + 4,local_134);
            if (local_144 == 0xcb) goto code_r0x00464ea4;
            local_a64 = local_a64 + 1;
          } while( true );
        // 0xcb END_SPEECH_BLOCK — close a 0xca speech-variant region
        case 0xcb:
          local_710 = 0;
          break;
        // 0xcc (no-op)
        case 0xcc:
          break;
        // 0xcd START_SPEECH — SndMem_StartSpeech + speech-active flag; unlimited subtitle lines
        case 0xcd:
          if (local_130 == 0) {
            if ((g_nAdvTickSuppressed != 0) && (local_704 == -1)) {
              local_704 = Txt_GetMaxLines();
              Txt_SetMaxLines(0xffffffff);
            }
            SndMem_StartSpeech(*(undefined4 *)(_DAT_0070dec4 + local_140 * 4));
            RunProg_TrackSound(1);
            iVar1 = g_nScriptNextOp;
          }
          break;
        // 0xce WAIT_SPEECH — SndMem_WaitSpeech; cutscene-abort if done in skip mode
        case 0xce:
          if (local_130 == 0) {
            local_714 = SndMem_WaitSpeech(0);
            iVar1 = g_nScriptNextOp;
            if ((local_714 != 0) && (local_148 != 0)) {
              Adv_TickFramesNoAsync(1);
              local_130 = 1;
              g_nAdvTickSuppressed = 4;
              iVar1 = g_nScriptNextOp;
            }
          }
          break;
        // 0xcf TURN_AROUND — Mov_TurnAround(arg1); stores carry hint in skip mode
        case 0xcf:
          if (local_130 == 0) {
            Mov_TurnAround(local_13c);
            iVar1 = g_nScriptNextOp;
          }
          else if (local_13c == 0) {
            g_nMovCarryHint = 3;
          }
          else {
            g_nMovCarryHint = local_13c;
          }
          break;
        // 0xd0 AREA_LOCK_ALL — set all walkable nodes (flags==0) to blocked (flags=2)
        case 0xd0:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 == 0) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            CONCAT11(2,(char)(&g_pAreaNodeTable)[local_714][4]));
            }
          }
          break;
        // 0xd1 AREA_UNLOCK_ALL — restore all script-blocked nodes (flags==2) to walkable (flags=0)
        case 0xd1:
          for (local_714 = 0; iVar1 = g_nScriptNextOp, (int)local_714 < g_nAreaNodeCount;
              local_714 = local_714 + 1) {
            if (((&g_pAreaNodeTable)[local_714][4] << 0x10) >> 0x18 == 2) {
              (&g_pAreaNodeTable)[local_714][4] =
                   CONCAT22((short)((uint)(&g_pAreaNodeTable)[local_714][4] >> 0x10),
                            (ushort)(byte)(&g_pAreaNodeTable)[local_714][4]);
            }
          }
          break;
        // 0xff HALT — script-end sentinel; interpreter loop exits (shares body with 0x100)
        case 0xff:
        case 0x100:
          break;
