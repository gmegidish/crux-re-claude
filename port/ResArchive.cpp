#include "ResArchive.h"
#include "Log.h"
#include <cstring>

ResArchive::~ResArchive() { close(); }

void ResArchive::close() {
    if (resFile_) { std::fclose(resFile_); resFile_ = nullptr; }
    entries_.clear();
    byName_.clear();
    resFileSize_ = 0;
}

bool ResArchive::open(const std::string& idxPath, const std::string& resPath) {
    close();

    // --- Read the whole index into memory (it's small, ~140 KB). ---
    FILE* idx = std::fopen(idxPath.c_str(), "rb");
    if (!idx) { Log::error("ResArchive: cannot open index '%s'", idxPath.c_str()); return false; }
    std::fseek(idx, 0, SEEK_END);
    long idxSize = std::ftell(idx);
    std::fseek(idx, 0, SEEK_SET);
    std::vector<uint8_t> buf(idxSize);
    if (std::fread(buf.data(), 1, idxSize, idx) != (size_t)idxSize) {
        Log::error("ResArchive: short read on index"); std::fclose(idx); return false;
    }
    std::fclose(idx);

    if (idxSize < 4) { Log::error("ResArchive: index too small"); return false; }

    // --- Parse: u32 count, then [u8 nameLen][name][i32 type][i32 offset][i32 size]. ---
    size_t p = 0;
    uint32_t count;
    std::memcpy(&count, &buf[p], 4); p += 4;

    entries_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (p + 1 > buf.size()) { Log::error("ResArchive: truncated at entry %u (namelen)", i); break; }
        uint8_t nameLen = buf[p++];
        if (p + nameLen + 12 > buf.size()) { Log::error("ResArchive: truncated at entry %u (body)", i); break; }
        ResEntry e;
        e.name.assign(reinterpret_cast<const char*>(&buf[p]), nameLen); p += nameLen;
        std::memcpy(&e.type,   &buf[p], 4); p += 4;
        std::memcpy(&e.offset, &buf[p], 4); p += 4;
        std::memcpy(&e.size,   &buf[p], 4); p += 4;
        byName_.emplace(e.name, entries_.size());  // keep first occurrence
        entries_.push_back(std::move(e));
    }

    if (entries_.size() != count) {
        Log::warn("ResArchive: header count=%u but parsed %zu entries", count, entries_.size());
    }

    // --- Open the data file (kept open for on-demand reads). ---
    resFile_ = std::fopen(resPath.c_str(), "rb");
    if (!resFile_) { Log::error("ResArchive: cannot open data '%s'", resPath.c_str()); return false; }
    std::fseek(resFile_, 0, SEEK_END);
#if defined(_WIN32)
    resFileSize_ = (uint64_t)_ftelli64(resFile_);
#else
    resFileSize_ = (uint64_t)ftello(resFile_);
#endif
    std::fseek(resFile_, 0, SEEK_SET);

    Log::info("ResArchive: opened '%s' (%zu entries) + '%s' (%llu bytes)",
              idxPath.c_str(), entries_.size(), resPath.c_str(),
              (unsigned long long)resFileSize_);
    return true;
}

const ResEntry* ResArchive::find(const std::string& name) const {
    auto it = byName_.find(name);
    return it == byName_.end() ? nullptr : &entries_[it->second];
}

std::vector<uint8_t> ResArchive::read(const ResEntry& e) const {
    std::vector<uint8_t> out;
    if (!resFile_) { Log::error("ResArchive::read with no open data file"); return out; }
    if ((uint64_t)e.offset + e.size > resFileSize_) {
        Log::error("ResArchive::read '%s' out of range (off=%u size=%u, file=%llu)",
                   e.name.c_str(), e.offset, e.size, (unsigned long long)resFileSize_);
        return out;
    }
#if defined(_WIN32)
    _fseeki64(resFile_, e.offset, SEEK_SET);
#else
    fseeko(resFile_, (off_t)e.offset, SEEK_SET);
#endif
    out.resize(e.size);
    size_t got = std::fread(out.data(), 1, e.size, resFile_);
    if (got != e.size) {
        Log::error("ResArchive::read '%s' short read (%zu/%u)", e.name.c_str(), got, e.size);
        out.resize(got);
    }
    return out;
}

std::vector<uint8_t> ResArchive::read(const std::string& name) const {
    const ResEntry* e = find(name);
    if (!e) { Log::error("ResArchive::read: '%s' not found", name.c_str()); return {}; }
    return read(*e);
}
