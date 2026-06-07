#include "Palette.h"
#include "Log.h"
#include <strings.h>
#include <vector>
#include <cmath>

namespace Palette {

namespace {

// The current 6-bit target palette (pre-gamma), accumulated across loads — the
// base scene palette plus any GENERAL overlay. realize() paints it through the
// gamma curve into the framebuffer, mirroring the engine's target->active step.
uint8_t g_target6[768] = {0};
int     g_gamma = 0;            // g_nPalGamma (0 = off)

// SetPal_ApplyGamma (SETPAL.cpp 0x0046d200): leaves a non-zero component as
// round(-comp*gamma/64 + gamma); 0 components and gamma==0 pass through.
uint8_t applyGamma6(uint8_t comp) {
    int c = comp & 0x3f;
    if (g_gamma != 0 && c != 0) {
        int r = (int)std::lround(-(double)c * (double)g_gamma / 64.0 + (double)g_gamma);
        if (r < 0) { r = 0; }
        if (r > 63) { r = 63; }
        c = r;
    }
    return (uint8_t)c;
}

inline uint8_t exp6(uint8_t v) { v &= 0x3f; return (uint8_t)((v << 2) | (v >> 4)); }

// Paint the gamma-corrected target palette into the framebuffer.
void realize(Framebuffer& fb) {
    for (int n = 0; n < 256; ++n) {
        fb.setPaletteEntry(n,
                           exp6(applyGamma6(g_target6[n*3 + 0])),
                           exp6(applyGamma6(g_target6[n*3 + 1])),
                           exp6(applyGamma6(g_target6[n*3 + 2])));
    }
}

}  // namespace

bool load(ResArchive& arc, Framebuffer& fb, const char* name, bool nonBlackOnly) {
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries()) {
        if (en.type == 3 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    }
    if (!e) { Log::warn("Palette: '%s' (type 3) not found", name); return false; }

    std::vector<uint8_t> data = arc.read(*e);
    if (data.size() < 18) { Log::warn("Palette: '%s' too small", name); return false; }

    // RLE-decompress payload (after the 18-byte header) to 768 bytes.
    uint8_t pal[768];
    size_t out = 0;
    size_t i = 18;
    while (out < 768 && i < data.size()) {
        uint8_t b = data[i++];
        if (b == 0xFF) {
            if (i + 1 >= data.size()) { break; }
            uint8_t val = data[i++];
            uint8_t cnt = data[i++];
            for (int k = 0; k < cnt && out < 768; ++k) { pal[out++] = val; }
        } else {
            pal[out++] = b;
        }
    }
    while (out < 768) { pal[out++] = 0; }   // pad if short

    // Merge into the target palette (GENERAL overlay skips its black entries so the
    // base palette shows through), then realize the composite through gamma.
    for (int n = 0; n < 256; ++n) {
        uint8_t r = pal[n*3], g = pal[n*3 + 1], b = pal[n*3 + 2];
        if (nonBlackOnly && r == 0 && g == 0 && b == 0) { continue; }
        g_target6[n*3 + 0] = r & 0x3f;
        g_target6[n*3 + 1] = g & 0x3f;
        g_target6[n*3 + 2] = b & 0x3f;
    }
    realize(fb);
    Log::info("Palette: applied '%s' (type 3%s)", name, nonBlackOnly ? ", non-black only" : "");
    return true;
}

void setGamma(Framebuffer& fb, int gamma) {
    g_gamma = gamma;
    realize(fb);
}

}  // namespace Palette
