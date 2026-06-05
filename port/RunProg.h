// RunProg.h — the Crux script bytecode VM (RunProg_Exec, 0x00462560).
//
// A program is a flat list of 16-byte instructions {op, a0, a1, a2}. The VM
// fetches/decodes/dispatches in order; IF/ELSE/ENDIF use a forward skip-scan.
// g_anSpeechPlayed[1500] doubles as the general-purpose variable/register file
// and PERSISTS across areas, so it lives in the VM (not the per-area Scene).
//
// This is an INCREMENTAL clean-room port: only the opcodes the game actually
// reaches are implemented. Any unimplemented opcode is fatal (logs op/args/pc
// and aborts) so the build-run loop surfaces exactly what to implement next.
// Opcodes for subsystems we haven't built yet (cursor, text, anim, sound) are
// implemented as logged no-ops; control-flow and movie opcodes are real.
#pragma once
#include "Scene.h"
#include "ResArchive.h"
#include "Display.h"
#include "Framebuffer.h"
#include <string>
#include <vector>

class RunProg {
public:
    RunProg(ResArchive& arc, Display& disp, Framebuffer& fb)
        : arc_(arc), disp_(disp), fb_(fb), vars_(1500, 0) {}

    // Execute program `progId` of `scene` to completion.
    void exec(const Scene& scene, int progId, int nId = 0);

    bool quit() const { return quit_; }                  // user closed the window
    const std::string& nextArea() const { return nextArea_; }  // INVCHAIN target ("" = none)
    void clearTransition() { nextArea_.clear(); }

private:
    ResArchive&  arc_;
    Display&     disp_;
    Framebuffer& fb_;
    const Scene* scene_ = nullptr;   // current program's scene (for name tables / skip)

    // g_anSpeechPlayed — variable/register file (persists across areas). Heap-backed
    // (not a 6KB stack array) so RunProg stays a small stack object.
    std::vector<int> vars_;
    uint8_t snapPal_[768] = {0};          // palette snapshot for op 0x12f / 0x130
    bool quit_ = false;
    std::string nextArea_;
    std::vector<std::string> playlist_;   // SCM list built by 0x78, played+cleared by 0x71
    std::string lastMusic_;               // PLAY_MUSIC dedup guard (DAT_00629880)
    int  scriptFlag_ = 0;                 // DAT_00629f54: set by 0x84f, read by 0x857
    int  curAnimSlot_ = -1;               // DAT_007c4108: "STANI" register — anim slot
                                          // selected by op 0x3f for later named-anim ops
    std::vector<int> fkeyCmd_, fkeyKey_;  // 0x179: F-key shortcut -> command registry (max 15)
    std::vector<int> objectList_;         // scene node/object presence list (queried by 0x6f);
                                          // populated by the not-yet-ported object-display ops

    // Bounds-guarded access to the variable file. A script that indexes out of
    // [0,1500) would otherwise write straight past vars_ into adjacent members
    // (nextArea_/playlist_) — a stack-smash. Guard it and log.
    int& var(int i);

    bool pumpFrame();                    // op 0x3b: advance/render one paced frame; false = stop
    void fadeToBlack(int msPerStep);     // op 0x204: smooth palette fade-out
    void showSpeech(const std::string& cp1255);   // op 0xcd: display a subtitle for its duration
    int  skipBlock(const ScriptProgram& p, int pc, bool stopAtElse) const;
    static bool isIfOpener(int op);
    [[noreturn]] void die(const ScriptInsn& in, int pc, int progId) const;
};
