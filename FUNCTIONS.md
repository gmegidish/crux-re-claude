# CRUX.EXE Function Translation Table

> **Game**: Crux Engine (Windows 95)  
> **Goal**: Full RE to C/C++ for ScummVM  
> **Binary**: `CRUX.EXE` — PE32, x86, VC6, image base `0x00400000`  
> **Total functions**: 1870  
> **Original source project**: `C:\DevStudio\Projects\Crux\`

> ### ⚠ What the "Reversed" column means
> **A ☑ means someone has looked at the function, NOT that its body was translated.**
> `ADVENT.cpp` scores 47/47 below while 46 of its 47 functions are empty stubs whose body
> is a comment (`// (See decompiled body at 0x...)`); `Advanim.cpp` scores 97/97 with 43
> stubs. A present body can also be wrong where Ghidra emitted opaque `FUN_`/`DAT_` calls
> — `SetPal_ApplyGamma` had a full body whose arithmetic was garbage.
>
> Run **`python3 tools/fidelity.py --verbose`** for the real per-module picture
> (stubbed / suspect / solid) before trusting anything here. See `RE_WORKFLOW.md`
> § "Verifying against the binary".

## Progress

| Source File | Functions | Reversed | % |
|-------------|-----------|----------|---|
| ADVENT.cpp | 47 | 47 | 100% |
| ANI32.cpp | 2 | 2 | 100% |
| AREAS.cpp | 15 | 15 | 100% |
| Advanim.cpp | 97 | 97 | 100% |
| BANI.cpp | 19 | 19 | 100% |
| CURSORS.cpp | 43 | 43 | 100% |
| DDRAWI.cpp | 35 | 35 | 100% |
| ERRORS.cpp | 33 | 33 | 100% |
| Except.cpp | 0 | 0 | 0% |
| FILES.cpp | 47 | 47 | 100% |
| FRMTIMER.cpp | 5 | 5 | 100% |
| FX.cpp | 11 | 11 | 100% |
| GI.cpp | 66 | 66 | 100% |
| Graninv.cpp | 68 | 68 | 100% |
| HELPSTK.cpp | 9 | 9 | 100% |
| INVMANG.cpp | 6 | 6 | 100% |
| Img.cpp | 18 | 18 | 100% |
| MIXER.cpp | 42 | 42 | 100% |
| MOVEMENT.cpp | 33 | 33 | 100% |
| Memalloc.cpp | 4 | 4 | 100% |
| ONTHEFLY.cpp | 3 | 3 | 100% |
| PLAYER.cpp | 32 | 32 | 100% |
| READRES.cpp | 42 | 42 | 100% |
| RESCALE.cpp | 2 | 2 | 100% |
| RUNPROG.cpp | 10 | 10 | 100% |
| SAFEHEAP.cpp | 2 | 2 | 100% |
| SCHED.cpp | 18 | 18 | 100% |
| SETPAL.cpp | 53 | 53 | 100% |
| SLIDER.cpp | 10 | 10 | 100% |
| SOUNDMEM.cpp | 20 | 20 | 100% |
| SPEECH.cpp | 7 | 7 | 100% |
| TEXT.cpp | 29 | 29 | 100% |
| THEMES.cpp | 43 | 43 | 100% |
| TIMERS.cpp | 21 | 21 | 100% |
| Tushtush.cpp | 47 | 47 | 100% |
| WINRES.cpp | 10 | 10 | 100% |
| WIZARDS.cpp | 10 | 10 | 100% |
| Winmain.cpp | 32 | 32 | 100% |
| magwrit.cpp | 25 | 25 | 100% |
| (startup/crt) | 499 | 0 | 0% |
| (msvc-crt) | 355 | 42 | 12% |
| **Total** | **1870** | **1058** | **57%** |

## Source Files

40 original source files recovered from embedded debug strings:

- ADVENT.cpp
- ANI32.cpp
- AREAS.cpp
- Advanim.cpp
- BANI.cpp
- CURSORS.cpp
- DDRAWI.cpp
- ERRORS.cpp
- Except.cpp
- FILES.cpp
- FRMTIMER.cpp
- FX.cpp
- GI.cpp
- Graninv.cpp
- HELPSTK.cpp
- INVMANG.cpp
- Img.cpp
- MIXER.cpp
- MOVEMENT.cpp
- Memalloc.cpp
- ONTHEFLY.cpp
- PLAYER.cpp
- READRES.cpp
- RESCALE.cpp
- RUNPROG.cpp
- SAFEHEAP.cpp
- SCHED.cpp
- SETPAL.cpp
- SLIDER.cpp
- SOUNDMEM.cpp
- SPEECH.cpp
- TEXT.cpp
- THEMES.cpp
- TIMERS.cpp
- Tushtush.cpp
- WINRES.cpp
- WIZARDS.cpp
- Winmain.cpp
- magwrit.cpp

## Function Table

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|

### (startup/crt)

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00401005` | `thunk_FUN_0041d9c0` |  | (startup/crt) | ☐ |  |
| `0x0040100f` | `thunk_FUN_00419a20` |  | (startup/crt) | ☐ |  |
| `0x00401019` | `thunk_FUN_0041fad0` |  | (startup/crt) | ☐ |  |
| `0x0040101e` | `thunk_FUN_00483200` |  | (startup/crt) | ☐ |  |
| `0x00401023` | `thunk_FUN_0042a090` |  | (startup/crt) | ☐ |  |
| `0x00401028` | `thunk_FUN_00475c70` |  | (startup/crt) | ☐ |  |
| `0x00401041` | `thunk_FUN_00442430` |  | (startup/crt) | ☐ |  |
| `0x0040104b` | `thunk_FUN_0041a7b0` |  | (startup/crt) | ☐ |  |
| `0x00401050` | `thunk_FUN_00405810` |  | (startup/crt) | ☐ |  |
| `0x00401055` | `thunk_FUN_0045e970` |  | (startup/crt) | ☐ |  |
| `0x0040105f` | `thunk_FUN_00403510` |  | (startup/crt) | ☐ |  |
| `0x00401064` | `thunk_FUN_00478240` |  | (startup/crt) | ☐ |  |
| `0x00401069` | `thunk_FUN_0042c370` |  | (startup/crt) | ☐ |  |
| `0x00401073` | `thunk_FUN_0040eec0` |  | (startup/crt) | ☐ |  |
| `0x00401082` | `thunk_FUN_0041b7c0` |  | (startup/crt) | ☐ |  |
| `0x00401091` | `thunk_FUN_00407e80` |  | (startup/crt) | ☐ |  |
| `0x004010aa` | `thunk_FUN_00452950` |  | (startup/crt) | ☐ |  |
| `0x004010b4` | `thunk_FUN_00462560` |  | (startup/crt) | ☐ |  |
| `0x004010c8` | `thunk_FUN_00472da0` |  | (startup/crt) | ☐ |  |
| `0x004010cd` | `thunk_FUN_0043bca0` |  | (startup/crt) | ☐ |  |
| `0x004010d2` | `thunk_FUN_00455f70` |  | (startup/crt) | ☐ |  |
| `0x004010e6` | `thunk_FUN_00406980` |  | (startup/crt) | ☐ |  |
| `0x004010f0` | `thunk_FUN_00426750` |  | (startup/crt) | ☐ |  |
| `0x004010fa` | `thunk_FUN_0041f510` |  | (startup/crt) | ☐ |  |
| `0x004010ff` | `thunk_FUN_0047f450` |  | (startup/crt) | ☐ |  |
| `0x00401104` | `thunk_FUN_00456410` |  | (startup/crt) | ☐ |  |
| `0x00401109` | `thunk_FUN_00442f40` |  | (startup/crt) | ☐ |  |
| `0x0040110e` | `thunk_FUN_0046c290` |  | (startup/crt) | ☐ |  |
| `0x00401113` | `thunk_FUN_0046bc40` |  | (startup/crt) | ☐ |  |
| `0x00401118` | `thunk_FUN_004782f0` |  | (startup/crt) | ☐ |  |
| `0x0040111d` | `thunk_FUN_0043a9c0` |  | (startup/crt) | ☐ |  |
| `0x00401122` | `thunk_FUN_0041e970` |  | (startup/crt) | ☐ |  |
| `0x00401127` | `thunk_FUN_00481610` |  | (startup/crt) | ☐ |  |
| `0x0040112c` | `thunk_FUN_0040aec0` |  | (startup/crt) | ☐ |  |
| `0x00401140` | `thunk_FUN_00422d00` |  | (startup/crt) | ☐ |  |
| `0x0040114a` | `thunk_FUN_0046c6c0` |  | (startup/crt) | ☐ |  |
| `0x00401154` | `thunk_FUN_00487810` |  | (startup/crt) | ☐ |  |
| `0x00401168` | `thunk_FUN_0040f7c0` |  | (startup/crt) | ☐ |  |
| `0x00401172` | `thunk_FUN_00455940` |  | (startup/crt) | ☐ |  |
| `0x00401177` | `thunk_FUN_00452a30` |  | (startup/crt) | ☐ |  |
| `0x0040117c` | `thunk_FUN_0041c340` |  | (startup/crt) | ☐ |  |
| `0x00401181` | `thunk_FUN_00404310` |  | (startup/crt) | ☐ |  |
| `0x00401186` | `thunk_FUN_0041efe0` |  | (startup/crt) | ☐ |  |
| `0x00401190` | `thunk_FUN_00429950` |  | (startup/crt) | ☐ |  |
| `0x00401195` | `thunk_FUN_0047d390` |  | (startup/crt) | ☐ |  |
| `0x0040119a` | `thunk_FUN_0047fca0` |  | (startup/crt) | ☐ |  |
| `0x004011b3` | `thunk_FUN_004578e0` |  | (startup/crt) | ☐ |  |
| `0x004011b8` | `thunk_FUN_0043c7c0` |  | (startup/crt) | ☐ |  |
| `0x004011c2` | `thunk_FUN_00480080` |  | (startup/crt) | ☐ |  |
| `0x004011cc` | `thunk_FUN_00474d80` |  | (startup/crt) | ☐ |  |
| `0x004011d1` | `thunk_FUN_00486cd0` |  | (startup/crt) | ☐ |  |
| `0x004011db` | `thunk_FUN_0041af50` |  | (startup/crt) | ☐ |  |
| `0x004011e0` | `thunk_FUN_00406fc0` |  | (startup/crt) | ☐ |  |
| `0x004011e5` | `thunk_FUN_0041bea0` |  | (startup/crt) | ☐ |  |
| `0x004011ea` | `thunk_FUN_004079e0` |  | (startup/crt) | ☐ |  |
| `0x004011ef` | `thunk_FUN_00443970` |  | (startup/crt) | ☐ |  |
| `0x004011f9` | `thunk_FUN_0041d4e0` |  | (startup/crt) | ☐ |  |
| `0x004011fe` | `thunk_FUN_0041a760` |  | (startup/crt) | ☐ |  |
| `0x00401203` | `thunk_FUN_004049e0` |  | (startup/crt) | ☐ |  |
| `0x00401208` | `thunk_FUN_004580a0` |  | (startup/crt) | ☐ |  |
| `0x0040120d` | `thunk_FUN_00452520` |  | (startup/crt) | ☐ |  |
| `0x00401217` | `thunk_FUN_004043d0` |  | (startup/crt) | ☐ |  |
| `0x0040121c` | `thunk_FUN_0041d8f0` |  | (startup/crt) | ☐ |  |
| `0x00401226` | `thunk_FUN_00403230` |  | (startup/crt) | ☐ |  |
| `0x0040122b` | `thunk_FUN_00472ea0` |  | (startup/crt) | ☐ |  |
| `0x00401235` | `thunk_FUN_0042f860` |  | (startup/crt) | ☐ |  |
| `0x00401253` | `thunk_FUN_00480460` |  | (startup/crt) | ☐ |  |
| `0x00401267` | `thunk_FUN_004101f0` |  | (startup/crt) | ☐ |  |
| `0x0040126c` | `thunk_FUN_004286a0` |  | (startup/crt) | ☐ |  |
| `0x00401276` | `thunk_FUN_0046c1c0` |  | (startup/crt) | ☐ |  |
| `0x0040127b` | `thunk_FUN_00443510` |  | (startup/crt) | ☐ |  |
| `0x00401280` | `thunk_FUN_0041f480` |  | (startup/crt) | ☐ |  |
| `0x0040128f` | `thunk_FUN_00453320` |  | (startup/crt) | ☐ |  |
| `0x00401294` | `thunk_FUN_004798e0` |  | (startup/crt) | ☐ |  |
| `0x004012bc` | `thunk_FUN_00403540` |  | (startup/crt) | ☐ |  |
| `0x004012c6` | `thunk_FUN_004074d0` |  | (startup/crt) | ☐ |  |
| `0x004012cb` | `thunk_FUN_00407b00` |  | (startup/crt) | ☐ |  |
| `0x004012d0` | `thunk_FUN_0040ecb0` |  | (startup/crt) | ☐ |  |
| `0x004012d5` | `thunk_FUN_00406db0` |  | (startup/crt) | ☐ |  |
| `0x004012e4` | `thunk_FUN_0047f010` |  | (startup/crt) | ☐ |  |
| `0x004012e9` | `thunk_FUN_0047d990` |  | (startup/crt) | ☐ |  |
| `0x004012f3` | `thunk_FUN_00478580` |  | (startup/crt) | ☐ |  |
| `0x004012fd` | `thunk_FUN_004810a0` |  | (startup/crt) | ☐ |  |
| `0x00401307` | `thunk_FUN_00424490` |  | (startup/crt) | ☐ |  |
| `0x00401316` | `thunk_FUN_00403250` |  | (startup/crt) | ☐ |  |
| `0x0040131b` | `thunk_FUN_0043a1b0` |  | (startup/crt) | ☐ |  |
| `0x0040132a` | `thunk_FUN_00424980` |  | (startup/crt) | ☐ |  |
| `0x00401339` | `thunk_FUN_0041a0e0` |  | (startup/crt) | ☐ |  |
| `0x0040134d` | `thunk_FUN_00419f70` |  | (startup/crt) | ☐ |  |
| `0x00401352` | `thunk_FUN_00413f10` |  | (startup/crt) | ☐ |  |
| `0x00401357` | `thunk_FUN_00474ea0` |  | (startup/crt) | ☐ |  |
| `0x0040135c` | `thunk_FUN_0041a740` |  | (startup/crt) | ☐ |  |
| `0x00401361` | `thunk_FUN_00474320` |  | (startup/crt) | ☐ |  |
| `0x00401366` | `thunk_FUN_00479c20` |  | (startup/crt) | ☐ |  |
| `0x00401375` | `thunk_FUN_00461210` |  | (startup/crt) | ☐ |  |
| `0x0040137a` | `thunk_FUN_004066e0` |  | (startup/crt) | ☐ |  |
| `0x0040137f` | `thunk_FUN_0041a790` |  | (startup/crt) | ☐ |  |
| `0x0040139d` | `thunk_FUN_00441690` |  | (startup/crt) | ☐ |  |
| `0x004013a2` | `thunk_FUN_00420e60` |  | (startup/crt) | ☐ |  |
| `0x004013a7` | `thunk_FUN_00417e80` |  | (startup/crt) | ☐ |  |
| `0x004013b6` | `thunk_FUN_0046f450` |  | (startup/crt) | ☐ |  |
| `0x004013c0` | `thunk_FUN_00452030` |  | (startup/crt) | ☐ |  |
| `0x004013ca` | `thunk_FUN_00474f40` |  | (startup/crt) | ☐ |  |
| `0x004013e8` | `thunk_FUN_00474770` |  | (startup/crt) | ☐ |  |
| `0x004013f2` | `thunk_FUN_00417bc0` |  | (startup/crt) | ☐ |  |
| `0x0040140b` | `thunk_FUN_00418210` |  | (startup/crt) | ☐ |  |
| `0x00401410` | `thunk_FUN_00458470` |  | (startup/crt) | ☐ |  |
| `0x00401424` | `thunk_FUN_00473de0` |  | (startup/crt) | ☐ |  |
| `0x00401429` | `thunk_FUN_0042dd10` |  | (startup/crt) | ☐ |  |
| `0x00401433` | `thunk_FUN_0042fd50` |  | (startup/crt) | ☐ |  |
| `0x00401442` | `thunk_FUN_00478380` |  | (startup/crt) | ☐ |  |
| `0x00401447` | `thunk_FUN_0042c030` |  | (startup/crt) | ☐ |  |
| `0x00401456` | `thunk_FUN_00481880` |  | (startup/crt) | ☐ |  |
| `0x0040145b` | `thunk_FUN_0045eaf0` |  | (startup/crt) | ☐ |  |
| `0x00401460` | `thunk_FUN_00404470` |  | (startup/crt) | ☐ |  |
| `0x00401465` | `thunk_FUN_00462380` |  | (startup/crt) | ☐ |  |
| `0x0040146a` | `thunk_FUN_0042c500` |  | (startup/crt) | ☐ |  |
| `0x0040146f` | `thunk_FUN_004068f0` |  | (startup/crt) | ☐ |  |
| `0x00401474` | `thunk_FUN_0042c180` |  | (startup/crt) | ☐ |  |
| `0x0040147e` | `thunk_FUN_004410d0` |  | (startup/crt) | ☐ |  |
| `0x00401488` | `thunk_FUN_0047d5b0` |  | (startup/crt) | ☐ |  |
| `0x0040148d` | `thunk_FUN_00432860` |  | (startup/crt) | ☐ |  |
| `0x00401492` | `thunk_FUN_0047d7a0` |  | (startup/crt) | ☐ |  |
| `0x00401497` | `thunk_FUN_0047e0c0` |  | (startup/crt) | ☐ |  |
| `0x004014a1` | `thunk_FUN_0046c120` |  | (startup/crt) | ☐ |  |
| `0x004014ab` | `thunk_FUN_0040dbe0` |  | (startup/crt) | ☐ |  |
| `0x004014b0` | `thunk_FUN_00486e20` |  | (startup/crt) | ☐ |  |
| `0x004014ba` | `thunk_FUN_0042e420` |  | (startup/crt) | ☐ |  |
| `0x004014c4` | `thunk_FUN_00486d80` |  | (startup/crt) | ☐ |  |
| `0x004014ce` | `thunk_FUN_0042bc00` |  | (startup/crt) | ☐ |  |
| `0x004014d3` | `thunk_FUN_00476e60` |  | (startup/crt) | ☐ |  |
| `0x004014e2` | `thunk_FUN_0046c920` |  | (startup/crt) | ☐ |  |
| `0x004014f1` | `thunk_FUN_00444350` |  | (startup/crt) | ☐ |  |
| `0x004014f6` | `thunk_FUN_0047ef70` |  | (startup/crt) | ☐ |  |
| `0x004014fb` | `thunk_FUN_004433b0` |  | (startup/crt) | ☐ |  |
| `0x00401500` | `thunk_FUN_0046ce10` |  | (startup/crt) | ☐ |  |
| `0x00401505` | `thunk_FUN_00412cc0` |  | (startup/crt) | ☐ |  |
| `0x0040150a` | `thunk_FUN_00483990` |  | (startup/crt) | ☐ |  |
| `0x0040150f` | `thunk_FUN_00443ee0` |  | (startup/crt) | ☐ |  |
| `0x00401514` | `thunk_FUN_00483c70` |  | (startup/crt) | ☐ |  |
| `0x00401519` | `thunk_FUN_0046db50` |  | (startup/crt) | ☐ |  |
| `0x00401523` | `thunk_FUN_0043eb70` |  | (startup/crt) | ☐ |  |
| `0x00401528` | `thunk_FUN_00412570` |  | (startup/crt) | ☐ |  |
| `0x0040152d` | `thunk_FUN_00413bd0` |  | (startup/crt) | ☐ |  |
| `0x00401532` | `thunk_FUN_0042d2a0` |  | (startup/crt) | ☐ |  |
| `0x00401537` | `thunk_FUN_00475a20` |  | (startup/crt) | ☐ |  |
| `0x00401541` | `thunk_FUN_00413530` |  | (startup/crt) | ☐ |  |
| `0x00401546` | `thunk_FUN_00412be0` |  | (startup/crt) | ☐ |  |
| `0x00401550` | `thunk_FUN_00430440` |  | (startup/crt) | ☐ |  |
| `0x00401555` | `thunk_FUN_0047e210` |  | (startup/crt) | ☐ |  |
| `0x0040155a` | `thunk_FUN_00457f90` |  | (startup/crt) | ☐ |  |
| `0x00401573` | `thunk_FUN_0041ea00` |  | (startup/crt) | ☐ |  |
| `0x00401578` | `thunk_FUN_0043ee50` |  | (startup/crt) | ☐ |  |
| `0x00401582` | `thunk_FUN_0046f940` |  | (startup/crt) | ☐ |  |
| `0x00401587` | `thunk_FUN_00409570` |  | (startup/crt) | ☐ |  |
| `0x0040158c` | `thunk_FUN_00409460` |  | (startup/crt) | ☐ |  |
| `0x00401596` | `thunk_FUN_0042e690` |  | (startup/crt) | ☐ |  |
| `0x0040159b` | `thunk_FUN_0047f5b0` |  | (startup/crt) | ☐ |  |
| `0x004015a0` | `thunk_FUN_00452b50` |  | (startup/crt) | ☐ |  |
| `0x004015b4` | `thunk_FUN_00460190` |  | (startup/crt) | ☐ |  |
| `0x004015b9` | `thunk_FUN_00461720` |  | (startup/crt) | ☐ |  |
| `0x004015be` | `thunk_FUN_00443df0` |  | (startup/crt) | ☐ |  |
| `0x004015c3` | `thunk_FUN_0045e880` |  | (startup/crt) | ☐ |  |
| `0x004015c8` | `thunk_FUN_00403790` |  | (startup/crt) | ☐ |  |
| `0x004015d2` | `thunk_FUN_00411870` |  | (startup/crt) | ☐ |  |
| `0x004015d7` | `thunk_FUN_004761d0` |  | (startup/crt) | ☐ |  |
| `0x004015e6` | `thunk_FUN_00458900` |  | (startup/crt) | ☐ |  |
| `0x004015f5` | `thunk_FUN_0040ed70` |  | (startup/crt) | ☐ |  |
| `0x004015fa` | `thunk_FUN_00406c50` |  | (startup/crt) | ☐ |  |
| `0x004015ff` | `thunk_FUN_0047fa10` |  | (startup/crt) | ☐ |  |
| `0x00401604` | `thunk_FUN_00483490` |  | (startup/crt) | ☐ |  |
| `0x00401609` | `thunk_FUN_0047f180` |  | (startup/crt) | ☐ |  |
| `0x0040160e` | `thunk_FUN_00458bb0` |  | (startup/crt) | ☐ |  |
| `0x00401613` | `thunk_FUN_00483130` |  | (startup/crt) | ☐ |  |
| `0x00401618` | `thunk_FUN_004746a0` |  | (startup/crt) | ☐ |  |
| `0x0040161d` | `thunk_FUN_00423710` |  | (startup/crt) | ☐ |  |
| `0x00401631` | `thunk_FUN_0043e100` |  | (startup/crt) | ☐ |  |
| `0x0040163b` | `thunk_FUN_0047f670` |  | (startup/crt) | ☐ |  |
| `0x00401645` | `thunk_FUN_0047a1c0` |  | (startup/crt) | ☐ |  |
| `0x0040164a` | `thunk_FUN_0047d0b0` |  | (startup/crt) | ☐ |  |
| `0x00401654` | `thunk_FUN_0041d620` |  | (startup/crt) | ☐ |  |
| `0x00401659` | `thunk_FUN_0047eed0` |  | (startup/crt) | ☐ |  |
| `0x0040165e` | `thunk_FUN_0046e810` |  | (startup/crt) | ☐ |  |
| `0x00401668` | `thunk_FUN_0042b510` |  | (startup/crt) | ☐ |  |
| `0x00401677` | `thunk_FUN_0041f160` |  | (startup/crt) | ☐ |  |
| `0x0040167c` | `thunk_FUN_00430d20` |  | (startup/crt) | ☐ |  |
| `0x00401686` | `thunk_FUN_0042bb60` |  | (startup/crt) | ☐ |  |
| `0x00401690` | `thunk_FUN_00458290` |  | (startup/crt) | ☐ |  |
| `0x0040169a` | `thunk_FUN_0047f2c0` |  | (startup/crt) | ☐ |  |
| `0x0040169f` | `thunk_FUN_004140a0` |  | (startup/crt) | ☐ |  |
| `0x004016a9` | `thunk_FUN_00487770` |  | (startup/crt) | ☐ |  |
| `0x004016ae` | `thunk_FUN_0046e040` |  | (startup/crt) | ☐ |  |
| `0x004016c2` | `thunk_FUN_00429320` |  | (startup/crt) | ☐ |  |
| `0x004016c7` | `thunk_FUN_0040eb70` |  | (startup/crt) | ☐ |  |
| `0x004016cc` | `thunk_FUN_0040cc30` |  | (startup/crt) | ☐ |  |
| `0x004016d1` | `thunk_FUN_00411980` |  | (startup/crt) | ☐ |  |
| `0x004016d6` | `thunk_FUN_00481320` |  | (startup/crt) | ☐ |  |
| `0x004016db` | `thunk_FUN_0041a2f0` |  | (startup/crt) | ☐ |  |
| `0x004016e5` | `thunk_FUN_00413320` |  | (startup/crt) | ☐ |  |
| `0x004016ea` | `thunk_FUN_0041a770` |  | (startup/crt) | ☐ |  |
| `0x004016ef` | `thunk_FUN_0047cd00` |  | (startup/crt) | ☐ |  |
| `0x004016f4` | `thunk_FUN_00473800` |  | (startup/crt) | ☐ |  |
| `0x00401703` | `thunk_FUN_004435a0` |  | (startup/crt) | ☐ |  |
| `0x0040170d` | `thunk_FUN_00457070` |  | (startup/crt) | ☐ |  |
| `0x00401717` | `thunk_FUN_00443820` |  | (startup/crt) | ☐ |  |
| `0x0040171c` | `thunk_FUN_00470520` |  | (startup/crt) | ☐ |  |
| `0x00401726` | `thunk_FUN_0046fdd0` |  | (startup/crt) | ☐ |  |
| `0x0040173a` | `thunk_FUN_0047f500` |  | (startup/crt) | ☐ |  |
| `0x0040173f` | `thunk_FUN_00472340` |  | (startup/crt) | ☐ |  |
| `0x00401744` | `thunk_FUN_00407050` |  | (startup/crt) | ☐ |  |
| `0x00401749` | `thunk_FUN_00470490` |  | (startup/crt) | ☐ |  |
| `0x00401753` | `thunk_FUN_0045db60` |  | (startup/crt) | ☐ |  |
| `0x0040176c` | `thunk_FUN_0042b760` |  | (startup/crt) | ☐ |  |
| `0x00401771` | `thunk_FUN_0041f700` |  | (startup/crt) | ☐ |  |
| `0x00401776` | `thunk_FUN_00405450` |  | (startup/crt) | ☐ |  |
| `0x00401785` | `thunk_FUN_004133c0` |  | (startup/crt) | ☐ |  |
| `0x0040178f` | `thunk_FUN_004038d0` |  | (startup/crt) | ☐ |  |
| `0x00401799` | `thunk_FUN_004389f0` |  | (startup/crt) | ☐ |  |
| `0x0040179e` | `thunk_FUN_0042cb40` |  | (startup/crt) | ☐ |  |
| `0x004017ad` | `thunk_FUN_00438d00` |  | (startup/crt) | ☐ |  |
| `0x004017b7` | `thunk_FUN_0042a440` |  | (startup/crt) | ☐ |  |
| `0x004017bc` | `thunk_FUN_0045d810` |  | (startup/crt) | ☐ |  |
| `0x004017c6` | `thunk_FUN_0043d690` |  | (startup/crt) | ☐ |  |
| `0x004017cb` | `thunk_FUN_00404000` |  | (startup/crt) | ☐ |  |
| `0x004017e9` | `thunk_FUN_0041abb0` |  | (startup/crt) | ☐ |  |
| `0x004017f3` | `thunk_FUN_00419130` |  | (startup/crt) | ☐ |  |
| `0x004017fd` | `thunk_FUN_00406f30` |  | (startup/crt) | ☐ |  |
| `0x00401802` | `thunk_FUN_0046e6d0` |  | (startup/crt) | ☐ |  |
| `0x0040180c` | `thunk_FUN_0040db20` |  | (startup/crt) | ☐ |  |
| `0x00401811` | `thunk_FUN_0041fb20` |  | (startup/crt) | ☐ |  |
| `0x00401816` | `thunk_FUN_0045e6f0` |  | (startup/crt) | ☐ |  |
| `0x00401825` | `thunk_FUN_00418da0` |  | (startup/crt) | ☐ |  |
| `0x0040182a` | `thunk_FUN_00481c70` |  | (startup/crt) | ☐ |  |
| `0x00401834` | `thunk_FUN_004824a0` |  | (startup/crt) | ☐ |  |
| `0x00401839` | `thunk_FUN_0041a020` |  | (startup/crt) | ☐ |  |
| `0x00401848` | `thunk_FUN_004071a0` |  | (startup/crt) | ☐ |  |
| `0x0040184d` | `thunk_FUN_00412ac0` |  | (startup/crt) | ☐ |  |
| `0x00401852` | `thunk_FUN_0042c470` |  | (startup/crt) | ☐ |  |
| `0x0040185c` | `thunk_FUN_0042bf00` |  | (startup/crt) | ☐ |  |
| `0x00401861` | `thunk_FUN_0045ec10` |  | (startup/crt) | ☐ |  |
| `0x0040186b` | `thunk_FUN_0042a660` |  | (startup/crt) | ☐ |  |
| `0x0040187a` | `thunk_FUN_0045d010` |  | (startup/crt) | ☐ |  |
| `0x00401889` | `thunk_FUN_0045d4e0` |  | (startup/crt) | ☐ |  |
| `0x0040188e` | `thunk_FUN_00452440` |  | (startup/crt) | ☐ |  |
| `0x00401898` | `thunk_FUN_00435020` |  | (startup/crt) | ☐ |  |
| `0x004018a2` | `thunk_FUN_0041eab0` |  | (startup/crt) | ☐ |  |
| `0x004018a7` | `thunk_FUN_004051c0` |  | (startup/crt) | ☐ |  |
| `0x004018ac` | `thunk_FUN_004188b0` |  | (startup/crt) | ☐ |  |
| `0x004018b1` | `thunk_FUN_0040b430` |  | (startup/crt) | ☐ |  |
| `0x004018bb` | `thunk_FUN_00404680` |  | (startup/crt) | ☐ |  |
| `0x004018ca` | `thunk_FUN_0040ac20` |  | (startup/crt) | ☐ |  |
| `0x004018cf` | `thunk_FUN_00403770` |  | (startup/crt) | ☐ |  |
| `0x004018de` | `thunk_FUN_00429130` |  | (startup/crt) | ☐ |  |
| `0x004018e8` | `thunk_FUN_0042e380` |  | (startup/crt) | ☐ |  |
| `0x004018ed` | `thunk_FUN_00412430` |  | (startup/crt) | ☐ |  |
| `0x004018f7` | `thunk_FUN_0040d400` |  | (startup/crt) | ☐ |  |
| `0x00401906` | `thunk_FUN_004052d0` |  | (startup/crt) | ☐ |  |
| `0x0040190b` | `thunk_FUN_00475d00` |  | (startup/crt) | ☐ |  |
| `0x0040191f` | `thunk_FUN_00461aa0` |  | (startup/crt) | ☐ |  |
| `0x00401929` | `thunk_FUN_00409070` |  | (startup/crt) | ☐ |  |
| `0x0040192e` | `thunk_FUN_0040b0c0` |  | (startup/crt) | ☐ |  |
| `0x0040193d` | `thunk_FUN_0042a7f0` |  | (startup/crt) | ☐ |  |
| `0x00401947` | `thunk_FUN_00430e40` |  | (startup/crt) | ☐ |  |
| `0x00401951` | `thunk_FUN_0046f7f0` |  | (startup/crt) | ☐ |  |
| `0x0040195b` | `thunk_FUN_00453130` |  | (startup/crt) | ☐ |  |
| `0x00401965` | `thunk_FUN_00487db0` |  | (startup/crt) | ☐ |  |
| `0x0040196a` | `thunk_FUN_00440da0` |  | (startup/crt) | ☐ |  |
| `0x0040196f` | `thunk_FUN_0046fa60` |  | (startup/crt) | ☐ |  |
| `0x00401974` | `thunk_FUN_00415210` |  | (startup/crt) | ☐ |  |
| `0x0040197e` | `thunk_FUN_004116b0` |  | (startup/crt) | ☐ |  |
| `0x00401983` | `thunk_FUN_00460630` |  | (startup/crt) | ☐ |  |
| `0x00401988` | `thunk_FUN_00414f90` |  | (startup/crt) | ☐ |  |
| `0x00401997` | `thunk_FUN_004705e0` |  | (startup/crt) | ☐ |  |
| `0x0040199c` | `thunk_FUN_0045e4c0` |  | (startup/crt) | ☐ |  |
| `0x004019a6` | `thunk_FUN_0040d630` |  | (startup/crt) | ☐ |  |
| `0x004019b0` | `thunk_FUN_0046ea40` |  | (startup/crt) | ☐ |  |
| `0x004019ba` | `thunk_FUN_00436810` |  | (startup/crt) | ☐ |  |
| `0x004019c9` | `thunk_FUN_0047ecf0` |  | (startup/crt) | ☐ |  |
| `0x004019d3` | `thunk_FUN_00405610` |  | (startup/crt) | ☐ |  |
| `0x004019dd` | `thunk_FUN_004619d0` |  | (startup/crt) | ☐ |  |
| `0x004019e2` | `thunk_FUN_0040d490` |  | (startup/crt) | ☐ |  |
| `0x004019e7` | `thunk_FUN_00477470` |  | (startup/crt) | ☐ |  |
| `0x004019ec` | `thunk_FUN_00483eb0` |  | (startup/crt) | ☐ |  |
| `0x004019f1` | `thunk_FUN_0047fb60` |  | (startup/crt) | ☐ |  |
| `0x004019f6` | `thunk_FUN_00477160` |  | (startup/crt) | ☐ |  |
| `0x004019fb` | `thunk_FUN_00406e70` |  | (startup/crt) | ☐ |  |
| `0x00401a05` | `thunk_FUN_0047ec10` |  | (startup/crt) | ☐ |  |
| `0x00401a14` | `thunk_FUN_0041c7e0` |  | (startup/crt) | ☐ |  |
| `0x00401a19` | `thunk_FUN_00475f90` |  | (startup/crt) | ☐ |  |
| `0x00401a1e` | `thunk_FUN_0042fbe0` |  | (startup/crt) | ☐ |  |
| `0x00401a23` | `thunk_FUN_00438870` |  | (startup/crt) | ☐ |  |
| `0x00401a28` | `thunk_FUN_0046f730` |  | (startup/crt) | ☐ |  |
| `0x00401a2d` | `thunk_FUN_00456180` |  | (startup/crt) | ☐ |  |
| `0x00401a32` | `thunk_FUN_00412030` |  | (startup/crt) | ☐ |  |
| `0x00401a37` | `thunk_FUN_00410190` |  | (startup/crt) | ☐ |  |
| `0x00401a3c` | `thunk_FUN_0046e780` |  | (startup/crt) | ☐ |  |
| `0x00401a41` | `thunk_FUN_00404ec0` |  | (startup/crt) | ☐ |  |
| `0x00401a5a` | `thunk_FUN_00443d50` |  | (startup/crt) | ☐ |  |
| `0x00401a5f` | `thunk_FUN_00432bb0` |  | (startup/crt) | ☐ |  |
| `0x00401a69` | `thunk_FUN_0042fa30` |  | (startup/crt) | ☐ |  |
| `0x00401a6e` | `thunk_FUN_00410170` |  | (startup/crt) | ☐ |  |
| `0x00401a78` | `thunk_FUN_004874a0` |  | (startup/crt) | ☐ |  |
| `0x00401a82` | `thunk_FUN_0046edf0` |  | (startup/crt) | ☐ |  |
| `0x00401a87` | `thunk_FUN_0045e550` |  | (startup/crt) | ☐ |  |
| `0x00401a8c` | `thunk_FUN_0046deb0` |  | (startup/crt) | ☐ |  |
| `0x00401a9b` | `thunk_FUN_00434ec0` |  | (startup/crt) | ☐ |  |
| `0x00401aa0` | `thunk_FUN_00455800` |  | (startup/crt) | ☐ |  |
| `0x00401aa5` | `thunk_FUN_0047d1b0` |  | (startup/crt) | ☐ |  |
| `0x00401aaa` | `thunk_FUN_00439d80` |  | (startup/crt) | ☐ |  |
| `0x00401ab4` | `thunk_FUN_00414840` |  | (startup/crt) | ☐ |  |
| `0x00401abe` | `thunk_FUN_00418750` |  | (startup/crt) | ☐ |  |
| `0x00401ac3` | `thunk_FUN_0040e370` |  | (startup/crt) | ☐ |  |
| `0x00401ac8` | `thunk_FUN_00475d90` |  | (startup/crt) | ☐ |  |
| `0x00401ad2` | `thunk_FUN_004586c0` |  | (startup/crt) | ☐ |  |
| `0x00401ad7` | `thunk_FUN_004833e0` |  | (startup/crt) | ☐ |  |
| `0x00401aeb` | `thunk_FUN_00443060` |  | (startup/crt) | ☐ |  |
| `0x00401af0` | `thunk_FUN_00483e20` |  | (startup/crt) | ☐ |  |
| `0x00401af5` | `thunk_FUN_00419e80` |  | (startup/crt) | ☐ |  |
| `0x00401aff` | `thunk_FUN_0046bd80` |  | (startup/crt) | ☐ |  |
| `0x00401b0e` | `thunk_FUN_00443b80` |  | (startup/crt) | ☐ |  |
| `0x00401b13` | `thunk_FUN_0042d500` |  | (startup/crt) | ☐ |  |
| `0x00401b18` | `thunk_FUN_0042fee0` |  | (startup/crt) | ☐ |  |
| `0x00401b1d` | `thunk_FUN_0046cf10` |  | (startup/crt) | ☐ |  |
| `0x00401b31` | `thunk_FUN_0046bcc0` |  | (startup/crt) | ☐ |  |
| `0x00401b36` | `thunk_FUN_00408810` |  | (startup/crt) | ☐ |  |
| `0x00401b3b` | `thunk_FUN_004861c0` |  | (startup/crt) | ☐ |  |
| `0x00401b40` | `thunk_FUN_0046faf0` |  | (startup/crt) | ☐ |  |
| `0x00401b45` | `thunk_FUN_0041d7b0` |  | (startup/crt) | ☐ |  |
| `0x00401b4f` | `thunk_FUN_00404a70` |  | (startup/crt) | ☐ |  |
| `0x00401b68` | `thunk_FUN_0042e600` |  | (startup/crt) | ☐ |  |
| `0x00401b77` | `thunk_FUN_0041f680` |  | (startup/crt) | ☐ |  |
| `0x00401b86` | `thunk_FUN_00406a10` |  | (startup/crt) | ☐ |  |
| `0x00401b95` | `thunk_FUN_00411a30` |  | (startup/crt) | ☐ |  |
| `0x00401b9a` | `thunk_FUN_0046e490` |  | (startup/crt) | ☐ |  |
| `0x00401b9f` | `thunk_FUN_0042b5a0` |  | (startup/crt) | ☐ |  |
| `0x00401ba4` | `thunk_FUN_0041f8f0` |  | (startup/crt) | ☐ |  |
| `0x00401bae` | `thunk_FUN_0046fc40` |  | (startup/crt) | ☐ |  |
| `0x00401bb3` | `thunk_FUN_00404d90` |  | (startup/crt) | ☐ |  |
| `0x00401bbd` | `thunk_FUN_00429a70` |  | (startup/crt) | ☐ |  |
| `0x00401bd6` | `thunk_FUN_00474e10` |  | (startup/crt) | ☐ |  |
| `0x00401be0` | `thunk_FUN_0047cae0` |  | (startup/crt) | ☐ |  |
| `0x00401bea` | `thunk_FUN_0042ec30` |  | (startup/crt) | ☐ |  |
| `0x00401bef` | `thunk_FUN_00477620` |  | (startup/crt) | ☐ |  |
| `0x00401bf4` | `thunk_FUN_0042ddd0` |  | (startup/crt) | ☐ |  |
| `0x00401bfe` | `thunk_FUN_00455760` |  | (startup/crt) | ☐ |  |
| `0x00401c12` | `thunk_FUN_00442fd0` |  | (startup/crt) | ☐ |  |
| `0x00401c1c` | `thunk_FUN_0041cea0` |  | (startup/crt) | ☐ |  |
| `0x00401c26` | `thunk_FUN_0047e000` |  | (startup/crt) | ☐ |  |
| `0x00401c2b` | `thunk_FUN_004037c0` |  | (startup/crt) | ☐ |  |
| `0x00401c30` | `thunk_FUN_0040bb20` |  | (startup/crt) | ☐ |  |
| `0x00401c35` | `thunk_FUN_00477f00` |  | (startup/crt) | ☐ |  |
| `0x00401c3a` | `thunk_FUN_0041a480` |  | (startup/crt) | ☐ |  |
| `0x00401c44` | `thunk_FUN_00474940` |  | (startup/crt) | ☐ |  |
| `0x00401c58` | `thunk_FUN_00475e80` |  | (startup/crt) | ☐ |  |
| `0x00401c62` | `thunk_FUN_00425020` |  | (startup/crt) | ☐ |  |
| `0x00401c71` | `thunk_FUN_0042ee10` |  | (startup/crt) | ☐ |  |
| `0x00401c76` | `thunk_FUN_0047d6a0` |  | (startup/crt) | ☐ |  |
| `0x00401c7b` | `thunk_FUN_004427e0` |  | (startup/crt) | ☐ |  |
| `0x00401c80` | `thunk_FUN_0040a350` |  | (startup/crt) | ☐ |  |
| `0x00401c85` | `thunk_FUN_0042b3e0` |  | (startup/crt) | ☐ |  |
| `0x00401c8a` | `thunk_FUN_0043c9d0` |  | (startup/crt) | ☐ |  |
| `0x00401c8f` | `thunk_FUN_0043f550` |  | (startup/crt) | ☐ |  |
| `0x00401c94` | `thunk_FUN_0041a510` |  | (startup/crt) | ☐ |  |
| `0x00401ca3` | `thunk_FUN_0047d510` |  | (startup/crt) | ☐ |  |
| `0x00401ca8` | `thunk_FUN_0041d1a0` |  | (startup/crt) | ☐ |  |
| `0x00401cad` | `thunk_FUN_0046c320` |  | (startup/crt) | ☐ |  |
| `0x00401cb2` | `thunk_FUN_0047c130` |  | (startup/crt) | ☐ |  |
| `0x00401cbc` | `thunk_FUN_004031c0` |  | (startup/crt) | ☐ |  |
| `0x00401cc1` | `thunk_FUN_00412ab0` |  | (startup/crt) | ☐ |  |
| `0x00401cc6` | `thunk_FUN_0040dcf0` |  | (startup/crt) | ☐ |  |
| `0x00401cd0` | `thunk_FUN_004832d0` |  | (startup/crt) | ☐ |  |
| `0x00401ce4` | `thunk_FUN_004780c0` |  | (startup/crt) | ☐ |  |
| `0x00401ce9` | `thunk_FUN_0047f840` |  | (startup/crt) | ☐ |  |
| `0x00401cf8` | `thunk_FUN_004436e0` |  | (startup/crt) | ☐ |  |
| `0x00401cfd` | `thunk_FUN_0041f120` |  | (startup/crt) | ☐ |  |
| `0x00401d02` | `thunk_FUN_004098c0` |  | (startup/crt) | ☐ |  |
| `0x00401d07` | `thunk_FUN_00476d90` |  | (startup/crt) | ☐ |  |
| `0x00401d16` | `thunk_FUN_004258c0` |  | (startup/crt) | ☐ |  |
| `0x00401d1b` | `thunk_FUN_0040b8f0` |  | (startup/crt) | ☐ |  |
| `0x00401d20` | `thunk_FUN_00487cb0` |  | (startup/crt) | ☐ |  |
| `0x00401d2a` | `thunk_FUN_0047bdc0` |  | (startup/crt) | ☐ |  |
| `0x00401d2f` | `thunk_FUN_004253c0` |  | (startup/crt) | ☐ |  |
| `0x00401d39` | `thunk_FUN_0041fa60` |  | (startup/crt) | ☐ |  |
| `0x00401d3e` | `thunk_FUN_0041d300` |  | (startup/crt) | ☐ |  |
| `0x00401d43` | `thunk_FUN_00481fe0` |  | (startup/crt) | ☐ |  |
| `0x00401d4d` | `thunk_FUN_0041b6d0` |  | (startup/crt) | ☐ |  |
| `0x00401d5c` | `thunk_FUN_004820c0` |  | (startup/crt) | ☐ |  |
| `0x00401d61` | `thunk_FUN_0047df40` |  | (startup/crt) | ☐ |  |
| `0x00401d66` | `thunk_FUN_004091d0` |  | (startup/crt) | ☐ |  |
| `0x00401d7a` | `thunk_FUN_004749f0` |  | (startup/crt) | ☐ |  |
| `0x00401d89` | `thunk_FUN_004303a0` |  | (startup/crt) | ☐ |  |
| `0x00401d93` | `thunk_FUN_0041dad0` |  | (startup/crt) | ☐ |  |
| `0x00401d9d` | `thunk_FUN_00425a20` |  | (startup/crt) | ☐ |  |
| `0x00401da2` | `thunk_FUN_0042bd40` |  | (startup/crt) | ☐ |  |
| `0x00401da7` | `thunk_FUN_0045dac0` |  | (startup/crt) | ☐ |  |
| `0x00401db1` | `thunk_FUN_0045eb80` |  | (startup/crt) | ☐ |  |
| `0x00401db6` | `thunk_FUN_00424ca0` |  | (startup/crt) | ☐ |  |
| `0x00401dbb` | `thunk_FUN_00443fe0` |  | (startup/crt) | ☐ |  |
| `0x00401dc0` | `thunk_FUN_0042bc90` |  | (startup/crt) | ☐ |  |
| `0x00401dca` | `thunk_FUN_0042dad0` |  | (startup/crt) | ☐ |  |
| `0x00401dcf` | `thunk_FUN_0042f6a0` |  | (startup/crt) | ☐ |  |
| `0x00401dd4` | `thunk_FUN_0043be40` |  | (startup/crt) | ☐ |  |
| `0x00401dde` | `thunk_FUN_00485ff0` |  | (startup/crt) | ☐ |  |
| `0x00401de8` | `thunk_FUN_00440ee0` |  | (startup/crt) | ☐ |  |
| `0x00401df7` | `thunk_FUN_0041f910` |  | (startup/crt) | ☐ |  |
| `0x00401dfc` | `thunk_FUN_0047f850` |  | (startup/crt) | ☐ |  |
| `0x00401e0b` | `thunk_FUN_004065e0` |  | (startup/crt) | ☐ |  |
| `0x00401e15` | `thunk_FUN_00412710` |  | (startup/crt) | ☐ |  |
| `0x00401e1f` | `thunk_FUN_0046bba0` |  | (startup/crt) | ☐ |  |
| `0x00401e24` | `thunk_FUN_00460770` |  | (startup/crt) | ☐ |  |
| `0x00401e29` | `thunk_FUN_0045f200` |  | (startup/crt) | ☐ |  |
| `0x00401e2e` | `thunk_FUN_004871b0` |  | (startup/crt) | ☐ |  |
| `0x00401e38` | `thunk_FUN_00484040` |  | (startup/crt) | ☐ |  |
| `0x00401e3d` | `thunk_FUN_0042f610` |  | (startup/crt) | ☐ |  |
| `0x00401e47` | `thunk_FUN_00472810` |  | (startup/crt) | ☐ |  |
| `0x00401e51` | `thunk_FUN_0045d0b0` |  | (startup/crt) | ☐ |  |
| `0x00401e5b` | `thunk_FUN_00473ee0` |  | (startup/crt) | ☐ |  |
| `0x00401e60` | `thunk_FUN_0045da20` |  | (startup/crt) | ☐ |  |
| `0x00401e65` | `thunk_FUN_00429b50` |  | (startup/crt) | ☐ |  |
| `0x00401e6a` | `thunk_FUN_0045d970` |  | (startup/crt) | ☐ |  |
| `0x00401e6f` | `thunk_FUN_004070f0` |  | (startup/crt) | ☐ |  |
| `0x00401e74` | `thunk_FUN_0043ea40` |  | (startup/crt) | ☐ |  |
| `0x00401e7e` | `thunk_FUN_00475ac0` |  | (startup/crt) | ☐ |  |
| `0x00401e83` | `thunk_FUN_0043e880` |  | (startup/crt) | ☐ |  |
| `0x00401e88` | `thunk_FUN_004612a0` |  | (startup/crt) | ☐ |  |
| `0x00401e8d` | `thunk_FUN_00414180` |  | (startup/crt) | ☐ |  |
| `0x00401e92` | `thunk_FUN_0046c750` |  | (startup/crt) | ☐ |  |
| `0x00401e9c` | `thunk_FUN_0041cdd0` |  | (startup/crt) | ☐ |  |
| `0x00401ea6` | `thunk_FUN_0040e550` |  | (startup/crt) | ☐ |  |
| `0x00401eb0` | `thunk_FUN_0045e0f0` |  | (startup/crt) | ☐ |  |
| `0x00401eba` | `thunk_FUN_0045ea60` |  | (startup/crt) | ☐ |  |
| `0x00401ec9` | `thunk_FUN_00443c20` |  | (startup/crt) | ☐ |  |
| `0x00401ed3` | `thunk_FUN_0040f700` |  | (startup/crt) | ☐ |  |
| `0x00401ed8` | `thunk_FUN_00432f00` |  | (startup/crt) | ☐ |  |
| `0x00401ee2` | `thunk_FUN_00483ac0` |  | (startup/crt) | ☐ |  |
| `0x00401eec` | `thunk_FUN_0042ae10` |  | (startup/crt) | ☐ |  |
| `0x00401ef1` | `thunk_FUN_0046f570` |  | (startup/crt) | ☐ |  |
| `0x00401ef6` | `thunk_FUN_0041c5a0` |  | (startup/crt) | ☐ |  |
| `0x00401efb` | `thunk_FUN_00408c40` |  | (startup/crt) | ☐ |  |
| `0x00401f0f` | `thunk_FUN_00456230` |  | (startup/crt) | ☐ |  |
| `0x00401f19` | `thunk_FUN_004100d0` |  | (startup/crt) | ☐ |  |
| `0x00401f1e` | `thunk_FUN_004306a0` |  | (startup/crt) | ☐ |  |
| `0x00401f32` | `thunk_FUN_00461ea0` |  | (startup/crt) | ☐ |  |
| `0x00401f37` | `thunk_FUN_00426530` |  | (startup/crt) | ☐ |  |
| `0x00401f3c` | `thunk_FUN_00452770` |  | (startup/crt) | ☐ |  |
| `0x00401f41` | `thunk_FUN_0043ef10` |  | (startup/crt) | ☐ |  |
| `0x00401f4b` | `thunk_FUN_00473be0` |  | (startup/crt) | ☐ |  |
| `0x00401f50` | `thunk_FUN_00456bc0` |  | (startup/crt) | ☐ |  |
| `0x00401f64` | `thunk_FUN_00424ec0` |  | (startup/crt) | ☐ |  |
| `0x00401f69` | `thunk_FUN_00460360` |  | (startup/crt) | ☐ |  |
| `0x00401f73` | `thunk_FUN_0043e7f0` |  | (startup/crt) | ☐ |  |
| `0x00401f78` | `thunk_FUN_00429d50` |  | (startup/crt) | ☐ |  |
| `0x00401f8c` | `thunk_FUN_00456f80` |  | (startup/crt) | ☐ |  |
| `0x00401f96` | `thunk_FUN_0045e7b0` |  | (startup/crt) | ☐ |  |
| `0x00401fa0` | `thunk_FUN_004773b0` |  | (startup/crt) | ☐ |  |
| `0x00401fa5` | `thunk_FUN_00486fe0` |  | (startup/crt) | ☐ |  |
| `0x00401faa` | `thunk_FUN_00439590` |  | (startup/crt) | ☐ |  |
| `0x00401fbe` | `thunk_FUN_0042d130` |  | (startup/crt) | ☐ |  |
| `0x00401fd2` | `thunk_FUN_0047f0e0` |  | (startup/crt) | ☐ |  |
| `0x00401fd7` | `thunk_FUN_0046bf40` |  | (startup/crt) | ☐ |  |
| `0x00401fdc` | `thunk_FUN_00403670` |  | (startup/crt) | ☐ |  |
| `0x00401fe6` | `thunk_FUN_004440c0` |  | (startup/crt) | ☐ |  |
| `0x00401feb` | `thunk_FUN_00416670` |  | (startup/crt) | ☐ |  |
| `0x00401ff5` | `thunk_FUN_004799d0` |  | (startup/crt) | ☐ |  |
| `0x00401ffa` | `thunk_FUN_004750d0` |  | (startup/crt) | ☐ |  |
| `0x00401fff` | `thunk_FUN_0041a6c0` |  | (startup/crt) | ☐ |  |
| `0x00402004` | `thunk_FUN_00487f50` |  | (startup/crt) | ☐ |  |
| `0x00402013` | `thunk_FUN_004075c0` |  | (startup/crt) | ☐ |  |
| `0x0040201d` | `thunk_FUN_00472ce0` |  | (startup/crt) | ☐ |  |
| `0x00402036` | `thunk_FUN_00417df0` |  | (startup/crt) | ☐ |  |
| `0x00402040` | `thunk_FUN_0041cd30` |  | (startup/crt) | ☐ |  |
| `0x00402045` | `thunk_FUN_00481ef0` |  | (startup/crt) | ☐ |  |
| `0x0040204f` | `thunk_FUN_00474850` |  | (startup/crt) | ☐ |  |
| `0x00402059` | `thunk_FUN_0043b9a0` |  | (startup/crt) | ☐ |  |
| `0x0040205e` | `thunk_FUN_00412ed0` |  | (startup/crt) | ☐ |  |
| `0x00402068` | `thunk_FUN_0041a5a0` |  | (startup/crt) | ☐ |  |
| `0x0040207c` | `thunk_FUN_00480780` |  | (startup/crt) | ☐ |  |
| `0x00402086` | `thunk_FUN_0046e9a0` |  | (startup/crt) | ☐ |  |
| `0x00402090` | `thunk_FUN_0046d900` |  | (startup/crt) | ☐ |  |
| `0x00402095` | `thunk_FUN_004821f0` |  | (startup/crt) | ☐ |  |
| `0x0040209f` | `thunk_FUN_0042ffe0` |  | (startup/crt) | ☐ |  |
| `0x004020a9` | `thunk_FUN_00411760` |  | (startup/crt) | ☐ |  |
| `0x004020ae` | `thunk_FUN_0046f890` |  | (startup/crt) | ☐ |  |
| `0x004020b3` | `thunk_FUN_00430300` |  | (startup/crt) | ☐ |  |
| `0x004020b8` | `thunk_FUN_00406b90` |  | (startup/crt) | ☐ |  |
| `0x004020bd` | `thunk_FUN_0047fc00` |  | (startup/crt) | ☐ |  |
| `0x004020c2` | `thunk_FUN_00420fb0` |  | (startup/crt) | ☐ |  |
| `0x004031c0` | `FUN_004031c0` |  | (startup/crt) | ☐ |  |
| `0x004031f0` | `FUN_004031f0` |  | (startup/crt) | ☐ |  |
| `0x00403230` | `FUN_00403230` |  | (startup/crt) | ☐ |  |
| `0x00403250` | `FUN_00403250` |  | (startup/crt) | ☐ |  |
| `0x00403510` | `FUN_00403510` |  | (startup/crt) | ☐ |  |
| `0x00403540` | `FUN_00403540` |  | (startup/crt) | ☐ |  |
| `0x00403670` | `FUN_00403670` |  | (startup/crt) | ☐ |  |
| `0x00403770` | `FUN_00403770` |  | (startup/crt) | ☐ |  |
| `0x00403790` | `FUN_00403790` |  | (startup/crt) | ☐ |  |
| `0x004037c0` | `FUN_004037c0` |  | (startup/crt) | ☐ |  |
| `0x00403840` | `FUN_00403840` |  | (startup/crt) | ☐ |  |
| `0x004038d0` | `FUN_004038d0` |  | (startup/crt) | ☐ |  |

### Advanim.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00403dd0` | `Anim_RemoveOnscreenByName` | `Anim_RemoveOnscreenByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00403ed0` | `Anim_FindOnscreenByName` | `Anim_FindOnscreenByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404000` | `Anim_ReloadOnscreenAnims` | `Anim_ReloadOnscreenAnims` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404280` | `Anim_ClearOnscreenList` | `Anim_ClearOnscreenList` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404310` | `Anim_GetNextFrame` | `Anim_GetNextFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004043d0` | `Anim_AdvanceFrame` | `Anim_AdvanceFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404470` | `Anim_PrepareForRead` | `Anim_PrepareForRead` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004045b0` | `Anim_FindFreeSlot` | `Anim_FindFreeSlot` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404680` | `Anim_SetMainCharAnim` | `Anim_SetMainCharAnim` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004049e0` | `Anim_SetPalCallback` | `Anim_SetPalCallback` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404a70` | `Anim_LoadPalette` | `Anim_LoadPalette` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404c50` | `Anim_DevLoadPalette` | `Anim_DevLoadPalette` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404d90` | `Anim_UpdatePalettes` | `Anim_UpdatePalettes` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00404ec0` | `Anim_TickPalette` | `Anim_TickPalette` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004051c0` | `Anim_NamesMatch` | `Anim_NamesMatch` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004052d0` | `Anim_FindSlotByName` | `Anim_FindSlotByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00405450` | `Anim_SetStopFrame` | `Anim_SetStopFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00405540` | `Anim_SetStopAtLastFrame` | `Anim_SetStopAtLastFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00405610` | `Anim_IsAtStopFrame` | `Anim_IsAtStopFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004056b0` | `Anim_MarkForDumpByName` | `Anim_MarkForDumpByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00405810` | `Anim_MarkForDump` | `Anim_MarkForDump` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004059c0` | `Anim_HandleFrameTick` | `Anim_HandleFrameTick` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406430` | `Anim_AddOnscreen` | `Anim_AddOnscreen` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004065e0` | `Anim_RegisterTickCallback` | `Anim_RegisterTickCallback` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004066e0` | `Anim_UnregisterTickCallback` | `Anim_UnregisterTickCallback` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406820` | `Anim_FireTickCallbacks` | `Anim_FireTickCallbacks` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004068f0` | `Anim_SetTickMode` | `Anim_SetTickMode` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406980` | `Anim_SetWalkTableBase` | `Anim_SetWalkTableBase` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406a10` | `Anim_SetCompletionCallback` | `Anim_SetCompletionCallback` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406ad0` | `Anim_ClearAllCompletionCallbacks` | `Anim_ClearAllCompletionCallbacks` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406b90` | `Anim_SetLoopingFlags` | `Anim_SetLoopingFlags` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406c50` | `Anim_SetPosition` | `Anim_SetPosition` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406d00` | `Anim_SetVelocity` | `Anim_SetVelocity` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406db0` | `Anim_SetCurrentFrame` | `Anim_SetCurrentFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406e70` | `Anim_GetCurrentFrame` | `Anim_GetCurrentFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406f30` | `Anim_SetFrameStep` | `Anim_SetFrameStep` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00406fc0` | `Anim_SetGlobalFrameStep` | `Anim_SetGlobalFrameStep` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407050` | `Anim_Freeze` | `Anim_Freeze` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004070f0` | `Anim_Unfreeze` | `Anim_Unfreeze` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004071a0` | `Anim_ResetFreeze` | `Anim_ResetFreeze` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407230` | `Anim_FreezeAll` | `Anim_FreezeAll` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407380` | `Anim_UnfreezeAll` | `Anim_UnfreezeAll` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004074d0` | `Anim_ProcessDumpQueue` | `Anim_ProcessDumpQueue` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004075c0` | `Anim_BuildDrawOrder` | `Anim_BuildDrawOrder` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407930` | `Anim_CompareByZ` | `Anim_CompareByZ` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004079e0` | `Anim_EnableDraw` | `Anim_EnableDraw` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407a70` | `Anim_EnableDraw2` | `Anim_EnableDraw2` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407b00` | `Anim_CompactFrameTable` | `Anim_CompactFrameTable` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00407e80` | `Anim_ShowFrame` | `Anim_ShowFrame` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408120` | `Anim_ShowFrameScaled` | `Anim_ShowFrameScaled` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408370` | `Anim_ShowFrameRescale` | `Anim_ShowFrameRescale` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004085c0` | `Anim_ShowFrameRotated` | `Anim_ShowFrameRotated` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408810` | `Anim_GetFrameTopLeft` | `Anim_GetFrameTopLeft` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408980` | `Anim_GetFrameBottomRight` | `Anim_GetFrameBottomRight` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408b20` | `Anim_GetCurrentFramePos` | `Anim_GetCurrentFramePos` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408c40` | `Anim_GetCurrentFrameRect` | `Anim_GetCurrentFrameRect` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408e10` | `Anim_GetPrevFrameRect` | `Anim_GetPrevFrameRect` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00408f80` | `Anim_GetFramePos` | `Anim_GetFramePos` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409070` | `Anim_GetFramePosAndSize` | `Anim_GetFramePosAndSize` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004091d0` | `Anim_CheckFreeSlot` | `Anim_CheckFreeSlot` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409260` | `Anim_StartGroup` | `Anim_StartGroup` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409340` | `Anim_IsInGroup` | `Anim_IsInGroup` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409460` | `Anim_AddToGroup` | `Anim_AddToGroup` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409570` | `Anim_AddByNum` | `Anim_AddByNum` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x004098c0` | `Anim_AddByName` | `Anim_AddByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409aa0` | `Anim_ExternalAddByName` | `Anim_ExternalAddByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x00409cc0` | `Anim_PutLastByName` | `Anim_PutLastByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040a100` | `Anim_PutLastByNum` | `Anim_PutLastByNum` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040a1d0` | `Anim_AddOnscreenByNum` | `Anim_AddOnscreenByNum` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040a350` | `Anim_Init` | `Anim_Init` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040a630` | `Anim_ReadFrameHeader` | `Anim_ReadFrameHeader` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040a740` | `Anim_SetFrameHeader` | `Anim_SetFrameHeader` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040ac20` | `Anim_SetFrameSound` | `Anim_SetFrameSound` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040aec0` | `Anim_ReleaseSoundRef` | `Anim_ReleaseSoundRef` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040af60` | `Anim_LoadMask` | `Anim_LoadMask` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b0c0` | `Anim_LoadByName` | `Anim_LoadByName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b1f0` | `Anim_LoadByNameGetCount` | `Anim_LoadByNameGetCount` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b340` | `Anim_LoadAndWait` | `Anim_LoadAndWait` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b430` | `Anim_SetIndiPal` | `Anim_SetIndiPal` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b690` | `Anim_DevSetIndiPal` | `Anim_DevSetIndiPal` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040b8f0` | `Anim_ReleaseIndiPal` | `Anim_ReleaseIndiPal` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040bb20` | `Anim_LoadToMem` | `Anim_LoadToMem` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040cc30` | `Anim_DevLoadToMem` | `Anim_DevLoadToMem` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040d2f0` | `Anim_DevReadFrameHeader` | `Anim_DevReadFrameHeader` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040d400` | `Anim_ClearSavedAnim` | `Anim_ClearSavedAnim` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040d490` | `Anim_TryRestoreSaved` | `Anim_TryRestoreSaved` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040d630` | `Anim_Free` | `Anim_Free` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040db20` | `Anim_SetShadowFlag` | `Anim_SetShadowFlag` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040dbe0` | `Anim_ResetPendingCallbacks` | `Anim_ResetPendingCallbacks` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040dcf0` | `Anim_HasPendingCallback` | `Anim_HasPendingCallback` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040de00` | `Anim_BeginNormalDraw` | `Anim_BeginNormalDraw` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040dea0` | `Anim_BeginAdditiveDraw` | `Anim_BeginAdditiveDraw` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040df40` | `Anim_FlushDraw` | `Anim_FlushDraw` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040dfe0` | `Anim_FindTopAtXY` | `Anim_FindTopAtXY` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040e220` | `Anim_GetName` | `Anim_GetName` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040e2b0` | `Anim_StopSound` | `Anim_StopSound` | Advanim.cpp | ☑ | `src/Advanim.cpp` |
| `0x0040e370` | `Anim_GameInit` | `Anim_GameInit` | Advanim.cpp | ☑ | `src/Advanim.cpp` |

### ADVENT.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0040e550` | `Adv_Init` | `Adv_Init` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040e650` | `Adv_LoadScreenshot` | `Adv_LoadScreenshot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040e960` | `Adv_PushCursorState` | `Adv_PushCursorState` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040ea20` | `Adv_PopCursorState` | `Adv_PopCursorState` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040eae0` | `Adv_ClearCursorState` | `Adv_ClearCursorState` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040eb70` | `Adv_UpdateHotspot` | `Adv_UpdateHotspot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040ecb0` | `Adv_PostVerb` | `Adv_PostVerb` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040ed70` | `Adv_GetVerb` | `Adv_GetVerb` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040ee20` | `Adv_TestAndClearVerb` | `Adv_TestAndClearVerb` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040eec0` | `Adv_CursorHandler` | `Adv_CursorHandler` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040f700` | `Adv_CheckRightClick` | `Adv_CheckRightClick` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040f7c0` | `Adv_InitAreaSlots` | `Adv_InitAreaSlots` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x0040fc40` | `Adv_RefreshCursor` | `Adv_RefreshCursor` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004100d0` | `Adv_FlushAnimCallbacks` | `Adv_FlushAnimCallbacks` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00410170` | `Adv_SetCDCheck` | `Adv_SetCDCheck` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00410190` | `Adv_AutoSaveRescue` | `Adv_AutoSaveRescue` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004101f0` | `Adv_RunScene` | `Adv_RunScene` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411570` | `Adv_FindVerbHandler` | `Adv_FindVerbHandler` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004116b0` | `Adv_RectsOverlap` | `Adv_RectsOverlap` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411760` | `Adv_RegisterCleanup` | `Adv_RegisterCleanup` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411870` | `Adv_UnregisterCleanup` | `Adv_UnregisterCleanup` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411980` | `Adv_RunCleanups` | `Adv_RunCleanups` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411a30` | `Adv_ClampInvScroll` | `Adv_ClampInvScroll` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411b40` | `Adv_AddInvItem` | `Adv_AddInvItem` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411c80` | `Adv_RemoveInvItem` | `Adv_RemoveInvItem` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411d70` | `Adv_ClearInvLayer` | `Adv_ClearInvLayer` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00411eb0` | `Adv_SetInvSlot` | `Adv_SetInvSlot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412030` | `Adv_SetInvSlotDirect` | `Adv_SetInvSlotDirect` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004120d0` | `Adv_FillFreeInvSlot` | `Adv_FillFreeInvSlot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004121c0` | `Adv_ClearInvSlot` | `Adv_ClearInvSlot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412260` | `Adv_RemoveInvSlotByNode` | `Adv_RemoveInvSlotByNode` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412350` | `Adv_IsNodeInSpecialSlot` | `Adv_IsNodeInSpecialSlot` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412430` | `Adv_CompactInvList` | `Adv_CompactInvList` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412570` | `Adv_SetUpdateCallback` | `Adv_SetUpdateCallback` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412600` | `Adv_ScrollInv` | `Adv_ScrollInv` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412710` | `Adv_UpdateInv` | `Adv_UpdateInv` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004129e0` | `Adv_DrawAllInvLayers` | `Adv_DrawAllInvLayers` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412ab0` | `Adv_SetDrawSuppressed` | `Adv_SetDrawSuppressed` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412ac0` | `Adv_Tick` | `Adv_Tick` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412be0` | `Adv_TickFrames` | `Adv_TickFrames` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412cc0` | `Adv_TickFramesNoAsync` | `Adv_TickFramesNoAsync` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412da0` | `Adv_WaitForMouse` | `Adv_WaitForMouse` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00412ed0` | `Adv_WaitForMouseNoAsync` | `Adv_WaitForMouseNoAsync` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00413000` | `Adv_WaitForFrameOrMouse` | `Adv_WaitForFrameOrMouse` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00413190` | `Adv_WaitForFrameOrMouseNoAsync` | `Adv_WaitForFrameOrMouseNoAsync` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x00413320` | `Adv_ResetAnimSentinel` | `Adv_ResetAnimSentinel` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |
| `0x004133c0` | `Adv_LoadAnimByName` | `Adv_LoadAnimByName` | ADVENT.cpp | ☑ | `src/ADVENT.cpp` |

### ANI32.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00413530` | `Ani32_DrawScaledRLE` | `Ani32_DrawScaledRLE` | ANI32.cpp | ☑ | `src/ANI32.cpp` |
| `0x00413bd0` | `Ani32_BuildAreaLookup` | `Ani32_BuildAreaLookup` | ANI32.cpp | ☑ | `src/ANI32.cpp` |

### AREAS.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00413f10` | `Area_AddNodeToYBuckets` | `Area_AddNodeToYBuckets` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x004140a0` | `Area_ClearYBuckets` | `Area_ClearYBuckets` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414180` | `Area_FindAt` | `Area_FindAt` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x004146d0` | `Area_GetLastHit` | `Area_GetLastHit` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414760` | `Area_FindNodeByTag` | `Area_FindNodeByTag` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414840` | `Area_RemoveSprite` | `Area_RemoveSprite` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414990` | `Area_RemoveSpriteAt` | `Area_RemoveSpriteAt` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414a80` | `Area_ResetList` | `Area_ResetList` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414b20` | `Area_RewindList` | `Area_RewindList` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414bb0` | `Area_SeekListEnd` | `Area_SeekListEnd` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414c40` | `Area_ListNext` | `Area_ListNext` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414d00` | `Area_ListPrev` | `Area_ListPrev` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414db0` | `Area_ListGet` | `Area_ListGet` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414e40` | `Area_ListSet` | `Area_ListSet` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414ed0` | `Area_ListAppend` | `Area_ListAppend` | AREAS.cpp | ☑ | `src/AREAS.cpp` |
| `0x00414f90` | `Bani_PutBlock` | `Bani_PutBlock` | BANI.cpp | ☑ | `src/AREAS.cpp` |

### BANI.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00415210` | `Bani_PutIndi` | `Bani_PutIndi` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x004154a0` | `Bani_DecodeRle6` | `Bani_DecodeRle6` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x004157f0` | `Bani_DecodeRleCont` | `Bani_DecodeRleCont` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00415a60` | `Bani_DecodeRle4` | `Bani_DecodeRle4` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00415d90` | `Bani_DecodeRle3` | `Bani_DecodeRle3` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x004160c0` | `Bani_DecodeRleNibble` | `Bani_DecodeRleNibble` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00416520` | `Bani_BlitRaw` | `Bani_BlitRaw` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00416670` | `Bani_DrawBlocks` | `Bani_DrawBlocks` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00416830` | `Bani_DecodeRle6Remap` | `Bani_DecodeRle6Remap` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00416bd0` | `Bani_SkipRleCont` | `Bani_SkipRleCont` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00416cf0` | `Bani_DecodeRle4Remap` | `Bani_DecodeRle4Remap` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417070` | `Bani_DecodeRle3Remap` | `Bani_DecodeRle3Remap` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x004173f0` | `Bani_DecodeRleNibbleRemap` | `Bani_DecodeRleNibbleRemap` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417890` | `Bani_BlitRawRemap` | `Bani_BlitRawRemap` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x004179f0` | `Bani_DrawBlocksIndi` | `Bani_DrawBlocksIndi` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417bc0` | `Bani_InitNoLoopList` | `Bani_InitNoLoopList` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417d20` | `Bani_IsNoLoopId` | `Bani_IsNoLoopId` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417df0` | `Bani_Noop` | `Bani_Noop` | BANI.cpp | ☑ | `src/BANI.cpp` |
| `0x00417e80` | `Curs_Init` | `Curs_Init` | CURSORS.cpp | ☑ | `src/BANI.cpp` |
| `0x00418210` | `Curs_LoadCursor` | `Curs_LoadCursor` | CURSORS.cpp | ☑ | `src/BANI.cpp` |

### CURSORS.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00418640` | `Curs_LoadCursorSelect` | `Curs_LoadCursorSelect` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00418750` | `Curs_UpdateAnimFrame` | `Curs_UpdateAnimFrame` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x004188b0` | `Curs_Animate` | `Curs_Animate` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00418a20` | `Curs_SetCursor` | `Curs_SetCursor` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00418da0` | `Curs_SetCursorByMode` | `Curs_SetCursorByMode` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419090` | `Curs_SetOffset` | `Curs_SetOffset` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419130` | `Curs_Tick` | `Curs_Tick` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419230` | `Curs_Update` | `Curs_Update` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419360` | `Curs_SetPosition` | `Curs_SetPosition` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419420` | `Curs_GetSurface` | `Curs_GetSurface` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x004194e0` | `Curs_Restore` | `Curs_Restore` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419a20` | `Curs_PutOnPage` | `Curs_PutOnPage` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419e80` | `Curs_RestoreFromPage` | `Curs_RestoreFromPage` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00419f70` | `Curs_Shutdown` | `Curs_Shutdown` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a020` | `Curs_ShowWin32` | `Curs_ShowWin32` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a0e0` | `Curs_HideWin32` | `Curs_HideWin32` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a1b0` | `Curs_SetPosCallback` | `Curs_SetPosCallback` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a240` | `Curs_GetSize` | `Curs_GetSize` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a2f0` | `Curs_SetWin32Cursor` | `Curs_SetWin32Cursor` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a3f0` | `Curs_SetWaitCursor` | `Curs_SetWaitCursor` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a480` | `Curs_ForceRestore` | `Curs_ForceRestore` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a510` | `Curs_DisableDraw` | `Curs_DisableDraw` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a5a0` | `Curs_EnableDraw` | `Curs_EnableDraw` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a630` | `Curs_SetOverrideCallback` | `Curs_SetOverrideCallback` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a6c0` | `Curs_InitLog` | `Curs_InitLog` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a740` | `Debug_Assert` | `Debug_Assert` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a760` | `Debug_AssertFatal` | `Debug_AssertFatal` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a770` | `Debug_Trace` | `Debug_Trace` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a790` | `Debug_TraceVal` | `Debug_TraceVal` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x0041a7b0` | `DDI_CreateOffscreenSurf` | `DDI_CreateOffscreenSurf` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |

### DDRAWI.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0041abb0` | `DDI_RecreateOffscreenSurf` | `DDI_RecreateOffscreenSurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041af50` | `DDI_CreateOverlaySurf` | `DDI_CreateOverlaySurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041b2c0` | `DDI_RefreshSurfs` | `DDI_RefreshSurfs` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041b610` | `DDI_ReleaseSurf` | `DDI_ReleaseSurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041b6d0` | `DDI_SetFullscreenMode` | `DDI_SetFullscreenMode` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041b7c0` | `DDI_InitDirectDraw` | `DDI_InitDirectDraw` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041bea0` | `DDI_SetClipRegion` | `DDI_SetClipRegion` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c200` | `DDI_DetachBackBuffer` | `DDI_DetachBackBuffer` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c2a0` | `DDI_AttachBackBuffer` | `DDI_AttachBackBuffer` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c340` | `DDI_SetDisplayMode` | `DDI_SetDisplayMode` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c5a0` | `DDI_BltToScreen` | `DDI_BltToScreen` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c7e0` | `DDI_BltSurfToSurf` | `DDI_BltSurfToSurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041c9d0` | `DDI_RestoreLostSurfs` | `DDI_RestoreLostSurfs` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041cae0` | `DDI_StretchBlt` | `DDI_StretchBlt` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041cd30` | `DDI_GetSurfPtr` | `DDI_GetSurfPtr` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041cdd0` | `DDI_GetRectPtr` | `DDI_GetRectPtr` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041cea0` | `DDI_LockSurf` | `DDI_LockSurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d080` | `DDI_EnterCritical` | `DDI_EnterCritical` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d110` | `DDI_LeaveCritical` | `DDI_LeaveCritical` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d1a0` | `DDI_UnlockSurf` | `DDI_UnlockSurf` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d300` | `DDI_GetScreenPtr` | `DDI_GetScreenPtr` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d4e0` | `DDI_ReleaseScreenPtr` | `DDI_ReleaseScreenPtr` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d620` | `DDI_GetSurfDC` | `DDI_GetSurfDC` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d7b0` | `DDI_ReturnSurfDC` | `DDI_ReturnSurfDC` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d8f0` | `DDI_WaitVerticalRetrace` | `DDI_WaitVerticalRetrace` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041d9c0` | `DDI_CheckFullscreenMode` | `DDI_CheckFullscreenMode` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041dad0` | `DDI_ErrorToString` | `DDI_ErrorToString` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041e600` | `DDI_SetBackBufferTarget` | `DDI_SetBackBufferTarget` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041e6a0` | `DDI_ClearBackBufferTarget` | `DDI_ClearBackBufferTarget` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041e740` | `DDI_ClearLostSurfs` | `DDI_ClearLostSurfs` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041e970` | `DDI_SurfLock_Acquire` | `DDI_SurfLock_Acquire` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041ea00` | `DDI_SurfLock_Switch` | `DDI_SurfLock_Switch` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041eab0` | `DDI_SurfLock_Release` | `DDI_SurfLock_Release` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041eb10` | `DDI_StartDissolveEffect` | `DDI_StartDissolveEffect` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041ed60` | `DDI_DissolveEffectTick` | `DDI_DissolveEffectTick` | DDRAWI.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041efe0` | `Err_FatalFileCorrupt` | `Err_FatalFileCorrupt` | ERRORS.cpp | ☑ | `src/DDRAWI.cpp` |
| `0x0041f010` | `Err_FatalWithMessage` | `Err_FatalWithMessage` | ERRORS.cpp | ☑ | `src/DDRAWI.cpp` |

### ERRORS.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0041f120` | `Err_ReadDebugLevel` | `Err_ReadDebugLevel` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f160` | `Err_LoadStrings` | `Err_LoadStrings` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f480` | `Err_Abort` | `Err_Abort` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f4b0` | `Debug_Assert` | `Debug_Assert` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f4e0` | `Err_BadResFile` | `Err_BadResFile` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f510` | `Err_BadIdxFile` | `Err_BadIdxFile` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f540` | `Err_BadVersion` | `Err_BadVersion` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f570` | `Err_BadSP` | `Err_BadSP` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f5a0` | `Err_MissingDefaultSP` | `Err_MissingDefaultSP` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f5c0` | `Err_MissingRes` | `Err_MissingRes` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f620` | `Err_MissingResFile` | `Err_MissingResFile` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f680` | `Err_BadResEntry` | `Err_BadResEntry` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f6d0` | `Err_OutOfMemory` | `Err_OutOfMemory` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f700` | `Err_ShowDialog` | `Err_ShowDialog` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f740` | `Err_BadSCN` | `Err_BadSCN` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f770` | `Err_DriveCheck` | `Err_DriveCheck` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f870` | `Err_BadSound` | `Err_BadSound` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f8a0` | `Err_BadMOV` | `Err_BadMOV` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f8d0` | `Err_BadSCNVersion` | `Err_BadSCNVersion` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f8f0` | `Err_GetString` | `Err_GetString` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f910` | `Err_AskDialog` | `Err_AskDialog` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041f960` | `Err_RestartGame` | `Err_RestartGame` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041fa60` | `Debug_TraceVal` | `Debug_TraceVal` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041fad0` | `Err_SetRecord3` | `Err_SetRecord3` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041fb20` | `Err_SetRecord2` | `Err_SetRecord2` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x0041fb70` | `Err_Dispatch` | `Err_Dispatch` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x00420a10` | `Err_GetSeverity` | `Err_GetSeverity` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x00420b50` | `Err_ClearStack` | `Err_ClearStack` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x00420b70` | `Err_PushStack` | `Err_PushStack` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x00420ba0` | `Err_UnhandledException` | `Err_UnhandledException` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |
| `0x00420bd0` | `Err_Fatal` | `Err_Fatal` | ERRORS.cpp | ☑ | `src/ERRORS.cpp` |

### Except.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00420e50` | `Files_SetCurrentRoom` | `Files_SetCurrentRoom` | FILES.cpp | ☑ | `src/Except.cpp` |
| `0x00420e60` | `Files_SetErrSource` | `Files_SetErrSource` | FILES.cpp | ☑ | `src/Except.cpp` |
| `0x00420ed0` | `Files_GetTime` | `Files_GetTime` | FILES.cpp | ☑ | `src/Except.cpp` |
| `0x00420fb0` | `Files_LoadScn` | `Files_LoadScn` | FILES.cpp | ☑ | `src/Except.cpp` |
| `0x004220e0` | `Files_DoRead` | `Files_DoRead` | FILES.cpp | ☑ | `src/Except.cpp` |
| `0x00422240` | `Files_FreadString` | `Files_FreadString` | FILES.cpp | ☑ | `src/Except.cpp` |

### FILES.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004223a0` | `Files_SaveState` | `Files_SaveState` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00422d00` | `Files_LoadState` | `Files_LoadState` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00423710` | `Files_SaveStateAlt` | `Files_SaveStateAlt` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00424490` | `Files_FreeAreaCache` | `Files_FreeAreaCache` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00424980` | `Files_RleDecompress` | `Files_RleDecompress` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00424af0` | `Files_LoadPalExternal` | `Files_LoadPalExternal` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00424ca0` | `Files_LoadPal` | `Files_LoadPal` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00424ec0` | `Files_DevLoadPal` | `Files_DevLoadPal` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00425020` | `Files_LoadSaveGame` | `Files_LoadSaveGame` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004253c0` | `Files_LoadInvMain` | `Files_LoadInvMain` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004258c0` | `Files_EraseStates` | `Files_EraseStates` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00425a20` | `Files_SaveGame` | `Files_SaveGame` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00426530` | `Files_RegisterSpecialSave` | `Files_RegisterSpecialSave` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00426620` | `Files_UnregisterSpecialSave` | `Files_UnregisterSpecialSave` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00426750` | `Files_SaveGameFull` | `Files_SaveGameFull` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00427530` | `Files_BroadcastSave` | `Files_BroadcastSave` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004275f0` | `Files_LoadGameFull` | `Files_LoadGameFull` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004285e0` | `Files_BroadcastLoad` | `Files_BroadcastLoad` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004286a0` | `Files_ReplaceExtension` | `Files_ReplaceExtension` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004287a0` | `Files_ReadLine` | `Files_ReadLine` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00428890` | `Files_FindWatcomDebugger` | `Files_FindWatcomDebugger` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004289f0` | `Files_OFNHookProc` | `Files_OFNHookProc` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00428bb0` | `Files_OFNTimerCallback` | `Files_OFNTimerCallback` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00428c80` | `Files_SelectLoadGame` | `Files_SelectLoadGame` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00428e30` | `Files_SelectSaveGame` | `Files_SelectSaveGame` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429010` | `Files_SetSaveDir` | `Files_SetSaveDir` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x004290a0` | `Files_ClearSaveDir` | `Files_ClearSaveDir` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429130` | `Files_SelectFile` | `Files_SelectFile` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429320` | `Files_LoadBitmapByNum` | `Files_LoadBitmapByNum` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429440` | `Files_BmpFileToHBitmap` | `Files_BmpFileToHBitmap` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429950` | `Files_GetEditText` | `Files_GetEditText` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429a70` | `Files_SetEditText` | `Files_SetEditText` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429b50` | `Files_IsWholeWord` | `Files_IsWholeWord` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429ca0` | `Files_InitFindReplace` | `Files_InitFindReplace` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x00429d50` | `Files_SearchText` | `Files_SearchText` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a090` | `Files_ReplaceSelection` | `Files_ReplaceSelection` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a130` | `Files_HandleFindMessage` | `Files_HandleFindMessage` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a440` | `Files_CloseFindDialog` | `Files_CloseFindDialog` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a4e0` | `Files_OpenFindDialog` | `Files_OpenFindDialog` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a660` | `Files_EnumWindowsProc` | `Files_EnumWindowsProc` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a6f0` | `Files_EnumWindowsProc2` | `Files_EnumWindowsProc2` | FILES.cpp | ☑ | `src/FILES.cpp` |
| `0x0042a7f0` | `FrmTimer_Init` | `FrmTimer_Init` | FRMTIMER.cpp | ☑ | `src/FILES.cpp` |

### FRMTIMER.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0042a930` | `FrmTimer_Reset` | `FrmTimer_Reset` | FRMTIMER.cpp | ☑ | `src/FRMTIMER.cpp` |
| `0x0042a9f0` | `FrmTimer_OnTick` | `FrmTimer_OnTick` | FRMTIMER.cpp | ☑ | `src/FRMTIMER.cpp` |
| `0x0042aaf0` | `FrmTimer_SetFps` | `FrmTimer_SetFps` | FRMTIMER.cpp | ☑ | `src/FRMTIMER.cpp` |
| `0x0042abf0` | `FrmTimer_GetFps` | `FrmTimer_GetFps` | FRMTIMER.cpp | ☑ | `src/FRMTIMER.cpp` |
| `0x0042ac80` | `Fx_PlayAnyChar` | `Fx_PlayAnyChar` | FX.cpp | ☑ | `src/FRMTIMER.cpp` |

### FX.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0042ae10` | `Fx_PlayChar` | `Fx_PlayChar` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042af90` | `Fx_LoopCallback` | `Fx_LoopCallback` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b0c0` | `Fx_WaitChannel3` | `Fx_WaitChannel3` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b170` | `Fx_PlayCharRestart` | `Fx_PlayCharRestart` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b2f0` | `Fx_Play` | `Fx_Play` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b3e0` | `Fx_StopLoop` | `Fx_StopLoop` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b480` | `Fx_Stop` | `Fx_Stop` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b510` | `Fx_ClearCallback` | `Fx_ClearCallback` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b5a0` | `Fx_Resume` | `Fx_Resume` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b6d0` | `Fx_SetVolume` | `Fx_SetVolume` | FX.cpp | ☑ | `src/FX.cpp` |
| `0x0042b760` | `GI_InitSurfaces` | `GI_InitSurfaces` | GI.cpp | ☑ | `src/GI.cpp` |

### GI.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0042ba60` | `GI_CalcRegion` | `GI_CalcRegion` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042bb60` | `GI_WaitForReady` | `GI_WaitForReady` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042bc00` | `GI_SetDrawMode` | `GI_SetDrawMode` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042bc90` | `GI_SetPageFlip` | `GI_SetPageFlip` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042bd40` | `GI_LockActiveSurf` | `GI_LockActiveSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042bf00` | `GI_PutImgToScreen` | `GI_PutImgToScreen` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c030` | `GI_BlitResource` | `GI_BlitResource` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c0e0` | `GI_LockActiveSurf_Thunk` | `GI_LockActiveSurf_Thunk` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c180` | `GI_LockActiveSurf_Debug` | `GI_LockActiveSurf_Debug` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c370` | `GI_SetClipper` | `GI_SetClipper` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c470` | `GI_ClearClipper` | `GI_ClearClipper` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042c500` | `GI_FlipToScreen` | `GI_FlipToScreen` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042cb40` | `GI_BltRegionToScreen` | `GI_BltRegionToScreen` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042cca0` | `GI_GetPixel` | `GI_GetPixel` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042cec0` | `GI_PlotPixel` | `GI_PlotPixel` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d130` | `GI_LockActiveSurf_v2` | `GI_LockActiveSurf_v2` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d2a0` | `GI_LockActiveSurf_v3` | `GI_LockActiveSurf_v3` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d460` | `GI_CalcRectSize` | `GI_CalcRectSize` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d500` | `GI_ClearBorder` | `GI_ClearBorder` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d730` | `GI_LockActiveSurf_v4` | `GI_LockActiveSurf_v4` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042d8a0` | `GI_FillRect` | `GI_FillRect` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042dad0` | `GI_LockActiveSurf_v5` | `GI_LockActiveSurf_v5` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042dc50` | `GI_CopyBackbufToOverlay` | `GI_CopyBackbufToOverlay` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042dd10` | `GI_LockBackbuf` | `GI_LockBackbuf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042ddd0` | `GI_UnlockBackbuf` | `GI_UnlockBackbuf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042de70` | `GI_LockFrontbuf` | `GI_LockFrontbuf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042df20` | `GI_UnlockFrontbuf` | `GI_UnlockFrontbuf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042dfc0` | `GI_LockDevSurf` | `GI_LockDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e070` | `GI_UnlockDevSurf` | `GI_UnlockDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e110` | `GI_LockOverlaySurf` | `GI_LockOverlaySurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e1b0` | `GI_GetOverlaySurf` | `GI_GetOverlaySurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e240` | `GI_UnlockOverlaySurf` | `GI_UnlockOverlaySurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e2e0` | `GI_UnlockCurrentSurf` | `GI_UnlockCurrentSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e380` | `GI_GetBackbufDC` | `GI_GetBackbufDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e420` | `GI_ReleaseBackbufDC` | `GI_ReleaseBackbufDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e4c0` | `GI_GetFrontbufDC` | `GI_GetFrontbufDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e560` | `GI_ReleaseFrontbufDC` | `GI_ReleaseFrontbufDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e600` | `GI_GetBackbufSurf` | `GI_GetBackbufSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e690` | `GI_ClearSeenSurf` | `GI_ClearSeenSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e840` | `GI_GetDevSurfDC` | `GI_GetDevSurfDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e8d0` | `GI_ClearDevSurf` | `GI_ClearDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042e9e0` | `GI_ReleaseDevSurfDC` | `GI_ReleaseDevSurfDC` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042ea80` | `GI_EnableDevSurf` | `GI_EnableDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042eb10` | `GI_DisableDevSurf` | `GI_DisableDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042eba0` | `GI_ToggleDevSurf` | `GI_ToggleDevSurf` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042ec30` | `GI_LockActiveSurf_v6` | `GI_LockActiveSurf_v6` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042ee10` | `GI_LockActiveSurf_v7` | `GI_LockActiveSurf_v7` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042efe0` | `GI_WriteStatistics` | `GI_WriteStatistics` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042f530` | `GI_IsPowerOfTwo` | `GI_IsPowerOfTwo` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042f610` | `GI_PercentOfWidth` | `GI_PercentOfWidth` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042f6a0` | `GI_BlitWithRoomOffset` | `GI_BlitWithRoomOffset` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042f860` | `GI_LockActiveSurf_v8` | `GI_LockActiveSurf_v8` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042fa30` | `GI_LockActiveSurf_v9` | `GI_LockActiveSurf_v9` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042fbe0` | `GI_LockActiveSurf_v10` | `GI_LockActiveSurf_v10` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042fd50` | `GI_FindNearestPalColor` | `GI_FindNearestPalColor` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042fee0` | `GI_BuildColorRemapTable` | `GI_BuildColorRemapTable` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x0042ffe0` | `GI_FindFarthestSnapshotColor` | `GI_FindFarthestSnapshotColor` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430180` | `GI_BuildSnapshotPalMap` | `GI_BuildSnapshotPalMap` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430300` | `GI_SetBufPixel` | `GI_SetBufPixel` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x004303a0` | `GI_ReadMemPixel` | `GI_ReadMemPixel` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430440` | `GI_DrawLine` | `GI_DrawLine` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x004306a0` | `GI_FloodFill` | `GI_FloodFill` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x004308c0` | `GI_FillHorizontalRow` | `GI_FillHorizontalRow` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430d20` | `GI_ApplyGeneralPalToTarget` | `GI_ApplyGeneralPalToTarget` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430e40` | `GI_LoadGeneralPal` | `GI_LoadGeneralPal` | GI.cpp | ☑ | `src/GI.cpp` |
| `0x00430f00` | `GV_UpdateButtons` | `GV_UpdateButtons` | Graninv.cpp | ☑ | `src/GI.cpp` |

### Graninv.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00431210` | `GV_AddButton` | `GV_AddButton` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00431370` | `GV_SetInitHandler` | `GV_SetInitHandler` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00431400` | `GV_SetDestroyHandler` | `GV_SetDestroyHandler` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00431490` | `GV_UpdateInventory` | `GV_UpdateInventory` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00431890` | `GV_ListViewWndProc` | `GV_ListViewWndProc` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00431980` | `GV_MainWndProc` | `GV_MainWndProc` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432340` | `GV_ResizeGI` | `GV_ResizeGI` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004323f0` | `GV_CloseGI` | `GV_CloseGI` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432480` | `GV_CloseWindow` | `GV_CloseWindow` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432530` | `GV_InitWindow` | `GV_InitWindow` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432750` | `GV_CloseInventory` | `GV_CloseInventory` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432820` | `GV_RedrawInventory` | `GV_RedrawInventory` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432860` | `GV_OpenInventory` | `GV_OpenInventory` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432980` | `GV_SetEnabled` | `GV_SetEnabled` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432990` | `GV_CanDrop` | `GV_CanDrop` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432a90` | `GV_HideAndClean` | `GV_HideAndClean` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432b30` | `GV_TickInventory` | `GV_TickInventory` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432bb0` | `GV_LoadDragGraphics` | `GV_LoadDragGraphics` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00432f00` | `GV_RotateBitmap` | `GV_RotateBitmap` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00433370` | `GV_DragUpdate` | `GV_DragUpdate` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434160` | `Gran_SetCapture` | `Gran_SetCapture` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434180` | `Gran_ReleaseCapture` | `Gran_ReleaseCapture` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434190` | `Gran_GetAngleDist` | `Gran_GetAngleDist` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434640` | `Gran_LoadItem` | `Gran_LoadItem` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434700` | `Gran_DrawItem` | `Gran_DrawItem` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434800` | `Gran_ResetCube` | `Gran_ResetCube` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434820` | `Gran_ShowCube` | `Gran_ShowCube` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434b10` | `Gran_FaceToIdx` | `Gran_FaceToIdx` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434c50` | `Gran_FreeCube` | `Gran_FreeCube` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434d00` | `Gran_ClearAnims` | `Gran_ClearAnims` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434dd0` | `Gran_SetAnim` | `Gran_SetAnim` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00434ec0` | `Gran_PlayAnim` | `Gran_PlayAnim` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435020` | `Gran_StartAnim` | `Gran_StartAnim` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435180` | `Gran_SetAnimHandle` | `Gran_SetAnimHandle` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435210` | `Gran_SetTrigger` | `Gran_SetTrigger` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004352a0` | `Gran_CheckAnimDone` | `Gran_CheckAnimDone` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435750` | `Gran_SetMovHandle` | `Gran_SetMovHandle` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004357e0` | `Gran_EndWait` | `Gran_EndWait` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435870` | `Gran_StartWait` | `Gran_StartWait` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435920` | `Gran_ConvPal` | `Gran_ConvPal` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00435ff0` | `Gran_BlitToScreen` | `Gran_BlitToScreen` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00436280` | `Gran_Dissolve` | `Gran_Dissolve` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00436810` | `Gran_InitTape` | `Gran_InitTape` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00436da0` | `Gran_TapeCommand` | `Gran_TapeCommand` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00436ef0` | `Gran_TapeFF` | `Gran_TapeFF` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004370c0` | `Gran_TapeRew` | `Gran_TapeRew` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437290` | `Gran_DiaryPlay` | `Gran_DiaryPlay` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437380` | `Gran_SetTapeState` | `Gran_SetTapeState` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437410` | `Gran_LoadTapeData` | `Gran_LoadTapeData` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004374d0` | `Gran_SaveTapeData` | `Gran_SaveTapeData` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437590` | `Gran_InitBoard` | `Gran_InitBoard` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004376e0` | `Gran_UpdateBoard` | `Gran_UpdateBoard` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437cd0` | `Gran_GetPosParity` | `Gran_GetPosParity` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437db0` | `Gran_RestoreBoardRow` | `Gran_RestoreBoardRow` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00437f00` | `Gran_AdvancePiece` | `Gran_AdvancePiece` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438070` | `Gran_MoveAlien` | `Gran_MoveAlien` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004383b0` | `Gran_CalcBoardMove` | `Gran_CalcBoardMove` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004387a0` | `Gran_UpdateGrannyPos` | `Gran_UpdateGrannyPos` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438870` | `Gran_InitSlider` | `Gran_InitSlider` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x004389f0` | `Gran_SetSliderRange` | `Gran_SetSliderRange` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438af0` | `Gran_UpdateSlider` | `Gran_UpdateSlider` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438c70` | `Gran_EndSlider` | `Gran_EndSlider` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438d00` | `Gran_StopSlider` | `Gran_StopSlider` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438d90` | `Gran_InitHelpQueue` | `Gran_InitHelpQueue` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438e50` | `Gran_RemoveHelp` | `Gran_RemoveHelp` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00438f90` | `Gran_ShiftHelp` | `Gran_ShiftHelp` | Graninv.cpp | ☑ | `src/Graninv.cpp` |
| `0x00439050` | `Gran_AddHelp` | `Gran_AddHelp` | Graninv.cpp | ☑ | `src/Graninv.cpp` |

### HELPSTK.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004391b0` | `Help_SaveHelpStack` | `Help_SaveHelpStack` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x004393a0` | `Help_LoadHelpStack` | `Help_LoadHelpStack` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439590` | `Help_PutLineScaled` | `Help_PutLineScaled` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x004398e0` | `Help_LineCopyRun` | `Help_LineCopyRun` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439980` | `Help_LineDecodeRLE` | `Help_LineDecodeRLE` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439ad0` | `Help_LineSkipRLE` | `Help_LineSkipRLE` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439bd0` | `Help_LineDecodeRLEOffset` | `Help_LineDecodeRLEOffset` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439d80` | `Help_BlitImage` | `Help_BlitImage` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |
| `0x00439f70` | `Help_PutLine` | `Help_PutLine` | HELPSTK.cpp | ☑ | `src/HELPSTK.cpp` |

### Img.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0043a1b0` | `BlitImg_ScaledPct` | `BlitImg_ScaledPct` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043a4f0` | `SkipLine` | `SkipLine` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043a6a0` | `BlitImg_Scaled` | `BlitImg_Scaled` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043a9c0` | `BlitImg_Indi` | `BlitImg_Indi` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043abc0` | `PutLine_Indi` | `PutLine_Indi` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043ada0` | `PutLine_Direct` | `PutLine_Direct` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043ae70` | `PutLine_RLE` | `PutLine_RLE` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043afd0` | `PutLine_RLE_Skip` | `PutLine_RLE_Skip` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b0e0` | `PutLine_RLE_Offset` | `PutLine_RLE_Offset` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b2a0` | `BlitImg_Blank` | `BlitImg_Blank` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b4d0` | `PutLine_Blank` | `PutLine_Blank` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b6a0` | `PutLine_Blank_RLE` | `PutLine_Blank_RLE` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b7c0` | `PutLine_Blank_RLE_Offset` | `PutLine_Blank_RLE_Offset` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043b9a0` | `PackRegion` | `PackRegion` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043bb70` | `PackLine` | `PackLine` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043bca0` | `InitImg` | `InitImg` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043be40` | `BlitImg_SizeScaled` | `BlitImg_SizeScaled` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043c300` | `PutLine_SizeScaled` | `PutLine_SizeScaled` | Img.cpp | ☑ | `src/Img.cpp` |
| `0x0043c7c0` | `InitInvMang` | `InitInvMang` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |

### INVMANG.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0043c910` | `Inv_FreeSlot` | `Inv_FreeSlot` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |
| `0x0043c9d0` | `Inv_GetResource` | `Inv_GetResource` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |
| `0x0043d040` | `Inv_AllocSlot` | `Inv_AllocSlot` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |
| `0x0043d4e0` | `Inv_GetByTag` | `Inv_GetByTag` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |
| `0x0043d5b0` | `Inv_GetByName` | `Inv_GetByName` | INVMANG.cpp | ☑ | `src/INVMANG.cpp` |
| `0x0043d690` | `Magwrit_InitPenNet` | `Magwrit_InitPenNet` | magwrit.cpp | ☑ | `src/INVMANG.cpp` |

### magwrit.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0043d760` | `Magwrit_CleanExit` | `Magwrit_CleanExit` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043d7c0` | `Magwrit_TabletInit` | `Magwrit_TabletInit` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043db90` | `Magwrit_LoadKwtDll` | `Magwrit_LoadKwtDll` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e100` | `Magwrit_PostInitCallback` | `Magwrit_PostInitCallback` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e120` | `Magwrit_BeginPage` | `Magwrit_BeginPage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e1d0` | `Magwrit_AddTask` | `Magwrit_AddTask` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e3c0` | `Magwrit_EndTask` | `Magwrit_EndTask` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e4d0` | `Magwrit_DetachTask` | `Magwrit_DetachTask` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e590` | `Magwrit_DequeueMessage` | `Magwrit_DequeueMessage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e610` | `Magwrit_AddButton` | `Magwrit_AddButton` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e7f0` | `Magwrit_DetachButton` | `Magwrit_DetachButton` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e860` | `Magwrit_DetachAllButtons` | `Magwrit_DetachAllButtons` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e880` | `Magwrit_OnButtonClicked` | `Magwrit_OnButtonClicked` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043e930` | `Magwrit_DrawInkSegment` | `Magwrit_DrawInkSegment` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043ea40` | `Magwrit_EnqueueTaskMessage` | `Magwrit_EnqueueTaskMessage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043eb70` | `Magwrit_OnTaskProcMessage` | `Magwrit_OnTaskProcMessage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043ee50` | `Magwrit_OnTaskManagerMessage` | `Magwrit_OnTaskManagerMessage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043ef10` | `Magwrit_DispatchPenNetMessage` | `Magwrit_DispatchPenNetMessage` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f030` | `Magwrit_ShowStatusText` | `Magwrit_ShowStatusText` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f0b0` | `Magwrit_SetTaskRangeLow` | `Magwrit_SetTaskRangeLow` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f110` | `Magwrit_SetTaskRangeHigh` | `Magwrit_SetTaskRangeHigh` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f170` | `Magwrit_GetTaskPointCount` | `Magwrit_GetTaskPointCount` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f1d0` | `Magwrit_PopTaskPoint` | `Magwrit_PopTaskPoint` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f280` | `Magwrit_DrawTaskInk` | `Magwrit_DrawTaskInk` | magwrit.cpp | ☑ | `src/magwrit.cpp` |
| `0x0043f550` | `Mem_InitFromIni` | `Mem_InitFromIni` | Memalloc.cpp | ☑ | `src/Memalloc.cpp` |

### Memalloc.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0043fa70` | `Mem_InitPools` | `Mem_InitPools` | Memalloc.cpp | ☑ | `src/Memalloc.cpp` |
| `0x0043fb00` | `Mem_PickPoolLayout` | `Mem_PickPoolLayout` | Memalloc.cpp | ☑ | `src/Memalloc.cpp` |
| `0x0043fd10` | `Mem_PartitionPool` | `Mem_PartitionPool` | Memalloc.cpp | ☑ | `src/Memalloc.cpp` |
| `0x00440050` | `Mixer_GetChannelType` | `Mixer_GetChannelType` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004400e0` | `Mixer_ThreadProc` | `Mixer_ThreadProc` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00440b60` | `Mixer_SkipBytes` | `Mixer_SkipBytes` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00440cf0` | `Mixer_UpdateTimers` | `Mixer_UpdateTimers` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00440da0` | `Mixer_Kick` | `Mixer_Kick` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00440ee0` | `Mixer_InitTables` | `Mixer_InitTables` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00441040` | `Mixer_WakeThread` | `Mixer_WakeThread` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004410d0` | `Mixer_Reinit` | `Mixer_Reinit` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00441690` | `Mixer_Init` | `Mixer_Init` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00442430` | `Mixer_Stop` | `Mixer_Stop` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004425a0` | `Mixer_ChannelDone` | `Mixer_ChannelDone` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004427e0` | `Mixer_PlayChannel` | `Mixer_PlayChannel` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00442f40` | `Mixer_SetChannelDoneCallback` | `Mixer_SetChannelDoneCallback` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00442fd0` | `Mixer_ClearChannelDoneCallback` | `Mixer_ClearChannelDoneCallback` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443060` | `Mixer_SetVolume` | `Mixer_SetVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004433b0` | `Mixer_SetVolumeOnly` | `Mixer_SetVolumeOnly` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443510` | `Mixer_GetChannelVolume` | `Mixer_GetChannelVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004435a0` | `Mixer_DuckVolume` | `Mixer_DuckVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004436e0` | `Mixer_RestoreVolume` | `Mixer_RestoreVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443820` | `Mixer_RemoveChannel` | `Mixer_RemoveChannel` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443970` | `Mixer_AddChannel` | `Mixer_AddChannel` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443b80` | `Mixer_SetChannelDucking` | `Mixer_SetChannelDucking` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443c20` | `Mixer_ClearChannelDucking` | `Mixer_ClearChannelDucking` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443cc0` | `Mixer_MuteChannel` | `Mixer_MuteChannel` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443d50` | `Mixer_IsChannelActive` | `Mixer_IsChannelActive` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443df0` | `Mixer_StopChannel` | `Mixer_StopChannel` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443ee0` | `Mixer_GetMasterVolume` | `Mixer_GetMasterVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00443fe0` | `Mixer_SetMasterVolume` | `Mixer_SetMasterVolume` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004440c0` | `Mixer_DsErrToStr` | `Mixer_DsErrToStr` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00444350` | `Mixer_SelectFillFunc` | `Mixer_SelectFillFunc` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004444e0` | `Mixer_FillBuf_S16_Stereo_StereoA` | `Mixer_FillBuf_S16_Stereo_StereoA` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00445b50` | `Mixer_FillBuf_S16_Stereo_StereoB` | `Mixer_FillBuf_S16_Stereo_StereoB` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00447140` | `Mixer_FillBuf_S16_Stereo_Mono44k` | `Mixer_FillBuf_S16_Stereo_Mono44k` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00447c80` | `Mixer_FillBuf_U8_Stereo_StereoB44k` | `Mixer_FillBuf_U8_Stereo_StereoB44k` | MIXER.cpp | ☑ | `src/MIXER.cpp` |

### MIXER.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00449450` | `Mixer_FillBuf_U8_Stereo_Mono` | `Mixer_FillBuf_U8_Stereo_Mono` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044ac50` | `Mixer_FillBuf_U8_Mono_Mono` | `Mixer_FillBuf_U8_Mono_Mono` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044b760` | `Mixer_FillBuf_S16_Stereo_Stereo` | `Mixer_FillBuf_S16_Stereo_Stereo` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044cb80` | `Mixer_FillBuf_S16_Stereo_StereoB` | `Mixer_FillBuf_S16_Stereo_StereoB` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044deb0` | `Mixer_FillBuf_S16_Stereo_Mono` | `Mixer_FillBuf_S16_Stereo_Mono` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044e8e0` | `Mixer_FillBuf_U8_Stereo_StereoA` | `Mixer_FillBuf_U8_Stereo_StereoA` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x0044fe50` | `Mixer_FillBuf_U8_Stereo_StereoB` | `Mixer_FillBuf_U8_Stereo_StereoB` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x004513f0` | `Mixer_FillBuf_U8_Mono_StereoMix` | `Mixer_FillBuf_U8_Mono_StereoMix` | MIXER.cpp | ☑ | `src/MIXER.cpp` |
| `0x00451e90` | `Curs_CancelDblClickTimer` | `Curs_CancelDblClickTimer` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00451f90` | `Curs_SetMouseOrigin` | `Curs_SetMouseOrigin` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452030` | `Curs_HandleMouseMsg` | `Curs_HandleMouseMsg` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452440` | `Curs_InitMouse` | `Curs_InitMouse` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452520` | `Curs_CloseMouseEvents` | `Curs_CloseMouseEvents` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x004525c0` | `Curs_Nop1` | `Curs_Nop1` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452650` | `Curs_Nop2` | `Curs_Nop2` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x004526e0` | `Curs_Nop3` | `Curs_Nop3` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452770` | `Curs_GetMouseState` | `Curs_GetMouseState` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452830` | `Curs_Nop4` | `Curs_Nop4` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x004528c0` | `Curs_Nop5` | `Curs_Nop5` | CURSORS.cpp | ☑ | `src/CURSORS.cpp` |
| `0x00452950` | `Mov_BuildDistLUT` | `Mov_BuildDistLUT` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00452a30` | `Mov_SelectZone` | `Mov_SelectZone` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00452b50` | `Mov_LockSurfaceAt` | `Mov_LockSurfaceAt` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00452c00` | `Mov_FindNearestNode` | `Mov_FindNearestNode` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00452d30` | `Mov_FindNearestNodeInBox` | `Mov_FindNearestNodeInBox` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00452ea0` | `Mov_FindNeighborByDir` | `Mov_FindNeighborByDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00453130` | `Mov_CompareEdgeDir` | `Mov_CompareEdgeDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00453320` | `Mov_PathfindTo` | `Mov_PathfindTo` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |

### MOVEMENT.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004545d0` | `Mov_Update` | `Mov_Update` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004556c0` | `Mov_StartPath` | `Mov_StartPath` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455760` | `Mov_Reset` | `Mov_Reset` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455800` | `Mov_FreezePos` | `Mov_FreezePos` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004558a0` | `Mov_RestorePos` | `Mov_RestorePos` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455940` | `Mov_SetFollower` | `Mov_SetFollower` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004559f0` | `Mov_IsMoveDone` | `Mov_IsMoveDone` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455ac0` | `Mov_TracePos` | `Mov_TracePos` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455b90` | `Mov_EnableSounds` | `Mov_EnableSounds` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455c20` | `Mov_DisableSounds` | `Mov_DisableSounds` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455cb0` | `Mov_SetDir` | `Mov_SetDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455da0` | `Mov_ClearDir` | `Mov_ClearDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455e30` | `Mov_GetPathHead` | `Mov_GetPathHead` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455ee0` | `Mov_WalkTo` | `Mov_WalkTo` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00455f70` | `Mov_InitChar` | `Mov_InitChar` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004560a0` | `Mov_FlipDir` | `Mov_FlipDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456180` | `Mov_WrapDir` | `Mov_WrapDir` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456230` | `Mov_AddDirSuffix` | `Mov_AddDirSuffix` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456410` | `Mov_RestoreMoves` | `Mov_RestoreMoves` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456bc0` | `Mov_FindPath` | `Mov_FindPath` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456dd0` | `Mov_FreeMoves` | `Mov_FreeMoves` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00456f80` | `Mov_SelectCarryAnim` | `Mov_SelectCarryAnim` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x00457070` | `Mov_TurnAround` | `Mov_TurnAround` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004572b0` | `Mov_TurnOnTheFly` | `Mov_TurnOnTheFly` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004574d0` | `Mov_FacePoint` | `Mov_FacePoint` | MOVEMENT.cpp | ☑ | `src/MOVEMENT.cpp` |
| `0x004576a0` | `OTF_AllocSlot` | `OTF_AllocSlot` | ONTHEFLY.cpp | ☑ | `src/MOVEMENT.cpp` |

### ONTHEFLY.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004578e0` | `OTF_AllocNodeList` | `OTF_AllocNodeList` | ONTHEFLY.cpp | ☑ | `src/ONTHEFLY.cpp` |
| `0x004579d0` | `OTF_AreaTip` | `OTF_AreaTip` | ONTHEFLY.cpp | ☑ | `src/ONTHEFLY.cpp` |
| `0x00457e60` | `Player_SetPalFreezeMode` | `Player_SetPalFreezeMode` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00457f00` | `Player_SetPalCallback` | `Player_SetPalCallback` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00457f90` | `Player_BuildPalLUT` | `Player_BuildPalLUT` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x004580a0` | `Player_FlushPalAndBlit` | `Player_FlushPalAndBlit` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458170` | `Player_SetSharedMode` | `Player_SetSharedMode` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458200` | `Player_SetAsyncReadMode` | `Player_SetAsyncReadMode` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458290` | `Player_ScmAddChar` | `Player_ScmAddChar` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458470` | `Player_ScmInit` | `Player_ScmInit` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458550` | `Player_SetSpeakingChar` | `Player_SetSpeakingChar` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x004586c0` | `Player_InitEvents` | `Player_InitEvents` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x004587e0` | `Player_WriteDebugChunk` | `Player_WriteDebugChunk` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458900` | `Player_PlayScm` | `Player_PlayScm` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x004589f0` | `Player_SetFlags` | `Player_SetFlags` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458a80` | `Player_ClearFlags` | `Player_ClearFlags` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458b10` | `Player_ResetFlags` | `Player_ResetFlags` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x00458bb0` | `Player_ScmPlayList` | `Player_ScmPlayList` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045ace0` | `Player_GetNextScmName` | `Player_GetNextScmName` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045ade0` | `Player_StreamThreadProc` | `Player_StreamThreadProc` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045b260` | `Player_RenderFrame` | `Player_RenderFrame` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045c610` | `Player_StartMusicLoop` | `Player_StartMusicLoop` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |

### PLAYER.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0045c7d0` | `Player_RemapPalette` | `Player_RemapPalette` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045c860` | `Player_StreamSyncCallback` | `Player_StreamSyncCallback` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045c960` | `Player_FlushVoices` | `Player_FlushVoices` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045ca80` | `Player_Init` | `Player_Init` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045cca0` | `Player_WriteData` | `Player_WriteData` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045ccf0` | `Player_ReadData` | `Player_ReadData` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045cd60` | `Player_BunchRead` | `Player_BunchRead` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045cf30` | `Player_IsAbortPressed` | `Player_IsAbortPressed` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045d010` | `Player_StartStream` | `Player_StartStream` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045d0b0` | `Player_Shutdown` | `Player_Shutdown` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045d220` | `Player_SetCoverSprite` | `Player_SetCoverSprite` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045d3c0` | `Player_DrawCoverSprite` | `Player_DrawCoverSprite` | PLAYER.cpp | ☑ | `src/PLAYER.cpp` |
| `0x0045d4e0` | `Res_FindByNumChar` | `Res_FindByNumChar` | READRES.cpp | ☑ | `src/READRES.cpp` |

### READRES.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0045d810` | `Res_GetDirectByNumChar` | `Res_GetDirectByNumChar` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045d970` | `Res_LockEntry` | `Res_LockEntry` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045da20` | `Res_UnlockEntry` | `Res_UnlockEntry` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045dac0` | `Res_IsWithinBudget` | `Res_IsWithinBudget` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045db60` | `Res_BunchFreadLoadPtr` | `Res_BunchFreadLoadPtr` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045de60` | `Res_BunchCompactQueue` | `Res_BunchCompactQueue` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045df60` | `Res_BunchResortTasks` | `Res_BunchResortTasks` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e0f0` | `Res_BunchFreadStreamLoadPtr` | `Res_BunchFreadStreamLoadPtr` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e4c0` | `Res_TickFrameCounter` | `Res_TickFrameCounter` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e550` | `Res_BunchFreadNow` | `Res_BunchFreadNow` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e6f0` | `Res_WaitForEntry` | `Res_WaitForEntry` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e7b0` | `Res_AdvanceFilePtr` | `Res_AdvanceFilePtr` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e880` | `Res_CancelFrameTasks` | `Res_CancelFrameTasks` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045e970` | `Res_RetargetFrameTasks` | `Res_RetargetFrameTasks` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045ea60` | `Res_AcquireFileLock` | `Res_AcquireFileLock` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045eaf0` | `Res_ReleaseFileLock` | `Res_ReleaseFileLock` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045eb80` | `Res_GetCurrentDiskNum` | `Res_GetCurrentDiskNum` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045ec10` | `Res_BunchOpen` | `Res_BunchOpen` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045f200` | `Res_BunchInit` | `Res_BunchInit` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045f470` | `Res_BunchReadingThread` | `Res_BunchReadingThread` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045f6f0` | `Res_MmioSeekRead` | `Res_MmioSeekRead` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045f800` | `Res_BunchSortTasks` | `Res_BunchSortTasks` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045fa80` | `Res_TestDiskSpeeds` | `Res_TestDiskSpeeds` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045fd10` | `Res_BunchIdDisk` | `Res_BunchIdDisk` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x0045fef0` | `Res_BunchInitDisk` | `Res_BunchInitDisk` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460190` | `Res_BunchShutdown` | `Res_BunchShutdown` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x004602c0` | `Res_AssertFramePos` | `Res_AssertFramePos` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460360` | `Res_BunchExtract` | `Res_BunchExtract` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460630` | `Res_BunchClose` | `Res_BunchClose` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460770` | `Res_BunchSelectDisk` | `Res_BunchSelectDisk` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460a20` | `Res_WaitQueueEmpty` | `Res_WaitQueueEmpty` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460af0` | `Res_IsRoomOnCurrentDisk` | `Res_IsRoomOnCurrentDisk` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00460bd0` | `Res_BunchReplaceDisk` | `Res_BunchReplaceDisk` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00461210` | `Res_GetTransferRate` | `Res_GetTransferRate` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x004612a0` | `Rescale_CalcZoomTable` | `Rescale_CalcZoomTable` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x004614a0` | `Rescale_CalcForBike` | `Rescale_CalcForBike` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00461720` | `Rescale_CalcForRoom` | `Rescale_CalcForRoom` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00461950` | `Rescale_Reset` | `Rescale_Reset` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x004619d0` | `Rescale_DrawScaledSprite` | `Rescale_DrawScaledSprite` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00461aa0` | `Rescale_GetCount` | `Rescale_GetCount` | READRES.cpp | ☑ | `src/READRES.cpp` |
| `0x00461b30` | `Rescale_StartBike` | `Rescale_StartBike` | READRES.cpp | ☑ | `src/READRES.cpp` |

### RESCALE.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00461c10` | `Rescale_DrawBikeScroll` | `Rescale_DrawBikeScroll` | RESCALE.cpp | ☑ | `src/RESCALE.cpp` |
| `0x00461d60` | `Rescale_DrawByIndexChecked` | `Rescale_DrawByIndexChecked` | RESCALE.cpp | ☑ | `src/RESCALE.cpp` |
| `0x00461ea0` | `Runprog_LoadEntryNames` | `Runprog_LoadEntryNames` | RUNPROG.cpp | ☑ | `src/RESCALE.cpp` |

### RUNPROG.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00462290` | `RunProg_WaitMoveDone` | `RunProg_WaitMoveDone` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x00462380` | `RunProg_SelectAreaContext` | `RunProg_SelectAreaContext` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x00462560` | `RunProg_Exec` | `RunProg_Exec` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046b9b0` | `RunProg_PlayScmWithPaletteGuard` | `RunProg_PlayScmWithPaletteGuard` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046baa0` | `RunProg_ClearTrackedSounds` | `RunProg_ClearTrackedSounds` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046bac0` | `RunProg_StopAndClearTrackedSounds` | `RunProg_StopAndClearTrackedSounds` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046bb20` | `RunProg_TrackSound` | `RunProg_TrackSound` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046bba0` | `RunProg_RestorePaletteSnapshot` | `RunProg_RestorePaletteSnapshot` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046bc40` | `RunProg_Nop` | `RunProg_Nop` | RUNPROG.cpp | ☑ | `src/RUNPROG.cpp` |
| `0x0046bcc0` | `SafeHeap_Alloc` | `SafeHeap_Alloc` | SAFEHEAP.cpp | ☑ | `src/SAFEHEAP.cpp` |
| `0x0046bd80` | `SafeHeap_Free` | `SafeHeap_Free` | SAFEHEAP.cpp | ☑ | `src/SAFEHEAP.cpp` |

### SAFEHEAP.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0046bf40` | `Sched_SetAllNormal` | `Sched_SetAllNormal` | SCHED.cpp | ☑ | `src/SCHED.cpp` |

### SCHED.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0046c120` | `Sched_BeginHighPriority` | `Sched_BeginHighPriority` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c1c0` | `Sched_EndHighPriority` | `Sched_EndHighPriority` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c290` | `Sched_SetNormalPriority` | `Sched_SetNormalPriority` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c320` | `Sched_SetAboveNormalPriority` | `Sched_SetAboveNormalPriority` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c3c0` | `Sched_Stub1` | `Sched_Stub1` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c440` | `Sched_Stub2` | `Sched_Stub2` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c4c0` | `Sched_GetPalEntryRaw` | `Sched_GetPalEntryRaw` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c5a0` | `Sched_SetGamma` | `Sched_SetGamma` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c630` | `Sched_GetGamma` | `Sched_GetGamma` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c6c0` | `Sched_SetBorderMode` | `Sched_SetBorderMode` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c750` | `Sched_ComparePalettes` | `Sched_ComparePalettes` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c820` | `Sched_FillPalBorders` | `Sched_FillPalBorders` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046c920` | `Sched_UpdatePalette` | `Sched_UpdatePalette` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046cb30` | `Sched_SetActivePalette` | `Sched_SetActivePalette` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046cc20` | `Sched_GetPalColor` | `Sched_GetPalColor` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046ccf0` | `Sched_SetPalColor` | `Sched_SetPalColor` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046ce10` | `Sched_SavePaletteSnapshot` | `Sched_SavePaletteSnapshot` | SCHED.cpp | ☑ | `src/SCHED.cpp` |
| `0x0046cf10` | `SetPal_PreChange` | `SetPal_PreChange` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d050` | `SetPal_SetCallbackEnabled` | `SetPal_SetCallbackEnabled` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d200` | `SetPal_FadeOut` | `SetPal_FadeOut` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d310` | `SetPal_QuickFadeToBlack` | `SetPal_QuickFadeToBlack` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d3a0` | `SetPal_SmoothFadeToBlack` | `SetPal_SmoothFadeToBlack` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d540` | `SetPal_FadeToTarget` | `SetPal_FadeToTarget` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d720` | `SetPal_FadeInFromBlack` | `SetPal_FadeInFromBlack` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046d900` | `SetPal_FadeInSnapshot` | `SetPal_FadeInSnapshot` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046db50` | `SetPal_RestoreSysColors` | `SetPal_RestoreSysColors` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046dd10` | `SetPal_ClearSysColors` | `SetPal_ClearSysColors` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046deb0` | `SetPal_Init` | `SetPal_Init` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046df60` | `SetPal_FillLogPalette` | `SetPal_FillLogPalette` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e040` | `SetPal_FindNearestColor` | `SetPal_FindNearestColor` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e210` | `SetPal_GetSysColorRef` | `SetPal_GetSysColorRef` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e3d0` | `SetPal_ApplyGamma` | `SetPal_ApplyGamma` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e490` | `SetPal_SetPalette` | `SetPal_SetPalette` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |

### SETPAL.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0046e6d0` | `SetPal_RestoreSysColorsRaw` | `SetPal_RestoreSysColorsRaw` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e780` | `SetPal_SetSyspalNostatic` | `SetPal_SetSyspalNostatic` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e810` | `SetPal_RestoreAndReset` | `SetPal_RestoreAndReset` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046e9a0` | `SetPal_WaitOrRealizeIfNeeded` | `SetPal_WaitOrRealizeIfNeeded` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046ea40` | `SetPal_RealizePalette` | `SetPal_RealizePalette` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046ebc0` | `SetPal_RemapSysColorTextRefs` | `SetPal_RemapSysColorTextRefs` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046edf0` | `SetPal_SetBlack` | `SetPal_SetBlack` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f060` | `SetPal_DestroyBlankWindow` | `SetPal_DestroyBlankWindow` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f1f0` | `SetPal_CreateBlankWindow` | `SetPal_CreateBlankWindow` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f3c0` | `Snd_SetSpeechVol` | `Snd_SetSpeechVol` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f450` | `Snd_GetSpeechVol` | `Snd_GetSpeechVol` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f4e0` | `Snd_SetSfxVol` | `Snd_SetSfxVol` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f570` | `Sound_Init` | `Sound_Init` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f730` | `Snd_PlayPanned` | `Snd_PlayPanned` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f7f0` | `Snd_PlayCentered` | `Snd_PlayCentered` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f890` | `Snd_PlayFull` | `Snd_PlayFull` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f940` | `Snd_PlayFull2` | `Snd_PlayFull2` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046f9e0` | `Snd_Nop1` | `Snd_Nop1` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fa60` | `Snd_Stop` | `Snd_Stop` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046faf0` | `Snd_GetSfxVol` | `Snd_GetSfxVol` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fb80` | `Snd_SetSfxVolClamped` | `Snd_SetSfxVolClamped` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fc40` | `Snd_IsIdle` | `Snd_IsIdle` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fcd0` | `Snd_Nop2` | `Snd_Nop2` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fd50` | `Snd_Nop3` | `Snd_Nop3` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fdd0` | `Snd_Nop4` | `Snd_Nop4` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fe50` | `Snd_Nop5` | `Snd_Nop5` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046fed0` | `Snd_Play` | `Snd_Play` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x0046ff70` | `Snd_PlayCore` | `Snd_PlayCore` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470180` | `Snd_PauseChannel` | `Snd_PauseChannel` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470210` | `Snd_ResumeChannel` | `Snd_ResumeChannel` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x004702a0` | `Snd_VolumeUp` | `Snd_VolumeUp` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470350` | `Snd_VolumeDown` | `Snd_VolumeDown` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470400` | `Snd_StopChannel` | `Snd_StopChannel` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470490` | `Snd_StopAll` | `Snd_StopAll` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470520` | `Snd_SetChannelPan` | `Snd_SetChannelPan` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x004705c0` | `Snd_GetActiveChanId` | `Snd_GetActiveChanId` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x004705e0` | `Snd_ResetChannelTable` | `Snd_ResetChannelTable` | SETPAL.cpp | ☑ | `src/SETPAL.cpp` |
| `0x00470780` | `Slider_Add` | `Slider_Add` | SLIDER.cpp | ☑ | `src/SETPAL.cpp` |

### SLIDER.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004709f0` | `Slider_Remove` | `Slider_Remove` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470b30` | `Slider_SetCurrent` | `Slider_SetCurrent` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470bc0` | `Slider_SetPosition` | `Slider_SetPosition` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470d40` | `Slider_SetValueRange` | `Slider_SetValueRange` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470e00` | `Slider_SetPixelRange` | `Slider_SetPixelRange` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470ec0` | `Slider_SetMaxStep` | `Slider_SetMaxStep` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00470f60` | `Slider_TrackClicked` | `Slider_TrackClicked` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00471820` | `Slider_Drag` | `Slider_Drag` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00471fe0` | `Slider_SetValue` | `Slider_SetValue` | SLIDER.cpp | ☑ | `src/SLIDER.cpp` |
| `0x00472340` | `SndMem_ReadSound` | `SndMem_ReadSound` | SOUNDMEM.cpp | ☑ | `src/SLIDER.cpp` |

### SOUNDMEM.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00472810` | `SndMem_Load` | `SndMem_Load` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00472ce0` | `SndMem_SetSlotState` | `SndMem_SetSlotState` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00472da0` | `SndMem_Init` | `SndMem_Init` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00472ea0` | `SndMem_AllocSlot` | `SndMem_AllocSlot` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473800` | `SndMem_Compact` | `SndMem_Compact` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473be0` | `SndMem_Reset` | `SndMem_Reset` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473cc0` | `SndMem_Free` | `SndMem_Free` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473de0` | `SndMem_UpdateAnim` | `SndMem_UpdateAnim` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473ed0` | `SndMem_SetSpeechEnabled` | `SndMem_SetSpeechEnabled` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00473ee0` | `SndMem_StartSpeech` | `SndMem_StartSpeech` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474320` | `SndMem_WaitSpeech` | `SndMem_WaitSpeech` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x004744f0` | `SndMem_IsSpeaking` | `SndMem_IsSpeaking` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474600` | `SndMem_SpeakAndWait` | `SndMem_SpeakAndWait` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x004746a0` | `SndMem_GetLipsyncByte` | `SndMem_GetLipsyncByte` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474770` | `SndMem_StopLipsync` | `SndMem_StopLipsync` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474850` | `SndMem_AdvanceLipsync` | `SndMem_AdvanceLipsync` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474940` | `SndMem_SetLipsyncPos` | `SndMem_SetLipsyncPos` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x004749f0` | `SndMem_InitLipsync` | `SndMem_InitLipsync` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |
| `0x00474aa0` | `SndMem_SetSpeechAnim` | `SndMem_SetSpeechAnim` | SOUNDMEM.cpp | ☑ | `src/SOUNDMEM.cpp` |

### SPEECH.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00474cf0` | `Speech_SetSentence` | `Speech_SetSentence` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x00474d80` | `Speech_GetSentence` | `Speech_GetSentence` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x00474e10` | `Speech_ResetPos` | `Speech_ResetPos` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x00474ea0` | `Speech_SetTag` | `Speech_SetTag` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x00474f40` | `Speech_Commit` | `Speech_Commit` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x00475030` | `Speech_Play` | `Speech_Play` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |
| `0x004750d0` | `Speech_Init` | `Speech_Init` | SPEECH.cpp | ☑ | `src/SPEECH.cpp` |

### TEXT.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00475970` | `Txt_SetRect` | `Txt_SetRect` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475a20` | `Txt_SetMaxLines` | `Txt_SetMaxLines` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475ac0` | `Txt_GetMaxLines` | `Txt_GetMaxLines` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475b50` | `Txt_GetAlign` | `Txt_GetAlign` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475be0` | `Txt_SetAlign` | `Txt_SetAlign` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475c70` | `Txt_SetMode` | `Txt_SetMode` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475d00` | `Txt_GetMode` | `Txt_GetMode` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475d90` | `Txt_CreateFont` | `Txt_CreateFont` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475e80` | `Txt_SetColor` | `Txt_SetColor` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00475f90` | `Txt_LookupString` | `Txt_LookupString` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x004761d0` | `Txt_SetString` | `Txt_SetString` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00476c20` | `Txt_ResolveDirectString` | `Txt_ResolveDirectString` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00476d90` | `Txt_IsScrollPending` | `Txt_IsScrollPending` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00476e60` | `Txt_SetMessage` | `Txt_SetMessage` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00477160` | `Txt_DrawShadow` | `Txt_DrawShadow` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x004773b0` | `Txt_Reset` | `Txt_Reset` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00477470` | `Txt_PageAdvance` | `Txt_PageAdvance` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00477620` | `Txt_Update` | `Txt_Update` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00477f00` | `Txt_DrawBackground` | `Txt_DrawBackground` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x004780c0` | `Txt_EraseBackground` | `Txt_EraseBackground` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00478240` | `Txt_IsDone` | `Txt_IsDone` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x004782f0` | `Thm_SetIndex` | `Thm_SetIndex` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00478380` | `Thm_PlayNextSegment` | `Thm_PlayNextSegment` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00478580` | `Thm_Play` | `Thm_Play` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00478a70` | `Thm_FindLabel` | `Thm_FindLabel` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00478b70` | `Thm_LoadTheme` | `Thm_LoadTheme` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00479430` | `Thm_ReadTable` | `Thm_ReadTable` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00479680` | `Thm_FreeData` | `Thm_FreeData` | TEXT.cpp | ☑ | `src/TEXT.cpp` |
| `0x00479810` | `Thm_FreeStringTable` | `Thm_FreeStringTable` | TEXT.cpp | ☑ | `src/TEXT.cpp` |

### THEMES.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004798e0` | `Theme_StopMusic` | `Theme_StopMusic` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x004799d0` | `Theme_StopMusicAndFree` | `Theme_StopMusicAndFree` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x00479ae0` | `Theme_PauseMusic` | `Theme_PauseMusic` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x00479b80` | `Theme_ResumeMusic` | `Theme_ResumeMusic` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x00479c20` | `Theme_MusicEvent` | `Theme_MusicEvent` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047a1c0` | `Theme_Init` | `Theme_Init` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047a350` | `Theme_ThreadProc` | `Theme_ThreadProc` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047a5a0` | `Theme_PrepNextSeg` | `Theme_PrepNextSeg` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047aa80` | `Theme_LoadSegs` | `Theme_LoadSegs` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047ad30` | `Theme_SegToMem` | `Theme_SegToMem` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047b410` | `Theme_FindFreeMemSlot` | `Theme_FindFreeMemSlot` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047b4e0` | `Theme_GetMemBlock` | `Theme_GetMemBlock` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047b730` | `Theme_PushSegOp` | `Theme_PushSegOp` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047bdc0` | `Theme_Shutdown` | `Theme_Shutdown` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047bef0` | `Theme_Nop1` | `Theme_Nop1` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047bf70` | `Theme_Nop2` | `Theme_Nop2` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047bff0` | `Theme_SetVolume` | `Theme_SetVolume` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c0a0` | `Theme_GetVolume` | `Theme_GetVolume` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c130` | `Theme_FadeOutHandler` | `Theme_FadeOutHandler` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c280` | `Theme_StartFadeOut` | `Theme_StartFadeOut` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c430` | `Theme_FadeOut` | `Theme_FadeOut` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c510` | `Theme_IsFading` | `Theme_IsFading` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c5b0` | `Theme_FadeIn` | `Theme_FadeIn` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c690` | `Theme_SetVolumeQuarter` | `Theme_SetVolumeQuarter` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c740` | `Theme_SetVolumeFull` | `Theme_SetVolumeFull` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c7e0` | `Theme_LockMemPool` | `Theme_LockMemPool` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c880` | `Theme_UnlockMemPool` | `Theme_UnlockMemPool` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047c920` | `Theme_VolumeDown` | `Theme_VolumeDown` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047ca00` | `Theme_VolumeUp` | `Theme_VolumeUp` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cae0` | `Theme_RestartCurrentTrack` | `Theme_RestartCurrentTrack` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cba0` | `Theme_SetRoom` | `Theme_SetRoom` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cc70` | `Theme_GetRoom` | `Theme_GetRoom` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cd00` | `Theme_StartPendingStreams` | `Theme_StartPendingStreams` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047ce10` | `Theme_FillMemAndStartStreams` | `Theme_FillMemAndStartStreams` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cf80` | `Theme_SetTransitionMode` | `Theme_SetTransitionMode` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047cf90` | `Theme_Nop3` | `Theme_Nop3` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d020` | `Theme_Nop4` | `Theme_Nop4` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d0b0` | `Theme_KillTimer` | `Theme_KillTimer` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d1b0` | `Theme_KillAllTimers` | `Theme_KillAllTimers` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d2b0` | `Theme_TimerCallback` | `Theme_TimerCallback` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d390` | `Theme_SetTimer` | `Theme_SetTimer` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d510` | `Theme_InitTimerTable` | `Theme_InitTimerTable` | THEMES.cpp | ☑ | `src/THEMES.cpp` |
| `0x0047d5b0` | `Theme_RegisterAsyncProg` | `Theme_RegisterAsyncProg` | THEMES.cpp | ☑ | `src/THEMES.cpp` |

### TIMERS.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0047d6a0` | `Timer_AddAsyncProg` | `Timer_AddAsyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047d7a0` | `Timer_AddSyncProg` | `Timer_AddSyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047d890` | `Timer_AddSyncProgWithData` | `Timer_AddSyncProgWithData` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047d990` | `Timer_DispatchAsyncProg` | `Timer_DispatchAsyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047da20` | `Timer_RunAsyncProg` | `Timer_RunAsyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047db10` | `Timer_PopAsyncProg` | `Timer_PopAsyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047dc60` | `Timer_DispatchProg` | `Timer_DispatchProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047dd00` | `Timer_RunSyncProg` | `Timer_RunSyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047ddf0` | `Timer_PopSyncProg` | `Timer_PopSyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047df40` | `Timer_HasPendingAsyncProg` | `Timer_HasPendingAsyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e000` | `Timer_HasPendingSyncProg` | `Timer_HasPendingSyncProg` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e0c0` | `Timer_Tick` | `Timer_Tick` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e210` | `Timer_Untick` | `Timer_Untick` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e3a0` | `Timer_AddAsync` | `Timer_AddAsync` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e500` | `Timer_TickCallback` | `Timer_TickCallback` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e6f0` | `Timer_Reset` | `Timer_Reset` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e7b0` | `Timer_AddSync` | `Timer_AddSync` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047e910` | `Timer_Kill` | `Timer_Kill` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047ea70` | `Timer_AddWithReset` | `Timer_AddWithReset` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047ec10` | `Timer_ResetCounters` | `Timer_ResetCounters` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |
| `0x0047ecf0` | `Timer_TriggerInit` | `Timer_TriggerInit` | TIMERS.cpp | ☑ | `src/TIMERS.cpp` |

### Tushtush.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x0047eed0` | `Tt_ObjDtor` | `Tt_ObjDtor` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047ef70` | `Tt_SetInitScript` | `Tt_SetInitScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f010` | `Tt_SetPeriodicScript` | `Tt_SetPeriodicScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f0e0` | `Tt_SetCollisionScript` | `Tt_SetCollisionScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f180` | `Tt_SetPos` | `Tt_SetPos` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f2c0` | `Tt_SetRange` | `Tt_SetRange` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f450` | `Tt_SetPers` | `Tt_SetPers` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f500` | `Tt_ObjIsIt` | `Tt_ObjIsIt` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f5b0` | `Tt_CheckProb` | `Tt_CheckProb` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f670` | `Tt_SobjCtor` | `Tt_SobjCtor` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f840` | `Tt_SobjDtor` | `Tt_SobjDtor` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047f850` | `Tt_SobjShow` | `Tt_SobjShow` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047fa10` | `Tt_SobjGetRect` | `Tt_SobjGetRect` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047fb60` | `Tt_SobjAdvanceFrame` | `Tt_SobjAdvanceFrame` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047fc00` | `Tt_SobjOverTheHill` | `Tt_SobjOverTheHill` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047fca0` | `Tt_SobjCheckCollision` | `Tt_SobjCheckCollision` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x0047fdf0` | `Tt_Handler` | `Tt_Handler` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480080` | `Tt_SobjListRemoveNode` | `Tt_SobjListRemoveNode` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480200` | `Tt_Cleanup` | `Tt_Cleanup` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480460` | `Tt_Init` | `Tt_Init` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480670` | `Tt_ObjAdd` | `Tt_ObjAdd` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480780` | `Tt_ObjListAppend` | `Tt_ObjListAppend` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x004808d0` | `Tt_ObjSetCur` | `Tt_ObjSetCur` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480a00` | `Tt_ObjSetInitScript` | `Tt_ObjSetInitScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480aa0` | `Tt_ObjSetCollisionScript` | `Tt_ObjSetCollisionScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480b40` | `Tt_ObjSetPeriodicScript` | `Tt_ObjSetPeriodicScript` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480bf0` | `Tt_ObjSetPos` | `Tt_ObjSetPos` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480ca0` | `Tt_ObjSetRange` | `Tt_ObjSetRange` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480d50` | `Tt_ObjSetPers` | `Tt_ObjSetPers` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00480e00` | `Tt_ObjRem` | `Tt_ObjRem` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x004810a0` | `Tt_ObjListRemoveNode` | `Tt_ObjListRemoveNode` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481220` | `Tt_SobjAddByItr` | `Tt_SobjAddByItr` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481320` | `Tt_SobjListAppend` | `Tt_SobjListAppend` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481470` | `Tt_SobjAdd` | `Tt_SobjAdd` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481500` | `Tt_SobjInsert` | `Tt_SobjInsert` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481610` | `Tt_SobjListInsertBefore` | `Tt_SobjListInsertBefore` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481770` | `Tt_SobjAppend` | `Tt_SobjAppend` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481880` | `Tt_SobjListInsertAfter` | `Tt_SobjListInsertAfter` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x004819f0` | `Tt_SobjRemove` | `Tt_SobjRemove` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481af0` | `Tt_GetSobj` | `Tt_GetSobj` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481b70` | `Tt_SobjLink` | `Tt_SobjLink` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481c70` | `Tt_CharSet` | `Tt_CharSet` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481ef0` | `Tt_CharRemove` | `Tt_CharRemove` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00481fe0` | `Tt_GetChar` | `Tt_GetChar` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x004820c0` | `Tt_GetCharResult` | `Tt_GetCharResult` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x00482150` | `Tt_SetAdventDir` | `Tt_SetAdventDir` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |
| `0x004821f0` | `Tt_CdFind` | `Tt_CdFind` | Tushtush.cpp | ☑ | `src/Tushtush.cpp` |

### Winmain.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x004824a0` | `Win_ParseCmdLine` | `Win_ParseCmdLine` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00482980` | `Win_GetWorkingDirs` | `Win_GetWorkingDirs` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00482e30` | `Win_BuildSubDir` | `Win_BuildSubDir` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00482f90` | `Win_EnsureTrailingSlash` | `Win_EnsureTrailingSlash` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483050` | `Win_GetModuleDir` | `Win_GetModuleDir` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483130` | `Win_CloseHandle` | `Win_CloseHandle` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004831d0` | `Win_Quit` | `Win_Quit` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483200` | `Win_RegisterExitHandler` | `Win_RegisterExitHandler` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004832d0` | `Win_UnregisterExitHandler` | `Win_UnregisterExitHandler` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004833e0` | `Win_RunExitHandlers` | `Win_RunExitHandlers` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483490` | `Win_CleanExit` | `Win_CleanExit` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483820` | `Win_CloseEventHandle` | `Win_CloseEventHandle` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004838b0` | `Win_PeekMsg` | `Win_PeekMsg` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483990` | `Win_UpdateInput` | `Win_UpdateInput` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483ac0` | `Win_PostCallback` | `Win_PostCallback` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483b60` | `Win_WriteThreadTimes` | `Win_WriteThreadTimes` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483c70` | `Win_RepositionWindow` | `Win_RepositionWindow` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483e20` | `Win_IsWindowed` | `Win_IsWindowed` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00483eb0` | `Win_ReadIni` | `Win_ReadIni` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00484040` | `Win_Main` | `Win_Main` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004849c0` | `Win_CreateMainWindow` | `Win_CreateMainWindow` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00484e20` | `Win_CreateHiddenWindow` | `Win_CreateHiddenWindow` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00484f60` | `Win_MessageLoop` | `Win_MessageLoop` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004850c0` | `Win_ExecutionThread` | `Win_ExecutionThread` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00485e90` | `Win_QueryDisplayMetrics` | `Win_QueryDisplayMetrics` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00485ff0` | `Win_ConfirmExit` | `Win_ConfirmExit` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00486100` | `Win_FixWindowZOrder` | `Win_FixWindowZOrder` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x004861c0` | `Win_IsAppActive` | `Win_IsAppActive` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00486250` | `Win_WndProc` | `Win_WndProc` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00486cd0` | `Win_UpdateCursor` | `Win_UpdateCursor` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00486d80` | `Win_BringToFront` | `Win_BringToFront` | Winmain.cpp | ☑ | `src/Winmain.cpp` |
| `0x00486e20` | `Win_BuildCursorMask` | `Win_BuildCursorMask` | Winmain.cpp | ☑ | `src/Winmain.cpp` |

### WINRES.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00486fe0` | `WinRes_BuildAndMask` | `WinRes_BuildAndMask` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x004871b0` | `WinRes_Sma2Icon` | `WinRes_Sma2Icon` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487250` | `WinRes_Sma2IconCore` | `WinRes_Sma2IconCore` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x004874a0` | `WinRes_MakeEmptyIcon` | `WinRes_MakeEmptyIcon` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487550` | `WinRes_Sma2Bitmap` | `WinRes_Sma2Bitmap` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x004875e0` | `WinRes_Sma2BitmapCore` | `WinRes_Sma2BitmapCore` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487770` | `WinRes_Sma2Cursor` | `WinRes_Sma2Cursor` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487810` | `WinRes_Sma2CursorMono` | `WinRes_Sma2CursorMono` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x004878b0` | `WinRes_Sma2IconMonoCore` | `WinRes_Sma2IconMonoCore` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487bb0` | `WinRes_RegInitKey` | `WinRes_RegInitKey` | WINRES.cpp | ☑ | `src/WINRES.cpp` |
| `0x00487cb0` | `Wiz_FindVar` | `Wiz_FindVar` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |

### WIZARDS.cpp

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00487db0` | `Wiz_AddVar` | `Wiz_AddVar` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x00487f50` | `Wiz_AskString` | `Wiz_AskString` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |

### (msvc-crt)

| Address | Ghidra Name | Original Name | Source File | Reversed | RE Output File |
|---------|-------------|---------------|-------------|----------|----------------|
| `0x00488110` | `Wiz_Run` | `Wiz_Run` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x00488300` | `Wiz_ResetStructs` | `Wiz_ResetStructs` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x004883b0` | `Wiz_SplitStrings` | `Wiz_SplitStrings` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x00488710` | `Wiz_VerifyString` | `Wiz_VerifyString` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x004887e0` | `Wiz_GetCommand` | `Wiz_GetCommand` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x004888d0` | `Wiz_GetNextLine` | `Wiz_GetNextLine` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x00488a10` | `Wiz_TrimStr` | `Wiz_TrimStr` | WIZARDS.cpp | ☑ | `src/WIZARDS.cpp` |
| `0x00488e4a` | `GetOpenFileNameA` |  | (msvc-crt) | ☑ |  |
| `0x00488e50` | `GetSaveFileNameA` |  | (msvc-crt) | ☑ |  |
| `0x00488e56` | `CommDlgExtendedError` |  | (msvc-crt) | ☑ |  |
| `0x00488e5c` | `ReplaceTextA` |  | (msvc-crt) | ☑ |  |
| `0x00488e62` | `FindTextA` |  | (msvc-crt) | ☑ |  |
| `0x00488ec8` | `DirectDrawCreate` |  | (msvc-crt) | ☑ |  |
| `0x00488ece` | `DirectSoundCreate` |  | (msvc-crt) | ☑ |  |
| `0x00488ee0` | `FUN_00488ee0` |  | (msvc-crt) | ☐ |  |
| `0x00488ef0` | `FUN_00488ef0` |  | (msvc-crt) | ☐ |  |
| `0x00488f20` | `FUN_00488f20` |  | (msvc-crt) | ☐ |  |
| `0x00489090` | `FUN_00489090` |  | (msvc-crt) | ☐ |  |
| `0x004890e0` | `FUN_004890e0` |  | (msvc-crt) | ☐ |  |
| `0x004890f0` | `FUN_004890f0` |  | (msvc-crt) | ☐ |  |
| `0x00489130` | `FUN_00489130` |  | (msvc-crt) | ☐ |  |
| `0x00489140` | `FUN_00489140` |  | (msvc-crt) | ☐ |  |
| `0x00489150` | `FUN_00489150` |  | (msvc-crt) | ☐ |  |
| `0x00489160` | `FUN_00489160` |  | (msvc-crt) | ☐ |  |
| `0x004891c0` | `FUN_004891c0` |  | (msvc-crt) | ☐ |  |
| `0x00489220` | `FUN_00489220` |  | (msvc-crt) | ☐ |  |
| `0x00489280` | `FUN_00489280` |  | (msvc-crt) | ☐ |  |
| `0x004892b0` | `FUN_004892b0` |  | (msvc-crt) | ☐ |  |
| `0x00489380` | `FUN_00489380` |  | (msvc-crt) | ☐ |  |
| `0x00489410` | `FUN_00489410` |  | (msvc-crt) | ☐ |  |
| `0x00489490` | `__global_unwind2` |  | (msvc-crt) | ☑ |  |
| `0x004894d2` | `__local_unwind2` |  | (msvc-crt) | ☑ |  |
| `0x0048953a` | `__abnormal_termination` |  | (msvc-crt) | ☑ |  |
| `0x0048955d` | `__NLG_Notify1` |  | (msvc-crt) | ☑ |  |
| `0x00489566` | `FUN_00489566` |  | (msvc-crt) | ☐ |  |
| `0x00489580` | `_memset` |  | (msvc-crt) | ☑ |  |
| `0x004895e0` | `FUN_004895e0` |  | (msvc-crt) | ☐ |  |
| `0x004895f0` | `FUN_004895f0` |  | (msvc-crt) | ☐ |  |
| `0x004896d0` | `FUN_004896d0` |  | (msvc-crt) | ☐ |  |
| `0x00489a10` | `_strcmp` |  | (msvc-crt) | ☑ |  |
| `0x00489aa0` | `FUN_00489aa0` |  | (msvc-crt) | ☐ |  |
| `0x00489c50` | `FUN_00489c50` |  | (msvc-crt) | ☐ |  |
| `0x00489cb0` | `FUN_00489cb0` |  | (msvc-crt) | ☐ |  |
| `0x00489ce0` | `FUN_00489ce0` |  | (msvc-crt) | ☐ |  |
| `0x00489cf0` | `FUN_00489cf0` |  | (msvc-crt) | ☐ |  |
| `0x00489d20` | `FUN_00489d20` |  | (msvc-crt) | ☐ |  |
| `0x0048a060` | `FUN_0048a060` |  | (msvc-crt) | ☐ |  |
| `0x0048a0d0` | `FUN_0048a0d0` |  | (msvc-crt) | ☐ |  |
| `0x0048a110` | `FUN_0048a110` |  | (msvc-crt) | ☐ |  |
| `0x0048a180` | `FUN_0048a180` |  | (msvc-crt) | ☐ |  |
| `0x0048a1c0` | `FUN_0048a1c0` |  | (msvc-crt) | ☐ |  |
| `0x0048a300` | `FUN_0048a300` |  | (msvc-crt) | ☐ |  |
| `0x0048a340` | `FUN_0048a340` |  | (msvc-crt) | ☐ |  |
| `0x0048a360` | `FUN_0048a360` |  | (msvc-crt) | ☐ |  |
| `0x0048a490` | `FUN_0048a490` |  | (msvc-crt) | ☐ |  |
| `0x0048a4d0` | `FUN_0048a4d0` |  | (msvc-crt) | ☐ |  |
| `0x0048a620` | `FUN_0048a620` |  | (msvc-crt) | ☐ |  |
| `0x0048a650` | `FUN_0048a650` |  | (msvc-crt) | ☐ |  |
| `0x0048a660` | `FUN_0048a660` |  | (msvc-crt) | ☐ |  |
| `0x0048a6a0` | `FUN_0048a6a0` |  | (msvc-crt) | ☐ |  |
| `0x0048a710` | `_strlen` |  | (msvc-crt) | ☑ |  |
| `0x0048a790` | `FUN_0048a790` |  | (msvc-crt) | ☐ |  |
| `0x0048a820` | `FUN_0048a820` |  | (msvc-crt) | ☐ |  |
| `0x0048a840` | `FID_conflict:_fwprintf` |  | (msvc-crt) | ☑ |  |
| `0x0048a890` | `_strncmp` |  | (msvc-crt) | ☑ |  |
| `0x0048a8d0` | `FUN_0048a8d0` |  | (msvc-crt) | ☐ |  |
| `0x0048a910` | `FUN_0048a910` |  | (msvc-crt) | ☐ |  |
| `0x0048a9b0` | `FUN_0048a9b0` |  | (msvc-crt) | ☐ |  |
| `0x0048a9e0` | `FUN_0048a9e0` |  | (msvc-crt) | ☐ |  |
| `0x0048aba0` | `_strchr` |  | (msvc-crt) | ☑ |  |
| `0x0048ac60` | `FUN_0048ac60` |  | (msvc-crt) | ☐ |  |
| `0x0048ac80` | `FUN_0048ac80` |  | (msvc-crt) | ☐ |  |
| `0x0048acd0` | `FUN_0048acd0` |  | (msvc-crt) | ☐ |  |
| `0x0048ad40` | `FUN_0048ad40` |  | (msvc-crt) | ☐ |  |
| `0x0048add0` | `FUN_0048add0` |  | (msvc-crt) | ☐ |  |
| `0x0048aed0` | `__fpmath` |  | (msvc-crt) | ☑ |  |
| `0x0048af00` | `FUN_0048af00` |  | (msvc-crt) | ☐ |  |
| `0x0048af50` | `__ftol` |  | (msvc-crt) | ☑ |  |
| `0x0048af80` | `FUN_0048af80` |  | (msvc-crt) | ☐ |  |
| `0x0048af94` | `FUN_0048af94` |  | (msvc-crt) | ☐ |  |
| `0x0048af9d` | `FUN_0048af9d` |  | (msvc-crt) | ☐ |  |
| `0x0048b030` | `FUN_0048b030` |  | (msvc-crt) | ☐ |  |
| `0x0048b044` | `FUN_0048b044` |  | (msvc-crt) | ☐ |  |
| `0x0048b04d` | `FUN_0048b04d` |  | (msvc-crt) | ☐ |  |
| `0x0048b0e0` | `FUN_0048b0e0` |  | (msvc-crt) | ☐ |  |
| `0x0048b0f4` | `FUN_0048b0f4` |  | (msvc-crt) | ☐ |  |
| `0x0048b0fd` | `FUN_0048b0fd` |  | (msvc-crt) | ☐ |  |
| `0x0048b1b0` | `FUN_0048b1b0` |  | (msvc-crt) | ☐ |  |
| `0x0048b1c4` | `FUN_0048b1c4` |  | (msvc-crt) | ☐ |  |
| `0x0048b1cd` | `FUN_0048b1cd` |  | (msvc-crt) | ☐ |  |
| `0x0048b270` | `FUN_0048b270` |  | (msvc-crt) | ☐ |  |
| `0x0048b2c0` | `FUN_0048b2c0` |  | (msvc-crt) | ☐ |  |
| `0x0048b2e0` | `FUN_0048b2e0` |  | (msvc-crt) | ☐ |  |
| `0x0048b380` | `FUN_0048b380` |  | (msvc-crt) | ☐ |  |
| `0x0048b460` | `FUN_0048b460` |  | (msvc-crt) | ☐ |  |
| `0x0048b4a0` | `FUN_0048b4a0` |  | (msvc-crt) | ☐ |  |
| `0x0048b510` | `_memcmp` |  | (msvc-crt) | ☑ |  |
| `0x0048b5c0` | `FUN_0048b5c0` |  | (msvc-crt) | ☐ |  |
| `0x0048b6d0` | `FUN_0048b6d0` |  | (msvc-crt) | ☐ |  |
| `0x0048b8b0` | `FUN_0048b8b0` |  | (msvc-crt) | ☐ |  |
| `0x0048b8e0` | `FUN_0048b8e0` |  | (msvc-crt) | ☐ |  |
| `0x0048b900` | `__exit` |  | (msvc-crt) | ☑ |  |
| `0x0048b940` | `FUN_0048b940` |  | (msvc-crt) | ☐ |  |
| `0x0048ba00` | `FUN_0048ba00` |  | (msvc-crt) | ☐ |  |
| `0x0048ba10` | `FUN_0048ba10` |  | (msvc-crt) | ☐ |  |
| `0x0048ba20` | `FUN_0048ba20` |  | (msvc-crt) | ☐ |  |
| `0x0048ba40` | `_strncpy` |  | (msvc-crt) | ☑ |  |
| `0x0048bb40` | `FUN_0048bb40` |  | (msvc-crt) | ☐ |  |
| `0x0048bc30` | `entry` |  | (msvc-crt) | ☑ |  |
| `0x0048bde0` | `__amsg_exit` |  | (msvc-crt) | ☑ |  |
| `0x0048be10` | `FUN_0048be10` |  | (msvc-crt) | ☐ |  |
| `0x0048bec0` | `FUN_0048bec0` |  | (msvc-crt) | ☐ |  |
| `0x0048bf40` | `FUN_0048bf40` |  | (msvc-crt) | ☐ |  |
| `0x0048bf60` | `FUN_0048bf60` |  | (msvc-crt) | ☐ |  |
| `0x0048bfa0` | `FUN_0048bfa0` |  | (msvc-crt) | ☐ |  |
| `0x0048bfd0` | `FUN_0048bfd0` |  | (msvc-crt) | ☐ |  |
| `0x0048c010` | `FUN_0048c010` |  | (msvc-crt) | ☐ |  |
| `0x0048c0f0` | `FUN_0048c0f0` |  | (msvc-crt) | ☐ |  |
| `0x0048c1c0` | `FUN_0048c1c0` |  | (msvc-crt) | ☐ |  |
| `0x0048c480` | `FUN_0048c480` |  | (msvc-crt) | ☐ |  |
| `0x0048c560` | `FUN_0048c560` |  | (msvc-crt) | ☐ |  |
| `0x0048c640` | `FUN_0048c640` |  | (msvc-crt) | ☐ |  |
| `0x0048c6d0` | `FUN_0048c6d0` |  | (msvc-crt) | ☐ |  |
| `0x0048c7c8` | `FUN_0048c7c8` |  | (msvc-crt) | ☐ |  |
| `0x0048c860` | `FUN_0048c860` |  | (msvc-crt) | ☐ |  |
| `0x0048ca70` | `FUN_0048ca70` |  | (msvc-crt) | ☐ |  |
| `0x0048caf0` | `FUN_0048caf0` |  | (msvc-crt) | ☐ |  |
| `0x0048cb20` | `__CallSettingFrame@12` |  | (msvc-crt) | ☑ |  |
| `0x0048cb70` | `FUN_0048cb70` |  | (msvc-crt) | ☐ |  |
| `0x0048cc00` | `FUN_0048cc00` |  | (msvc-crt) | ☐ |  |
| `0x0048cc20` | `FUN_0048cc20` |  | (msvc-crt) | ☐ |  |
| `0x0048cd60` | `FUN_0048cd60` |  | (msvc-crt) | ☐ |  |
| `0x0048cdd7` | `_abort` |  | (msvc-crt) | ☑ |  |
| `0x0048ce10` | `FUN_0048ce10` |  | (msvc-crt) | ☐ |  |
| `0x0048ce7e` | `FUN_0048ce7e` |  | (msvc-crt) | ☐ |  |
| `0x0048cea0` | `FUN_0048cea0` |  | (msvc-crt) | ☐ |  |
| `0x0048cfd0` | `FUN_0048cfd0` |  | (msvc-crt) | ☐ |  |
| `0x0048d960` | `FUN_0048d960` |  | (msvc-crt) | ☐ |  |
| `0x0048d9b0` | `FUN_0048d9b0` |  | (msvc-crt) | ☐ |  |
| `0x0048d9f0` | `FUN_0048d9f0` |  | (msvc-crt) | ☐ |  |
| `0x0048da30` | `FUN_0048da30` |  | (msvc-crt) | ☐ |  |
| `0x0048da50` | `FUN_0048da50` |  | (msvc-crt) | ☐ |  |
| `0x0048da70` | `FUN_0048da70` |  | (msvc-crt) | ☐ |  |
| `0x0048da90` | `FUN_0048da90` |  | (msvc-crt) | ☐ |  |
| `0x0048db00` | `FUN_0048db00` |  | (msvc-crt) | ☐ |  |
| `0x0048db90` | `FUN_0048db90` |  | (msvc-crt) | ☐ |  |
| `0x0048dc50` | `FUN_0048dc50` |  | (msvc-crt) | ☐ |  |
| `0x0048dd90` | `FUN_0048dd90` |  | (msvc-crt) | ☐ |  |
| `0x0048de80` | `FUN_0048de80` |  | (msvc-crt) | ☐ |  |
| `0x0048df00` | `FUN_0048df00` |  | (msvc-crt) | ☐ |  |
| `0x0048e130` | `FUN_0048e130` |  | (msvc-crt) | ☐ |  |
| `0x0048e300` | `FUN_0048e300` |  | (msvc-crt) | ☐ |  |
| `0x0048e3e0` | `FUN_0048e3e0` |  | (msvc-crt) | ☐ |  |
| `0x0048e4d0` | `FUN_0048e4d0` |  | (msvc-crt) | ☐ |  |
| `0x0048e550` | `FUN_0048e550` |  | (msvc-crt) | ☐ |  |
| `0x0048e760` | `FUN_0048e760` |  | (msvc-crt) | ☐ |  |
| `0x0048e7e0` | `FUN_0048e7e0` |  | (msvc-crt) | ☐ |  |
| `0x0048e7f0` | `FUN_0048e7f0` |  | (msvc-crt) | ☐ |  |
| `0x0048e800` | `FUN_0048e800` |  | (msvc-crt) | ☐ |  |
| `0x0048e8f0` | `FUN_0048e8f0` |  | (msvc-crt) | ☐ |  |
| `0x0048e930` | `FUN_0048e930` |  | (msvc-crt) | ☐ |  |
| `0x0048eb20` | `FUN_0048eb20` |  | (msvc-crt) | ☐ |  |
| `0x0048ebc0` | `FUN_0048ebc0` |  | (msvc-crt) | ☐ |  |
| `0x0048ec00` | `FUN_0048ec00` |  | (msvc-crt) | ☐ |  |
| `0x0048ec80` | `FUN_0048ec80` |  | (msvc-crt) | ☐ |  |
| `0x0048ed00` | `FUN_0048ed00` |  | (msvc-crt) | ☐ |  |
| `0x0048efb0` | `FUN_0048efb0` |  | (msvc-crt) | ☐ |  |
| `0x0048efd0` | `FUN_0048efd0` |  | (msvc-crt) | ☐ |  |
| `0x0048f080` | `FUN_0048f080` |  | (msvc-crt) | ☐ |  |
| `0x0048f1f0` | `FUN_0048f1f0` |  | (msvc-crt) | ☐ |  |
| `0x0048f250` | `FUN_0048f250` |  | (msvc-crt) | ☐ |  |
| `0x0048f320` | `FUN_0048f320` |  | (msvc-crt) | ☐ |  |
| `0x0048f380` | `FUN_0048f380` |  | (msvc-crt) | ☐ |  |
| `0x0048f3e0` | `FUN_0048f3e0` |  | (msvc-crt) | ☐ |  |
| `0x0048f620` | `FUN_0048f620` |  | (msvc-crt) | ☐ |  |
| `0x0048f7a0` | `FUN_0048f7a0` |  | (msvc-crt) | ☐ |  |
| `0x00490370` | `FUN_00490370` |  | (msvc-crt) | ☐ |  |
| `0x00490580` | `FUN_00490580` |  | (msvc-crt) | ☐ |  |
| `0x004905c0` | `FUN_004905c0` |  | (msvc-crt) | ☐ |  |
| `0x004907e0` | `FUN_004907e0` |  | (msvc-crt) | ☐ |  |
| `0x00490810` | `FUN_00490810` |  | (msvc-crt) | ☐ |  |
| `0x004908b0` | `__setdefaultprecision` |  | (msvc-crt) | ☑ |  |
| `0x004908d0` | `FUN_004908d0` |  | (msvc-crt) | ☐ |  |
| `0x00490920` | `FUN_00490920` |  | (msvc-crt) | ☐ |  |
| `0x00490950` | `FUN_00490950` |  | (msvc-crt) | ☐ |  |
| `0x00490aa0` | `FUN_00490aa0` |  | (msvc-crt) | ☐ |  |
| `0x00490b20` | `FUN_00490b20` |  | (msvc-crt) | ☐ |  |
| `0x00490c20` | `FUN_00490c20` |  | (msvc-crt) | ☐ |  |
| `0x00490c90` | `FUN_00490c90` |  | (msvc-crt) | ☐ |  |
| `0x00490d50` | `FUN_00490d50` |  | (msvc-crt) | ☐ |  |
| `0x00490e80` | `FUN_00490e80` |  | (msvc-crt) | ☐ |  |
| `0x00490eb0` | `__trandisp1` |  | (msvc-crt) | ☑ |  |
| `0x00490f17` | `__trandisp2` |  | (msvc-crt) | ☑ |  |
| `0x00490fb6` | `FUN_00490fb6` |  | (msvc-crt) | ☐ |  |
| `0x00491073` | `FUN_00491073` |  | (msvc-crt) | ☐ |  |
| `0x00491095` | `FUN_00491095` |  | (msvc-crt) | ☐ |  |
| `0x004910ac` | `FUN_004910ac` |  | (msvc-crt) | ☐ |  |
| `0x004910c5` | `__fload_withFB` |  | (msvc-crt) | ☑ |  |
| `0x00491108` | `FUN_00491108` |  | (msvc-crt) | ☐ |  |
| `0x0049112b` | `__math_exit` |  | (msvc-crt) | ☑ |  |
| `0x00491210` | `FUN_00491210` |  | (msvc-crt) | ☐ |  |
| `0x00491227` | `__startOneArgErrorHandling` |  | (msvc-crt) | ☑ |  |
| `0x00491270` | `FUN_00491270` |  | (msvc-crt) | ☐ |  |
| `0x00491fb0` | `FUN_00491fb0` |  | (msvc-crt) | ☐ |  |
| `0x00491ff0` | `FUN_00491ff0` |  | (msvc-crt) | ☐ |  |
| `0x00492020` | `FUN_00492020` |  | (msvc-crt) | ☐ |  |
| `0x00492040` | `FUN_00492040` |  | (msvc-crt) | ☐ |  |
| `0x004920fc` | `FUN_004920fc` |  | (msvc-crt) | ☐ |  |
| `0x00492140` | `__cintrindisp2` |  | (msvc-crt) | ☑ |  |
| `0x0049217e` | `__cintrindisp1` |  | (msvc-crt) | ☑ |  |
| `0x004921bb` | `__ctrandisp2` |  | (msvc-crt) | ☑ |  |
| `0x00492203` | `FUN_00492203` |  | (msvc-crt) | ☐ |  |
| `0x0049220a` | `FUN_0049220a` |  | (msvc-crt) | ☐ |  |
| `0x0049233b` | `__ctrandisp1` |  | (msvc-crt) | ☑ |  |
| `0x0049236e` | `__fload` |  | (msvc-crt) | ☑ |  |
| `0x004923b0` | `FUN_004923b0` |  | (msvc-crt) | ☐ |  |
| `0x00492950` | `FUN_00492950` |  | (msvc-crt) | ☐ |  |
| `0x004929c0` | `FUN_004929c0` |  | (msvc-crt) | ☐ |  |
| `0x00492a00` | `FUN_00492a00` |  | (msvc-crt) | ☐ |  |
| `0x00492af0` | `FUN_00492af0` |  | (msvc-crt) | ☐ |  |
| `0x00492b90` | `FUN_00492b90` |  | (msvc-crt) | ☐ |  |
| `0x00492fa0` | `FUN_00492fa0` |  | (msvc-crt) | ☐ |  |
| `0x00493100` | `FUN_00493100` |  | (msvc-crt) | ☐ |  |
| `0x00493320` | `FUN_00493320` |  | (msvc-crt) | ☐ |  |
| `0x00493370` | `FUN_00493370` |  | (msvc-crt) | ☐ |  |
| `0x004933d0` | `FUN_004933d0` |  | (msvc-crt) | ☐ |  |
| `0x00493410` | `FUN_00493410` |  | (msvc-crt) | ☐ |  |
| `0x004934e5` | `FUN_004934e5` |  | (msvc-crt) | ☐ |  |
| `0x00493500` | `FUN_00493500` |  | (msvc-crt) | ☐ |  |
| `0x00493540` | `FUN_00493540` |  | (msvc-crt) | ☐ |  |
| `0x00493830` | `FUN_00493830` |  | (msvc-crt) | ☐ |  |
| `0x00493850` | `FUN_00493850` |  | (msvc-crt) | ☐ |  |
| `0x00493870` | `FUN_00493870` |  | (msvc-crt) | ☐ |  |
| `0x00493890` | `FUN_00493890` |  | (msvc-crt) | ☐ |  |
| `0x00493940` | `_abort` |  | (msvc-crt) | ☑ |  |
| `0x00493960` | `FUN_00493960` |  | (msvc-crt) | ☐ |  |
| `0x004939c0` | `FUN_004939c0` |  | (msvc-crt) | ☐ |  |
| `0x004939f0` | `FUN_004939f0` |  | (msvc-crt) | ☐ |  |
| `0x00493a60` | `FUN_00493a60` |  | (msvc-crt) | ☐ |  |
| `0x00493ae0` | `FUN_00493ae0` |  | (msvc-crt) | ☐ |  |
| `0x00493b50` | `FUN_00493b50` |  | (msvc-crt) | ☐ |  |
| `0x00493bd0` | `FUN_00493bd0` |  | (msvc-crt) | ☐ |  |
| `0x00493d40` | `FUN_00493d40` |  | (msvc-crt) | ☐ |  |
| `0x00493df0` | `FUN_00493df0` |  | (msvc-crt) | ☐ |  |
| `0x00493e90` | `FUN_00493e90` |  | (msvc-crt) | ☐ |  |
| `0x00493f90` | `FUN_00493f90` |  | (msvc-crt) | ☐ |  |
| `0x00494000` | `FUN_00494000` |  | (msvc-crt) | ☐ |  |
| `0x004940d0` | `FUN_004940d0` |  | (msvc-crt) | ☐ |  |
| `0x004940f0` | `FUN_004940f0` |  | (msvc-crt) | ☐ |  |
| `0x00494490` | `FUN_00494490` |  | (msvc-crt) | ☐ |  |
| `0x004944f0` | `FUN_004944f0` |  | (msvc-crt) | ☐ |  |
| `0x004947d0` | `__isindst` |  | (msvc-crt) | ☑ |  |
| `0x00494800` | `FUN_00494800` |  | (msvc-crt) | ☐ |  |
| `0x00494a70` | `FUN_00494a70` |  | (msvc-crt) | ☐ |  |
| `0x00494c10` | `FUN_00494c10` |  | (msvc-crt) | ☐ |  |
| `0x00494e30` | `FUN_00494e30` |  | (msvc-crt) | ☐ |  |
| `0x004951b0` | `FUN_004951b0` |  | (msvc-crt) | ☐ |  |
| `0x004953f0` | `FUN_004953f0` |  | (msvc-crt) | ☐ |  |
| `0x004956f0` | `FUN_004956f0` |  | (msvc-crt) | ☐ |  |
| `0x00495820` | `FUN_00495820` |  | (msvc-crt) | ☐ |  |
| `0x00495970` | `FUN_00495970` |  | (msvc-crt) | ☐ |  |
| `0x004959b0` | `FUN_004959b0` |  | (msvc-crt) | ☐ |  |
| `0x00495cf0` | `_strcspn` |  | (msvc-crt) | ☑ |  |
| `0x00495d30` | `_strpbrk` |  | (msvc-crt) | ☑ |  |
| `0x00496270` | `FUN_00496270` |  | (msvc-crt) | ☐ |  |
| `0x00496400` | `FUN_00496400` |  | (msvc-crt) | ☐ |  |
| `0x00496530` | `FUN_00496530` |  | (msvc-crt) | ☐ |  |
| `0x00496550` | `FUN_00496550` |  | (msvc-crt) | ☐ |  |
| `0x00496570` | `FUN_00496570` |  | (msvc-crt) | ☐ |  |
| `0x004965b0` | `FUN_004965b0` |  | (msvc-crt) | ☐ |  |
| `0x00496600` | `FUN_00496600` |  | (msvc-crt) | ☐ |  |
| `0x004966a0` | `FUN_004966a0` |  | (msvc-crt) | ☐ |  |
| `0x00496730` | `FUN_00496730` |  | (msvc-crt) | ☐ |  |
| `0x00496770` | `FUN_00496770` |  | (msvc-crt) | ☐ |  |
| `0x004967e0` | `FUN_004967e0` |  | (msvc-crt) | ☐ |  |
| `0x00496850` | `FUN_00496850` |  | (msvc-crt) | ☐ |  |
| `0x004968f0` | `FUN_004968f0` |  | (msvc-crt) | ☐ |  |
| `0x00496910` | `FUN_00496910` |  | (msvc-crt) | ☐ |  |
| `0x00496920` | `FUN_00496920` |  | (msvc-crt) | ☐ |  |
| `0x00496940` | `FUN_00496940` |  | (msvc-crt) | ☐ |  |
| `0x00496a00` | `FUN_00496a00` |  | (msvc-crt) | ☐ |  |
| `0x00496bd0` | `FUN_00496bd0` |  | (msvc-crt) | ☐ |  |
| `0x00496bf0` | `FUN_00496bf0` |  | (msvc-crt) | ☐ |  |
| `0x00496c90` | `FUN_00496c90` |  | (msvc-crt) | ☐ |  |
| `0x00496d10` | `FUN_00496d10` |  | (msvc-crt) | ☐ |  |
| `0x00496d50` | `FUN_00496d50` |  | (msvc-crt) | ☐ |  |
| `0x00496df0` | `FUN_00496df0` |  | (msvc-crt) | ☐ |  |
| `0x00496e80` | `FUN_00496e80` |  | (msvc-crt) | ☐ |  |
| `0x00496f40` | `FUN_00496f40` |  | (msvc-crt) | ☐ |  |
| `0x00496f50` | `FUN_00496f50` |  | (msvc-crt) | ☐ |  |
| `0x00497070` | `FUN_00497070` |  | (msvc-crt) | ☐ |  |
| `0x004970f0` | `FUN_004970f0` |  | (msvc-crt) | ☐ |  |
| `0x004971f0` | `FUN_004971f0` |  | (msvc-crt) | ☐ |  |
| `0x00497240` | `FUN_00497240` |  | (msvc-crt) | ☐ |  |
| `0x00498320` | `FUN_00498320` |  | (msvc-crt) | ☐ |  |
| `0x00498340` | `FUN_00498340` |  | (msvc-crt) | ☐ |  |
| `0x00498470` | `FUN_00498470` |  | (msvc-crt) | ☐ |  |
| `0x00498730` | `FUN_00498730` |  | (msvc-crt) | ☐ |  |
| `0x00498940` | `FUN_00498940` |  | (msvc-crt) | ☐ |  |
| `0x00498a10` | `FUN_00498a10` |  | (msvc-crt) | ☐ |  |
| `0x00498b60` | `FUN_00498b60` |  | (msvc-crt) | ☐ |  |
| `0x00498be0` | `FUN_00498be0` |  | (msvc-crt) | ☐ |  |
| `0x00498dd0` | `FUN_00498dd0` |  | (msvc-crt) | ☐ |  |
| `0x00498e40` | `FUN_00498e40` |  | (msvc-crt) | ☐ |  |
| `0x00499590` | `FUN_00499590` |  | (msvc-crt) | ☐ |  |
| `0x00499b30` | `FUN_00499b30` |  | (msvc-crt) | ☐ |  |
| `0x00499b60` | `FUN_00499b60` |  | (msvc-crt) | ☐ |  |
| `0x00499c00` | `FUN_00499c00` |  | (msvc-crt) | ☐ |  |
| `0x00499c70` | `FUN_00499c70` |  | (msvc-crt) | ☐ |  |
| `0x00499fc0` | `FUN_00499fc0` |  | (msvc-crt) | ☐ |  |
| `0x0049a3a0` | `FUN_0049a3a0` |  | (msvc-crt) | ☐ |  |
| `0x0049a4d0` | `FUN_0049a4d0` |  | (msvc-crt) | ☐ |  |
| `0x0049a630` | `FUN_0049a630` |  | (msvc-crt) | ☐ |  |
| `0x0049a830` | `FUN_0049a830` |  | (msvc-crt) | ☐ |  |
| `0x0049a900` | `FUN_0049a900` |  | (msvc-crt) | ☐ |  |
| `0x0049a930` | `FUN_0049a930` |  | (msvc-crt) | ☐ |  |
| `0x0049a9a0` | `FUN_0049a9a0` |  | (msvc-crt) | ☐ |  |
| `0x0049a9d0` | `FUN_0049a9d0` |  | (msvc-crt) | ☐ |  |
| `0x0049aa00` | `FUN_0049aa00` |  | (msvc-crt) | ☐ |  |
| `0x0049ab00` | `FUN_0049ab00` |  | (msvc-crt) | ☐ |  |
| `0x0049b2e0` | `FUN_0049b2e0` |  | (msvc-crt) | ☐ |  |
| `0x0049b8f0` | `FUN_0049b8f0` |  | (msvc-crt) | ☐ |  |
| `0x0049bc30` | `FUN_0049bc30` |  | (msvc-crt) | ☐ |  |
| `0x0049c010` | `FUN_0049c010` |  | (msvc-crt) | ☐ |  |
| `0x0049c0b0` | `FUN_0049c0b0` |  | (msvc-crt) | ☐ |  |
| `0x0049c0c0` | `FUN_0049c0c0` |  | (msvc-crt) | ☐ |  |
| `0x0049c0d0` | `FUN_0049c0d0` |  | (msvc-crt) | ☐ |  |
| `0x0049c0f0` | `FUN_0049c0f0` |  | (msvc-crt) | ☐ |  |
| `0x0049c120` | `FUN_0049c120` |  | (msvc-crt) | ☐ |  |
| `0x0049c180` | `FUN_0049c180` |  | (msvc-crt) | ☐ |  |
| `0x0049c2b0` | `FUN_0049c2b0` |  | (msvc-crt) | ☐ |  |
| `0x0049c3b0` | `FUN_0049c3b0` |  | (msvc-crt) | ☐ |  |
| `0x0049c9d0` | `FUN_0049c9d0` |  | (msvc-crt) | ☐ |  |
| `0x0049ca40` | `FUN_0049ca40` |  | (msvc-crt) | ☐ |  |
| `0x0049ca80` | `FUN_0049ca80` |  | (msvc-crt) | ☐ |  |
| `0x0049cb00` | `FUN_0049cb00` |  | (msvc-crt) | ☐ |  |
| `0x0049cb90` | `FUN_0049cb90` |  | (msvc-crt) | ☐ |  |
| `0x0049cc10` | `FUN_0049cc10` |  | (msvc-crt) | ☐ |  |
| `0x0049cca0` | `FUN_0049cca0` |  | (msvc-crt) | ☐ |  |
| `0x0049cf60` | `FUN_0049cf60` |  | (msvc-crt) | ☐ |  |
| `0x0049d4a0` | `FUN_0049d4a0` |  | (msvc-crt) | ☐ |  |
| `0x0049d770` | `FUN_0049d770` |  | (msvc-crt) | ☐ |  |
| `0x0049d7a0` | `FUN_0049d7a0` |  | (msvc-crt) | ☐ |  |
| `0x0049d9b0` | `FUN_0049d9b0` |  | (msvc-crt) | ☐ |  |
| `0x0049da30` | `FUN_0049da30` |  | (msvc-crt) | ☐ |  |
| `0x0049daa0` | `FUN_0049daa0` |  | (msvc-crt) | ☐ |  |
| `0x0049dc50` | `FUN_0049dc50` |  | (msvc-crt) | ☐ |  |
| `0x0049dd20` | `FUN_0049dd20` |  | (msvc-crt) | ☐ |  |
| `0x0049dd76` | `RtlUnwind` |  | (msvc-crt) | ☑ |  |
| `0x0049def0` | `FUN_0049def0` |  | (msvc-crt) | ☐ |  |
| `0x0049e070` | `FUN_0049e070` |  | (msvc-crt) | ☐ |  |
| `0x0049e120` | `FUN_0049e120` |  | (msvc-crt) | ☐ |  |
| `0x0049e130` | `FUN_0049e130` |  | (msvc-crt) | ☐ |  |
| `0x0049e150` | `FUN_0049e150` |  | (msvc-crt) | ☐ |  |
| `0x0049e3f0` | `FUN_0049e3f0` |  | (msvc-crt) | ☐ |  |
| `0x0049e410` | `FUN_0049e410` |  | (msvc-crt) | ☐ |  |
| `0x0049e560` | `FUN_0049e560` |  | (msvc-crt) | ☐ |  |
| `0x0049e640` | `FUN_0049e640` |  | (msvc-crt) | ☐ |  |
| `0x0049e690` | `FUN_0049e690` |  | (msvc-crt) | ☐ |  |
| `0x0049e700` | `FUN_0049e700` |  | (msvc-crt) | ☐ |  |
| `0x0049e830` | `FUN_0049e830` |  | (msvc-crt) | ☐ |  |
| `0x0049e870` | `FUN_0049e870` |  | (msvc-crt) | ☐ |  |
