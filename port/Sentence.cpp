#include "Sentence.h"
#include "Log.h"
#include <cstdio>
#include <map>
#include <vector>

namespace {

std::map<std::string, std::string> g_table;   // key -> Hebrew (CP1255 bytes)
bool g_loaded = false;

}  // namespace

namespace Sentence {

bool load(const std::string& path) {
    g_table.clear();
    g_loaded = false;

    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) { Log::warn("Sentence: cannot open '%s'", path.c_str()); return false; }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> d((size_t)(sz > 0 ? sz : 0));
    if (!d.empty()) { if (std::fread(d.data(), 1, d.size(), f) != d.size()) { std::fclose(f); return false; } }
    std::fclose(f);

    size_t p = 0;
    auto u32 = [&](void) -> uint32_t {
        if (p + 4 > d.size()) { return 0xFFFFFFFFu; }
        uint32_t v = (uint32_t)d[p] | (d[p+1] << 8) | (d[p+2] << 16) | ((uint32_t)d[p+3] << 24);
        p += 4;
        return v;
    };
    auto str = [&](uint32_t n) -> std::string {
        if (p + n > d.size()) { p = d.size(); return {}; }
        std::string s((const char*)&d[p], n);
        p += n;
        return s;
    };

    uint32_t first = u32();
    uint32_t version, count;
    if (first == 0) { version = u32(); count = u32(); }   // leading-0 sentinel form
    else { version = 0; count = first; }
    if (count > 100000) { Log::warn("Sentence: absurd count %u", count); return false; }

    for (uint32_t i = 0; i < count && p < d.size(); ++i) {
        std::string key = str(u32());
        if (version > 0) { (void)str(u32()); }   // speaker/voice-direction note (unused)
        std::string hebrew = str(u32());
        if (!key.empty()) { g_table[key] = hebrew; }
    }

    g_loaded = !g_table.empty();
    Log::info("Sentence: loaded %zu lines from '%s' (version %u)", g_table.size(), path.c_str(), version);
    return g_loaded;
}

bool loaded() { return g_loaded; }

const std::string* lookup(const char* key) {
    if (key == nullptr) { return nullptr; }
    auto it = g_table.find(key);
    return (it != g_table.end()) ? &it->second : nullptr;
}

uint32_t cp1255Codepoint(uint8_t b) {
    if (b < 0x80) { return b; }                          // ASCII
    if (b >= 0xE0 && b <= 0xFA) { return 0x05D0 + (b - 0xE0); }   // Hebrew alef..tav
    if (b >= 0xC0 && b <= 0xD1) { return 0x05B0 + (b - 0xC0); }   // niqqud (vowel points)
    switch (b) {                                         // common CP1255 specials
        case 0xA0: return 0x00A0;   // nbsp
        case 0xA4: return 0x20AA;   // new sheqel sign
        case 0xD3: return 0x05F3;   // geresh
        case 0xD4: return 0x05F4;   // gershayim
        case 0xD7: return 0x05F0;   // double vav
        case 0xD8: return 0x05F1;   // vav-yod
        case 0x96: return 0x2013;   // en dash
        case 0x97: return 0x2014;   // em dash
        case 0x91: return 0x2018; case 0x92: return 0x2019;   // single quotes
        case 0x93: return 0x201C; case 0x94: return 0x201D;   // double quotes
        case 0x85: return 0x2026;   // ellipsis
        default:   return 0xFFFD;   // replacement char
    }
}

std::string cp1255ToUtf8(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char b : s) {
        uint32_t cp = cp1255Codepoint(b);
        if (cp < 0x80) {
            out += (char)cp;
        } else if (cp < 0x800) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

}  // namespace Sentence
