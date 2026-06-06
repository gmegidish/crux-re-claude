#include "RunProg.h"
#include "ScmPlayer.h"
#include "Audio.h"
#include "Anim.h"
#include "Slider.h"
#include "Palette.h"
#include "Text.h"
#include "Theme.h"
#include "Gv.h"
#include "Sentence.h"
#include "TextRender.h"
#include "Area.h"
#include "Log.h"
#include <SDL.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Opcodes that open an IF-block needing a matching ENDIF. Used to balance nested
// blocks during a skip-scan. Mirrors the engine's is-opener predicate; expand as
// the game surfaces more IF-family opcodes.
bool RunProg::isIfOpener(int op) {
    switch (op) {
    case 0x09:  // IF_VAR_LE
    case 0x0a:  // IF_VAR_NE
    case 0x0b:  // IF_VAR_GE
    case 0x0e:  // IF_VAR_EQ
    case 0x11:  // IF_INV_HAS
    case 0x6f:  // IF_OBJECT_NOT_IN_LIST
        return true;
    default:
        return false;
    }
}

int& RunProg::var(int i) {
    static int sink;
    if (i < 0 || i >= 1500) {
        Log::warn("RunProg: variable index %d out of range [0,1500) — ignoring", i);
        sink = 0;
        return sink;
    }
    return vars_[i];
}

int RunProg::skipBlock(const ScriptProgram& p, int pc, bool stopAtElse) const {
    int nest = 0;
    const int count = (int)p.insns.size();
    while (++pc < count) {
        int op = p.insns[pc].op;
        if (op == 0x0f) {                 // ENDIF
            if (nest == 0) break;
            --nest;
        } else if (stopAtElse && op == 0x10 && nest == 0) {  // ELSE at our level
            break;
        } else if (isIfOpener(op)) {
            ++nest;
        }
    }
    return pc;   // index of the matching ENDIF/ELSE (caller's pc++ steps past it)
}

void RunProg::die(const ScriptInsn& in, int pc, int progId) const {
    Log::error("RunProg: UNIMPLEMENTED opcode 0x%x (args %d, %d, %d) "
               "at program %d pc %d", in.op, in.a0, in.a1, in.a2, progId, pc);
    Log::error("  -> implement this opcode in RunProg::exec (see RUNPROG_OPCODES.md 0x%x)", in.op);
    std::exit(2);
}

// SetPal_SmoothFadeToBlack: ramp the active palette to black over 64 steps,
// presenting each (msPerStep delay between, mirroring the engine's Sleep). The
// engine decrements 6-bit components by the step index; our framebuffer palette is
// 8-bit, so decrement by step*4 and clamp. Headless settles straight to black.
void RunProg::fadeToBlack(int msPerStep) {
    uint8_t orig[768];
    std::memcpy(orig, fb_.palette(), sizeof orig);
    for (int s = 0; s < 64; ++s) {
        int dec = s * 4;
        for (int i = 0; i < 256; ++i) {
            int r = (int)orig[i*3+0] - dec; if (r < 0) { r = 0; }
            int g = (int)orig[i*3+1] - dec; if (g < 0) { g = 0; }
            int b = (int)orig[i*3+2] - dec; if (b < 0) { b = 0; }
            fb_.setPaletteEntry(i, (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
        disp_.present(fb_);
        if (!disp_.isRealtime()) { break; }            // headless: skip the animation
        if (msPerStep > 0) { SDL_Delay(msPerStep); }
    }
    for (int i = 0; i < 256; ++i) { fb_.setPaletteEntry(i, 0, 0, 0); }
    disp_.present(fb_);
}

// Show a Hebrew subtitle over the current frame for an estimated duration (the
// engine waits for the speech audio; we don't have it, so estimate from length).
// Skippable with a click/key; restores the frame afterwards. Realtime only.
void RunProg::showSpeech(const std::string& cp1255) {
    if (cp1255.empty() || !TextRender::ready() || !disp_.isRealtime()) { return; }
    const size_t n = (size_t)fb_.width() * fb_.height();
    std::vector<uint8_t> saved(fb_.pixels(), fb_.pixels() + n);

    TextRender::drawSentence(fb_, cp1255, fb_.width() / 2, fb_.height() - 40);   // bottom-centered

    int ms = (int)cp1255.size() * 55;
    if (ms < 1500) { ms = 1500; }
    if (ms > 8000) { ms = 8000; }
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < (Uint32)ms) {
        disp_.present(fb_);
        PumpResult pr = disp_.pump();
        if (pr == PumpResult::Quit) { quit_ = true; break; }
        if (pr == PumpResult::Skip) { break; }          // click/key skips the line
        SDL_Delay(33);
    }
    std::memcpy(fb_.pixels(), saved.data(), n);          // remove the subtitle
    disp_.present(fb_);
}

// Per-restart frame budget for op 0x3b in headless / non-realtime runs. The
// engine's RESTART_SCRIPT loops forever waiting on a per-frame poll; in realtime
// that's fine (the window/input can terminate it), but headless has no operator,
// so a self-restarting poll would hang. Cap the restarts at a generous number of
// frames (well past any anim's length) and log when the cap is hit.
static const int kHeadlessRestartCap = 1000;

// pumpFrame — advance one paced frame of the world from inside the VM. Used by op
// 0x3b (RESTART_SCRIPT), whose engine analog calls Adv_Tick()+Timer_DispatchAsyncProg()
// once per dispatch-loop iteration (RUNPROG.cpp:266) so anims advance and per-frame
// polls can terminate. We mirror that minimal per-frame work here: advance anims,
// keep room music streaming, render+present a frame, and pump input so quit/area-
// change can break the loop. This is a self-contained pump (it does NOT reuse
// main.cpp's runScene compositor, which owns menu-flower/backdrop state we don't
// have here); it composes whatever anims are live over the current framebuffer.
// Realtime paces at ~30fps; headless settles instantly (no SDL_Delay), like
// fadeToBlack/showSpeech/0x16d. Returns false if the caller should stop looping
// (quit requested, or an area change was queued by the script).
bool RunProg::pumpFrame() {
    Anim::tick();
    Theme::advance();
    Anim::drawAll(fb_);
    disp_.present(fb_);
    if (disp_.isRealtime()) {
        PumpResult pr = disp_.pump();
        if (pr == PumpResult::Quit) { quit_ = true; }
        SDL_Delay(33);
    }
    if (quit_) { return false; }
    if (!nextArea_.empty()) { return false; }   // script requested an area transition
    return true;
}

void RunProg::exec(const Scene& scene, int progId, int /*nId*/) {
    scene_ = &scene;
    const ScriptProgram* prog = scene.program(progId);
    if (!prog) { Log::error("RunProg: no program %d", progId); return; }

    int count = (int)prog->insns.size();
    Log::info("RunProg: exec program %d (%d insns)", progId, count);

    // Opcode trace (RP_TRACE=1): log every instruction as it executes.
    static const bool trace = std::getenv("RP_TRACE") != nullptr;

    // Script-local value stack (ops 0x173 PUSH / 0x174 POP). Local to this exec
    // invocation, so a GOSUB'd subroutine gets its own — matching the engine,
    // where local_708/iStack_594 are stack locals of RunProg_Exec.
    int valStack[100];
    int sp = 0;

    // Speech-variant countdown (ops 0xca/0xcb). 0xca seeks into a table of
    // equal-size variant blocks and sets this to (step+1); it decrements once per
    // instruction (the +1 absorbs 0xca's own cycle), and when it expires the rest
    // of the block is skipped forward to its 0xcb terminator. Mirrors local_710.
    int speechPlay = 0;

    // RESTART_SCRIPT (0x3b) restart counter — bounds the headless restart loop so
    // a per-frame poll that never satisfies its exit condition can't hang.
    int restartCount = 0;

    for (int pc = 0; pc < count && !quit_; ++pc) {
        const ScriptInsn& in = prog->insns[pc];
        if (trace) {
            Log::info("RP %s prog%d pc=0x%x, op=0x%x, a0=0x%x, a1=0x%x, a2=0x%x",
                      scene_->name(), progId, pc, in.op, in.a0, in.a1, in.a2);
        }
        switch (in.op) {

        // -- no-op / separator --
        case 0xff:
        case 0x100: break;                                  // padding/no-op (0x100 shares 0xff)

        // -- variables --
        case 0x04:  var(in.a0) = in.a1; break;            // SET_VAR
        case 0x05:  var(in.a0) += 1;    break;            // INC_VAR
        case 0x06:  var(in.a0) -= 1;    break;            // DEC_VAR

        // -- SET_VAR_RAND (RunProg_Exec @0x00462560, case 0x1c): seed a variable with
        //    a pseudo-random value in [0,a1). Engine: var[a0] = (timeGetTime()*rand()) % a1,
        //    all unsigned 32-bit. SDL_GetTicks() is the ms-counter analog of timeGetTime();
        //    std::rand() stands in for the CRT rand(). Guard a1==0 to avoid div-by-zero. --
        case 0x1c: {
            if (in.a1 != 0) {
                uint32_t r = (uint32_t)SDL_GetTicks() * (uint32_t)std::rand();
                var(in.a0) = (int)(r % (uint32_t)in.a1);
            } else {
                var(in.a0) = 0;
            }
            break;
        }

        // -- AREA_NODE_DISABLE (RunProg_Exec @0x00462560, case 0x7): for every
        //    area-node tagged a0, clear its enabled byte (bits 8-15 of flags p[4],
        //    record offset 0x11) to 0, then Win_UpdateCursor(). The engine only
        //    rewrites nodes whose byte is currently non-zero and raises a "cursor
        //    dirty" flag (local_120) when any changed. The port hit-test (Area.cpp)
        //    treats byte 0x11 == 0 as clickable, so this makes the matching nodes
        //    hit-testable. The cursor is re-resolved per frame by the area loop, so
        //    no explicit cursor refresh is needed here. --
        case 0x07:  Area::setEnabledByteByTag(in.a0, 0); break;

        // -- AREA_NODE_ENABLE (RunProg_Exec @0x00462560, case 0x8): inverse of 0x7 —
        //    for every area-node tagged a0 whose enabled byte (bits 8-15 of p[4])
        //    isn't already 1, set it to 1, then Win_UpdateCursor(). Per the engine's
        //    inverted polarity (hit-test = byte 0), this makes the matching nodes
        //    NON-clickable. --
        case 0x08:  Area::setEnabledByteByTag(in.a0, 1); break;

        // -- LINKFULL (RunProg_Exec @0x00462560 case 0x169 flags=0 / case 0x2c3 flags=1):
        //    register the current STANI anim slot (curAnimSlot_, set by 0x3f) as a dynamic
        //    clickable area whose hit-rect is the anim's painted frame bbox, resolving to the
        //    node whose tag == a0 (func 0x0040116d = Area_FindNodeByTag). Appends to
        //    g_anAreaSpriteList with offX/offY = -1 (so the rect comes straight from the anim).
        //    The engine also flags the anim slot 0x1000 (moving-area marker, used for redraw)
        //    and fatally asserts a current STANI slot; the port doesn't need the marker and
        //    guards instead of asserting. --
        case 0x169:
        case 0x2c3:
            Area::linkFull(curAnimSlot_, in.a0, in.op == 0x169 ? 0 : 1);
            break;

        // -- IF guards: skip the following block when the condition holds --
        case 0x09:  if (var(in.a0) <= in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_LE
        case 0x0a:  if (var(in.a0) != in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_NE
        case 0x0b:  if (in.a1 <= var(in.a0)) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_GE
        case 0x0e:  if (var(in.a0) == in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_EQ
        // -- IF_SPEECH_NOT_IN_RANGE (RunProg_Exec @0x00462560, case 0x200): skip the
        //    following block when var[a0] is OUTSIDE the inclusive range [a1,a2].
        //    Disasm guard: `g_anSpeechPlayed[a0] < a1 || a2 < g_anSpeechPlayed[a0]`
        //    (g_anSpeechPlayed is the variable file == var()). The skip walk is the
        //    standard one shared with 0x09/0x0e (stops at matching ENDIF or ELSE). --
        case 0x200: if (var(in.a0) < in.a1 || in.a2 < var(in.a0)) pc = skipBlock(*prog, pc, true); break;  // IF_SPEECH_NOT_IN_RANGE
        case 0x0f:  break;                                            // ENDIF (no-op)
        case 0x10:  pc = skipBlock(*prog, pc, false); break;          // ELSE -> skip to ENDIF

        // -- value stack: PUSH var a0 / POP into var a0 (script-local, max 100) --
        case 0x173:                                         // PUSH
            if (sp < 100) { valStack[sp++] = var(in.a0); }
            else { Log::warn("RunProg: value-stack overflow (0x173)"); }
            break;
        case 0x174:                                         // POP
            if (sp > 0) { var(in.a0) = valStack[--sp]; }
            else { Log::warn("RunProg: value-stack underflow (0x174)"); }
            break;
        case 0x183:                                         // STACK_SUB: a-b (pop b, leave a-b)
            if (sp >= 2) { int b = valStack[--sp]; valStack[sp - 1] -= b; }
            else { Log::warn("RunProg: value-stack underflow (0x183)"); }
            break;

        // -- REGISTER_FKEY: bind keyboard shortcut a1 -> command a0 (Adv_RunScene's
        //    F-key table DAT_00629980/DAT_00629ef0, max 15). The op only registers;
        //    F-key dispatch is separate input handling, not built yet. --
        case 0x179:
            if (fkeyCmd_.size() < 15) {
                fkeyCmd_.push_back(in.a0);
                fkeyKey_.push_back(in.a1);
            } else {
                Log::warn("RunProg: F-key registry full (0x179)");
            }
            break;

        // -- IF_OBJECT_NOT_IN_LIST: run the guarded block only if object a0 is in
        //    the scene's node/object presence list; skip it otherwise. (The list is
        //    populated by the object-display ops, not ported yet — so it's empty and
        //    the block is skipped, the truthful "object absent" path.) --
        case 0x6f: {
            bool inList = false;
            for (int id : objectList_) { if (id == in.a0) { inList = true; break; } }
            if (!inList) { pc = skipBlock(*prog, pc, true); }
            break;
        }

        // -- SPEECH_SKIP: pick the speech/anim variant (var a0 - a1), each `a2`
        //    instructions, from the block ending at 0xcb. Jump to that variant; the
        //    speechPlay countdown then skips the rest to 0xcb after a2 instructions. --
        case 0xca: {
            int step = (in.a2 != 0) ? in.a2 : 1;
            int variant = var(in.a0) - in.a1;
            if (variant < 0) { variant = 0; }
            int cb = pc + 1;
            while (cb < count && prog->insns[cb].op != 0xcb) { ++cb; }
            int target = (pc + 1) + variant * step;         // first instr of the variant
            if (target >= cb) {                             // variant past the block
                pc = cb;                                    // for ++ steps past 0xcb
                speechPlay = 0;
            } else {
                pc = target - 1;                            // for ++ lands on `target`
                speechPlay = step + 1;
            }
            break;
        }
        case 0xcb:  speechPlay = 0; break;                  // END_SPEECH_BLOCK (terminator)

        // -- START_SPEECH: look the line up in SENTENCE.BIN by its snd-table id and
        //    display the Hebrew subtitle for its duration (blocking, skippable). The
        //    speech AUDIO isn't ported; the wait/end ops below become no-ops. --
        case 0xcd: {
            const std::string& key = scene_->soundName(in.a0);
            const std::string* heb = key.empty() ? nullptr : Sentence::lookup(key.c_str());
            Log::info("START_SPEECH snd[%d]='%s' -> %s", in.a0, key.c_str(),
                      heb ? Sentence::cp1255ToUtf8(*heb).c_str() : "(no subtitle)");
            if (heb != nullptr) { showSpeech(*heb); }
            break;
        }
        case 0xce:  break;  // WAIT_SPEECH: the wait already happened in 0xcd's hold

        // -- SHOW_TEXT_WAIT (RunProg_Exec @0x00462560, case 0x50): speak the line for
        //    character a0 and block until it finishes. The engine looks the speaker's
        //    sentence id up in the per-character speech table (_DAT_0070dec4[a0]) and
        //    calls SndMem_SpeakAndWait (SOUNDMEM thunk @0x00474600 -> SndMem_StartSpeech
        //    + SndMem_WaitSpeech), then RunProg_TrackSound(1). It is guarded by the
        //    loop's in-progress flag (local_130) and, when the async-dialog mode is
        //    active (op 0x131 sets local_148), kicks one async frame and latches the
        //    in-progress state; on a plain blocking line that async branch is skipped.
        //    Reads only a0 (the speaker index) — a1/a2 are unused, and it does NOT
        //    touch the current-anim register (DAT_007c4108 / curAnimSlot_).
        //    a0 indexes the same per-character speech-line table that 0xcd uses, so the
        //    port mirrors START_SPEECH (0xcd): look the line up in SENTENCE.BIN by its
        //    snd-table id and display the Hebrew subtitle for its duration (blocking,
        //    skippable). The speech AUDIO, lipsync and async-tick mode aren't ported
        //    (see the 0xcd/0x16d/0x131 notes), so those parts are faithful no-ops.
        case 0x50: {
            const std::string& key = scene_->soundName(in.a0);
            const std::string* heb = key.empty() ? nullptr : Sentence::lookup(key.c_str());
            Log::info("SHOW_TEXT_WAIT snd[%d]='%s' -> %s", in.a0, key.c_str(),
                      heb ? Sentence::cp1255ToUtf8(*heb).c_str() : "(no subtitle)");
            if (heb != nullptr) { showSpeech(*heb); }
            break;
        }

        // -- PLAY_SOUND (RunProg_Exec @0x00462560 case 0x15): play soundName(a0) as a looping
        //    room effect. Engine: Fx_PlayChar -> SndMem_ReadSound + Mixer_PlayChannel(3),
        //    re-armed each time the channel drains (FX.cpp), then RunProg_TrackSound(3). Sound
        //    resources are type-32 raw 8-bit-unsigned mono PCM @22050 with no header, which
        //    Audio::queue consumes directly (fmt=0). We loop it on the spare SFX channel; it's
        //    cleared on area change and by the stop ops (0x16f Fx_StopLoop / 0x132 stop-tracked).
        //    The engine's subtitle-only / skip-mode guard isn't modeled (the port has audio). --
        case 0x15: {
            const std::string& key = scene_->soundName(in.a0);
            if (!key.empty()) {
                const ResEntry* e = nullptr;
                for (const auto& en : arc_.entries()) {
                    if (en.type == 32 && strcasecmp(en.name.c_str(), key.c_str()) == 0) { e = &en; break; }
                }
                if (e != nullptr) {
                    std::vector<uint8_t> snd = arc_.read(*e);
                    if (!snd.empty()) {
                        Audio::clearChannel(Audio::SFX);
                        Audio::queue(Audio::SFX, snd.data(), snd.size(), 0);  // 8-bit unsigned mono 22050
                        Audio::setLoop(Audio::SFX, true);
                    }
                }
                Log::info("PLAY_SOUND snd[%d]='%s' (looping ch%d, %s)", in.a0, key.c_str(),
                          Audio::SFX, e ? "playing" : "no type-32 resource");
            }
            break;
        }

        // -- FX_STOP_LOOP (case 0x16f -> Fx_StopLoop) / STOP_TRACKED (case 0x132 ->
        //    RunProg_StopAndClearTrackedSounds): silence the looping room SFX. The SFX channel
        //    is the only tracked audio the port actually plays (voice/speech is subtitle-only),
        //    so clearing it covers the tracked-stop. 0x132 also ends a blocking-anim sequence +
        //    finalises the skip path, which the port doesn't model (cf. 0x131/0x134). --
        case 0x16f:
        case 0x132:
            Audio::clearChannel(Audio::SFX);
            break;

        // -- SCHED fade-to-black: smoothly ramp the active palette to black over 64
        //    steps (a1 = ms/step). A screen fade-out transition. --
        case 0x204:  fadeToBlack(in.a1); break;

        // -- flow --
        case 0x70:  pc = count; break;                      // END_SCRIPT
        case 0x26a: pc = count; break;                      // BREAK_LOOP: end (engine also unwinds GOSUBs)

        // -- RESTART_SCRIPT (RunProg_Exec @0x00462560, case 0x3b, src ops_00_3f.inc):
        //    abandon the rest of this program and re-enter the interpreter with
        //    program a0. The engine does `param_1 = arg0; goto LAB_00462616` — an
        //    ITERATIVE re-entry that reuses the same stack frame (NOT a native call),
        //    so a self-restarting poll (e.g. vvk/prog4: poll an anim's frame, restart
        //    until it reaches 8) must be a LOOP here, not a recursive exec() call —
        //    recursion would overflow the C++ stack. We reload prog/count and reset
        //    pc to the start of the dispatch loop. (Adv_CompactInvList on a dirty
        //    inventory is skipped — no inventory yet.)
        //    The engine's loop calls Adv_Tick()+Timer_DispatchAsyncProg() once per
        //    iteration (RUNPROG.cpp:266) so anims advance and the poll can exit; our
        //    exec advances no frames on its own, so we PUMP ONE PACED FRAME per
        //    restart (pumpFrame): tick anims, render+present, pump input, and honor
        //    quit / area-change so the loop can terminate. Headless has no operator,
        //    so cap the restarts to avoid hanging an automated run. --
        case 0x3b: {
            const ScriptProgram* next = scene.program(in.a0);
            if (next == nullptr) {
                Log::error("RunProg: RESTART_SCRIPT to missing program %d (0x3b)", in.a0);
                pc = count;
                break;
            }
            if (!pumpFrame()) { pc = count; break; }   // quit / area-change → stop
            if (!disp_.isRealtime() && ++restartCount >= kHeadlessRestartCap) {
                Log::warn("RunProg: RESTART_SCRIPT hit headless cap (%d) restarting prog %d "
                          "— stopping (poll never satisfied)", kHeadlessRestartCap, in.a0);
                pc = count;
                break;
            }
            prog = next;                               // reload the target program
            progId = in.a0;                            // keep the trace label accurate
            count = (int)prog->insns.size();
            pc = -1;                                   // the for-loop's ++pc lands on 0
            break;
        }

        // -- GOSUB (CALL_SCRIPT): run subroutine program arg0 to completion, then
        //    continue after this instruction. Subroutines are separate programs in
        //    the same scene; vars_/playlist_ persist (members). The engine pushes a
        //    call frame and restarts; here the C++ stack is the call stack (the
        //    engine caps depth at 100, so recursion is safe). --
        case 0x65:  exec(scene, in.a0); break;

        // -- INVCHAIN: record the next area (exit-name table[arg0]); script continues.
        //    The engine sets a transition flag; we surface the target to the area loop.
        case 0x03: {
            const std::string& dst = scene_->exitName(in.a0);
            if (!dst.empty()) nextArea_ = dst;
            break;
        }

        // -- ADD_CHAR: prepend an SCM to the pending playlist (Player_ScmAddChar). --
        case 0x77:
        case 0x78: {
            const std::string& nm = scene_->scaScm(in.a0);
            if (!nm.empty()) playlist_.insert(playlist_.begin(), nm);
            break;
        }

        // -- RUN_SCENE: add the base SCM then play the whole queue, then reset
        //    (Player_PlayScm -> ScmAddChar -> ScmPlayList -> ScmInit). --
        case 0x6c:
        case 0x71: {
            const std::string& nm = scene_->scaScm(in.a0);
            if (!nm.empty()) playlist_.insert(playlist_.begin(), nm);
            // 0x78/0x71 prepend (Player_ScmAddChar inserts at slot 0). Player_ScmPlayList
            // pops via Player_GetNextScmName from the TOP index downward, which undoes the
            // prepend and replays in original script/add order. So iterate back-to-front.
            for (size_t i = playlist_.size(); i-- > 0; ) {
                if (!playScmByName(arc_, disp_, fb_, playlist_[i].c_str())) { quit_ = true; break; }
            }
            playlist_.clear();
            break;
        }

        // -- ANIM_ADD_FROZEN_GROUPED: load anim arg0 (by name-table index), frozen at
        //    frame 0. Anim_AddByNum(num,loop=1) + Anim_Freeze + SetCurrentFrame(0). --
        case 0x13ba: {
            const std::string& nm = scene_->animName(in.a0);
            if (!nm.empty()) { Anim::addByName(arc_, nm.c_str(), /*looping*/true, /*frozen*/true); }
            break;
        }

        // -- LOAD_ANIM (RunProg_Exec @0x00462560 case 0x1): add anim animName(a0) NON-looping,
        //    not frozen — Anim_AddByNum(a0,0,0) + Anim_SetWalkTableBase(slot,a1). a1 is the
        //    walk-table base (unused by our renderer, cf. 0x19/0x137); the skip-mode branch
        //    (GI_SetDrawMode(0)+draw) isn't modeled. Non-looping sibling of 0x19. --
        case 0x1: {
            const std::string& nm = scene_->animName(in.a0);
            if (!nm.empty()) { Anim::addByName(arc_, nm.c_str(), /*looping*/false, /*frozen*/false); }
            break;
        }

        // -- LOAD_ANIM_LOOPING: add anim animName(a0) looping (not frozen). a1 is the
        //    walk-table base (unused by our renderer). Sibling of 0x13ba. --
        case 0x19: {
            const std::string& nm = scene_->animName(in.a0);
            if (!nm.empty()) { Anim::addByName(arc_, nm.c_str(), /*looping*/true, /*frozen*/false); }
            break;
        }

        // -- STORE_ANIM_SLOT / "STANI" (RunProg_Exec @0x00462560, case 0x3f): select
        //    the anim slot named animName(a0) and stash it in the STANI register
        //    DAT_007c4108 (curAnimSlot_) for later named-anim ops. The engine special-
        //    cases the literal name "this" (FUN_0049a830 == case-insensitive stricmp,
        //    returns 0 on match): when the table entry is "this" it reuses local_a68,
        //    the most-recently-added anim slot; otherwise it does
        //    Anim_FindSlotByName(a0) (== Anim::findByName). On miss the engine logs
        //    "STANI couldn't find ani %s" and the register stays -1. a1/a2 are unused.
        //    The port has no persistent "last-added slot" tracker, so for the "this"
        //    case we approximate local_a68 with the highest active slot (last add
        //    order); the common path (a0 a real name index, e.g. args 8,0,0) uses
        //    findByName and is exact. --
        case 0x3f: {
            const std::string& nm = scene_->animName(in.a0);
            if (strcasecmp(nm.c_str(), "this") == 0) {
                curAnimSlot_ = -1;
                for (int s = Anim::MAX_SLOTS - 1; s >= 0; --s) {
                    if (Anim::active(s)) { curAnimSlot_ = s; break; }
                }
            } else {
                curAnimSlot_ = Anim::findByName(nm.c_str());
            }
            if (curAnimSlot_ == -1) {
                Log::warn("RunProg: STANI couldn't find ani '%s' (0x3f)", nm.c_str());
            }
            break;
        }

        // -- FREE_ANIM: free the anim slot whose name = animName(arg0). --
        case 0x13: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::freeSlot(slot); }
            break;
        }

        // -- ANIM_RESET_FREEZE: fully unfreeze the anim named animName(arg0)
        //    (Anim_ResetFreeze sets its freeze count to 0 so it animates again). --
        case 0x195: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::resetFreeze(slot); }
            break;
        }

        // -- ANIM_FREEZE_ALL (Anim_FreezeAll @0x00407230, dispatcher calls func_0x004012ee
        //    thunk with no operands): freeze every active anim slot — the inverse of
        //    0x194. The engine loops all 0x96 slots, freezing those whose active/visible
        //    flag bits are set, then fires tick-callbacks(0) + Timer_Tick (not ported, as
        //    with 0x194). a0/a1/a2 are unused — the args in the bytecode (incl. the garbage
        //    a0=0x65766100) are read by the decoder but ignored by this op. --
        case 0x193:
            for (int s = 0; s < Anim::MAX_SLOTS; ++s) {
                if (Anim::active(s)) { Anim::freeze(s); }
            }
            break;

        // -- ANIM_UNFREEZE_ALL (Anim_UnfreezeAll @0x00407380): resume every active anim
        //    slot. (Engine also fires tick-callbacks(1) + Timer_Untick — not ported.) --
        case 0x194:
            for (int s = 0; s < Anim::MAX_SLOTS; ++s) {
                if (Anim::active(s)) { Anim::resetFreeze(s); }
            }
            break;

        // -- WAIT_FRAME (RunProg_Exec @0x00462560 case 0x13b): resolve slot = animName(a0)
        //    (or the current "this" slot), `Anim_SetStopFrame(slot, a1)`, then busy-wait —
        //    `Adv_Tick` + `Timer_DispatchAsyncProg` each iteration — until the anim reaches
        //    frame a1; a right-click during a blocking anim aborts into skip mode. Out-of-
        //    range a1 is a debug no-op ("Frame %d out of range").
        //    The port's VM runs a whole program to completion without yielding to the render
        //    loop, so the real-time play-to-frame and the right-click abort can't be
        //    reproduced (same limit that makes 0x1f/0x134 no-ops). We apply the observable
        //    resting state the engine leaves once the stop frame is hit: advance the anim to
        //    frame a1 and hold it there. setCurrentFrame clamps a1 to the frame range, and
        //    freeze stops the render loop from advancing past it (0x13d/UNFREEZE resumes). --
        case 0x13b: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = (strcasecmp(nm.c_str(), "this") == 0) ? curAnimSlot_
                                                             : Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::setCurrentFrame(slot, in.a1); Anim::freeze(slot); }
            break;
        }

        // -- FREEZE_ANIM: freeze animName(a0) at frame a1 (SetCurrentFrame + stop). --
        case 0x13c: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::setCurrentFrame(slot, in.a1); Anim::freeze(slot); }
            break;
        }

        // -- UNFREEZE_ANIM: set the anim's frame-step to play (+ clear trigger frame);
        //    like 0x195, makes animName(a0) animate again. --
        case 0x13d: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::resetFreeze(slot); }
            break;
        }

        // -- ANIM_FREEZE: freeze the anim named animName(arg0) (inverse of 0x195). --
        case 0x191: {
            const std::string& nm = scene_->animName(in.a0);
            int slot = Anim::findByName(nm.c_str());
            if (slot >= 0) { Anim::freeze(slot); }
            break;
        }

        // -- PLAY_MUSIC: load+start the theme track thm[arg0] (Thm_Play). A dedup
        //    guard skips replaying the track that's already selected. --
        case 0x14: {
            const std::string& nm = scene_->themeName(in.a0);
            if (!nm.empty() && strcasecmp(nm.c_str(), lastMusic_.c_str()) != 0) {
                lastMusic_ = nm;
                Theme::play(nm.c_str(), nullptr);
            }
            break;
        }

        // -- STOP_MUSIC: Theme_StopMusic + clear the dedup guard. --
        case 0x1a:
            Theme::stopMusic();
            lastMusic_.clear();
            break;

        // -- THEME_MUSIC_EVENT (RunProg_Exec @0x00462560, case 0x16c): fire a named
        //    music-transition event against the current track. Reads the same name
        //    table (_DAT_0070dec0 == thm table) and the same dedup buffer (DAT_00629880)
        //    as PLAY_MUSIC (0x14); the only difference is it calls Theme_MusicEvent
        //    (THEMES.cpp @0x00479c20) instead of Thm_Play. The engine uppercases the
        //    name (FUN_0049def0) then strcmp-dedups; we case-insensitively dedup against
        //    lastMusic_ (== DAT_00629880, shared with 0x14), matching that behavior.
        //    The engine also gates on subtitle-only mode; we have no such global, and
        //    Theme::musicEvent already returns early when the system isn't ready. --
        case 0x16c: {
            const std::string& nm = scene_->themeName(in.a0);
            if (!nm.empty() && strcasecmp(nm.c_str(), lastMusic_.c_str()) != 0) {
                lastMusic_ = nm;
                Theme::musicEvent(nm.c_str());
            }
            break;
        }

        // -- ADV_TICK_FRAMES (RunProg_Exec @0x00462560, case 0x16d): synchronously
        //    advance the adventure engine a1 display frames. The engine calls
        //    Adv_TickFramesNoAsync(a1) (ADV.cpp @0x00412cc0), guarded by the loop's
        //    right-click abort flag (local_130); a0/a2 are ignored — only a1 (the
        //    frame count) is read. Adv_TickFramesNoAsync itself loops Adv_Tick() once
        //    per frame, gated on g_nAdvTickSuppressed == 0.
        //    The port doesn't own an Adv_Tick scene compositor inside RunProg (the
        //    menu loop in main.cpp composes frames; here we only drive anims/audio
        //    directly), and the abort flag can only be raised by an in-loop right-click
        //    we don't model, so it's always clear here. Faithful analog of one frame:
        //    tick animations + present, pumping events to honor quit. Headless settles
        //    instantly (matching fadeToBlack/showSpeech).
        case 0x16d:
            if (disp_.isRealtime()) {
                for (int f = 0; f < in.a1 && !quit_; ++f) {
                    Anim::tick();
                    Theme::advance();                       // keep room music streaming
                    disp_.present(fb_);
                    if (disp_.pump() == PumpResult::Quit) { quit_ = true; break; }
                    SDL_Delay(33);
                }
            }
            break;

        // -- STOP_SPEECH_TEXT (RunProg_Exec @0x00462560, case 0x17a): cut any
        //    in-progress speech line. The engine does three things and reads no
        //    operand (a0/a1/a2 are unused — a0's large value is just trailing
        //    instruction-stream bytes):
        //      Snd_Stop(1)         (SETPAL.cpp @0x0046fa60 -> Mixer_StopChannel(1)):
        //                          stop the speech voice channel.
        //      SndMem_StopLipsync  (SOUNDMEM.cpp @0x00474770): cancel the lipsync
        //                          anim slot and reset the lipsync cursor.
        //      Txt_Reset           (TEXT.cpp @0x004773b0): clear/restart the active
        //                          text block.
        //    Only the audio half has a ported analog: channel 1 is the first SCM
        //    voice channel (Audio::VOICE0), so clear it. Lipsync and the GDI text
        //    block aren't ported (see the 0xcd/0x12d/0x12e speech-no-op comments),
        //    so those two calls are faithful no-ops here.
        case 0x17a:
            if (Audio::isOpen()) { Audio::clearChannel(Audio::VOICE0); }
            break;

        // -- THEME_SET_ROOM: gate/cut the room music (Theme_SetRoom). --
        case 0x84e:
            Theme::setRoom(var(in.a0));
            break;

        // -- SET_AMBIENT_MUSIC: fade the current music out over a1 ms (Theme_FadeOut)
        //    and clear the play-music dedup guard. --
        case 0x1a8:
            Theme::fadeOut(in.a1);
            lastMusic_.clear();
            break;

        // -- WAIT music fade-out: block until Theme_IsFading() is false. Realtime
        //    drives the fade (advance + present pacing); headless finishes instantly. --
        case 0x1a7:
            if (disp_.isRealtime()) {
                while (Theme::isFading() && !quit_) {
                    Theme::advance();
                    if (disp_.pump() == PumpResult::Quit) { quit_ = true; break; }
                    SDL_Delay(33);
                }
            } else if (Theme::isFading()) {
                Theme::stopMusic();
            }
            break;

        // -- MASTER_VOLUME: clamp var a0 to 0..10 (writing it back) and set the
        //    master mixer volume (Mixer_SetMasterVolume): 0 -> -10000 mB (mute),
        //    else (v-10)*300 mB. --
        case 0x84d: {
            int v = var(in.a0);
            if (v < 0) { v = 0; }
            if (v > 10) { v = 10; }
            var(in.a0) = v;
            Audio::setMasterVolumeMillibels(v == 0 ? -10000 : (v - 10) * 300);
            break;
        }

        // -- SET_FLAG_IS_ZERO: g_nTheme... no — DAT_00629f54 = (var a0 == 0).
        //    A script condition flag (read back by op 0x857). --
        case 0x84f:
            scriptFlag_ = (var(in.a0) == 0) ? 1 : 0;
            break;

        // -- TXT_SET_MODE: text subsystem mode/enable (Txt_SetMode). --
        case 0x852:
            Text::setMode(var(in.a0));
            break;

        // -- SET_GAMMA: Sched_SetGamma(var a0) + re-realize the palette
        //    (SetPal_WaitOrRealizeIfNeeded). --
        case 0x853:
            Palette::setGamma(fb_, var(in.a0));
            break;

        // -- SHOW_ENTRY (RunProg_Exec @0x00462560, case 0x8fd, handler @0x0046809b):
        //    build an inventory-resource name "entry%d%d%d" from the top 3 value-stack
        //    entries, then load it as an anim slot via Anim_LoadByName (0x0040b0c0:
        //    Anim_AddByName + set walk-table Z-column = a0 + flags). The engine reads
        //    iStack_594[count], [count-1], [count-2] (count = next-free index) and pops 3.
        //    Needs >= 3 entries (else the engine logs "Not enough data on the stack to
        //    show" via Err_BadResEntry and reads past the top); we require it. The
        //    Z-column (a0) tunes the walk-table column, which our flat renderer (no Z
        //    sorting / walk tables) ignores, so a0 is unused here. --
        case 0x8fd: {
            if (sp < 3) {
                Log::warn("RunProg: not enough data on the stack to show (0x8fd, sp=%d)", sp);
                break;
            }
            // Engine: sprintf(buf, "%s%d%d%d", "entry", iStack_594[count-2], [count-1],
            // [count]). [count] is the next-free slot (push writes [count] then count++),
            // so the engine's 3rd %d reads one past the top — stale memory, an off-by-one.
            // We substitute the 3rd-from-top valid entry ([sp-3]) so the loaded resource
            // name is deterministic from the three values the script actually pushed.
            char name[64];
            std::snprintf(name, sizeof name, "entry%d%d%d",
                          valStack[sp - 2], valStack[sp - 1], valStack[sp - 3]);
            sp -= 3;
            Anim::addByName(arc_, name, /*looping*/true, /*frozen*/false);
            break;
        }

        // -- GV (verb/inventory toolbar) state. Toolbar rendering isn't ported, so
        //    these write GV state for a future toolbar (like 0x901/0x903). --
        case 0x918:  Gv::setEnabled(true);           break;  // GV_SetEnabled(1)
        case 0x9c6:  Gv::setDestroyHandler(in.a0);   break;  // GV_SetDestroyHandler(a0)
        case 0x91c:  break;  // GV_RedrawInventory: redraws the GV window only if it's
                             // open; the toolbar window isn't rendered -> no-op

        // -- CD_CHANGE_MODE_INIT: the part that matters for visuals is loading the
        //    GENERAL UI palette (GI_LoadGeneralPal); the rest is cursor/anim/speech
        //    setup we handle elsewhere or don't need. GENERAL is a sparse overlay. --
        case 0x1004:
            Palette::load(arc_, fb_, "GENERAL", /*nonBlackOnly*/true);
            break;

        // -- FILES_LOAD_PAL: load the scene palette paletteName(a0) (Files_LoadPal into
        //    the target palette) + enable draw. The port realizes immediately. --
        case 0x19a: {
            const std::string& nm = scene_->paletteName(in.a0);
            if (!nm.empty()) { Palette::load(arc_, fb_, nm.c_str(), /*nonBlackOnly*/false); }
            break;
        }

        // -- PAL_SNAPSHOT / PAL_RESTORE: save/restore the live palette. --
        case 0x12f:
            std::memcpy(snapPal_, fb_.palette(), sizeof snapPal_);
            break;
        case 0x130:
            fb_.setPaletteRGB(snapPal_);
            break;

        // -- GV_AddButton: register a toolbar button (0x901 = one, 0x903 = all).
        //    We don't render the GV toolbar yet, so this is tracked as a no-op. --
        case 0x901: case 0x903:  break;

        // -- TXT_SET_ALIGN: set horizontal text alignment to variable a0's value
        //    (Txt_SetAlign: 0=left, 1=center, 2=right). --
        case 0x851:
            Text::setAlign(var(in.a0));
            break;

        // -- BEGIN_ANIM_GROUP (RunProg_Exec @0x00462560, case 0x6b): start defining an
        //    animation group. The engine stores a1 (member count) into the "pending
        //    group members" counter DAT_00574bec, then calls Anim_StartGroup(a1, a2)
        //    (Advanim.cpp @0x00409260): it opens a new slot in g_anGroupSize[g_nGroupCount]
        //    = max(a1,1) and g_anGroupTriggerPct[g_nGroupCount] = a2 (the trigger
        //    percentage), and marks the group's active slot -1. The following anim-add
        //    ops (e.g. 0x19/0x13ba) each call Anim_AddToGroup and decrement DAT_00574bec
        //    until a1 members are registered. a0 is unused.
        //    The group subsystem (Anim_StartGroup/Anim_AddToGroup + the sprite-trigger
        //    tables read by Tushtush/RESCALE) isn't ported — our Anim has no group concept
        //    and the corresponding anim-add ops don't register members — so there's
        //    nothing to track here: faithful no-op, like the timer/slider/callback ops.
        case 0x6b:    break;

        // -- ANIM_GET_CURRENT_FRAME (RunProg_Exec @0x00462560, case 0x156): store the
        //    current frame index of the "current" anim slot (DAT_007c4108 / curAnimSlot_,
        //    set by 0x3f/STANI) into variable register a0. The engine reads
        //    Anim_GetCurrentFrame(curAnimSlot_) (@0x004019fb), which returns the slot's
        //    curFrame only when the slot is active and not frozen, else -1; and if
        //    curAnimSlot_ itself is < 0 it stores -1 directly. a1/a2 are unused. --
        case 0x156:
            var(in.a0) = (curAnimSlot_ < 0) ? -1 : Anim::getCurrentFrame(curAnimSlot_);
            break;

        // -- SLIDER ops (animated UI sliders; Slider.cpp ports ../src/SLIDER.cpp) --
        //    Each maps 1:1 to a Slider:: call. var(a0) is the slider id (or, for
        //    SET_VALUE, the value). The engine stores ids in g_anSpeechPlayed[a0]
        //    (== var(a0)) and resolves -1 to the "current" slider.

        // -- SLIDER_ADD (RunProg_Exec @0x00462560 case 0x19d): var(a0) =
        //    Slider_Add(curAnimSlot_, a1). a1 = flags (bit0 = vertical). The engine
        //    asserts a current STANI slot; the port guards curAnimSlot_ >= 0. --
        case 0x19d:
            if (curAnimSlot_ >= 0) {
                var(in.a0) = Slider::add(curAnimSlot_, (unsigned int)in.a1);
            } else {
                Log::warn("RunProg: SLIDER_ADD with no current anim slot");
                var(in.a0) = -1;
            }
            break;

        // -- SLIDER_REMOVE (case 0x19e): Slider_Remove(var a0). --
        case 0x19e:   Slider::remove(var(in.a0)); break;

        // -- SLIDER_SET_CURRENT (case 0x19f): Slider_SetCurrent(var a0). --
        case 0x19f:   Slider::setCurrent(var(in.a0)); break;

        // -- SLIDER_SET_POSITION (case 0x1a0): Slider_SetPosition(var a0, a1, a2). --
        case 0x1a0:   Slider::setPosition(var(in.a0), in.a1, in.a2); break;

        // -- SLIDER_SET_PIXEL_RANGE (case 0x1a1): Slider_SetPixelRange(var a0, a1, a2). --
        case 0x1a1:   Slider::setPixelRange(var(in.a0), in.a1, in.a2); break;

        // -- SLIDER_SET_VALUE_RANGE (case 0x1a2): Slider_SetValueRange(var a0, a1, a2). --
        case 0x1a2:   Slider::setValueRange(var(in.a0), in.a1, in.a2); break;

        // -- SLIDER_TRACK_CLICKED (case 0x1a3): var(a0) = Slider_TrackClicked(-1).
        //    Blocking in the engine (loops while the button is held); the port runs
        //    a real per-frame loop via pumpFrame() when realtime, else returns the
        //    real current value (see Slider::trackClicked). -1 = current slider. --
        case 0x1a3:
            var(in.a0) = Slider::trackClicked(-1, disp_, [this]() { return pumpFrame(); });
            break;

        // -- SLIDER_DRAG (case 0x1a4): var(a0) = Slider_Drag(-1). Same blocking/
        //    headless model as 0x1a3. -1 = current slider. --
        case 0x1a4:
            var(in.a0) = Slider::drag(-1, disp_, [this]() { return pumpFrame(); });
            break;

        // -- SLIDER_SET_MAX_STEP (case 0x1a5): Slider_SetMaxStep(var a0, a1). --
        case 0x1a5:   Slider::setMaxStep(var(in.a0), in.a1); break;

        // -- SLIDER_SET_VALUE (case 0x1a6): Slider_SetValue(-1, var a0). -1 = current
        //    slider; the value to set is var(a0). --
        case 0x1a6:   Slider::setValue(-1, var(in.a0)); break;

        // -- subsystem ops we haven't built yet: faithful no-ops --
        case 0x49:    break;  // BREAK_IF_SKIP: tick 1 anim frame; our render loop drives frames
        case 0x1f:    break;  // WAIT_ANIM_END: blocks until an anim's last frame; no mid-script ticking
        case 0x2c0:   break;  // g_nPalState=5 (fade-to-target): a request to the per-frame palette
                              // state machine (Anim_TickPalette), not ported — palettes realize directly
        case 0x18f:   break;  // Timer_Kill(a0): remove an async timer; the timer subsystem isn't
                              // ported (no timers ever registered), so this finds nothing -> no-op
        case 0x17e:   break;  // Timer_AddWithReset(a1 frames, a0): schedule a repeating async-prog
                              // callback; the timer/async-prog dispatch isn't ported yet -> no-op
        case 0x196:   break;  // Timer_AddAsync(a1, a0) (case 0x196 -> 0x0047e3a0): register a
                              // per-frame async timer firing program a0; same unported timer/
                              // async-prog dispatch as 0x17e/0x18f -> no-op
        case 0x905:   break;  // FILES_LOAD_DIALOG (case 0x905): open the Win32 load-game file
                              // dialog, and on a pick load the save + restart. The port has no
                              // save/load subsystem (no file dialog, no save format) -> no-op
        case 0x41:    break;  // REMOVE_AREA_SPRITE: unregister a sprite-area hotspot; we don't port
                              // the op-0x40 sprite-area registry, so there's nothing to remove
        case 0x15a:   break;  // Anim_SetCompletionCallback(animName(a0), none): clears an anim's
                              // completion callback; the port has no anim callbacks -> no-op
        case 0x159:   break;  // Anim_SetCompletionCallback(slot animName(a0)/"this", iRam007c4998,
                              // a1, 0xffffffff): arm a completion callback (run pending script id
                              // iRam007c4998 with arg a1 when the anim finishes). Same subsystem as
                              // 0x15a/0x3c/0x3d/0x3e — the port models no anim completion callbacks
                              // (the VM runs each program straight to completion), so there is no
                              // callback registry to arm -> faithful no-op.
        case 0x185:   break;  // Anim_SetCompletionCallback(slot animName(a0)/"this", iRam007c4998,
                              // a1, a2[ or frameCount+a2 if a2<-1]): same as 0x159 but fires the
                              // callback at a specific frame (a2) rather than at the very end. Same
                              // unported anim-completion-callback subsystem (cf. 0x159/0x15a) -> no-op.
        case 0x3c:    break;  // SET_CALLBACK_ID (case 0x3c): iRam007c4998 = a0 — stash the pending
                              // completion-callback script id. It is consumed only by 0x3d/0x3e
                              // (Anim_SetCompletionCallback(slot, id, ...): run program `id` when the
                              // anim finishes). The port models neither anim completion callbacks
                              // (cf. 0x15a) nor 0x3d/0x3e, so there's nothing to feed -> no-op.
        case 0x137:   break;  // SET_WALKTABLE (case 0x137): Anim_SetWalkTableBase(slot, a1) for
                              // slot animName(a0)/"this" — binds the anim to a character walk-table
                              // entry (per-char 0x58-byte record: depth/position). The port models
                              // neither the per-char walk table nor walk-table-driven depth/position
                              // (anim positions come from explicit ADD_ANIM/0x198 setPosition), so
                              // there's nothing to bind -> no-op (cf. 0x19's ignored a1 base).
        case 0x19b:   break;  // Anim_BeginNormalDraw (RunProg_Exec @0x00462560 case 0x19b ->
                              // thunk 0x004014ec -> 0x0040de00): GI_SetDrawMode(0) +
                              // GI_LockActiveSurf_v9(&DAT_005296f8). Twin of 0x19c — a DirectDraw
                              // draw-mode select + active-surface lock. The port renders to a flat
                              // framebuffer (no draw modes/locks), so faithful no-op (cf. 0x19a's
                              // dropped Anim_EnableDraw).
        case 0x19c:   break;  // Anim_FlushDraw (case 0x19c -> 0x0040df40): GI_SetDrawMode(0) +
                              // GI_LockActiveSurf_v10. Same as 0x19b; flat framebuffer -> no-op
        case 0x12d:   break;  // SPEECH_WAIT: tick frames while speech plays; no speech yet
        case 0x12e:   break;  // SPEECH_END: clears tick-suppress / restores cursor; no speech yet
        case 0x1838:  break;  // GRAN_INIT_TAPE
        case 0x157:   break;  // Bani_Noop(soundName(a0)): RunProg_Exec @0x00462560 calls
                              // Bani_Noop(*(_DAT_0070d558 + a0*4)); the thunk @0x00402036 JMPs to
                              // the real Bani_Noop @0x00417df0, which is an empty SEH-only stub
                              // (a compiled-out debug-trace, cf. magwrit.cpp Bani_Noop("...",..)).
                              // It produces NO speech/subtitle. (vvk/prog44's variant-block of 12
                              // 0xff slots is likewise no-op padding; that program speaks nothing in
                              // the engine. Subtitles come only from 0xcd -> SndMem_StartSpeech.)
        case 0x02:    break;  // empty `case 2: break;` in the engine (RunProg_Exec) -> no-op
        case 0x158:   break;  // engine no-op (paired with 0x1fd)
        case 0x850: case 0x855: case 0x856: case 0x857: case 0x858:
        case 0x84c:   break;  // text/speech timing ops
        case 0x170: case 0x172: case 0x1ab:  break;  // text/misc ops

        default:
            die(in, pc, progId);
        }

        // Speech-variant countdown: once the selected variant's instructions have
        // run, skip the remaining variants forward to the 0xcb terminator.
        if (speechPlay > 0 && --speechPlay == 0) {
            while (pc + 1 < count && prog->insns[pc + 1].op != 0xcb) { ++pc; }
            if (pc + 1 < count) { ++pc; }      // step onto 0xcb; the for ++ moves past it
        }
    }
}
