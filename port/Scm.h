// Scm.h — parser for the SCM cutscene/video container (type-16 resources).
//
// Format (validated against INTRO1/2/3 — see SCM_FORMAT.md):
//   [16-byte header: u16 magic=0x10, u16 ver, u16 frameCount, u16 rate, 8 reserved]
//   per frame: [u16 chunkCount(0=end)] + chunkCount * { [u32 size][u16 type][u16 param][data] }
//
// We parse a whole in-memory blob into frames-of-chunks; the caller decodes
// the video (0x10) and palette (0x02) chunks per frame.
#pragma once
#include <cstdint>
#include <vector>

class Scm {
public:
    enum ChunkType : uint16_t {
        VIDEO   = 0x0010,
        PALETTE = 0x0002,
        SPEECH  = 0x0100,
        MUSIC0  = 0x0040,  // 0x40..0x43
        AUDIO0  = 0x0080,  // 0x80..0x83
        LIP0    = 0x0400,  // 0x400..0x402
        TEXT    = 0x1000,
    };

    struct Chunk {
        uint16_t type   = 0;
        uint16_t param  = 0;
        uint32_t offset = 0;   // offset of payload within the blob
        uint32_t size   = 0;   // payload byte count
    };
    struct Frame {
        uint32_t first = 0;    // index into chunks_
        uint32_t count = 0;    // number of chunks in this frame
    };

    // Parse a complete SCM blob. Returns false on malformed data (logs why).
    bool parse(std::vector<uint8_t> blob);

    uint16_t frameCount() const { return frameCount_; }
    uint16_t rate()       const { return rate_; }
    size_t   frames()     const { return frameList_.size(); }

    const Frame& frame(size_t i) const { return frameList_[i]; }
    const Chunk& chunk(size_t i) const { return chunks_[i]; }
    const uint8_t* data() const { return blob_.data(); }
    const uint8_t* payload(const Chunk& c) const { return blob_.data() + c.offset; }

private:
    std::vector<uint8_t> blob_;
    std::vector<Chunk>   chunks_;
    std::vector<Frame>   frameList_;
    uint16_t frameCount_ = 0;
    uint16_t rate_       = 0;
};
