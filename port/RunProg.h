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

// Opcode -> human name (e.g. "SET_VAR"), for the RP_TRACE log; "?" if unknown. (OpNames.cpp)
const char* rpOpName(int op);

class RunProg {
public:
    RunProg(ResArchive& arc, Display& disp, Framebuffer& fb)
        : arc_(arc), disp_(disp), fb_(fb), vars_(1500, 0) {}

    // Execute program `progId` of `scene` to completion.
    void exec(const Scene& scene, int progId, int nId = 0);

    // Area node under (x,y) — the fine-grained cache hit-strips first, then static node
    // bboxes / LINKFULL sprites. While the options sub-screen is up (var 0x28 != 0) the
    // menu's flower strips aren't the active set, so they are skipped. Shared by the
    // render loop's hover/click dispatch and by pumpFrame's cursor.
    int nodeAt(int x, int y);

    bool quit() const { return quit_; }                  // user closed the window
    const std::string& nextArea() const { return nextArea_; }  // INVCHAIN target ("" = none)
    void clearTransition() { nextArea_.clear(); }

    // Clean backdrop (indexed pixels) that pumpFrame() restores before drawing anims, so
    // walk/blocking-op animations don't smear frame-on-frame. Set per area by runScene.
    void setBackground(const uint8_t* px, size_t n) { bg_.assign(px, px + n); }
    void clearBackground() { bg_.clear(); }   // drop the backdrop (on area change)

    // True ~[Flow] FPS (9) times/sec in realtime so anims advance at the engine's rate
    // (the render loop still presents at ~30fps); always true headless (deterministic).
    bool animFrameDue();

private:
    ResArchive&  arc_;
    Display&     disp_;
    Framebuffer& fb_;
    uint32_t     lastAnimMs_ = 0;    // last world-advance time for animFrameDue() pacing
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
    int  execDepth_ = 0;                  // nesting depth of exec() (0 = a fresh RunProg_Exec)
    int  curAnimSlot_ = -1;               // DAT_007c4108: "STANI" register — anim slot
                                          // selected by op 0x3f for later named-anim ops
    int  pendingCb_ = -1;                 // iRam007c4998: pending completion-callback prog id
                                          // (op 0x3c), consumed by 0x159/0x185 to arm an anim cb
    bool dispatchingCb_ = false;          // re-entrancy guard for dispatchAnimCallbacks()
    std::vector<int> fkeyCmd_, fkeyKey_;  // 0x179: F-key shortcut -> command registry (max 15)
    std::vector<int> objectList_;         // scene node/object presence list (queried by 0x6f);
                                          // populated by the not-yet-ported object-display ops
    std::vector<uint8_t> bg_;             // clean backdrop for pumpFrame() to restore each frame

    // Bounds-guarded access to the variable file. A script that indexes out of
    // [0,1500) would otherwise write straight past vars_ into adjacent members
    // (nextArea_/playlist_) — a stack-smash. Guard it and log.
    int& var(int i);

public:
    // Read-only peek at the script variable file (g_anSpeechPlayed). Used by the menu loop
    // to read the game's own "in options sub-screen" flag (var 0x28: prog26 sets 1, prog59
    // clears it) so the flower hotspots are suppressed while options is up.
    int varValue(int i) { return var(i); }
    void setVar(int i, int v) { var(i) = v; }

    // Run any anim completion callbacks that fired this frame (ops 0x3c/0x159/0x167/0x185).
    // Called after each Anim::tick() (render loop + pumpFrame). Re-entrancy-guarded.
    void dispatchAnimCallbacks();
private:

    // Resolve an anim-name-table index to a slot. The table's "_THIS" entry is not a
    // resource name — it means "the current STANI slot" (curAnimSlot_). Every anim opcode
    // must go through this; calling Anim::findByName on the raw name silently no-ops on
    // "_THIS" (that is how FREE_ANIM stopped dumping VVI2FRMQ, leaving it looping forever).
    int animSlotFor(int nameIdx);

    // Scene sound-table entry `idx` as raw type-32 PCM bytes (empty if unset/missing).
    std::vector<uint8_t> soundBytes(int idx, std::string& keyOut);

    // Adv_TickFrames (@0x00412be0, ops 0x20c/0x20e): advance the world `n` frames —
    // Adv_Tick() + Timer_DispatchAsyncProg() per frame, then spin until the frame boundary
    // actually passes. The engine returns immediately when its cutscene/speech FSM
    // (DAT_00629f50) is set; the port has no model of that flag, so it always ticks.
    void tickFrames(int n);

    bool pumpFrame();                    // op 0x3b: advance/render one paced frame; false = stop

    // Subtitle currently on screen (op 0xcd/0x50). pumpFrame draws it over the composited
    // frame, so the world keeps running underneath instead of freezing on a snapshot.
    std::string speechText_;
    // Drag rubber band (op 0x1ff / GV_DragUpdate). While a drag is in progress pumpFrame
    // draws a line from the origin to the live cursor, as the engine's drag loop does.
    static constexpr unsigned char kDragLineColor = 0xF1;   // GI_Line colour at 0x00433e3f
    bool dragLineActive_ = false;
    int  dragLineX_ = 0, dragLineY_ = 0;

    bool        pumpSkipped_ = false;    // a click/key arrived during the last pumpFrame

    // SndMem_IsSpeaking (@0x004744f0): is a speech line playing right now? The port only
    // "speaks" while showSpeech holds its subtitle, and that call blocks, so this is false
    // everywhere an opcode can observe it.
    bool speechActive() const { return !speechText_.empty(); }

    // Block until `slot` reaches its armed stop frame (ops 0x1f / 0x13b), pumping a
    // frame per iteration. Watchdog: if the wait outlives WAIT_WATCHDOG frames it logs
    // the full slot state once (naming the program/pc/opcode that armed it) and keeps
    // waiting — the engine's behaviour, but no longer a silent freeze.
    void waitForStopFrame(int slot, int progId, int pc, int op);
    void fadeToBlack(int msPerStep);     // op 0x204: smooth palette fade-out
    void showSpeech(const std::string& cp1255);   // op 0xcd: display a subtitle for its duration
    int  skipBlock(const ScriptProgram& p, int pc, bool stopAtElse) const;
    static bool isIfOpener(int op);
    [[noreturn]] void die(const ScriptInsn& in, int pc, int progId) const;
};
