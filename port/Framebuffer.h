// Framebuffer.h — an 8-bit paletted framebuffer, the way the original engine drew.
//
// The game renders into a 640x480 8bpp surface and realises a 256-colour palette.
// We keep that model: an index buffer + a 256-entry RGB palette, converted to
// RGBA8888 on demand for an SDL streaming texture (or a PPM dump when headless).
#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Framebuffer {
public:
    static constexpr int W = 640;
    static constexpr int H = 480;

    Framebuffer() : pixels_(W * H, 0), pal_(256 * 3, 0) {}

    int width()  const { return W; }
    int height() const { return H; }

    uint8_t*       pixels()       { return pixels_.data(); }
    const uint8_t* pixels() const { return pixels_.data(); }

    void clear(uint8_t index = 0) { std::fill(pixels_.begin(), pixels_.end(), index); }

    // Set the palette from 256 RGB triples (8-bit per channel, 768 bytes).
    void setPaletteRGB(const uint8_t* rgb768) {
        std::copy(rgb768, rgb768 + 256 * 3, pal_.begin());
    }
    // Set the palette from the game's 6-bit-per-channel format (values 0..63).
    void setPalette6bit(const uint8_t* rgb768_6) {
        for (int i = 0; i < 256 * 3; ++i) {
            uint8_t v = rgb768_6[i] & 0x3f;
            pal_[i] = (uint8_t)((v << 2) | (v >> 4));  // 6-bit -> 8-bit
        }
    }
    void setPaletteEntry(int idx, uint8_t r, uint8_t g, uint8_t b) {
        pal_[idx*3+0] = r; pal_[idx*3+1] = g; pal_[idx*3+2] = b;
    }
    const uint8_t* palette() const { return pal_.data(); }

    // Expand the indexed buffer into a caller-provided RGBA8888 buffer (W*H*4).
    void toRGBA(uint32_t* out) const {
        for (int i = 0; i < W * H; ++i) {
            const uint8_t* c = &pal_[pixels_[i] * 3];
            out[i] = 0xff000000u | (c[0] << 16) | (c[1] << 8) | c[2];  // ARGB8888
        }
    }

    // Dump current frame as a binary PPM (P6) — for headless verification.
    bool savePPM(const std::string& path) const;

private:
    std::vector<uint8_t> pixels_;  // W*H indices
    std::vector<uint8_t> pal_;     // 256*3 RGB (8-bit)
};
