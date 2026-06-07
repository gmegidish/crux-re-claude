#include "Framebuffer.h"
#include "Log.h"
#include <cstdio>

bool Framebuffer::savePPM(const std::string& path) const {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { Log::error("savePPM: cannot open '%s'", path.c_str()); return false; }
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    std::vector<uint8_t> row(W * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const uint8_t* c = &pal_[pixels_[y * W + x] * 3];
            row[x*3+0] = c[0]; row[x*3+1] = c[1]; row[x*3+2] = c[2];
        }
        std::fwrite(row.data(), 1, row.size(), f);
    }
    std::fclose(f);
    Log::info("savePPM: wrote '%s' (%dx%d)", path.c_str(), W, H);
    return true;
}
