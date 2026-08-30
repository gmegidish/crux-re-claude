# CRUX.EXE Reverse Engineering Workflow

How to continue this RE project in a future Claude Code session.

## Prerequisites

- Ghidra running with CRUX.EXE open (project at `/Users/gilm/ghidra`)
- `ghidra-mcp` MCP server connected (tools appear as `mcp__ghidra-mcp__*`)
- This repo checked out at `/Users/gilm/git/granny/granny-re/`

## Project context

| Field | Value |
|---|---|
| Target | `CRUX.EXE` — Crux engine, Windows 95 game |
| Goal | Full C/C++ decompilation for ScummVM |
| Original source root | `C:\DevStudio\Projects\Crux\` |
| Total functions | 1,870 |
| Source files recovered | 40 `.cpp` files (from embedded `__FILE__` assert strings) |
| Translation table | `FUNCTIONS.md` |
| RE'd source | `src/` |

---

## Session startup

```
1. Open Claude Code in /Users/gilm/git/granny/granny-re
2. Confirm Ghidra MCP is connected:
     mcp__ghidra-mcp__list_instances
3. Switch context to CRUX.EXE:
     mcp__ghidra-mcp__switch_program  →  program: "CRUX.EXE"
4. Read FUNCTIONS.md to see what's done and what to tackle next
     ⚠ but read § "Verifying against the binary" FIRST — its ☑ column lies
5. For port work, Ghidra is NOT required: objdump + tools/ cover most of it
```

---

## Reversing a source file (the core loop)

### Step 1 — Pick a source file

Open `FUNCTIONS.md`, find a `### SourceFile.cpp` section with unchecked (☐) rows.
Good targets to start with (largest remaining):
`ADVENT.cpp` (47), `Graninv.cpp` (67), `GI.cpp` (66), `THEMES.cpp` (43)

### Step 2 — Decompile all functions in the file

```
mcp__ghidra-mcp__decompile_function
  address: <hex address>
  program: CRUX.EXE
```

Run all functions in parallel (one tool call per function, all in the same message).

### Step 3 — Identify shared globals

Look for `DAT_XXXXXXXX` references that appear in multiple functions.
These are global variables. Name them using `set_global` (not `rename_global_variable`
— the latter rejects names missing Hungarian notation).

```
mcp__ghidra-mcp__set_global
  address:       <hex without 0x>
  name:          g_n<Name>        ← int
                 g_p<Name>        ← pointer
                 g_dw<Name>       ← uint/dword
                 g_sz<Name>       ← char*
                 g_ab<Name>       ← byte array
  type_name:     int / char / void* / etc.
  array_length:  N  (omit or 0 if not an array)
  plate_comment: one-line description of what this global is
  program:       CRUX.EXE
```

### Step 4 — Name all functions in the file

```
mcp__ghidra-mcp__rename_function_by_address
  function_address: <hex without 0x>
  new_name:         <PascalCase name>
  program:          CRUX.EXE
```

Warnings about PascalCase / verb list are non-blocking — renames still succeed.
The original code used `Module_Verb` style (e.g. `BlitImg_Scaled`) — keep that.

### Step 5 — Write the C++ source

Create `src/<OriginalName>.cpp` and `src/<OriginalName>.h`.

- Declare all globals `extern` in the `.h`
- Define them once in the `.cpp`
- Forward-declare functions from other modules as `extern`
- Int-to-pointer cast warnings are expected (Win95 32-bit, `int` == pointer size)

### Step 6 — Update FUNCTIONS.md

For each reversed function, update its row:
- Column 2 (Ghidra Name): new function name
- Column 3 (Original Name): same new name (or recovered original if found)
- Column 5 (Reversed): `☐` → `☑`
- Column 6 (RE Output File): `` `src/FileName.cpp` ``

Also update the progress table at the top (Reversed count and %).

### Step 7 — Commit

```bash
git add src/ FUNCTIONS.md
git commit -m "re: reverse SourceFile.cpp (N functions)"
```

---

## Verifying against the binary (READ THIS FIRST)

**`src/` is not the engine — it is notes.** `FUNCTIONS.md`'s ☑ means "someone looked at
this function", not "the code is here". Every port bug chased in Aug 2026 came from
trusting a prose summary; every fix came from reading the instructions at the address the
summary points to.

Two ways `src/` misleads you:

| Symptom | Example |
|---|---|
| **STUB** — body is empty / `// (See decompiled body at 0x...)` | `Anim_HandleFrameTick`, 46 of 47 `Adv_*` |
| **SUSPECT** — body exists but carries decompiler residue (`FUN_`, `DAT_`, `undefined`) | `SetPal_ApplyGamma` — its FPU gamma curve came out as two opaque calls, and transcribing it blacked out the screen |

```bash
python3 tools/fidelity.py --verbose | grep <FunctionName>   # STUB / SUSPECT / solid + address
objdump -d --start-address=0x004059c0 --stop-address=0x00406200 CRUX.EXE
```
Linear disassembly desyncs on data; if output looks like nonsense, try a start address a
few bytes later until a known instruction lands on a boundary.

## Tooling (tools/)

| Tool | Use |
|---|---|
| `fidelity.py [--verbose]` | Per-module stubbed / suspect / solid. **Run before trusting `src/`.** |
| `opcode.py 0x26f ...` | Resolve a RunProg opcode to its handler via the jump table at `0x00468f27` (index = `op-1`, range `1..0x2c4`), disassemble it, annotate arg slots, name call targets. Some opcodes (e.g. `0x9c4`) are dispatched by direct compares instead and won't resolve — search for `cmpl $0x<op>` in `0x462800-0x462960`. |
| `callers.py Adv_ Anim_` | Call-site count per function — "is this stub even called?" |
| `opcode_usage.py [--all]` | Opcode frequency across all 29 scenes vs what `RunProg.cpp` implements. ⚠ a high count can mean duplicated debug code (a give-all-items program using `0xc` is copied into every scene). |
| `callgraph.py 0x004101f0` | The call sequence of one function, in order — how to read a 5 KB function without decoding it. |
| `patch_trace.py` | Build `CRUX_DEBUG.EXE` — see below. |
| `tracediff.py` | Diff the engine's trace against the port's log. |

**Argument slots** in `RunProg_Exec`, calibrated against `SET_VAR`
(`var[[ebp-0x13c]] = [ebp-0x138]`, variable file at `0x0070fa38`):
`[ebp-0x140]` = opcode, `[ebp-0x13c]` = arg0, `[ebp-0x138]` = arg1, `[ebp-0x134]` = arg2.
Several opcodes read **arg1**, not arg0 — check, don't assume.

## The trace oracle — running the real engine

The release build compiled `Debug_Trace(line, srcFile, fmt, ...)` out: its body at
`0x0041a770` is an empty stub, so **no flag re-enables it**. `tools/patch_trace.py` rewrites
that stub to jump into a code cave that formats via the game's own CRT and appends to
`CRUXTRC.LOG`, lighting up all 177 call sites with file + line + message.

```bash
python3 tools/patch_trace.py          # CRUX.EXE -> CRUX_DEBUG.EXE (original untouched)
wine CRUX_DEBUG.EXE                   # runs under wine; game data lives in ../granny
ANIM_LOG=1 RP_TRACE=1 ./port/crux . 2>&1 | tee /tmp/port.log
python3 tools/tracediff.py ../granny/CRUXTRC.LOG /tmp/port.log
```

Known-permanent divergences to ignore in the diff: the engine loads cursors
(`CURSAREA`/`CURSEXIT`/`CURSHOUR`) and area backdrops (`MENU/8`, `VVI2/8`) as anim slots;
the port has separate `Cursor` and backdrop paths.

## Debugging the port

```bash
make -C port all && make -C port test          # build + self-checks
SKIP_VIDEOS=1 START_AREA=VVE ANIM_LOG=1 RP_TRACE=1 ./port/crux . 2>&1 | tee /tmp/port.log
SDL_VIDEODRIVER=dummy ANIM_LOG=1 RP_TRACE=1 RUN_PROG=VVE:1 ./port/crux .   # headless, one program
./port/dumpprog . --scenes x                   # list all 29 scenes
./port/dumpprog . vvi2 all                     # cache slots + anim-name table + opcodes
```

`RUN_PROG` headless is the tightest loop — it reproduced the whole VVI2 anim chain in a
second and is how the `_THIS` bug was found. Scenes with the most anim traffic:
**VVE (728), VVB (638), VVW (612), GAZBIG (562)**.

Env vars: `START_AREA`, `SKIP_VIDEOS`, `SCALE` (1-8, nearest-neighbour window upscale),
`RUN_PROG`, `RP_TRACE`, `ANIM_LOG`, `AREA_LOG`, `AREA_OVERLAY`, `LOOP_LOG`,
`MENU_DUMP`/`MENU_HOVER`/`MENU_CLICK`, plus the dump modes in `main.cpp`.

### Gotchas learned the hard way

- **`_THIS` in the anim-name table.** An anim-table entry named `_THIS` is not a resource —
  it means the current STANI slot. Always resolve via `RunProg::animSlotFor()`;
  `Anim::findByName("_THIS")` silently returns -1 and the opcode no-ops.
- **Junk arguments are normal.** No-arg opcodes (`0xf`, `0x10`, `0xff`, `0x12d`, `0x12e`,
  `0xcb`, `0xce`, `0x1f9`, …) carry uninitialised slot data. **The script parser is
  correct** — 111 opcodes never show junk; only the no-arg family does. Do not "fix" it.
- **New IF opcodes must be added to `RunProg::isIfOpener`**, or `skipBlock` mis-balances
  nested blocks and resumes at the wrong instruction — a silent control-flow bug.
- **`RUNPROG_OPCODES.md`'s 0x200–0x2c5 block was largely invented.** ~20 rows have been
  corrected against the binary and marked ⚠. Re-verify any row with `tools/opcode.py`.

## MCP tools reference

### Navigation
| Tool | Purpose |
|---|---|
| `list_instances` | Show running Ghidra instances |
| `list_open_programs` | List all programs open in Ghidra |
| `switch_program` | Set active program for subsequent calls |
| `get_current_program_info` | Confirm active program + stats |

### Analysis
| Tool | Purpose |
|---|---|
| `decompile_function` | Decompile one function to pseudocode |
| `get_function_by_address` | Get function metadata by address |
| `get_function_callers` | Who calls this function |
| `get_function_callees` | What this function calls |
| `get_xrefs_to` | All references to an address |
| `get_bulk_xrefs` | Batch XREFs for many addresses at once |
| `list_strings` | Search defined strings (use `filter:` for patterns) |
| `list_imports` | All imported symbols |
| `list_functions` | All functions (large — pipe through subagent) |

### Renaming
| Tool | Purpose |
|---|---|
| `set_global` | **Preferred for globals** — name + type + comment in one atomic write |
| `rename_global_variable` | Rename a global (requires Hungarian prefix, use `set_global` instead) |
| `rename_function_by_address` | Rename a function at a given address |
| `rename_variable` | Rename a local variable inside a decompiled function |
| `rename_variables` | Batch rename local variables |

### Batch operations
| Tool | Purpose |
|---|---|
| `get_bulk_xrefs` | XREFs for a comma-separated list of addresses |
| `batch_decompile` | Decompile multiple functions at once |
| `batch_set_comments` | Set plate/EOL/pre/post comments on many addresses |
| `batch_rename_function_components` | Rename params + locals across a function |

### Type system
| Tool | Purpose |
|---|---|
| `apply_data_type` | Set the type of a data item |
| `create_struct` | Define a new struct type |
| `add_struct_field` | Add a field to a struct |
| `get_struct_layout` | Show a struct's field layout |
| `set_local_variable_type` | Change the type of a local variable |
| `set_parameter_type` | Change a function parameter's type |

---

## Global naming conventions

All globals must use Hungarian notation after `g_`:

| Prefix | Type |
|---|---|
| `g_n` | `int` |
| `g_dw` | `unsigned int` / DWORD |
| `g_p` | pointer |
| `g_sz` | `char*` (string) |
| `g_ab` | byte array |
| `g_pfn` | function pointer |

Examples: `g_nClipX`, `g_pFramebuffer`, `g_szWindowTitle`, `g_abPalette`

## Function naming conventions

- Use the original module prefix style: `Module_Verb` or `VerbNoun`
- Examples: `BlitImg_Scaled`, `PutLine_RLE`, `InitImg`, `PackRegion`
- Warnings about PascalCase are non-blocking — the rename still applies

---

## Named globals so far

| Address | Name | Type | Source file | Description |
|---|---|---|---|---|
| `0x006d7040` | `g_nXScaleSteps` | `int[640]` | Img.cpp | X-axis scale delta table |
| `0x006d68b8` | `g_nYScaleRows` | `int[480]` | Img.cpp | Y-axis row-position table |
| `0x006d8450` | `g_nSizeScaleTables` | `int[640]` | Img.cpp | Per-width scale table pointers |
| `0x006d68b0` | `g_nSizeScalePoolBase` | `int` | Img.cpp | Base of 820KB scale pool |
| `0x006d7a48` | `g_nTempScaleRow` | `int[640]` | Img.cpp | Scratch copy of a scale row |
| `0x004d11c0` | `g_nCachedScaleWidth` | `int` | Img.cpp | Cached width for temp scale row |
| `0x007c3fd8` | `g_nClipX` | `int` | Img.cpp | Viewport left boundary |
| `0x007c3fdc` | `g_nClipY` | `int` | Img.cpp | Viewport top boundary |

---

## Source file anchors

The first code address in each source file (from embedded assert strings).
Use these to orient yourself when a function's source file is uncertain.

```
0x00403955 → Advanim.cpp     0x0040e3aa → ADVENT.cpp
0x00413406 → ANI32.cpp       0x00413c97 → AREAS.cpp
0x004150c9 → BANI.cpp        0x0041825f → CURSORS.cpp
0x0041a89c → DDRAWI.cpp      0x0041f026 → ERRORS.cpp
0x00420d68 → Except.cpp      0x00422298 → FILES.cpp
0x0042a83c → FRMTIMER.cpp    0x0042acf9 → FX.cpp
0x0042b7ca → GI.cpp          0x00430f5b → Graninv.cpp
0x004390ec → HELPSTK.cpp     0x0043a061 → Img.cpp
0x0043c7fd → INVMANG.cpp     0x0043d6cc → magwrit.cpp
0x0043f65d → Memalloc.cpp    0x00447ce8 → MIXER.cpp
0x0045401c → MOVEMENT.cpp    0x004576dd → ONTHEFLY.cpp
0x0045c68a → PLAYER.cpp      0x0045d6b5 → READRES.cpp
0x00461b92 → RESCALE.cpp     0x00461f1a → RUNPROG.cpp
0x0046bddc → SAFEHEAP.cpp    0x0046bf7e → SCHED.cpp
0x0046e4e8 → SETPAL.cpp      0x004707e8 → SLIDER.cpp
0x004725bc → SOUNDMEM.cpp    0x00474b69 → SPEECH.cpp
0x00475200 → TEXT.cpp        0x0047985f → THEMES.cpp
0x0047d5f0 → TIMERS.cpp      0x0047ed5e → Tushtush.cpp
0x00482242 → Winmain.cpp     0x00486e78 → WINRES.cpp
0x00487ce9 → WIZARDS.cpp
```

---

## Tips

- **Global renames propagate instantly** — rename `DAT_006d7040` once and every
  function that references that address shows the new name in all future decompiles.
- **Batch decompiles** — send all N functions in one message for parallel execution.
- **`get_bulk_xrefs`** — pass a comma-separated list; much faster than N individual
  `get_xrefs_to` calls.
- **`(startup/crt)` zone** (`0x00401000–0x00403954`) — 499 thunks forming the
  import table; usually not worth reversing individually.
- **`(msvc-crt)` zone** (`0x00488000+`) — statically linked MSVC runtime; already
  named by Ghidra's FLIRT signatures, skip these.
- **Assert strings** — every assert in the original code embedded `__FILE__` as a
  string literal. These are at `0x004c4fa4–0x004debxx` in the data section and
  are the source of the 40 recovered file names.
