#include "TextRender.h"
#include "Sentence.h"
#include "Log.h"
#include <cstdio>
#include <vector>
#include <algorithm>

// stb_truetype generates many -Wall/-Wextra warnings; silence them locally.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wcast-qual"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#pragma clang diagnostic pop

namespace {

stbtt_fontinfo       g_font;
std::vector<uint8_t> g_fontBuf;     // must outlive g_font
bool  g_ready = false;
float g_scale = 0.0f;
int   g_ascent = 0, g_descent = 0, g_lineGap = 0;

constexpr int kPixelHeight = 20;    // ~ the engine's GDI height 18, a touch larger
constexpr int kCoverThresh = 96;    // glyph coverage -> opaque pixel

bool loadFontFile(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) { return false; }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return false; }
    g_fontBuf.resize((size_t)sz);
    size_t got = std::fread(g_fontBuf.data(), 1, g_fontBuf.size(), f);
    std::fclose(f);
    if (got != g_fontBuf.size()) { return false; }

    int off = stbtt_GetFontOffsetForIndex(g_fontBuf.data(), 0);
    if (off < 0) { return false; }
    return stbtt_InitFont(&g_font, g_fontBuf.data(), off) != 0;
}

// Nearest palette index to an RGB (used to pick white-ish text + black-ish shadow).
int nearestIndex(const Framebuffer& fb, int r, int g, int b) {
    const uint8_t* pal = fb.palette();
    int best = 0;
    long bestD = 1L << 30;
    for (int i = 0; i < 256; ++i) {
        long dr = pal[i*3+0] - r, dg = pal[i*3+1] - g, db = pal[i*3+2] - b;
        long d = dr*dr + dg*dg + db*db;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// Total advance width (px) of a run of codepoints.
float measureRun(const std::vector<uint32_t>& cps) {
    float total = 0.0f;
    for (uint32_t cp : cps) {
        int aw, lsb;
        stbtt_GetCodepointHMetrics(&g_font, (int)cp, &aw, &lsb);
        total += aw * g_scale;
    }
    return total;
}

// Render one line of logical-order codepoints, reversed for RTL, centered on
// centerX with the baseline at baselineY, with a drop shadow.
void renderLine(Framebuffer& fb, std::vector<uint32_t> cps, int centerX, int baselineY) {
    std::reverse(cps.begin(), cps.end());
    float total = measureRun(cps);
    const int textIdx   = nearestIndex(fb, 255, 255, 255);
    const int shadowIdx = nearestIndex(fb, 0, 0, 0);
    uint8_t* px = fb.pixels();
    const int W = Framebuffer::W, H = Framebuffer::H;
    for (int pass = 0; pass < 2; ++pass) {
        const int ox = (pass == 0) ? 1 : 0;
        const int oy = (pass == 0) ? 1 : 0;
        const uint8_t idx = (pass == 0) ? (uint8_t)shadowIdx : (uint8_t)textIdx;
        float x = (float)centerX - total / 2.0f;
        for (uint32_t cp : cps) {
            int aw, lsb;
            stbtt_GetCodepointHMetrics(&g_font, (int)cp, &aw, &lsb);
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&g_font, (int)cp, g_scale, g_scale, &x0, &y0, &x1, &y1);
            int gw = x1 - x0, gh = y1 - y0;
            if (gw > 0 && gh > 0) {
                std::vector<uint8_t> bmp((size_t)gw * gh);
                stbtt_MakeCodepointBitmap(&g_font, bmp.data(), gw, gh, gw, g_scale, g_scale, (int)cp);
                int gx = (int)(x + 0.5f) + x0;
                int gy = baselineY + y0;
                for (int yy = 0; yy < gh; ++yy) {
                    for (int xx = 0; xx < gw; ++xx) {
                        if (bmp[(size_t)yy * gw + xx] < kCoverThresh) { continue; }
                        int dx = gx + xx + ox, dy = gy + yy + oy;
                        if (dx < 0 || dx >= W || dy < 0 || dy >= H) { continue; }
                        px[dy * W + dx] = idx;
                    }
                }
            }
            x += aw * g_scale;
        }
    }
}

}  // namespace

namespace TextRender {

bool init() {
    if (g_ready) { return true; }
    // Prefer fonts with BOTH Hebrew and Latin/punctuation coverage (SFHebrew lacks
    // Latin punctuation, so commas/periods render as .notdef boxes).
    const char* candidates[] = {
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/ArialHB.ttc",
        "/System/Library/Fonts/SFHebrew.ttf",
    };
    for (const char* p : candidates) {
        if (loadFontFile(p)) { Log::info("TextRender: using font '%s'", p); g_ready = true; break; }
    }
    if (!g_ready) { Log::warn("TextRender: no Hebrew TTF found"); return false; }
    g_scale = stbtt_ScaleForPixelHeight(&g_font, (float)kPixelHeight);
    stbtt_GetFontVMetrics(&g_font, &g_ascent, &g_descent, &g_lineGap);
    return true;
}

bool ready() { return g_ready; }

void drawSentence(Framebuffer& fb, const std::string& cp1255, int centerX, int baselineY) {
    if (!g_ready || cp1255.empty()) { return; }

    // Decode CP1255 -> codepoints (logical order).
    std::vector<uint32_t> all;
    all.reserve(cp1255.size());
    for (unsigned char b : cp1255) { all.push_back(Sentence::cp1255Codepoint(b)); }

    // Greedy word-wrap to fit the screen width (the engine auto-wraps long lines).
    const float maxW = (float)fb.width() - 40.0f;
    std::vector<std::vector<uint32_t>> lines;
    std::vector<uint32_t> cur;
    size_t i = 0;
    while (i < all.size()) {
        size_t j = i;
        while (j < all.size() && all[j] != ' ') { ++j; }      // next word [i, j)
        std::vector<uint32_t> word(all.begin() + i, all.begin() + j);
        std::vector<uint32_t> cand = cur;
        if (!cand.empty()) { cand.push_back(' '); }
        cand.insert(cand.end(), word.begin(), word.end());
        if (cur.empty() || measureRun(cand) <= maxW) { cur.swap(cand); }
        else { lines.push_back(cur); cur = word; }
        i = j;
        while (i < all.size() && all[i] == ' ') { ++i; }       // skip the space(s)
    }
    if (!cur.empty()) { lines.push_back(cur); }

    // Stack lines so the last logical line sits on baselineY, earlier ones above.
    const int lineH = kPixelHeight + 4;
    const int n = (int)lines.size();
    for (int k = 0; k < n; ++k) {
        renderLine(fb, lines[k], centerX, baselineY - (n - 1 - k) * lineH);
    }
}

}  // namespace TextRender
