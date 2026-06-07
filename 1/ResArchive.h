// ResArchive.h — the Crux "bunch" resource archive (ADVENT.IDX + ADVENT.RES).
//
// Reverse-engineered from READRES.cpp. Format:
//   ADVENT.IDX:  [u32 count][ entry x count ]
//     entry =    [u8 nameLen][char name[nameLen]][i32 type][i32 offset][i32 size]
//   ADVENT.RES:  raw concatenated blobs; entry data is RES[offset .. offset+size).
//
// The original engine streamed this asynchronously off CD for pacing; we just
// open ADVENT.RES once and read on demand (seek+read). Names may repeat across
// entries (different type/disk), so lookups return the first match by default.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

struct ResEntry {
    std::string name;
    int32_t     type   = 0;   // resource type / disk selector (field 0)
    uint32_t    offset = 0;   // byte offset into ADVENT.RES
    uint32_t    size   = 0;   // byte length
};

class ResArchive {
public:
    ResArchive() = default;
    ~ResArchive();

    // Open the index + data files. Returns false (and logs) on failure.
    bool open(const std::string& idxPath, const std::string& resPath);
    void close();

    size_t count() const { return entries_.size(); }
    const std::vector<ResEntry>& entries() const { return entries_; }

    // First entry matching name (case-sensitive), or nullptr.
    const ResEntry* find(const std::string& name) const;

    // Read an entry's bytes from ADVENT.RES. Returns empty vector on failure.
    std::vector<uint8_t> read(const ResEntry& e) const;
    std::vector<uint8_t> read(const std::string& name) const;

    uint64_t resFileSize() const { return resFileSize_; }

private:
    std::vector<ResEntry> entries_;
    std::unordered_map<std::string, size_t> byName_;  // name -> first index
    FILE*    resFile_     = nullptr;
    uint64_t resFileSize_ = 0;
};
