// dumpprog.cpp — standalone tool: dump a scene program's opcodes + parameters
// WITHOUT executing anything. Handy for seeing every opcode a script uses up
// front, instead of discovering them one runtime-crash at a time.
//
// Usage:
//   dumpprog [datadir] <scene> <progId|all>
//
//   datadir   directory holding ADVENT.IDX/ADVENT.RES (default "..")
//   scene     scene/area name, e.g. vvi2, menu, entry
//   progId    program index, or "all" to dump every program in the scene
//
// Examples:
//   ./dumpprog vvi2 70
//   ./dumpprog .. menu all
#include "ResArchive.h"
#include "Scene.h"
#include "Log.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

static void dumpProgram(const Scene& sc, const char* scene, int id) {
    const ScriptProgram* p = sc.program(id);
    if (p == nullptr) {
        std::printf("program %d: <none> (scene has %zu program(s))\n",
                    id, sc.programCount());
        return;
    }
    std::printf("scene '%s' program %d — %zu instruction(s)\n",
                scene, id, p->insns.size());

    std::map<int, int> histogram;   // opcode -> count, ordered
    for (size_t pc = 0; pc < p->insns.size(); ++pc) {
        const ScriptInsn& in = p->insns[pc];
        std::printf("  %3zu: op=0x%-5x a0=%-7d a1=%-7d a2=%-7d\n",
                    pc, in.op, in.a0, in.a1, in.a2);
        histogram[in.op]++;
    }

    std::printf("  -- %zu distinct opcode(s): ", histogram.size());
    bool first = true;
    for (const auto& kv : histogram) {
        std::printf("%s0x%x(x%d)", first ? "" : " ", kv.first, kv.second);
        first = false;
    }
    std::printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
                     "usage: %s [datadir] <scene> <progId|all>\n"
                     "  e.g. %s vvi2 70   |   %s .. menu all\n",
                     argv[0], argv[0], argv[0]);
        return 2;
    }

    // (scene prog) with default datadir "..", or (datadir scene prog).
    const char* dataDir;
    const char* scene;
    const char* progArg;
    if (argc >= 4) { dataDir = argv[1]; scene = argv[2]; progArg = argv[3]; }
    else           { dataDir = "..";    scene = argv[1]; progArg = argv[2]; }

    Log::setLevel(Log::ERROR);   // silence the loader's info/warn chatter
    ResArchive arc;
    if (!arc.open(std::string(dataDir) + "/ADVENT.IDX",
                  std::string(dataDir) + "/ADVENT.RES")) {
        std::fprintf(stderr, "error: cannot open archive in '%s'\n", dataDir);
        return 1;
    }
    Scene sc;
    if (!sc.load(arc, scene)) {
        std::fprintf(stderr, "error: scene '%s' not found / failed to load\n", scene);
        return 1;
    }

    if (std::strcmp(progArg, "all") == 0) {
        for (int id = 0; id < (int)sc.programCount(); ++id) { dumpProgram(sc, scene, id); }
    } else {
        dumpProgram(sc, scene, std::atoi(progArg));
    }
    return 0;
}
