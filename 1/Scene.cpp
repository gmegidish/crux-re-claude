#include "Scene.h"
#include "Log.h"
#include <cstring>
#include <strings.h>   // strcasecmp — resource names are matched case-insensitively

namespace {

// Sequential cursor over the .SCN blob, mirroring the engine's Res_BunchFreadNow
// calls (which read exact byte counts in order). ok() goes false past the end.
struct Reader {
    const uint8_t* p;
    const uint8_t* end;
    bool ok = true;

    bool need(size_t n) { if ((size_t)(end - p) < n) { ok = false; return false; } return true; }
    uint8_t  u8()  { if (!need(1)) return 0; return *p++; }
    int32_t  i32() { if (!need(4)) return 0; int32_t v; std::memcpy(&v, p, 4); p += 4; return v; }
    uint32_t u32() { return (uint32_t)i32(); }

    // [u8 len][len bytes] -> std::string (engine NUL-terminates; we don't store the NUL).
    std::string str() {
        uint8_t len = u8();
        if (!need(len)) return {};
        std::string s((const char*)p, len);
        p += len;
        // legacy fix-up the engine applies in Files_FreadString.
        if (s == "FLIWINS") s = "AATALK";
        return s;
    }

    // [u32 count] then count length-prefixed strings.
    std::vector<std::string> stringTable() {
        std::vector<std::string> out;
        int32_t count = i32();
        if (count < 0 || count > 100000) { ok = false; return out; }
        out.reserve(count);
        for (int i = 0; i < count && ok; ++i) out.push_back(str());
        return out;
    }

    void skip(size_t n) { if (need(n)) p += n; }
};

} // namespace

bool Scene::load(ResArchive& arc, const char* name) {
    name_ = name ? name : "";
    // Resolve the type-4 .SCN entry (names repeat across types).
    const ResEntry* e = nullptr;
    for (const auto& en : arc.entries())
        if (en.type == 4 && strcasecmp(en.name.c_str(), name) == 0) { e = &en; break; }
    if (!e) { Log::error("Scene '%s' (type 4) not found", name); return false; }

    std::vector<uint8_t> blob = arc.read(*e);
    if (blob.empty()) { Log::error("Scene '%s': empty/failed read", name); return false; }
    Reader r{ blob.data(), blob.data() + blob.size() };

    // -- format tag: a single 4-byte little-endian version int (1 or 2). --
    //    Matches Files_LoadScn (Except.cpp): it reads 4 bytes, then branches on the
    //    whole int — `local_128[0] == 1` ⇒ old format (1-byte on-the-fly list counts),
    //    `== 2` ⇒ new (4-byte counts). Reading the version from the 4th byte instead
    //    (an earlier port) mis-detected old-format scenes. All shipped .SCNs are v2.
    int version = r.i32();
    bool oldFormat = (version == 1);
    Log::info("Scene '%s': version=%d (%s) %zu bytes",
              name, version, oldFormat ? "old" : "new", blob.size());

    // -- 7 string tables --
    areaCacheNames_ = r.stringTable();
    paletteNames_   = r.stringTable();
    exitNames_      = r.stringTable();
    animNames_      = r.stringTable();
    scaScmNames_    = r.stringTable();
    themeNames_     = r.stringTable();
    soundNames_     = r.stringTable();
    if (!r.ok) { Log::error("Scene '%s': truncated in string tables", name); return false; }
    Log::info("Scene '%s': tables ac=%zu pal=%zu exit=%zu anim=%zu sca/scm=%zu thm=%zu snd=%zu",
              name, areaCacheNames_.size(), paletteNames_.size(), exitNames_.size(),
              animNames_.size(), scaScmNames_.size(), themeNames_.size(), soundNames_.size());

    // -- area-node array (0xB0-byte records) — captured for hit-testing --
    int32_t nodeCount = r.i32();
    if (nodeCount < 0 || nodeCount > 0x96) { Log::error("Scene '%s': bad node count %d", name, nodeCount); return false; }
    {
        const size_t bytes = (size_t)nodeCount * 0xB0;
        if (r.need(bytes)) {
            areaNodes_.assign(r.p, r.p + bytes);
            areaNodeCount_ = nodeCount;
        }
        r.skip(bytes);
    }

    // -- area-cache record array (0x14-byte records, skipped) --
    int32_t cacheCount = r.i32();
    if (cacheCount < 0 || cacheCount > 1000) { Log::error("Scene '%s': bad cache count %d", name, cacheCount); return false; }
    r.skip((size_t)cacheCount * 0x14);

    // -- 15-entry cache-slot table (program IDs) --
    for (int i = 0; i < 15; ++i) cacheSlots_[i] = r.i32();
    if (!r.ok) { Log::error("Scene '%s': truncated at cache slots", name); return false; }

    // -- on-the-fly node lists = script programs --
    int32_t listCount = r.i32();
    if (listCount < 0 || listCount > 0x15e) { Log::error("Scene '%s': bad program count %d", name, listCount); return false; }
    programs_.resize(listCount);
    for (int i = 0; i < listCount && r.ok; ++i) {
        int32_t nodeSize = oldFormat ? (int32_t)r.u8() : r.i32();
        if (nodeSize < 0) { r.ok = false; break; }
        programs_[i].insns.resize(nodeSize);
        for (int j = 0; j < nodeSize && r.ok; ++j) {
            ScriptInsn& in = programs_[i].insns[j];
            in.op = r.i32(); in.a0 = r.i32(); in.a1 = r.i32(); in.a2 = r.i32();
        }
    }
    if (!r.ok) { Log::error("Scene '%s': truncated in programs", name); return false; }

    Log::info("Scene '%s': %zu programs, boot slot[0]=%d", name, programs_.size(), cacheSlots_[0]);
    return true;
}
