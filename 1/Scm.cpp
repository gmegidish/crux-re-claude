#include "Scm.h"
#include "Log.h"
#include <cstring>

static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t* p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

bool Scm::parse(std::vector<uint8_t> blob) {
    blob_ = std::move(blob);
    chunks_.clear();
    frameList_.clear();

    if (blob_.size() < 16) { Log::error("Scm: blob too small (%zu)", blob_.size()); return false; }
    uint16_t magic = rd16(&blob_[0]);
    if (magic != 0x0010)
        Log::warn("Scm: unexpected magic 0x%04x (expected 0x0010)", magic);
    frameCount_ = rd16(&blob_[4]);
    rate_       = rd16(&blob_[6]);

    size_t p = 16;  // skip the 16-byte stream header
    size_t guardFrames = 0;
    while (p + 2 <= blob_.size()) {
        uint16_t nch = rd16(&blob_[p]); p += 2;
        if (nch == 0) break;                       // 0 chunk-count terminates the stream
        if (nch > 1024) { Log::error("Scm: absurd chunkCount %u at %zu", nch, p - 2); return false; }

        Frame fr;
        fr.first = (uint32_t)chunks_.size();
        fr.count = nch;
        for (uint16_t c = 0; c < nch; ++c) {
            if (p + 8 > blob_.size()) { Log::error("Scm: truncated chunk header at %zu", p); return false; }
            Chunk ch;
            ch.size  = rd32(&blob_[p]);
            ch.type  = rd16(&blob_[p + 4]);
            ch.param = rd16(&blob_[p + 6]);
            p += 8;
            ch.offset = (uint32_t)p;
            if (p + ch.size > blob_.size()) {
                Log::error("Scm: chunk payload overruns (off=%zu size=%u blob=%zu)",
                           p, ch.size, blob_.size());
                return false;
            }
            p += ch.size;
            chunks_.push_back(ch);
        }
        frameList_.push_back(fr);
        ++guardFrames;
        if (guardFrames > 1000000) { Log::error("Scm: runaway frame loop"); return false; }
    }

    Log::info("Scm: parsed %zu frames (hdr says %u), %zu chunks, consumed %zu/%zu bytes",
              frameList_.size(), frameCount_, chunks_.size(), p, blob_.size());
    return true;
}
