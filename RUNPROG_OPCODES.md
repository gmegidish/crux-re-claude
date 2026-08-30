# CRUX.EXE Script Bytecode Reference — `RunProg_Exec`

> The game's script virtual machine. `RunProg_Exec` (`0x00462560`, ~3,232 decompiled
> lines) is a single giant `switch` over ~400 opcodes. Every verb interaction,
> dialogue line, cutscene, room transition, inventory action, and puzzle in the game
> is encoded as a stream of these opcodes.

## VM model

- **`RunProg_Exec(uint param_1, int id)`** — `param_1` = script **program ID**, an index into `g_pScriptPrograms[]` (`0x007114c8`).
- Each program is a flat buffer: `int count` header followed by `count` instructions.
- **Instruction stride** = `0x10` bytes (16). Decoded via `memcpy(&local_144, pc+1, 0x10)`:
  - `local_144` = **opcode**
  - `local_140` = **arg0** (entity / variable index / item ID)
  - `local_13c` = **arg1**
  - `local_138` = **arg2**
- **`g_anSpeechPlayed[]`** (`0x004da...`) — despite the name, this 1500-entry int array doubles as the **general-purpose script variable / register file**. Opcodes read/write it as `var[arg0]`.
- **`g_nScriptNextOp`** (`0x0070a1f0`) — continuation value reloaded after most cases.
- **Control flow:** IF/ELSE/ENDIF use opcodes `0x09–0x11` with a forward-scan that
  skips to the matching `0x0f` (ENDIF) / `0x10` (ELSE), balancing nested blocks.
- **Subroutines:** `0x065` / `0x168` push PC + context onto a call stack and jump (GOSUB).
- **RPN evaluation stack:** `0x173/0x174` push/pop, `0x182/0x183/0x184` add/sub/mul.
- **Cutscene/skip state:** `DAT_00629f50` (speech-wait state machine), `local_130`
  (fast-forward), `local_148` (blocking-anim flag). Right-click aborts cutscenes.

## Opcode map

Opcodes not listed fall through to the `default` handler, which emits the
`"RTSI Unrec cmd"` debug trace (i.e. they are unused / reserved). The implemented
opcode count is **~390**; the address space is sparse with large reserved gaps
(e.g. nearly all of `0x80–0xc6`, `0xd2–0xfe`, `0x101–0x124` are unused).

### Flow control & variables (0x00–0x1f)

| Opcode | Name | Action |
|--------|------|--------|
| 0x001 | LOAD_ANIM | `Anim_AddByNum(animName(a0),loop=0,0)` + `Anim_SetWalkTableBase(slot,a1)` — load anim NON-looping (non-looping sibling of 0x19). Skip-mode branch: `GI_SetDrawMode(0)`+draw. ⚠ NOT "INVCHAIN" — the old table here was wrong (verified vs `RunProg_Exec` case 1) |
| 0x002 | NOP | empty `case 2: break;` in the engine |
| 0x003 | NEXT_AREA (INVCHAIN_3) | `DAT_00712838 = 1; DAT_0070ded0 = areaBank[a0]` — queue a transition; target = the selected area-name bank `[a0]`. `_DAT_0070b5c4` is set by `RunProg_SelectAreaContext`: in the default/primary context (`DAT_0070e130==0`) it = the exit-name table (`0x0070d560`), so this is `exitName(a0)`. The port's `case 0x03 → nextArea_=exitName(a0)` is correct for the primary context (the secondary bank isn't modeled). |
| 0x004 | SET_VAR | `var[arg0] = arg1` |
| 0x005 | INC_VAR | `var[arg0]++` |
| 0x006 | DEC_VAR | `var[arg0]--` |
| 0x007 | AREA_NODE_DISABLE | Clear enabled-flag on all area nodes with tag==arg0; refresh slots |
| 0x008 | AREA_NODE_ENABLE | Set enabled-flag on all area nodes with tag==arg0; refresh slots |
| 0x009 | IF_VAR_LE | Skip block unless `var[arg0] <= arg1` |
| 0x00a | IF_VAR_NE | Skip block unless `var[arg0] != arg1` |
| 0x00b | IF_VAR_GE | Skip block unless `var[arg0] >= arg1` |
| 0x00c | INV_ADD_ITEM | Add item arg0 to inventory (resolves "current" alias; waits) |
| 0x00d | INV_REMOVE_ITEM | Remove item arg0 from inventory; clears drag slot; cursor→talk |
| 0x00e | IF_VAR_EQ | Skip block unless `var[arg0] == arg1` |
| 0x00f | ENDIF | End-of-IF marker (no-op on normal fall-through) |
| 0x010 | ELSE | Unconditional skip to matching ENDIF |
| 0x011 | IF_INV_HAS | Skip block unless inventory contains item arg0 |
| 0x013 | FREE_ANIM | Mark anim slot (by name arg0) for dump, or free immediately in cutscene |
| 0x014 | PLAY_MUSIC | `Thm_Play` track arg0 only if different from current (skipped in load) |
| 0x015 | PLAY_SOUND | Start SFX from resource arg0; cursor→use |
| 0x018 | WALK_TO | Walk char to (arg1,arg2); freezes pos in cutscene; sets move-done callback |
| 0x019 | LOAD_ANIM_LOOPING | `Anim_AddByNum` looping; set walk-table base arg1; add to active group |
| 0x01a | STOP_MUSIC | `Theme_StopMusic` + clear current-music name (skipped in load) |
| 0x01c | SET_VAR_RAND | `var[arg0] = (timeGetTime() * rand()) % arg1` |
| 0x01d | FREEZE_POS | `Mov_FreezePos()` |
| 0x01e | RESTORE_POS | Restore frozen movement position |
| 0x01f | WAIT_ANIM_END | Set anim stop-frame to last; spin-wait until reached (abort on skip) |

### Audio / UI / system (0x20–0x3f)

| Opcode | Name | Action |
|--------|------|--------|
| 0x020 | WAIT_MOUSE_CLICK | Cursor→talk; pump events until mouse button pressed; store clicked area |
| 0x026 | HIDE_INVENTORY | Hide/close inventory panel (skipped in load) |
| 0x027 | SHOW_INVENTORY | Show/open inventory panel (skipped in load) |
| 0x02b | HIDE_CURSOR | Hide the cursor (skipped in load) |
| 0x02c | SHOW_CURSOR | Show the cursor (skipped in load) |
| 0x02d | WAIT_SOUND_END | Spin-wait until sound channel 3 finishes (abort on skip) |
| 0x02e | SET_CARRY_HINT_7 | `g_nMovCarryHint = 7` |
| 0x030 | SET_CARRY_HINT | `g_nMovCarryHint = lookup(arg1)` |
| 0x031 | CLEAR_CARRY_HINT | `g_nMovCarryHint = 0` |
| 0x032 | SET_ENTRY_POINT | Set scene entry-point name to "entry" |
| 0x033 | SET_STATE_2 | `DAT_00712838 = 2` |
| 0x036 | RESET_GAME | `Adv_*` reset/restart game state |
| 0x039 | RESET_AUDIO | Tear down all 4 audio channels, `SndMem_Init`, clear music name |
| 0x03a | SET_WALK_TARGET | Store node (field[0x25]==arg1+0x1e) position as walk destination |
| 0x03b | RESTART_SCRIPT | Reset if inventory dirty; set param_1=arg0; restart interpreter |
| 0x03c | SET_CALLBACK_ID | Store arg0 as pending completion-callback script ID |
| 0x03d | LOAD_ANIM_ONESHOT | `Anim_AddByNum` non-looping; walk-table base arg1; completion cb |
| 0x03e | LOAD_ANIM_CB | (non-cutscene) load looping anim arg0; cb with arg1 repeat count |
| 0x03f | STORE_ANIM_SLOT | Find anim slot by name arg0, store into `DAT_007c4108` (current "stani") |

### Dialog, scene, character (0x40–0x78)

| Opcode | Name | Action |
|--------|------|--------|
| 0x42 | SET_CURRENT_ITEM ⚠ | Reset item state, then `g_nCurrentItem`(0x007d67b4)`= a0` and item-mode 1 — "start using item a0". The `_current` alias in 0xc/0xd/0x11 resolves to this |
| 0x43 | CLEAR_CURRENT_ITEM ⚠ | **No arguments** (junk in the arg slot). Same reset with `g_nCurrentItem = -1`: cancel. Both also clear the drag slot (0x00629c08), set `g_nCursorMode`(0x00646754)`= 3`, `Adv_ClearCursorState()` + `Win_UpdateCursor()`. **Not dialog ops** |
| 0x041 | REMOVE_AREA_SPRITE | Find & remove sprite matching arg0 from `g_anAreaSpriteList` |
| 0x042 | BEGIN_DIALOG | Enter conversation mode (disable cursor, set dialog char, cursor→1) |
| 0x043 | END_DIALOG | Exit conversation mode (dialog char→-1, cursor→walk) |
| 0x044 | PLAY_ANIM_WAIT | `Anim_AddByName`; read trigger hotspot; loop ticking arg1 times |
| 0x045 | SET_INTERACT_MODE1 | `DAT_004c4c48 = 1` |
| 0x046 | SET_INTERACT_MODE2 | Copy named-anim string to global; `DAT_004c4c48 = 2` |
| 0x047 | ENABLE_INPUT_FLAG | `DAT_00629f4c = 1` |
| 0x048 | DISABLE_INPUT_FLAG | `DAT_00629f4c = 0` |
| 0x049 | BREAK_IF_SKIP | Interrupt current program if skip active |
| 0x04a | SET_INTERACT_MODE3 | `DAT_004c4c48 = 3` |
| 0x04d | FADE_TO_ROOM | Timed fade (default 3000ms) + copy destination room name |
| 0x04f | ADD_WALK_ANIM | `Anim_AddByNum` type 3 (walk cycle) + walk-table base |
| 0x050 | SHOW_TEXT_WAIT | Display text string, wait (interrupt on skip) |
| 0x054 | SET_ANIM_TICK_MODE | `Anim_SetTickMode(arg1)` |
| 0x055 | SET_WALK_MODE | Movement-mode configuration switch on arg1 |
| 0x061 | PLACE_ANIM | `Anim_AddByNum` type 1 + position snapped to reference anim |
| 0x065 | CALL_SCRIPT | Push PC/scene/state; param_1=arg0; jump (subroutine call) |
| 0x067 | SAY_TEXT | Char setup + display text + wait (text-only speech) |
| 0x068 | SET_CHAR_FLAG | Set property/flag on character arg0 |
| 0x06b | SET_TIMER | Store timer/delay value |
| 0x06f | IF_OBJECT_NOT_IN_LIST | Skip block if object arg0 not in `DAT_0070e458` list |
| 0x070 | END_SCRIPT | Force instruction loop to terminate (early break) |
| 0x071 | RUN_SCENE | Run named subscene; if arg1==1000 `Player_ScmInit` after |
| 0x072 | SET_ROOM | Resolve/normalize room name, change active room if different |
| 0x073 | SET_SPEECH_PLAYED | `var[arg0] = DAT_00629dc0` (record speech result) |
| 0x076 | SAY_SPEECH | Char setup + `SndMem_StartSpeech` (voice-only) |
| 0x078 | ADD_CHAR_TO_SCENE | `Player_ScmAddChar(name)` |

### Speech variants & area locking (0xc7–0xd1, 0xff)

| Opcode | Name | Action |
|--------|------|--------|
| 0x0c7 | SCM_ADD_CHAR | Register character arg0 into SCM playlist (variant of 0x78) |
| 0x0c8 | MUSIC_CTL | Sign-dispatched: arg1<0 → stop music; arg1>=0 → play music channel |
| 0x0ca | SPEECH_SKIP | Seek to correct speech variant by `var[arg0]` heard-count; scan to 0xcb |
| 0x0cb | END_SPEECH_BLOCK | Close a 0xca speech-variant region |
| 0x0cd | START_SPEECH | `SndMem_StartSpeech` + speech-active flag; unlimited subtitle lines |
| 0x0ce | WAIT_SPEECH | `SndMem_WaitSpeech`; cutscene-abort if done in skip mode |
| 0x0cf | TURN_AROUND | `Mov_TurnAround(arg1)`; stores carry hint in skip mode |
| 0x0d0 | AREA_LOCK_ALL | Set all walkable nodes (flags==0) to blocked (flags=2) |
| 0x0d1 | AREA_UNLOCK_ALL | Restore all script-blocked nodes (flags==2) to walkable (flags=0) |
| 0x0ff | HALT | Script-end sentinel; interpreter loop exits |

### Speech sync, palette, anim brackets, slot/item lists (0x100–0x14f)

| Opcode | Name | Action |
|--------|------|--------|
| 0x125 | SET_VAR_FROM_ACC | `var[arg0] = local_404` (internal accumulator) |
| 0x12c | ANIM_SET_FRAME_SOUND | Attach sound to anim frame (`Anim_SetFrameSound` + `SndMem_Load`) |
| 0x12d | SPEECH_WAIT | Speech-wait barrier; set `DAT_00629f50=2` |
| 0x12e | SPEECH_END | Clear speech-wait state; cursor→use unless last instruction |
| 0x12f | PAL_SNAPSHOT | `Sched_SavePaletteSnapshot` + copy to `DAT_007c4110` |
| 0x130 | PAL_RESTORE | Copy snapshot back to target palette; `Anim_EnableDraw` |
| 0x131 | ANIM_BEGIN | Start blocking-anim sequence (`local_148=1`) |
| 0x132 | ANIM_END | End blocking-anim; finalise skip path |
| 0x133 | SET_FOLLOWER | Load anim, clear flag bit 3, `Mov_SetFollower` |
| 0x134 | WAIT_ANIM | Busy-wait until blocking anim interrupted/finished |
| 0x135 | CLEAR_FOLLOWER | `Mov_SetFollower(-1)` |
| 0x136 | ADD_ANIM | `Anim_AddByNum`; optional group; position (arg1,arg2) |
| 0x137 | SET_WALKTABLE | `Anim_SetWalkTableBase(slot, arg1)` for object arg0 |
| 0x13b | WAIT_FRAME | Set stop frame; busy-wait until anim reaches it |
| 0x13c | FREEZE_ANIM | `Anim_SetFrameStep(slot,0)` + `Anim_SetCurrentFrame(slot,arg1)` |
| 0x13d | UNFREEZE_ANIM | `Anim_SetFrameStep(slot,1)` + clear trigger |
| 0x13e | SET_ANI_FRAME | Set current frame of "stani" slot to `var[arg0]` |
| 0x13f | ENSURE_SLOTS | Ensure `arg1+1` anim slots allocated |
| 0x140 | SET_ANIM_PARAM | 3-param anim property (speed/loop/flags) |
| 0x141 | SET_ANIM_PARAM2 | Variant 3-param anim config |
| 0x142 | SET_ANIM_2PARAM | 2-param anim property |
| 0x143 | SET_ANIM_ID | Associate ID/index pair with anim slot |
| 0x144 | ENABLE_SLOT | Activate per-slot record at `DAT_0070e65c + arg1*0x230` |
| 0x145 | COUNT_VALID_ITEMS | Count non-(-1) entries in slot's item array → `var[arg0]` |
| 0x146 | ADD_ITEM_TO_SLOT | Add item to slot container |
| 0x147 | READ_SCRATCH | `var[arg0] = iRam0070decc` |
| 0x148 | WRITE_SCRATCH | `iRam0070decc = var[arg0]` |
| 0x149 | SLOT_ADD_ITEM | Append item arg0 to slot's 10-element list |
| 0x14a | SLOT_REMOVE_ITEM | Remove item arg0 from slot's 10-element list |
| 0x14b | COUNT_ACTIVE_ITEMS | Count active items in slot → `var[arg0]` |
| 0x14c | IF_ITEM_NOT_IN_SLOT | Skip block if item not present in slot arg1 |
| 0x14d | FREEZE_ANIM_LAST | Freeze anim on final frame |
| 0x14e | SCENE_CHANGE | Optional freeze + scene transition (`thunk_FUN_00453320`) |
| 0x14f | SET_ANIM_SUBFRAME | Set sub-frame / frame-range parameter |

### Registers, RPN stack, anim control, palette, save (0x150–0x1ff)

| Opcode | Name | Action |
|--------|------|--------|
| 0x150 | ANIM_ADD_WALKTABLE | Add anim (layer 1), position relative to frame top-left |
| 0x152 | GI_GETPIXEL_TO_REG | Sample pixel at (x,y), map through LUT → `var[reg]` |
| 0x153 | SET_PIXEL_FROM_REG | Write pixel using `var[reg]` as colour; flush |
| 0x154 | IF_NOT_AT_NODE_SKIP | Skip block if pathfind node at (x,y) != `g_nMovDestNode` |
| 0x156 | ANIM_GET_CURRENT_FRAME | Current frame of stani slot → `var[reg]` |
| 0x157 | BANI_NOOP | `Bani_Noop(areaCache[a0])` — `Bani_Noop` (0x00417df0) is an empty stub, so this is a no-op |
| 0x158 | NOP | No-op — the engine's case body is empty (shares the handler with 0x1fd) |
| 0x159 | ANIM_SET_COMPLETION_CB | Set completion callback to current prog + target |
| 0x15a | ANIM_CLEAR_COMPLETION_CB | Clear completion callback |
| 0x15b | REG_ADD_IMM | `var[reg] += imm` |
| 0x15c | REG_SUB_IMM | `var[reg] -= imm` |
| 0x15d | ENTER_CURSOR_SELECT | Enter cursor-selection mode (`DAT_00629f70=2`, cursor→1) |
| 0x15e | AREA_LIST_RESET | `Area_ResetList` (count=0, cursor=0) — selection-list family @0x00414a80.. |
| 0x15f | AREA_LIST_REWIND | `Area_RewindList` (cursor=0) |
| 0x160 | AREA_LIST_SEEK_END | `Area_SeekListEnd` (cursor=count-1) |
| 0x161 | AREA_LIST_NEXT | `var[reg] = Area_ListNext()` (0 ok / -1 at end) |
| 0x162 | AREA_LIST_PREV | `var[reg] = Area_ListPrev()` (0 ok / -1 at start) |
| 0x163 | AREA_LIST_GET | `var[reg] = Area_ListGet()` (value at cursor) |
| 0x164 | AREA_LIST_SET | `Area_ListSet(var[reg])` |
| 0x165 | AREA_LIST_APPEND | `Area_ListAppend()` (append 0, cursor→it) |
| 0x166 | ANIM_SHOW_FRAME | `Anim_ShowFrame` at slot X/Y |
| 0x167 | ANIM_SET_TRIGGER_FRAME | Set `g_anAnimSlotTriggerFrame[slot] = arg1` |
| 0x168 | CALL_PROG | GOSUB: push return + context, jump to `iRam007c4998` |
| 0x16a | EXIT_CURSOR_SELECT | Exit cursor-selection mode (cursor→use) |
| 0x16c | THEME_MUSIC_EVENT | `Theme_MusicEvent` if track changed |
| 0x16d | PLAY_SOUND_COND | Conditional sound play (not in fast-forward) |
| 0x173 | STACK_PUSH | Push `var[reg]` onto RPN stack (max 99) |
| 0x174 | STACK_POP | Pop RPN stack → `var[reg]` |
| 0x175 | ANIM_SET_X | `g_anAnimSlotX[stani] = var[reg]` |
| 0x176 | ANIM_SET_Y | `g_anAnimSlotY[stani] = var[reg]` |
| 0x178 | TIMER_ADD_SYNC | `Timer_AddSync(a1, a0)` (@0x0047e7b0): one-shot timer — run program a0 after a1 ticks (sync variant of 0x196; only the dispatch timing differs). Timer table @0x007d4dc8, ticked by `Timer_Tick_Callback`. |
| 0x179 | PUSH_EVENT | Push (arg1,arg0) onto event/callback queue (max 15) |
| 0x17a | STOP_LIPSYNC_TXT | `SndMem_StopLipsync` + `Txt_Reset` (canonical "end speech") |
| 0x17b | ANIM_SELECT_SLOT | Resolve & select anim slot for deferred use |
| 0x17c | ANIM_DUMP_OR_FREE | Mark selected slot for dump (or free in fast-forward) |
| 0x17d | IF_NOT_CURRENT_SCENE_SKIP | Skip block if arg1 != current scene ID |
| 0x17e | TIMER_ADD_REPEAT | `Timer_AddWithReset(a1, a0)` (@0x0047ea70): repeating timer — run program a0 every a1 ticks (re-arms itself), vs the one-shot 0x178/0x196 |
| 0x17f | PALETTE_FADEIN | Copy snapshot palette + `SetPal_FadeInFromBlack` |
| 0x180 | SET_RENDER_PASS | `DAT_004c4c48 = 4` |
| 0x181 | ANIM_SET_POS_TOPLEFT | Position anim from frame top-left |
| 0x182 | STACK_ADD | RPN add |
| 0x183 | STACK_SUB | RPN subtract |
| 0x184 | STACK_MUL | RPN multiply |
| 0x185 | ANIM_SET_COMPLETION_CB_FRAME | Like 0x159 but with relative frame offset |
| 0x187 | ANIM_FREEZE_SEL | `Anim_Freeze` on selected slot |
| 0x188 | ANIM_UNFREEZE_SEL | `Anim_ResetFreeze` on selected slot |
| 0x18b | ANIM_SHOW_FRAME_Z | Draw frame with Z/depth parameter |
| 0x18c | ANIM_EXPLODE | Render 100 scatter copies (explosion effect) |
| 0x18e | AREA_SET_NODE_PROPS | Set props on nodes whose field[5]==saved actor |
| 0x190 | MOV_MOVE_TO | Move character to absolute (arg1,arg2) |
| 0x191 | ANIM_FREEZE | `Anim_Freeze` (by name/this) |
| 0x192 | ANIM_UNFREEZE | `Anim_Unfreeze` |
| 0x195 | ANIM_RESET_FREEZE | `Anim_ResetFreeze` |
| 0x197 | ANIM_SET_FRAMESTEP_0 | Pause frame advancement |
| 0x198 | ANIM_SET_POSITION | `Anim_SetPosition(slot, x, y)` |
| 0x19a | FILES_LOAD_PAL | `Files_LoadPal` + `Anim_EnableDraw` |
| 0x19b | ANIM_BEGIN_DRAW | `Anim_BeginNormalDraw` (0x0040de00): `GI_SetDrawMode(0)` + `GI_LockActiveSurf_v9` — DirectDraw draw-mode + surface lock; no-op on a flat framebuffer |
| 0x19c | ANIM_FLUSH_DRAW | `Anim_FlushDraw` (0x0040df40): `GI_SetDrawMode(0)` + `GI_LockActiveSurf_v10` — twin of 0x19b; no-op on a flat framebuffer |
| 0x19d | SLIDER_ADD | Add slider value → `var[reg]` |
| 0x1a7 | WAIT_FOR_UNK | Busy-wait until condition clears |
| 0x1a8 | SET_AMBIENT_MUSIC | Timed ambient music + clear music name |
| 0x1a9 | THEME_FILL_AND_START ⚠ | No args. `Theme_FillMemAndStartStreams()` — prime the music pool and start streaming |
| 0x1aa | ANIM_CLEAR_ALL_CALLBACKS ⚠ | No args. `Anim_ClearAllCompletionCallbacks()` (@0x00406ad0) — loops all 0x96 (=150) slots calling `Anim_SetCompletionCallback(i,-1,-1,-1)`, disarming every pending script trigger |
| 0x1ad | FILES_SAVE_GAME | `Files_SaveGame("entry")` + `Files_SaveGameFull` |
| 0x1f4 | MOV_MOVE_TO_SAVED | Move to saved hero position |
| 0x1f5 | TXT_SET_COLOR | `Txt_SetColor(r,g,b)` from packed arg1 |
| 0x1f6 | ANIM_GET_FRAME_COUNT | Frame count of stani → `var[reg]` |
| 0x1f7 | REGS_CLEAR_ALL | Zero entire `g_anSpeechPlayed[]` (1500 entries) |
| 0x1f8 | ANIM_SET_TRIGGER_LAST | Trigger on last frame |
| 0x1f9 | IF_SPEAKING ⚠ | **No arguments** (its arg slots hold uninitialised junk). `SndMem_IsSpeaking()` (@0x004744f0); block runs only WHILE a speech line plays, else forward-scan to ELSE/ENDIF. Verified at handler 0x004678e6 |
| 0x1fd | NOP | No-op |
| 0x1fe | ANIM_ADD_CENTERED | Add anim positioned centred on last frame size |
| 0x1ff | GRAN_GET_ANGLE_DIST | `Gran_GetAngleDist` (Graninv @0x00434190): block on a mouse drag, push angle (deg 0-360, 0=right CW) + pixel distance. (Old table said "push hero XY" — wrong.) |

### Cursor, audio channels, math, area sprites (0x200–0x2c5)

> **Verified 2026-08-30 against the dispatch table at `0x00468f27`** (indexed `opcode - 1`
> over `0..0x2c4`). Rows marked ⚠ were previously WRONG — this block had been filled in by
> inference, and most of the "SND_*" names in the 0x262–0x26f range describe something else
> entirely (SCM voice panning and PLAYER mode flags, not mixer volume/pitch/stop).
> Argument slots, calibrated against `SET_VAR` (`var[[ebp-0x13c]] = [ebp-0x138]`, variable
> file at `0x0070fa38`): `[ebp-0x13c]`=arg0, `[ebp-0x138]`=arg1, `[ebp-0x134]`=arg2.
> Re-check any row here with `python3 tools/opcode.py 0x<op>` before relying on it.
>
> **Dispatch is not one table.** `RunProg_Exec` uses several range tables, each guarded by
> `sub $base` / `cmp $count` / `ja default`: `0x001+0x2c4 @0x00468f27` (main),
> `0x84d+0xb @0x00469a3b`, `0x8fe+0x1e @0x00469a6b`, `0x961+0x14 @0x00469ae7`,
> `0x9c5+0x4 @0x00469b3b`, `0x906+0x5 @0x00469be4`, `0x1b59+0x18 @0x00469b77`.
> A few opcodes (e.g. `0x9c4`) are handled by direct `cmpl $0x<op>` compares in
> `0x462800..0x462960` instead. `tools/opcode.py` knows all the tables.

| Opcode | Name | Action |
|--------|------|--------|
| 0x200 | IF_SPEECH_NOT_IN_RANGE | Skip block if `var[id]` outside [arg2,arg3] |
| 0x201 | CURS_DISABLE_DRAW | Disable cursor rendering |
| 0x202 | CURS_ENABLE_DRAW | Enable cursor rendering |
| 0x204 | SCHED_SET_MODE | Set scheduler/game mode to arg |
| 0x205 | SCHED_TICK | Tick scheduler/game loop once |
| 0x206 | ANIM_WAIT_LAST_FRAME | Set stop-frame to last; busy-wait until reached |
| 0x207 | SND_PLAY_ON_OBJ | Play sound attached to `_DAT_0070dec4[id]` |
| 0x208 | CURS_LOAD_SELECT ⚠ | `Curs_LoadCursorSelect(a1, obj->[0xd0], obj->[0xcc], obj->[0xce])` where `obj = g_0070d6f0[a0]`. **Not a sound op** |
| 0x209 | PLAYER_SET_COVER_SPRITE ⚠ | `Player_SetCoverSprite(a0, a1)`. **Not a sound op** |
| 0x20a | PLAYER_CLEAR_COVER_SPRITE ⚠ | `Player_SetCoverSprite(-1, -1)`. **Not a sound op** |
| 0x20b | ANIM_SET_REVERSE | Set anim slot to reverse step (-1) |
| 0x20c | ADV_TICK_FRAMES ⚠ | `Adv_TickFrames(var[a0])` (@0x00412be0), skipped while the fast-forward local is set. **Not a sound op** |
| 0x20d | SPEECH_SET_TAG ⚠ | `Speech_SetTag(a0, ...)` |
| 0x20e | ADV_TICK_FRAMES ⚠ | `Adv_TickFrames(a1)` — count taken literally from **arg1**. `Adv_TickFrames` = per frame `Adv_Tick()` + `Timer_DispatchAsyncProg()`, then spin to the frame boundary; returns at once if the cutscene FSM `DAT_00629f50` is set. **Not a sound op** |
| 0x20f–0x211 | SCHED_INTERRUPT_* | Enable/disable/set interrupt flag |
| 0x258 | SCHED_SET_MODE2 | Set second scheduler mode |
| 0x259 | GFX_INIT_IMG | `InitImg` |
| 0x25a | ANIM_ADD_FROZEN | Add anim by num, freeze it |
| 0x262 | VOICE_PAN_1 | `Snd_SetChannelPan(1, GI_PercentOfWidth(a1, a2))` — pan SCM voice channel 1 from a screen X |
| 0x265 | VOICE_PAN_2 | `Snd_SetChannelPan(2, ...)` — same, channel 2 |
| 0x264 | VOICE_PAN_3 | `Snd_SetChannelPan(3, ...)` — same, channel 3 |
| 0x266 | PLAYER_VOICE_MASK_OR | `Player_SetFlags(a1)`: `g_nPlayerVoiceMask \|= a1`. Takes **arg1** |
| 0x267 | PLAYER_VOICE_MASK_AND | `Player_ClearFlags(a1)`: `g_nPlayerVoiceMask &= a1` (an AND, not AND-NOT). **arg1** |
| 0x268 | PLAYER_VOICE_MASK_SET | `Player_ResetFlags(a1)`: sets mask *and* its saved default. **arg1** |
| 0x269 | PLAYER_SET_SPEAKING_CHAR | `Player_SetSpeakingChar(a0, a1)` |
| 0x26a | BREAK_LOOP | Terminate script loop immediately |
| 0x26b | PLAYER_SET_PAL_FREEZE | `Player_SetPalFreezeMode(a1)` |
| 0x26c | PLAYER_SET_SHARED_MODE | `Player_SetSharedMode()` — no args |
| 0x26d | PLAYER_SET_ASYNC_READ | `Player_SetAsyncReadMode()` — no args |
| 0x26f | SND_PLAY_ON_OBJ_ONESHOT | `Fx_PlayAnyChar(sndTable[a0])` — **plays** a one-shot on the first idle channel 4..6. NOT a stop |
| 0x2bc | WAIT_SPEECH_OR_SKIP | Wait for speech; abort if done + skip-allowed |
| 0x2bd | DIV_VAR | `var[id] /= arg` |
| 0x2be | MUL_VAR | `var[id] *= arg` |
| 0x2bf | MOD_VAR | `var[id] %= arg` |
| 0x2c0 | SET_EXIT_CODE_5 | `DAT_004c4c48 = 5` (exit/restart) |
| 0x2c1 | SCHED_YIELD_FRAME | Yield one frame |
| 0x2c2 | AREA_LINK_SPRITE | Link stani node to area as moving sprite |
| 0x2c3 | AREA_LINK_SPRITE_FULL | Like 0x2c2, full-coverage offsets |
| 0x2c4 | IF_NOT_GAME_MODE | Skip block if cursor mode != 1 |
| 0x2c5 | SCHED_SET_SUBMODE | Set sub-mode/state |

### Master volume / theme room / text align+mode / gamma — get/set pairs (0x84d–0x858)

Verified against the engine switch (RunProg_Exec @0x00462560, thunks resolved). This block
is **not** speech/sound — it's get/set accessors for the master mixer volume, theme room,
text alignment/mode, a global flag, and screen gamma. Earlier `SND_*` names here were wrong.

| Opcode | Name | Action |
|--------|------|--------|
| 0x84d | MIXER_SET_MASTER_VOL | Clamp `var[id]` to [0,10] (write back), then `Mixer_SetMasterVolume(var==0 ? -10000 : (var-10)*300)` millibels (0x00470400, Ghidra-misnamed "Snd_StopChannel"). 0 = mute, 10 = full. |
| 0x84e | THEME_SET_ROOM | `Theme_SetRoom(var[id])` (0x0047cba0) — select the room-music sequencer track |
| 0x84f | GFLAG_SET_FROM_VAR_ZERO | `DAT_00629f54 = (var[id]==0)` — set the global flag from a var |
| 0x850 | TXT_GET_ALIGN | `var[id] = Txt_GetAlign()` (0x00475b50) |
| 0x851 | TXT_SET_ALIGN | `Txt_SetAlign(var[id])` (0x00475be0) → `g_nTxtAlign` (0=left/1=center/2=right) |
| 0x852 | TXT_SET_MODE | `Txt_SetMode(var[id])` |
| 0x853 | SCHED_SET_GAMMA | `Sched_SetGamma(var[id])` (0x0046c5a0) + `SetPal_WaitOrRealizeIfNeeded()` (re-realizes the palette through the new gamma). Has a Debug_Assert on the var; the action is the gamma set, not a restart. |
| 0x855 | THEME_GET_ROOM | `var[id] = Theme_GetRoom()` (0x0047cc70) |
| 0x856 | TXT_GET_MODE | `var[id] = Txt_GetMode()` |
| 0x857 | GFLAG_GET_ZERO | `var[id] = (DAT_00629f54==0)` — read the global flag |
| 0x858 | SCHED_GET_GAMMA | `var[id] = Sched_GetGamma()` (0x0046c630) |

### Stack-based gfx, save dialogs, Gran mini-games (0x8fd–0x91c)

| Opcode | Name | Action |
|--------|------|--------|
| 0x8fd | ANIM_LOAD_FROM_STACK | Pop 3 stack values (entry+x/y); `Anim_LoadByName` |
| 0x8fe/0x8ff | SCHED_RESET/RESET_SUB | Reset scheduler state |
| 0x900 | GFX_DRAW_RECT | Pop 8 stack values, draw rectangle |
| 0x901 | GFX_SHOW_OBJ | Show object id |
| 0x902 | GFX_REFRESH_SCREEN | Refresh/blit screen |
| 0x903 | GFX_SHOW_ALL | Show all objects |
| 0x904 | FILES_SAVE_DIALOG | Save-game dialog → `Files_SaveGame` + `Files_SaveGameFull` |
| 0x905 | FILES_LOAD_DIALOG | Load-game dialog → load + restart |
| 0x906–0x90b | GRAN_PLAY/START_ANIM_T{3,1,2} | Pop stack; `Gran_PlayAnim`/`Gran_StartAnim` type N |
| 0x90c | MOV_SET_CHAR_STATE | Set character id state |
| 0x90d | ANIM_SET_STOP_FRAME | Set stop-frame on named slot |
| 0x90e | ANIM_FREE_SLOT | Free anim slot for id |
| 0x90f | GFX_CLEAR_BACK | Clear background buffer |
| 0x910/0x911 | SCHED_PUSH/POP_STATE | Push/pop scheduler state |
| 0x912–0x914 | ANIM_SET_TRIGGER_{1,2,3} | Set trigger type N on named slot |
| 0x915 | MOV_STOP_CHAR | Stop/deactivate character id |
| 0x916 | ANIM_REWIND_SLOT | Rewind named slot to start |
| 0x917 | GV_CAN_DROP | Push `GV_CanDrop(slot, curSlot)` (Graninv @0x00432990): 3=GV-inventory disabled, 2=incompatible, 1=open, 0=can-drop. (Old table said "frame count" — wrong.) |
| 0x918 / 0x919 | GV_SET_ENABLED ⚠ | `GV_SetEnabled(1)` / `GV_SetEnabled(0)` (@0x00432ad0, `g_nGVEnabled = arg`) — enable/disable the granular inventory. The value is an **immediate in the handler**, not an instruction operand. Dispatched from the `0x8fe..0x91c` table at `0x00469a6b`. **Not a scheduler op** |
| 0x91a | GFX_FLIP | Flip/present display |
| 0x91c | SCHED_TRIGGER_EVENT | Fire a scheduler event |

### Inventory ops (0x960–0x96c)

| Opcode | Name | Action |
|--------|------|--------|
| 0x960 | GV_CLOSE_INVENTORY | Close inventory view |
| 0x961 | INV_SET_ITEM_COUNT | Set item count |
| 0x962 | INV_MOVE_ITEM | Move item |
| 0x963 | INV_SELECT_ITEM | Select inventory item |
| 0x964 | INV_GET_ITEM_COUNT | Item count → var |
| 0x965 | INV_SET_ITEM_STATE | Set item state |
| 0x966 | INV_SET_ITEM_FLAG | Set item flag |
| 0x967 | INV_ENABLE_ITEM | Enable/show item |
| 0x968 | INV_GET_ITEM_STATE | Item state → var |
| 0x969 | INV_IS_ITEM_ENABLED | Item enabled? → var |
| 0x96a | INV_SET_ITEM_OWNER | Set item owner/group |
| 0x96b | GV_OPEN_INV_WITH_ANIM | Open inventory with anim |
| 0x96c | INV_RESET_ALL | Reset all inventory items |

### Focus, slider, game init (0x974–0xc1c)

| Opcode | Name | Action |
|--------|------|--------|
| 0x974 | ADV_SET_FOCUS | Set adventure focus/target |
| 0x975 | AREA_DRAW_NODE_RECT | Draw bounding rect of node |
| 0x9c4 | GV_OPEN_INVENTORY ⚠ | `GV_OpenInventory()` (@0x00432860) — show the native Win95 inventory panel. No args. Dispatched by a direct compare at `0x004628d1`, NOT the main table |
| 0x9c5–0x9c6 | GV_* | (`0x9c6` = `GV_SetDestroyHandler(a0)`) |
| 0x9c7 | GV_HIDE_AND_CLEAN ⚠ | `GV_HideAndClean()`. No args. **Not a slider op** |
| 0x9c8 | GV_TICK_INVENTORY ⚠ | `GV_TickInventory()` — service the panel. No args. **Not a slider op** |
| 0x9c9 | GV_CLOSE_INVENTORY ⚠ | `GV_CloseInventory()` (@0x00432750). No args. **Not a slider op** |
| 0xc02 | GRAN_INIT_SLIDER | Initialize slider for object |
| 0xc1c | MOV_INIT_GAME_MODE | Full game-mode init: reset area, `Mov_InitChar`, register all cursor types, border mode 0 |
| 0x13ba | ANIM_ADD_FROZEN | `Anim_AddByNum(a0, loop=1, 0)` (joins the open group like 0x19) + `Anim_SetWalkTableBase(a1)` + `SetFrameStep(0)` + `SetCurrentFrame(0)` + `Anim_Freeze` — the frozen-at-frame-0 sibling of 0x19 |

### Object props & theme/display (0x1839–0x5b23)

| Opcode | Name | Action |
|--------|------|--------|
| 0x1839 | ADV_SET_OBJ_PROPERTY | Set object property |
| 0x183a | ADV_TRIGGER_OBJ_EVENT | Trigger event on object |
| 0x183c | ADV_RESET_INPUT | Reset adventure input/click state |
| 0x1840 | ADV_SET_OBJ_POS | Set object position |
| 0x1842 | ADV_ATTACH_OBJ_TO_NODE | Attach object to node |
| 0x1b59 | THEME_SET_THEME | Set theme/scene from `DAT_0070c24c[id]` |
| 0x1b5a | THEME_FADE_THEME | Fade to theme |
| 0x1b5b | THEME_SET_BG_BY_VAR | Set background from var |
| 0x1b5c | THEME_CLEAR_BG | Clear background |
| 0x1b5d | THEME_SET_LAYER_MODE | Set layer/overlay mode |
| 0x1b5e | THEME_SET_FADE_RANGE | Set fade range |
| 0x1b5f | THEME_SET_FADE_STEP | Set fade step |
| 0x1b60 | THEME_APPLY_PALETTE | Apply palette |
| 0x1b61 | THEME_CROSSFADE | Cross-fade |
| 0x1b6c | GI_GET_SCREEN_MODE | Screen/display mode → var |
| 0x1b6d | GI_SET_SCREEN_MODE | Set screen/display mode |
| 0x1b6e | GI_GET_PROG_PARAM | Get program param (param_2) → var |
| 0x1b6f | GI_SET_PROG_PARAM | Set program param |
| 0x1b70 | GI_GET_WIDTH | Display width → var |
| 0x1b71 | GI_GET_HEIGHT | Display height → var |
| 0x1b76 | GFX_FLUSH_BITMAP | Flush bitmap/sprite |
| 0x1b80 | GFX_RELOAD_PALETTE | Reload/reinitialise palette |
| 0x5b23 | SPEECH_INIT | (Re-)initialise speech subsystem |

## Helper functions (around the dispatcher)

| Address | Name | Role |
|---------|------|------|
| `0x00462290` | `RunProg_WaitMoveDone` | Busy-wait until `g_nMovDone`; per-iteration `Adv_Tick`+async dispatch; right-click abort |
| `0x00462380` | `RunProg_SelectAreaContext` | Switch runtime area-data globals between primary/secondary banks (`DAT_0070e130`) |
| `0x00462560` | `RunProg_Exec` | **The dispatcher** — ~400-opcode switch script VM |
| `0x0046b9b0` | `RunProg_PlayScmWithPaletteGuard` | `Player_PlayScm` wrapped in palette snapshot/restore |
| `0x0046baa0` | `RunProg_ClearTrackedSounds` | Reset tracked-sound counter |
| `0x0046bac0` | `RunProg_StopAndClearTrackedSounds` | `Snd_Stop` all tracked sounds, then clear |
| `0x0046bb20` | `RunProg_TrackSound` | Add a sound handle to tracked list (max 10, dedup) |
| `0x0046bba0` | `RunProg_RestorePaletteSnapshot` | memcpy `g_abSnapshotPal` → live palette buffer (0x300 bytes) |
| `0x0046bc40` | `RunProg_Nop` | No-op placeholder |

## ScummVM porting notes

- This entire dispatcher should be re-implemented as a **clean opcode-handler table**
  in the ScummVM engine — one C++ method per opcode, dispatched from the script
  program data. The opcode semantics above are the spec.
- `g_anSpeechPlayed[]` is the **script variable file** (1500 ints) — persisted in saves.
- The IF/ELSE/ENDIF forward-scan (`0x09–0x11`) and GOSUB (`0x065`/`0x168`) plus the
  RPN stack (`0x173/0x174/0x182-0x184`) define the control-flow primitives.
- Cutscene skip is driven by `DAT_00629f50` / `local_130` / `local_148` —
  right-click abort must fast-forward through blocking opcodes (WAIT_*, SPEECH_*).
- Large reserved opcode gaps are normal — the script compiler only emitted a subset.
