// main.cpp — Crux/Granny SDL2 reimplementation, milestone 1: resource loader + verification.
//
// Loads the real ADVENT.IDX/ADVENT.RES and prints verification logs:
//   - entry count, first/last entries
//   - offset-chain integrity (each blob should follow the previous, 4-byte aligned)
//   - a read test of a known resource
//   - coverage of ADVENT.RES vs the indexed data span
// SDL2 is initialised (video) so we confirm the lib links/works; it degrades
// gracefully when there's no display (headless CI), since milestone 1 is data-only.
#include "Log.h"
#include "ResArchive.h"
#include "Framebuffer.h"
#include "Display.h"
#include "Scene.h"
#include "RunProg.h"
#include "Audio.h"
#include "Anim.h"
#include "Cursor.h"
#include "Area.h"
#include "Palette.h"
#include "Theme.h"
#include "Sentence.h"
#include "TextRender.h"
#include "ScmPlayer.h"
#include <SDL.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <strings.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(x)
#include "stb_image_write.h"
#pragma clang diagnostic pop

static std::string dataDir = ".";

static void hexdump(const std::vector<uint8_t>& d, size_t n) {
    n = std::min(n, d.size());
    std::string s;
    char b[8];
    for (size_t i = 0; i < n; ++i) { std::snprintf(b, sizeof b, "%02x ", d[i]); s += b; }
    Log::info("    first %zu bytes: %s", n, s.c_str());
}

static void verifyArchive(const ResArchive& arc) {
    const auto& es = arc.entries();
    if (es.empty()) { Log::error("verify: no entries"); return; }

    Log::info("--- first 10 entries ---");
    for (size_t i = 0; i < es.size() && i < 10; ++i)
        Log::info("  [%zu] %-12s type=%d off=%u size=%u", i,
                  es[i].name.c_str(), es[i].type, es[i].offset, es[i].size);

    // Integrity: blobs may be listed in any order in the index, so sort by
    // offset and check that none overlap and all stay within ADVENT.RES.
    // (The index is name-ordered; the .RES is laid out separately, so a strict
    // "next == prev+size" chain does NOT hold — overlap is the real invariant.)
    std::vector<const ResEntry*> byOff;
    byOff.reserve(es.size());
    for (const auto& e : es) byOff.push_back(&e);
    std::sort(byOff.begin(), byOff.end(),
              [](const ResEntry* a, const ResEntry* b){ return a->offset < b->offset; });

    size_t overlaps = 0, oob = 0;
    uint64_t maxEnd = 0, gapBytes = 0, prevEnd = 0;
    for (const ResEntry* e : byOff) {
        uint64_t end = (uint64_t)e->offset + e->size;
        if (end > maxEnd) maxEnd = end;
        if (end > arc.resFileSize()) oob++;
        if (e->offset < prevEnd) overlaps++;
        else gapBytes += e->offset - prevEnd;
        if (end > prevEnd) prevEnd = end;
    }
    Log::info("--- integrity ---");
    Log::info("  overlapping blobs:   %zu / %zu", overlaps, es.size());
    Log::info("  out-of-range blobs:  %zu", oob);
    Log::info("  inter-blob padding:  %llu bytes", (unsigned long long)gapBytes);
    Log::info("  indexed data span:   %llu bytes (max offset+size)", (unsigned long long)maxEnd);
    Log::info("  ADVENT.RES size:     %llu bytes", (unsigned long long)arc.resFileSize());
    if (overlaps == 0 && oob == 0)
        Log::info("  -> no overlaps, all blobs within ADVENT.RES  OK");
    else
        Log::error("  -> integrity problem (overlaps=%zu oob=%zu)", overlaps, oob);

    // Read test: pull a couple of known resources and show their bytes.
    Log::info("--- read test ---");
    for (const char* name : {"CSDEF", "CSEXIT", "CURSDRAG"}) {
        const ResEntry* e = arc.find(name);
        if (!e) { Log::warn("  '%s' not found", name); continue; }
        auto bytes = arc.read(*e);
        Log::info("  read '%s': %zu bytes", name, bytes.size());
        hexdump(bytes, 16);
    }

    // Name stats: how many unique names vs total (the format allows dupes).
    size_t dupes = es.size();
    {
        // crude unique count via the archive's first-match map size
        // (find() returns first; count of distinct names = map size, exposed indirectly)
    }
    (void)dupes;
}

// The area's lifecycle scripts, in Adv_RunScene order (early group 6,0,7 run with
// ticking suppressed in the engine; late group 8,3,9). cacheSlot==-1 → skipped.
static const int kLifecycleSlots[] = { 6, 0, 7, 8, 3, 9 };

// A hoverable menu flower: a paired normal anim ("...1") + highlight anim ("...2")
// drawn at the same spot, plus the normal frame's painted bbox as the hover rect.
struct MenuButton {
    int normalSlot;
    int highlightSlot;
    int node;            // area-node index (0..5) carrying verb-0 click handler
    int x, y, w, h;
};

// Each menu flower is an anim PAIR. Despite the suffixes, "...1" (MN_INT1, ...) is
// the LIT/glowing sprite and "...2" is the DIM idle sprite — so idle flowers show
// "...2" and the hovered flower lights up to "...1". The hover rect is the idle
// flower's painted pixels (whole-flower bbox); the lit sprite starts hidden.
static std::vector<MenuButton> buildMenuButtons(const Scene& scene) {
    std::vector<MenuButton> btns;
    for (int s = 0; s < Anim::MAX_SLOTS; ++s) {
        if (!Anim::active(s)) { continue; }
        std::string name = Anim::slotName(s);
        if (name.empty() || name.back() != '1') { continue; }     // s = the lit "...1" sprite
        int idle = Anim::findByName((name.substr(0, name.size() - 1) + '2').c_str());
        if (idle < 0) { continue; }                               // idle = the dim "...2" sprite
        MenuButton b{};
        b.normalSlot    = idle;          // shown by default (dim)
        b.highlightSlot = s;             // shown on hover (lit/glow)
        if (!Anim::frameBounds(b.normalSlot, b.x, b.y, b.w, b.h)) { continue; }
        b.node = -1;                     // resolved by anim-link below
        Anim::setVisible(b.highlightSlot, false);     // glow hidden until hovered
        btns.push_back(b);
    }
    // Link each flower to the area-node whose verb-0 handler controls that flower's
    // idle anim (MN_*2). The .SCN node order is NOT the flower add-order — e.g. OPT
    // is node 4 and EXIT is node 5, but flowers are added EXIT-then-OPT — so we must
    // match by the anim the handler touches, not by index.
    for (int node = 0; node < Area::count(); ++node) {
        const ScriptProgram* p = scene.program(Area::verbHandler(node, 0));
        if (p == nullptr) { continue; }
        std::string animNm;
        for (const ScriptInsn& in : p->insns) {
            if (in.op == 0x195 || in.op == 0x191 || in.op == 0x13c || in.op == 0x13d) {
                animNm = scene.animName(in.a0);
                break;
            }
        }
        if (animNm.empty()) { continue; }
        for (auto& b : btns) {
            if (animNm == Anim::slotName(b.normalSlot)) { b.node = node; break; }
        }
    }
    for (size_t i = 0; i < btns.size(); ++i) {
        Log::info("menu button %zu: idle %s / lit %s -> node %d rect=(%d,%d %dx%d)", i,
                  Anim::slotName(btns[i].normalSlot), Anim::slotName(btns[i].highlightSlot),
                  btns[i].node, btns[i].x, btns[i].y, btns[i].w, btns[i].h);
    }
    return btns;
}

static int hitMenuButton(const std::vector<MenuButton>& btns, int mx, int my) {
    for (int i = 0; i < (int)btns.size(); ++i) {
        const MenuButton& b = btns[i];
        if (mx >= b.x && mx < b.x + b.w && my >= b.y && my < b.y + b.h) { return i; }
    }
    return -1;
}

// 5x7 bitmap font, digits 0-9 only (MSB = leftmost column of 5).
static const uint8_t kDigit5x7[10][7] = {
    {0x1E,0x11,0x13,0x15,0x19,0x11,0x1E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x1E,0x11,0x01,0x0E,0x10,0x10,0x1F}, // 2
    {0x1F,0x01,0x02,0x06,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
};

// Write an RGB888 pixel into an interleaved buffer (no-op if out of bounds).
static void putRgb(uint8_t* rgb, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= Framebuffer::W || y < 0 || y >= Framebuffer::H) { return; }
    uint8_t* p = rgb + ((size_t)y * Framebuffer::W + x) * 3;
    p[0] = r; p[1] = g; p[2] = b;
}

// Draw a base-10 integer at (x,y) in the given color, returning the x advance.
static void drawNumber(uint8_t* rgb, int x, int y, int value, uint8_t r, uint8_t g, uint8_t b) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", value);
    for (const char* c = buf; *c; ++c) {
        if (*c < '0' || *c > '9') { continue; }
        const uint8_t* glyph = kDigit5x7[*c - '0'];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (0x10 >> col)) {
                    // 1px black outline behind each lit pixel for legibility.
                    putRgb(rgb, x + col,     y + row,     r, g, b);
                }
            }
        }
        x += 6;
    }
}

// Dump a 640x480 PNG of the area nodes: the backdrop in grey, every node's
// bbox outlined (green = hit-testable, red = filtered out), labelled with its
// index. Also logs each node's fields. Gated by the AREA_PNG env var (path).
static void dumpAreaPng(const Framebuffer& bg, const char* path) {
    std::vector<uint8_t> rgb((size_t)Framebuffer::W * Framebuffer::H * 3);
    // Backdrop, dimmed to ~40% so the coloured boxes stand out.
    const uint8_t* pal = bg.palette();
    const uint8_t* px = bg.pixels();
    for (int i = 0; i < Framebuffer::W * Framebuffer::H; ++i) {
        const uint8_t* c = &pal[px[i] * 3];
        rgb[i*3+0] = (uint8_t)(c[0] * 4 / 10);
        rgb[i*3+1] = (uint8_t)(c[1] * 4 / 10);
        rgb[i*3+2] = (uint8_t)(c[2] * 4 / 10);
    }

    Log::info("--- area nodes (%d) ---", Area::count());
    for (int n = 0; n < Area::count(); ++n) {
        Area::NodeInfo ni;
        if (!Area::nodeInfo(n, ni)) { continue; }
        Log::info("  node %2d: bbox=(%d,%d)-(%d,%d) tag=%d type=%d z=%d cursor=%d en=0x%02x %s",
                  n, ni.x1, ni.y1, ni.x2, ni.y2, ni.tag, ni.type, ni.z, ni.cursor,
                  ni.enabledByte, ni.hittable ? "HIT" : "filtered");
        uint8_t r = ni.hittable ? 0   : 255;
        uint8_t g = ni.hittable ? 255 : 40;
        uint8_t b = 40;
        // Rectangle outline.
        for (int x = ni.x1; x <= ni.x2; ++x) { putRgb(rgb.data(), x, ni.y1, r, g, b); putRgb(rgb.data(), x, ni.y2, r, g, b); }
        for (int y = ni.y1; y <= ni.y2; ++y) { putRgb(rgb.data(), ni.x1, y, r, g, b); putRgb(rgb.data(), ni.x2, y, r, g, b); }
        // Index label just inside the top-left corner.
        drawNumber(rgb.data(), ni.x1 + 2, ni.y1 + 2, n, 255, 255, 0);
    }

    if (stbi_write_png(path, Framebuffer::W, Framebuffer::H, 3, rgb.data(), Framebuffer::W * 3)) {
        Log::info("wrote area map: %s", path);
    } else {
        Log::error("failed to write area map: %s", path);
    }
}

// Interactive scene loop: compose the z-ordered anims each frame and present,
// until the user quits (window close) or a verb script requests an area change.
// Returns the next area name, or "" to quit. Headless renders one frame and ends.
static std::string runScene(Scene& scene, RunProg& vm, Display& disp,
                            Framebuffer& fb, ResArchive& arc, const std::string& areaName) {
    // The area palette and backdrop are named after the area (e.g. "MENU"): a
    // type-3 palette + a type-6 full-screen sprite, with the GENERAL UI palette's
    // non-black entries overlaid. The intro SCMs' frames/palette are transient.
    Palette::load(arc, fb, areaName.c_str(), /*nonBlackOnly*/false);   // area palette
    Palette::load(arc, fb, "GENERAL", /*nonBlackOnly*/true);           // UI overlay
    Area::load(scene.areaNodes(), scene.areaNodeCount());
    // The default arrow plus the context cursors the engine maps to area cursor
    // ids (op 0x1004 / CD_CHANGE_MODE_INIT). Missing ones fall back to the arrow.
    Cursor::load(arc, "CSDEF");                          // default arrow
    Cursor::loadMode(arc, 0, "CURSAREA");                // generic clickable area
    Cursor::loadMode(arc, 8, "CURSAREA");
    Cursor::loadMode(arc, 2, "CSEXIT");                  // exit / doorway (engine name CURSEXIT absent)
    Cursor::loadMode(arc, 3, "CURSINV");                 // inventory
    Cursor::loadMode(arc, 9, "CURSHOUR");                // wait / hourglass

    std::vector<MenuButton> buttons = buildMenuButtons(scene);

    // Background plate = palette + backdrop, WITHOUT the flowers. The flowers are
    // re-drawn each frame so hovering can swap a flower's normal anim for its
    // glowing highlight.
    fb.clear(0);
    Anim::blitResourceFrame0(arc, fb, areaName.c_str(), 6, 0, 0);      // area backdrop
    std::vector<uint8_t> bgPlate(fb.pixels(), fb.pixels() + (size_t)fb.width() * fb.height());

    // Optional area-node map: AREA_PNG=<path> dumps a 640x480 PNG of every node's
    // bbox over the (dimmed) backdrop, plus a per-node field log.
    if (const char* ap = std::getenv("AREA_PNG")) { dumpAreaPng(fb, ap); }

    int animTick = 0;
    for (;;) {
        Theme::advance();                     // keep room music streaming
        // Advance animations at ~15 fps (every other ~30 fps render frame).
        if (++animTick % 2 == 0) { Anim::tick(); }
        int mx = disp.mouseX(), my = disp.mouseY();
        // Headless: optionally hover a button (MENU_HOVER=index) to verify the
        // highlight in the dump.
        if (!disp.isRealtime()) {
            const char* hv = std::getenv("MENU_HOVER");
            if (hv != nullptr) {
                int hb = std::atoi(hv);
                if (hb >= 0 && hb < (int)buttons.size()) {
                    mx = buttons[hb].x + buttons[hb].w / 2;
                    my = buttons[hb].y + buttons[hb].h / 2;
                }
            }
        }

        // Swap normal<->highlight for the hovered flower.
        int hover = hitMenuButton(buttons, mx, my);
        for (int i = 0; i < (int)buttons.size(); ++i) {
            Anim::setVisible(buttons[i].normalSlot,    i != hover);
            Anim::setVisible(buttons[i].highlightSlot, i == hover);
        }

        std::memcpy(fb.pixels(), bgPlate.data(), bgPlate.size());   // restore backdrop
        Anim::drawAll(fb);                                          // flowers (hover-aware)

        // Cursor reflects the hovered region's cursor id: a hovered menu flower
        // (its area-node), else the topmost .SCN area-node under the mouse, else
        // the default arrow.
        int curMode;
        if (hover >= 0) {
            curMode = Area::cursorId(buttons[hover].node);
        } else {
            int node = Area::hitTest(mx, my);
            curMode = (node >= 0) ? Area::cursorId(node) : -1;
        }
        Cursor::drawMode(fb, curMode, mx, my);
        disp.present(fb);

        // Headless / offscreen "dummy" driver: render one frame and return.
        if (!disp.isRealtime()) {
            if (std::getenv("MENU_DUMP")) { fb.savePPM("menu.ppm"); }
            // MENU_CLICK=<flower> simulates a click for headless verification.
            const char* clk = std::getenv("MENU_CLICK");
            if (clk != nullptr) {
                int cb = std::atoi(clk);
                if (cb >= 0 && cb < (int)buttons.size()) {
                    int handler = Area::verbHandler(buttons[cb].node, 0);
                    Log::info("MENU_CLICK flower %d node %d -> handler %d",
                              cb, buttons[cb].node, handler);
                    if (handler >= 0) { vm.exec(scene, handler, 0); }
                }
            }
            return "";
        }

        if (disp.pump() == PumpResult::Quit) { return ""; }     // window closed

        int cx, cy, btn;
        if (disp.takeClick(cx, cy, btn)) {
            // A flower → its area-node verb-0 (click) handler; otherwise fall back
            // to the bottom-strip nodes' AABB hit-test.
            int node, verb = 0;
            int hb = hitMenuButton(buttons, cx, cy);
            if (hb >= 0) {
                node = buttons[hb].node;
                Log::info("menu click -> flower %d (%s) node %d",
                          hb, Anim::slotName(buttons[hb].normalSlot), node);
            } else {
                node = Area::hitTest(cx, cy);
                verb = (btn == SDL_BUTTON_RIGHT) ? 1 : 0;       // left=look(0), right=use(1)
            }
            if (node >= 0) {
                int handler = Area::verbHandler(node, verb);
                Log::info("click (%d,%d) -> node %d verb %d -> handler %d", cx, cy, node, verb, handler);
                if (handler >= 0) {
                    vm.exec(scene, handler, 0);
                    if (vm.quit()) { return ""; }
                    if (!vm.nextArea().empty()) { return vm.nextArea(); }
                }
            }
        }
        SDL_Delay(33);                                          // ~30 fps idle
    }
}

int main(int argc, char** argv) {
    Log::setLevel(Log::INFO);
    if (argc > 1) dataDir = argv[1];
    Log::info("Crux/Granny SDL2 port — milestone 1 (resource loader)");
    Log::info("data dir: %s", dataDir.c_str());

    ResArchive arc;
    if (!arc.open(dataDir + "/ADVENT.IDX", dataDir + "/ADVENT.RES")) {
        Log::error("failed to open resource archive");
        return 1;
    }
    verifyArchive(arc);

    // THEME_DUMP=<name>: parse a type-12 theme file, log its tables, and exit.
    if (const char* tn = std::getenv("THEME_DUMP")) {
        ThemeFile tf;
        if (!tf.load(arc, tn)) { return 1; }
        for (size_t i = 0; i < tf.commands.size(); ++i) {
            const auto& c = tf.commands[i];
            Log::info("  cmd %2zu: type=%d arg=%d cnt=%d evt=[%d..%d]",
                      i, c.type, c.arg, c.cnt, c.evtStart, c.evtEnd);
        }
        for (size_t i = 0; i < tf.labels.size(); ++i) {
            Log::info("  label '%s' -> cmd %d", tf.labels[i].c_str(),
                      i < tf.labelOffsets.size() ? tf.labelOffsets[i] : -1);
        }
        return 0;
    }

    // --- Milestone 2: open the 640x480 window + 8-bit framebuffer, show a test pattern. ---
    Display disp;
    disp.open("Crux / Granny (SDL2 port)", 1);
    Log::info("display: %s", disp.isHeadless() ? "HEADLESS" : "windowed");
    Audio::open();   // 22050/16-bit; stays silent if it can't init
    Theme::init(arc);   // room-music subsystem ready (mirrors Theme_Init at startup)
    Sentence::load(dataDir + "/SENTENCE.BIN");   // Hebrew speech/subtitle text table
    TextRender::init();                          // Hebrew glyph renderer (for subtitles)

    // RUN_PROG=<scene>:<prog>: load a scene and execute one program (dynamic
    // complement to dumpprog) — verifies every opcode it uses is implemented.
    if (const char* rp = std::getenv("RUN_PROG")) {
        std::string s(rp);
        size_t colon = s.find(':');
        std::string sc = s.substr(0, colon);
        int pid = (colon != std::string::npos) ? std::atoi(s.c_str() + colon + 1) : 0;
        Scene scene;
        if (scene.load(arc, sc.c_str())) {
            Framebuffer rfb;
            RunProg vm(arc, disp, rfb);
            vm.exec(scene, pid, 0);
            Log::info("RUN_PROG %s:%d completed", sc.c_str(), pid);
        }
        return 0;
    }

    // SENTENCE_DUMP=<key>: load SENTENCE.BIN and print one sentence (UTF-8) to verify.
    if (const char* sk = std::getenv("SENTENCE_DUMP")) {
        if (Sentence::load(dataDir + "/SENTENCE.BIN")) {
            const std::string* h = Sentence::lookup(sk);
            if (h) { Log::info("sentence '%s' = \"%s\"", sk, Sentence::cp1255ToUtf8(*h).c_str()); }
            else   { Log::info("sentence '%s' not found", sk); }
        }
        return 0;
    }

    // SENTENCE_RENDER=<key>: render a Hebrew sentence over the menu palette and dump.
    if (const char* rk = std::getenv("SENTENCE_RENDER")) {
        Sentence::load(dataDir + "/SENTENCE.BIN");
        TextRender::init();
        Framebuffer tfb;
        const char* area = std::getenv("SENTENCE_AREA");
        if (!area) { area = "MENU"; }
        Palette::load(arc, tfb, area, false);
        Palette::load(arc, tfb, "GENERAL", true);
        tfb.clear(0);
        Anim::blitResourceFrame0(arc, tfb, area, 6, 0, 0);   // area backdrop, if any
        const std::string* h = Sentence::lookup(rk);
        if (h) { TextRender::drawSentence(tfb, *h, 320, tfb.height() - 40); }
        tfb.savePPM("sentence.ppm");
        return 0;
    }

    // SCM_PLAY=<name>: play a SCM headless (with SCM_DUMP to capture the last frame,
    // including its subtitle) — verifies in-video subtitles.
    if (const char* sn = std::getenv("SCM_PLAY")) {
        Framebuffer sfb;
        playScmByName(arc, disp, sfb, sn);
        return 0;
    }

    // CURSOR_DUMP=<name>: blit a cursor at (200,150) on a gray ramp and dump, to
    // inspect its decoded shape.
    if (const char* cn = std::getenv("CURSOR_DUMP")) {
        Framebuffer cfb;
        for (int i = 0; i < 256; ++i) { cfb.setPaletteEntry(i, (uint8_t)i, (uint8_t)i, (uint8_t)i); }
        cfb.clear(0);
        if (Cursor::load(arc, cn)) { Cursor::draw(cfb, 200, 150); }
        cfb.savePPM("cursor.ppm");
        return 0;
    }

    // THEME_PLAY=<track>: verify the sequencer. Dry-walk the cue sequence, then
    // do a real streaming pass and report how many samples got queued.
    if (const char* tk = std::getenv("THEME_PLAY")) {
        Theme::setRoom(5);                 // activate a room so playback engages
        Theme::play(tk, "start");
        Theme::debugWalk(95);              // log the sequence (no audio)
        Theme::play(tk, "start");          // reload, then stream for real
        for (int i = 0; i < 8; ++i) { Theme::advance(); }
        Log::info("THEME: %zu samples queued after streaming (audio %s)",
                  Audio::queuedSamples(Audio::THEME), Audio::isOpen() ? "open" : "closed");
        return 0;
    }

    Framebuffer fb;
    // Grayscale ramp palette + a diagonal index pattern to prove index->palette->RGBA works.
    for (int i = 0; i < 256; ++i) fb.setPaletteEntry(i, i, i, i);
    for (int y = 0; y < fb.height(); ++y)
        for (int x = 0; x < fb.width(); ++x)
            fb.pixels()[y * fb.width() + x] = (uint8_t)((x + y) & 0xff);

    disp.present(fb);

    // --- Boot + area loop. Mirrors Win_ExecutionThread: load a scene's .SCN,
    // run its boot script (g_anAreaCacheSlots[0]); INVCHAIN opcodes transition to
    // the next area. Starts at "entry" (the bootstrap that branches to "menu",
    // where the startup logos play). The variable file persists across areas.
    RunProg vm(arc, disp, fb);
    // START_AREA jumps straight to a named area (skips the intro chain) — handy
    // for headless diagnostics like AREA_PNG.
    std::string area = "entry";
    if (const char* sa = std::getenv("START_AREA")) { area = sa; }
    while (!area.empty() && strcasecmp(area.c_str(), "__end__") != 0) {
        Scene scene;
        if (!scene.load(arc, area.c_str())) break;

        Anim::reset();              // fresh anim pool per area
        vm.clearTransition();

        // Run the area's lifecycle scripts (sets up anims, plays intros, etc.),
        // stopping early if one requests an area change or the user quit.
        for (int slotIdx : kLifecycleSlots) {
            int prog = scene.cacheSlot(slotIdx);
            if (prog >= 0) { vm.exec(scene, prog, 0); }
            if (vm.quit() || !vm.nextArea().empty()) break;
        }
        if (vm.quit()) break;

        std::string next = vm.nextArea();
        if (next.empty()) {
            // No transition requested → this area is interactive (e.g. the menu).
            next = runScene(scene, vm, disp, fb, arc, area);
        }
        if (next.empty()) break;    // quit
        Log::info("area transition: %s -> %s", area.c_str(), next.c_str());
        area = next;
    }

    Audio::close();
    disp.close();
    Log::info("boot/area loop complete.");
    return 0;
}
