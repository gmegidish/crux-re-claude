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
#include "HelpBlit.h"
#include "Area.h"
#include "Slider.h"
#include "Timer.h"
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
#include <unordered_set>
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

// --- Cursor sheet (CURSOR_SHEET=<path.png>): render every mouse cursor to one PNG, a
// row per cursor and a column per frame. Each frame is rendered through the real
// Help_BlitImage path (exactly how the live cursor is drawn) and cropped to its opaque
// pixels — detected by blitting twice over two different clear colours and keeping the
// pixels that came out identical (the rest are untouched/transparent). Lets us eyeball
// every cursor's sprite + colours at once when the on-screen cursor looks wrong. ---
namespace {
struct CurFrame { int w = 0, h = 0; std::vector<uint8_t> rgb, opaque; };  // cropped

inline uint32_t rd32(const uint8_t* p) {
    return (uint32_t)p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

// drawNumber for an arbitrary-width RGB buffer (the file-scope one is hardwired to 640).
void drawNumberSheet(uint8_t* img, int sheetW, int sheetH, int x, int y, int value,
                     uint8_t r, uint8_t g, uint8_t b) {
    char buf[16]; std::snprintf(buf, sizeof buf, "%d", value);
    for (const char* c = buf; *c; ++c) {
        if (*c < '0' || *c > '9') { continue; }
        const uint8_t* glyph = kDigit5x7[*c - '0'];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (!(glyph[row] & (0x10 >> col))) { continue; }
                int xx = x + col, yy = y + row;
                if (xx >= 0 && xx < sheetW && yy >= 0 && yy < sheetH) {
                    uint8_t* p = img + ((size_t)yy * sheetW + xx) * 3; p[0] = r; p[1] = g; p[2] = b;
                }
            }
        }
        x += 6;
    }
}

// Render one Help_BlitImage frame blob and crop to its drawn pixels.
CurFrame renderCursorFrame(Framebuffer& fb, const uint8_t* blob, size_t sz) {
    const int W = Framebuffer::W, H = Framebuffer::H, OX = 48, OY = 48;
    fb.clear(0);   blitHelpImage(blob, sz, fb, OX, OY);
    std::vector<uint8_t> a(fb.pixels(), fb.pixels() + (size_t)W * H);
    fb.clear(255); blitHelpImage(blob, sz, fb, OX, OY);
    const uint8_t* b = fb.pixels();
    const uint8_t* pal = fb.palette();
    int minx = W, miny = H, maxx = -1, maxy = -1;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (a[(size_t)y * W + x] == b[(size_t)y * W + x]) {   // identical under both clears => drawn
                if (x < minx) { minx = x; } if (x > maxx) { maxx = x; }
                if (y < miny) { miny = y; } if (y > maxy) { maxy = y; }
            }
        }
    }
    CurFrame f;
    if (maxx < minx) { f.w = 1; f.h = 1; f.rgb.assign(3, 0); f.opaque.assign(1, 0); return f; }
    f.w = maxx - minx + 1; f.h = maxy - miny + 1;
    f.rgb.assign((size_t)f.w * f.h * 3, 0);
    f.opaque.assign((size_t)f.w * f.h, 0);
    for (int y = 0; y < f.h; ++y) {
        for (int x = 0; x < f.w; ++x) {
            size_t si = (size_t)(miny + y) * W + (minx + x);
            size_t di = (size_t)y * f.w + x;
            if (a[si] == b[si]) {
                const uint8_t* c = &pal[a[si] * 3];
                f.rgb[di*3+0] = c[0]; f.rgb[di*3+1] = c[1]; f.rgb[di*3+2] = c[2];
                f.opaque[di] = 1;
            }
        }
    }
    return f;
}
}  // namespace

static void dumpCursorSheet(ResArchive& arc, Framebuffer& fb, const char* path) {
    // Cursors are palette-indexed; load the menu + GENERAL UI palette so they get the
    // colours they'd have on the menu (where the arrow/context cursors live).
    Palette::load(arc, fb, "MENU", /*nonBlackOnly*/false);
    Palette::load(arc, fb, "GENERAL", /*nonBlackOnly*/true);

    // CURSOR_TYPE selects the archive type: 2 = held-item / UI cursors (all single-frame,
    // ~1939), 7 = ANIMATIONS (multi-frame). The engine prefers a type-7 over a type-2 for a
    // cursor (Curs_LoadCursorSelect), so animated cursors — opening/closing doors, etc. —
    // live in type-7. CURSOR_FILTER=<substr> narrows by name (case-insensitive).
    int type = 2;
    if (const char* ct = std::getenv("CURSOR_TYPE")) { type = std::atoi(ct); }
    const char* filt = std::getenv("CURSOR_FILTER");
    std::string fup;
    if (filt) { for (const char* c = filt; *c; ++c) { fup += (char)toupper((unsigned char)*c); } }

    std::vector<std::pair<std::string, std::vector<CurFrame>>> cursors;   // all frames each
    int maxW = 1, maxH = 1, maxFrames = 1;
    for (const auto& en : arc.entries()) {
        if (en.type != type) { continue; }
        if (!fup.empty()) {
            std::string up = en.name;
            for (char& c : up) { c = (char)toupper((unsigned char)c); }
            if (up.find(fup) == std::string::npos) { continue; }
        }
        std::vector<uint8_t> data = arc.read(en);
        if (data.size() < 12 || data[0] != 0x10) { continue; }
        const uint8_t* d = data.data();
        int frameCount = (int)rd32(d + 8);
        if (frameCount < 1) { continue; }
        size_t off = 12 + (size_t)frameCount * 8;
        if (off > data.size()) { continue; }
        std::vector<CurFrame> frames;
        for (int i = 0; i < frameCount; ++i) {
            int32_t fsz = (int32_t)rd32(d + 12 + (size_t)i * 8 + 4);
            if (fsz < 0 || off + (size_t)fsz > data.size()) { break; }
            CurFrame f = renderCursorFrame(fb, d + off, (size_t)fsz);
            maxW = std::max(maxW, f.w); maxH = std::max(maxH, f.h);
            frames.push_back(std::move(f));
            off += (size_t)fsz;
        }
        if (frames.empty()) { continue; }
        maxFrames = std::max(maxFrames, (int)frames.size());
        cursors.emplace_back(en.name, std::move(frames));
    }
    if (cursors.empty()) { Log::error("CURSOR_SHEET: no type-%d cursors found", type); return; }

    // Archive order groups by room/state, so identical items scatter. Sort by name.
    std::sort(cursors.begin(), cursors.end(), [](const auto& a, const auto& b) {
        int c = strcasecmp(a.first.c_str(), b.first.c_str());
        return c != 0 ? c < 0 : a.first < b.first;
    });

    // CURSOR_DEDUP=1: collapse entries that render to identical pixels (the same sprite
    // re-stored per room) for a clean unique catalogue.
    if (std::getenv("CURSOR_DEDUP")) {
        std::unordered_set<uint64_t> seen;
        std::vector<std::pair<std::string, std::vector<CurFrame>>> uniq;
        for (auto& cu : cursors) {
            uint64_t h = 1469598103934665603ULL;
            auto mix = [&](const void* p, size_t n) {
                const uint8_t* b = (const uint8_t*)p;
                for (size_t k = 0; k < n; ++k) { h = (h ^ b[k]) * 1099511628211ULL; }
            };
            for (const CurFrame& f : cu.second) {
                mix(&f.w, sizeof f.w); mix(&f.h, sizeof f.h);
                mix(f.rgb.data(), f.rgb.size()); mix(f.opaque.data(), f.opaque.size());
            }
            if (seen.insert(h).second) { uniq.push_back(std::move(cu)); }
        }
        Log::info("CURSOR_SHEET: dedup %d -> %d unique", (int)cursors.size(), (int)uniq.size());
        cursors.swap(uniq);
    }

    const int pad = 3, labelH = 8, cellW = maxW + 2 * pad, cellH = maxH + 2 * pad + labelH;
    const bool multi = maxFrames > 1;   // animations: one row per cursor, one column per frame

    // Single-frame (type-2): packed grid, CURSOR_COLS wide. Multi-frame (type-7): one row per
    // cursor with its frames across the columns (capped at CURSOR_MAXCOLS, default 64).
    int gridCols = 32;
    if (const char* cc = std::getenv("CURSOR_COLS")) { gridCols = std::max(1, std::atoi(cc)); }
    int maxCols = 64;
    if (const char* mc = std::getenv("CURSOR_MAXCOLS")) { maxCols = std::max(1, std::atoi(mc)); }
    const int labelW = multi ? 44 : 0;
    const int cols = multi ? std::min(maxFrames, maxCols) : std::min(gridCols, (int)cursors.size());
    const int rows = multi ? (int)cursors.size() : ((int)cursors.size() + cols - 1) / cols;
    const int sheetW = labelW + cols * cellW, sheetH = rows * cellH;
    std::vector<uint8_t> img((size_t)sheetW * sheetH * 3);
    auto px = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= sheetW || y < 0 || y >= sheetH) { return; }
        uint8_t* p = img.data() + ((size_t)y * sheetW + x) * 3; p[0] = r; p[1] = g; p[2] = b;
    };
    for (int y = 0; y < sheetH; ++y) {
        for (int x = 0; x < sheetW; ++x) { uint8_t v = (((x >> 3) ^ (y >> 3)) & 1) ? 70 : 50; px(x, y, v, v, v); }
    }
    auto drawIcon = [&](int ox, int oy, int innerTop, const CurFrame& f) {
        int fx = ox + (cellW - f.w) / 2, fy = oy + innerTop + (cellH - innerTop - f.h) / 2;
        for (int y = 0; y < f.h; ++y) {
            for (int x = 0; x < f.w; ++x) {
                size_t di = (size_t)y * f.w + x;
                if (f.opaque[di]) { px(fx + x, fy + y, f.rgb[di*3], f.rgb[di*3+1], f.rgb[di*3+2]); }
            }
        }
    };
    for (size_t i = 0; i < cursors.size(); ++i) {
        if (multi) {
            int oy = (int)i * cellH;
            for (int x = 0; x < sheetW; ++x) { px(x, oy, 90, 90, 90); }          // row separator
            drawNumberSheet(img.data(), sheetW, sheetH, 2, oy + cellH / 2 - 3, (int)i, 255, 255, 0);
            const auto& frames = cursors[i].second;
            for (int c = 0; c < cols && c < (int)frames.size(); ++c) {
                drawIcon(labelW + c * cellW, oy, 0, frames[c]);
            }
        } else {
            int ox = ((int)i % cols) * cellW, oy = ((int)i / cols) * cellH;
            for (int x = 0; x < cellW; ++x) { px(ox + x, oy, 90, 90, 90); }
            for (int y = 0; y < cellH; ++y) { px(ox, oy + y, 90, 90, 90); }
            drawNumberSheet(img.data(), sheetW, sheetH, ox + 1, oy + 1, (int)i, 255, 255, 0);
            drawIcon(ox, oy, labelH, cursors[i].second[0]);
        }
    }
    // Sidecar index -> name (+ frame count) map.
    std::string mapPath = std::string(path) + ".txt";
    if (FILE* mf = std::fopen(mapPath.c_str(), "w")) {
        for (size_t i = 0; i < cursors.size(); ++i) {
            std::fprintf(mf, "%zu\t%s\t%dframe(s)\n", i, cursors[i].first.c_str(), (int)cursors[i].second.size());
        }
        std::fclose(mf);
    }
    if (stbi_write_png(path, sheetW, sheetH, 3, img.data(), sheetW * 3)) {
        Log::info("wrote cursor sheet: %s (%dx%d, type-%d, %d cursors, maxFrames=%d) + map %s",
                  path, sheetW, sheetH, type, (int)cursors.size(), maxFrames, mapPath.c_str());
    } else {
        Log::error("failed to write cursor sheet: %s", path);
    }
}

// --- Live on-screen area overlay (AREA_OVERLAY=1): the same node boxes dumpAreaPng
// writes to a PNG, but painted into the 8-bit framebuffer every frame so they're always
// visible while playing. Outlines each node's bbox (green = hit-testable, red = filtered)
// with its index, using the nearest palette index to those colours (so it works on the
// indexed surface without disturbing the scene palette). ---
static int nearestPaletteIndex(const Framebuffer& fb, int r, int g, int b) {
    const uint8_t* pal = fb.palette();
    int best = 0;
    long bestD = 0x7fffffffL;
    for (int i = 0; i < 256; ++i) {
        int dr = pal[i*3+0] - r, dg = pal[i*3+1] - g, db = pal[i*3+2] - b;
        long d = (long)dr*dr + (long)dg*dg + (long)db*db;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

static void putIdx(Framebuffer& fb, int x, int y, uint8_t idx) {
    if (x < 0 || x >= Framebuffer::W || y < 0 || y >= Framebuffer::H) { return; }
    fb.pixels()[(size_t)y * Framebuffer::W + x] = idx;
}

static void drawNumberIdx(Framebuffer& fb, int x, int y, int value, uint8_t idx) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", value);
    for (const char* c = buf; *c; ++c) {
        if (*c < '0' || *c > '9') { continue; }
        const uint8_t* glyph = kDigit5x7[*c - '0'];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (0x10 >> col)) { putIdx(fb, x + col, y + row, idx); }
            }
        }
        x += 6;
    }
}

static void drawAreaOverlay(Framebuffer& fb) {
    uint8_t green  = (uint8_t)nearestPaletteIndex(fb, 0, 255, 0);
    uint8_t yellow = (uint8_t)nearestPaletteIndex(fb, 255, 255, 0);
    for (int n = 0; n < Area::count(); ++n) {
        Area::NodeInfo ni;
        if (!Area::nodeInfo(n, ni)) { continue; }
        // Only show currently hit-testable nodes (type != 2 && enabled); filtered/disabled
        // ones are just noise on screen.
        if (!ni.hittable) { continue; }
        // Skip degenerate / off-screen rects (e.g. anim-driven nodes with bbox -1,-1,-1,-1).
        if (ni.x2 < ni.x1 || ni.y2 < ni.y1 || ni.x2 < 0 || ni.y2 < 0) { continue; }
        for (int x = ni.x1; x <= ni.x2; ++x) { putIdx(fb, x, ni.y1, green); putIdx(fb, x, ni.y2, green); }
        for (int y = ni.y1; y <= ni.y2; ++y) { putIdx(fb, ni.x1, y, green); putIdx(fb, ni.x2, y, green); }
        drawNumberIdx(fb, ni.x1 + 2, ni.y1 + 2, n, yellow);
    }

    // Dynamic LINKFULL hotspots: the anim-driven clickable objects whose static node
    // bbox is (-1,-1). Their real hit-rect is the live anim's painted frame bbox
    // (Anim::frameBounds), exactly what Area_FindAt's sprite path tests. Drawn in cyan
    // and labelled with their resolved node, so a room's actual objects are visible.
    uint8_t cyan = (uint8_t)nearestPaletteIndex(fb, 0, 255, 255);
    int animSlot, node, flags;
    for (int i = 0; i < Area::spriteCount(); ++i) {
        if (!Area::spriteInfo(i, animSlot, node, flags)) { continue; }
        int bx, by, bw, bh;
        if (!Anim::frameBounds(animSlot, bx, by, bw, bh)) { continue; }
        int x2 = bx + bw, y2 = by + bh;
        for (int x = bx; x <= x2; ++x) { putIdx(fb, x, by, cyan); putIdx(fb, x, y2, cyan); }
        for (int y = by; y <= y2; ++y) { putIdx(fb, bx, y, cyan); putIdx(fb, x2, y, cyan); }
        if (node >= 0) { drawNumberIdx(fb, bx + 2, by + 2, node, cyan); }
    }
}

// Load an area's visuals BEFORE its lifecycle scripts run (mirrors Adv_RunScene): the
// type-3 area palette + GENERAL UI overlay, the area-node table, the context cursors, and
// the type-6 backdrop. Returns the clean backdrop plate (no anims) and hands it to the VM
// so pumpFrame() restores it during blocking lifecycle ops (e.g. an intro anim's 0x1f
// WAIT_ANIM_END). Doing this here — not inside runScene, which used to run AFTER the
// lifecycle — is what makes a scene's intro play on its OWN palette/background instead of
// the previous area's (vvi2's intro was rendering on the menu's backdrop + palette).
static std::vector<uint8_t> loadAreaVisuals(Scene& scene, RunProg& vm, Framebuffer& fb,
                                            ResArchive& arc, const std::string& areaName) {
    // The area palette and backdrop are named after the area (e.g. "MENU"): a type-3
    // palette + a type-6 full-screen sprite, with GENERAL's non-black entries overlaid.
    Palette::load(arc, fb, areaName.c_str(), /*nonBlackOnly*/false);   // area palette
    Palette::load(arc, fb, "GENERAL", /*nonBlackOnly*/true);           // UI overlay
    Area::load(scene.areaNodes(), scene.areaNodeCount());
    // The default arrow plus the context cursors the engine maps to area cursor
    // ids (op 0x1004 / CD_CHANGE_MODE_INIT). Missing ones fall back to the arrow.
    Cursor::load(arc, "CSDEF");                          // default arrow
    Cursor::loadMode(arc, 0, "CURSAREA");                // generic clickable area
    Cursor::loadMode(arc, 8, "CURSAREA");
    Cursor::loadMode(arc, 2, "CURSEXIT");                // exit / doorway — type-7, a 10-frame
                                                         // opening/closing door animation
    Cursor::loadMode(arc, 3, "CURSINV");                 // inventory
    Cursor::loadMode(arc, 9, "CURSHOUR");                // wait / hourglass (type-7, 13-frame)

    fb.clear(0);
    Anim::blitResourceFrame0(arc, fb, areaName.c_str(), 6, 0, 0);      // area backdrop
    std::vector<uint8_t> bgPlate(fb.pixels(), fb.pixels() + (size_t)fb.width() * fb.height());
    // Hand the backdrop to the VM so pumpFrame() (blocking ops like 0x1f WAIT_ANIM_END that
    // drive walk animations) restores a clean background each frame instead of smearing
    // every frame on top of the last (the "20 grannies" bug).
    vm.setBackground(bgPlate.data(), bgPlate.size());
    return bgPlate;
}

// Interactive scene loop: compose the z-ordered anims each frame and present, until the
// user quits (window close) or a verb script requests an area change. Visuals (palette/
// backdrop/area nodes) are already loaded by loadAreaVisuals; `bgPlate` is the clean
// backdrop plate WITHOUT anims, re-laid each frame. Returns the next area name, or "" to
// quit. Headless renders one frame and ends.
static std::string runScene(Scene& scene, RunProg& vm, Display& disp, Framebuffer& fb,
                            const std::vector<uint8_t>& bgPlate) {
    std::vector<MenuButton> buttons = buildMenuButtons(scene);

    // Optional area-node map: AREA_PNG=<path> dumps a 640x480 PNG of every node's bbox over
    // the (dimmed) backdrop. The lifecycle may have drawn anims into fb, so restore the
    // clean plate first.
    if (const char* ap = std::getenv("AREA_PNG")) {
        std::memcpy(fb.pixels(), bgPlate.data(), bgPlate.size());
        dumpAreaPng(fb, ap);
    }
    // Live area overlay: paint the node boxes into every frame (always visible).
    const bool areaOverlay = std::getenv("AREA_OVERLAY") != nullptr;

    for (;;) {
        Theme::advance();                     // keep room music streaming
        // Advance anims/timers at the engine's ~9fps ([Flow] FPS), not per present frame,
        // then run any anim completion callbacks that fired (ops 0x159/0x167/0x185).
        if (vm.animFrameDue()) { Anim::tick(); Timer::tick(); vm.dispatchAnimCallbacks(); }
        if (vm.quit()) { return ""; }
        if (!vm.nextArea().empty()) { return vm.nextArea(); }
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

        // The flowers are the menu's clickable hotspots, but they belong to the menu's
        // top-level screen only. When the options sub-screen is up (the game's own flag
        // var 0x28 == 1, set by prog26 / cleared by prog59) the menu's area nodes are no
        // longer the active set — the engine swaps to the OPTIONS context — so we stop
        // puppeteering and hit-testing the flowers entirely. Clicks then fall through to
        // Area::hitTest, which sees the option-widget nodes prog26 enabled via op 0x7.
        const bool inMenu = vm.varValue(0x28) == 0;
        // Swap normal<->highlight for the hovered flower (menu screen only).
        int hover = -1;
        if (inMenu) {
            hover = hitMenuButton(buttons, mx, my);
            for (int i = 0; i < (int)buttons.size(); ++i) {
                Anim::setVisible(buttons[i].normalSlot,    i != hover);
                Anim::setVisible(buttons[i].highlightSlot, i == hover);
            }
        }

        std::memcpy(fb.pixels(), bgPlate.data(), bgPlate.size());   // restore backdrop
        Anim::drawAll(fb);                                          // flowers (hover-aware)
        if (areaOverlay) {
            drawAreaOverlay(fb);                                    // static nodes + LINKFULL sprites
            // Menu flowers are anim-driven node-0..5 hotspots resolved via buildMenuButtons
            // (the port's own hack, separate from Area's static bbox / sprite list), so the
            // real clickable things wouldn't show otherwise. Outline them in magenta.
            uint8_t magenta = (uint8_t)nearestPaletteIndex(fb, 255, 0, 255);
            for (const MenuButton& b : buttons) {
                for (int x = b.x; x <= b.x + b.w; ++x) { putIdx(fb, x, b.y, magenta); putIdx(fb, x, b.y + b.h, magenta); }
                for (int y = b.y; y <= b.y + b.h; ++y) { putIdx(fb, b.x, y, magenta); putIdx(fb, b.x + b.w, y, magenta); }
                if (b.node >= 0) { drawNumberIdx(fb, b.x + 2, b.y + 2, b.node, magenta); }
            }
        }

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
            int hb = inMenu ? hitMenuButton(buttons, cx, cy) : -1;
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

    // CURSOR_SHEET=<path.png>: render every mouse cursor (all frames) to one PNG sheet,
    // a row per cursor, a column per frame — for eyeballing cursor sprite/colour bugs.
    if (const char* cs = std::getenv("CURSOR_SHEET")) {
        Framebuffer cfb;
        dumpCursorSheet(arc, cfb, cs);
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

    // --- NOT IN THE ORIGINAL ENGINE ---
    // Seed the master-volume script variable to full. In the original, the option vars are
    // never set by a script or a default; var 0x32 is master volume, applied by op 0x84d
    // (MIXER_SET_MASTER_VOL) which mutes when the var is 0. The engine only ever restores it
    // from a saved game (Files_LoadGameFull) — there is no shipped default and "new game"
    // doesn't load a save, so on a truly fresh profile the original also starts muted until
    // the player sets volume in Options and it persists. We have no save/load and no Options
    // screen yet, so we default var 0x32 to 10 (== full: op 0x84d computes (10-10)*300 = 0 mB
    // of attenuation) so SCM speech/music isn't silenced. Remove this once save/load lands.
    vm.setVar(0x32, 10);

    // START_AREA jumps straight to a named area (skips the intro chain) — handy
    // for headless diagnostics like AREA_PNG.
    std::string area = "entry";
    if (const char* sa = std::getenv("START_AREA")) { area = sa; }
    while (!area.empty() && strcasecmp(area.c_str(), "__end__") != 0) {
        Scene scene;
        if (!scene.load(arc, area.c_str())) break;

        Anim::reset();              // fresh anim pool per area
        Area::clearSprites();       // drop the previous area's LINKFULL hotspots (op 0x169)
        Area::resetList();          // clear the script selection list (ops 0x15e-0x165)
        Timer::clear();             // drop the previous area's script timers (ops 0x178/0x196/0x17e)
        vm.clearBackground();       // forget the previous area's backdrop (pumpFrame restore)
        Slider::clearAll();         // drop the previous area's sliders (op 0x19d)
        Audio::clearChannel(Audio::SFX);   // stop the previous room's looping SFX (op 0x15)
        vm.clearTransition();

        // Load this area's palette/backdrop/area-nodes BEFORE its lifecycle runs, so an
        // intro anim plays on the correct background + palette (not the previous area's).
        std::vector<uint8_t> bgPlate = loadAreaVisuals(scene, vm, fb, arc, area);

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
            // The lifecycle may have played startup SCMs that left their own palette in
            // place (the menu logos do), so re-apply the area palette before the loop.
            // (vvi2-style intros transition away and never reach here, so they keep the
            // pre-lifecycle palette loadAreaVisuals set.)
            Palette::load(arc, fb, area.c_str(), /*nonBlackOnly*/false);
            Palette::load(arc, fb, "GENERAL", /*nonBlackOnly*/true);
            next = runScene(scene, vm, disp, fb, bgPlate);
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
