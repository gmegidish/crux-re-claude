// test_dumpqueue.cpp — self-check for the deferred anim dump (Anim_MarkForDump /
// Anim_ProcessDumpQueue). The script path that exercises it (op 0x13 on a LOADED
// anim) only happens mid-game, so this drives the two calls directly against the
// real archive.
//
//   make test
#include "Anim.h"
#include "Inventory.h"
#include "Framebuffer.h"
#include "ResArchive.h"
#include "Log.h"
#include <cassert>
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    const std::string dir = (argc > 1) ? argv[1] : "..";
    Log::setLevel(Log::ERROR);

    ResArchive arc;
    if (!arc.open(dir + "/ADVENT.IDX", dir + "/ADVENT.RES")) {
        std::fprintf(stderr, "cannot open archive in '%s'\n", dir.c_str());
        return 1;
    }

    const int outgoing = Anim::addByName(arc, "OPRBDF", true, false);
    assert(outgoing >= 0 && "test anim should load");

    Anim::markForDump(outgoing);
    assert(Anim::active(outgoing) && "a marked anim stays ALIVE — the dump is deferred");

    const int incoming = Anim::addByName(arc, "OPT1DF1", true, false);
    assert(incoming >= 0 && incoming != outgoing && "loading another anim must not free it");
    assert(Anim::active(outgoing) && "still alive: the dump waits for a frame boundary");

    Anim::tick();
    assert(!Anim::active(outgoing) && "the marked anim is dumped at the next frame boundary");

    // Marking twice must not queue twice (engine checks the dump-pending flag).
    Anim::markForDump(incoming);
    Anim::markForDump(incoming);
    Anim::processDumpQueue();
    assert(!Anim::active(incoming));

    // A one-shot anim disposes of itself when it reaches its end; a looping one never does.
    const int oneShot = Anim::addByName(arc, "OPRBDF", /*looping*/false, false);
    const int looping = Anim::addByName(arc, "OPT1DF1", /*looping*/true, false);
    for (int i = 0; i < Anim::frameCount(oneShot) + 3; ++i) { Anim::tick(); }
    assert(!Anim::active(oneShot) && "a finished one-shot marks itself for dump");
    assert(Anim::active(looping) && "a looping anim keeps going");

    std::printf("deferred-dump: ok\n");

    // --- inventory (ops 0xc / 0xd / 0x11) ---
    Inventory::clear();
    Inventory::add(34);
    assert(Inventory::has(34) && "an added item is held");
    Inventory::add(34);
    assert(Inventory::items().size() == 1 && "adding twice keeps one entry");
    Inventory::add(7);
    Inventory::remove(34);
    assert(!Inventory::has(34) && Inventory::has(7) && "remove drops only that item");
    Inventory::remove(99);        // not held: a no-op, not a crash
    assert(Inventory::items().size() == 1);
    std::printf("inventory: ok\n");

    // --- drag rubber band (op 0x1ff / GV_DragUpdate draws GI_Line in colour 0xF1) ---
    Framebuffer fb;
    fb.clear(0);
    fb.drawLine(10, 10, 20, 10, 0xF1);
    for (int x = 10; x <= 20; ++x) { assert(fb.pixels()[10 * Framebuffer::W + x] == 0xF1); }
    fb.drawLine(0, 0, 5, 5, 0xF1);
    for (int i = 0; i <= 5; ++i) { assert(fb.pixels()[i * Framebuffer::W + i] == 0xF1); }
    fb.drawLine(-50, -50, -40, -40, 0xF1);          // fully offscreen: must not write or crash
    fb.drawLine(630, 470, 700, 520, 0xF1);          // runs off the far edge
    assert(fb.pixels()[470 * Framebuffer::W + 630] == 0xF1);
    std::printf("drawLine: ok\n");
    return 0;
}
