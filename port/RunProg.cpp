#include "RunProg.h"
#include "ScmPlayer.h"
#include "Audio.h"
#include "Anim.h"
#include "Palette.h"
#include "Text.h"
#include "Theme.h"
#include "Gv.h"
#include "Sentence.h"
#include "TextRender.h"
#include "Log.h"
#include <SDL.h>
#include <vector>
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

void RunProg::exec(const Scene& scene, int progId, int /*nId*/) {
    scene_ = &scene;
    const ScriptProgram* prog = scene.program(progId);
    if (!prog) { Log::error("RunProg: no program %d", progId); return; }

    const int count = (int)prog->insns.size();
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

    for (int pc = 0; pc < count && !quit_; ++pc) {
        const ScriptInsn& in = prog->insns[pc];
        if (trace) {
            Log::info("RP prog%d pc%-3d op=0x%-4x a0=%d a1=%d a2=%d", progId, pc, in.op, in.a0, in.a1, in.a2);
        }
        switch (in.op) {

        // -- no-op / separator --
        case 0xff:
        case 0x100: break;                                  // padding/no-op (0x100 shares 0xff)

        // -- variables --
        case 0x04:  var(in.a0) = in.a1; break;            // SET_VAR
        case 0x05:  var(in.a0) += 1;    break;            // INC_VAR
        case 0x06:  var(in.a0) -= 1;    break;            // DEC_VAR

        // -- AREA_NODE_ENABLE: toggles a flag byte (record offset 0x11) on area-nodes
        //    tagged a0 + refreshes the cursor. Our hit-test keys off a different
        //    enabled byte (0x12) and the cursor refreshes each frame, so no-op for
        //    now. TODO: reconcile the enabled-byte offset (0x11 vs 0x12). --
        case 0x08:  break;

        // -- IF guards: skip the following block when the condition holds --
        case 0x09:  if (var(in.a0) <= in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_LE
        case 0x0a:  if (var(in.a0) != in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_NE
        case 0x0b:  if (in.a1 <= var(in.a0)) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_GE
        case 0x0e:  if (var(in.a0) == in.a1) pc = skipBlock(*prog, pc, true); break;  // IF_VAR_EQ
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

        // -- SCHED fade-to-black: smoothly ramp the active palette to black over 64
        //    steps (a1 = ms/step). A screen fade-out transition. --
        case 0x204:  fadeToBlack(in.a1); break;

        // -- flow --
        case 0x70:  pc = count; break;                      // END_SCRIPT
        case 0x26a: pc = count; break;                      // BREAK_LOOP: end (engine also unwinds GOSUBs)

        // -- RESTART_SCRIPT: re-enter the interpreter with program a0, abandoning
        //    the rest of this one. (Adv_CompactInvList on a dirty inventory is
        //    skipped — no inventory yet.) --
        case 0x3b:
            exec(scene, in.a0);
            pc = count;
            break;

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

        // -- LOAD_ANIM_LOOPING: add anim animName(a0) looping (not frozen). a1 is the
        //    walk-table base (unused by our renderer). Sibling of 0x13ba. --
        case 0x19: {
            const std::string& nm = scene_->animName(in.a0);
            if (!nm.empty()) { Anim::addByName(arc_, nm.c_str(), /*looping*/true, /*frozen*/false); }
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

        // -- ANIM_UNFREEZE_ALL (Anim_UnfreezeAll @0x00407380): resume every active anim
        //    slot. (Engine also fires tick-callbacks(1) + Timer_Untick — not ported.) --
        case 0x194:
            for (int s = 0; s < Anim::MAX_SLOTS; ++s) {
                if (Anim::active(s)) { Anim::resetFreeze(s); }
            }
            break;

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

        // -- subsystem ops we haven't built yet: faithful no-ops --
        case 0x49:    break;  // BREAK_IF_SKIP: tick 1 anim frame; our render loop drives frames
        case 0x1f:    break;  // WAIT_ANIM_END: blocks until an anim's last frame; no mid-script ticking
        case 0x2c0:   break;  // g_nPalState=5 (fade-to-target): a request to the per-frame palette
                              // state machine (Anim_TickPalette), not ported — palettes realize directly
        case 0x18f:   break;  // Timer_Kill(a0): remove an async timer; the timer subsystem isn't
                              // ported (no timers ever registered), so this finds nothing -> no-op
        case 0x17e:   break;  // Timer_AddWithReset(a1 frames, a0): schedule a repeating async-prog
                              // callback; the timer/async-prog dispatch isn't ported yet -> no-op
        case 0x41:    break;  // REMOVE_AREA_SPRITE: unregister a sprite-area hotspot; we don't port
                              // the op-0x40 sprite-area registry, so there's nothing to remove
        case 0x19e:   break;  // Slider_Remove(var a0): clear a sound-channel slider; the slider/
                              // sound-channel subsystem isn't ported, so nothing to remove -> no-op
        case 0x15a:   break;  // Anim_SetCompletionCallback(animName(a0), none): clears an anim's
                              // completion callback; the port has no anim callbacks -> no-op
        case 0x19c:   break;  // Anim_FlushDraw: GI_SetDrawMode(0) + lock the DirectDraw surface; the
                              // port renders to a flat framebuffer (no draw modes/locks) -> no-op
        case 0x12d:   break;  // SPEECH_WAIT: tick frames while speech plays; no speech yet
        case 0x12e:   break;  // SPEECH_END: clears tick-suppress / restores cursor; no speech yet
        case 0x1838:  break;  // GRAN_INIT_TAPE
        case 0x157:   break;  // Bani_Noop(name): debug-trace stub (empty in engine)
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
