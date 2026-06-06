// Scene.h — load a compiled .SCN scene/room file (Files_LoadScn, 0x00420fb0).
//
// On-disk layout (resource type 4, e.g. "entry"):
//   [4]    tag: 3 signature chars + 1 version byte (version 1 = old format)
//   7 string tables, each: [u32 count] then count x ([u8 len][len bytes])
//          order: area-cache, palette, exit, animation, SCA/SCM, theme, sound
//   [u32]  area-node count, then count x 0xB0-byte records        (skipped)
//   [u32]  area-cache record count, then count x 0x14-byte records (skipped)
//   [15 x u32]  cache-slot table -> g_anAreaCacheSlots (program IDs; -1 = none)
//   [u32]  on-the-fly node-list count, then per list:
//            [u32|u8 nodeSize] then nodeSize x 16-byte instructions
//          These lists ARE the script programs (g_pScriptPrograms).
//
// The boot/intro script is the program referenced by cacheSlots[0]; op 0x71
// (RUN_SCENE) plays SCMs named in the SCA/SCM table (scaScmNames[arg0]).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "ResArchive.h"

// One decoded script instruction: opcode + three signed args (16 bytes on disk).
struct ScriptInsn { int32_t op, a0, a1, a2; };

// A script program = flat list of instructions (count is insns.size()).
struct ScriptProgram { std::vector<ScriptInsn> insns; };

class Scene {
public:
    // Load the type-4 .SCN resource `name`. Returns false (and logs) on failure.
    bool load(ResArchive& arc, const char* name);

    // Resource name this scene was loaded from (e.g. "VVQ"); "" if not loaded.
    const char* name() const { return name_.c_str(); }

    int  cacheSlot(int i) const { return (i >= 0 && i < 15) ? cacheSlots_[i] : -1; }
    const ScriptProgram* program(int id) const {
        return (id >= 0 && id < (int)programs_.size()) ? &programs_[id] : nullptr;
    }
    size_t programCount() const { return programs_.size(); }
    const std::string& scaScm(int i) const { return at(scaScmNames_, i); }
    const std::string& exitName(int i) const { return at(exitNames_, i); }
    const std::string& animName(int i) const { return at(animNames_, i); }
    const std::string& paletteName(int i) const { return at(paletteNames_, i); }
    const std::string& themeName(int i) const { return at(themeNames_, i); }  // 'thm' table (PLAY_MUSIC)
    const std::string& soundName(int i) const { return at(soundNames_, i); }   // 'snd' table (PLAY_SOUND/SPEECH)

    // Raw area-node records (count x 0xB0 bytes) for hit-testing; see Area.
    const uint8_t* areaNodes() const { return areaNodes_.data(); }
    int            areaNodeCount() const { return areaNodeCount_; }

private:
    static const std::string& at(const std::vector<std::string>& v, int i) {
        static const std::string empty;
        return (i >= 0 && i < (int)v.size()) ? v[i] : empty;
    }
    std::vector<std::string> areaCacheNames_, paletteNames_, exitNames_,
                             animNames_, scaScmNames_, themeNames_, soundNames_;
    int cacheSlots_[15];
    std::vector<ScriptProgram> programs_;
    std::string name_;
    std::vector<uint8_t> areaNodes_;     // raw 0xB0-byte area-node records
    int                  areaNodeCount_ = 0;
};
