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
