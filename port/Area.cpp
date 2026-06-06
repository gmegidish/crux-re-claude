#include "Area.h"
#include "Anim.h"
#include "Log.h"
#include <cstring>
#include <vector>

namespace {

// One area-node record is 0xB0 bytes = 44 int32s.
const int kRecordInts  = 44;
const int kRecordBytes = kRecordInts * 4; // 0xB0

// Word indices into a record (see Area.h for the full layout).
const int kBboxX1   = 0;
const int kBboxY1   = 1;
const int kBboxX2   = 2;
const int kBboxY2   = 3;
const int kFlags    = 4;
const int kHandler0 = 6;     // p[6..0x14]
const int kVerb0    = 0x15;  // p[0x15..0x23]
const int kVerbSlots = 15;
const int kZPriority = 0x24;
const int kType      = 0x25;

const int kTypeExcluded = 2; // type==2 => excluded from hit-test

// Flat array of records, kRecordInts ints each. Owned (copied at load).
std::vector<int32_t> g_records;
int g_count = 0;

// LINKFULL sprite-area links (RunProg op 0x169/0x2c3 -> g_anAreaSpriteList). Each
// links an anim slot's painted frame bbox to a node, resolved by tag at hit time (so
// it's independent of node-load order). flags 0 = immediate hit. Engine caps at 20.
struct SpriteLink { int animSlot; int tag; int flags; };
std::vector<SpriteLink> g_sprites;

// Read one int32 from record `node` at word index `w`. Bounds-safe.
int32_t recordInt(int node, int w) {
    if (node < 0 || node >= g_count) {
        return 0;
    }
    if (w < 0 || w >= kRecordInts) {
        return 0;
    }
    return g_records[(size_t)node * kRecordInts + w];
}

// Enabled byte. The engine tests `((flags << 16) >> 24) == 0` (AREAS.cpp
// Area_FindAt), which extracts bits 8-15 of p[4] — i.e. the byte at record
// offset 0x11. (An earlier version read bits 16-23 / byte 0x12, which is the
// WRONG byte and filtered out every clickable node.)
int enabledByte(int node) {
    int32_t flags = recordInt(node, kFlags);
    return (int)((flags >> 8) & 0xff);
}

} // namespace

void Area::load(const uint8_t* records, int count) {
    g_records.clear();
    g_count = 0;
    if (!records || count <= 0) {
        Log::info("Area: loaded 0 nodes");
        return;
    }

    g_records.resize((size_t)count * kRecordInts);
    // Records are little-endian x86 ints; a direct memcpy matches on the host.
    std::memcpy(g_records.data(), records, (size_t)count * kRecordBytes);
    g_count = count;
    Log::info("Area: loaded %d node(s)", g_count);
}

void Area::clear() {
    g_records.clear();
    g_count = 0;
    g_sprites.clear();
}

int Area::findNodeByTag(int tag) {
    for (int node = 0; node < g_count; ++node) {
        if (recordInt(node, 5) == tag) {
            return node;
        }
    }
    return -1;
}

void Area::linkFull(int animSlot, int tag, int flags) {
    // Engine fatally asserts a current STANI slot ("No stani to LINKFULL to"); we guard.
    if (animSlot < 0) {
        return;
    }
    // Engine cap (0x14): "too many moving areas".
    if (g_sprites.size() >= 20) {
        return;
    }
    g_sprites.push_back({ animSlot, tag, flags });
}

void Area::clearSprites() {
    g_sprites.clear();
}

bool Area::removeSprite(int tag) {
    for (size_t i = 0; i < g_sprites.size(); ++i) {
        if (g_sprites[i].tag == tag) {
            g_sprites.erase(g_sprites.begin() + (long)i);
            return true;
        }
    }
    return false;
}

int Area::spriteCount() {
    return (int)g_sprites.size();
}

bool Area::spriteInfo(int i, int& animSlot, int& node, int& flags) {
    if (i < 0 || i >= (int)g_sprites.size()) {
        return false;
    }
    const SpriteLink& s = g_sprites[i];
    animSlot = s.animSlot;
    node = findNodeByTag(s.tag);
    flags = s.flags;
    return true;
}

int Area::hitTest(int x, int y) {
    // Dynamic LINKFULL sprite-areas first (engine Area_FindAt step 1): the hit-rect is
    // the linked anim's painted frame bbox (Anim::frameBounds), resolved to the node by
    // tag. flags==0 is an immediate hit; flags!=0 is a lower-priority candidate (the
    // engine z-orders those via the character walk-table, which the port doesn't model,
    // so the static nodes win over a flags!=0 sprite; last-registered wins among them).
    int spriteBest = -1;
    for (int i = (int)g_sprites.size(); i-- > 0; ) {
        const SpriteLink& s = g_sprites[i];
        int bx, by, bw, bh;
        if (!Anim::frameBounds(s.animSlot, bx, by, bw, bh)) {
            continue;
        }
        if (x < bx || x > bx + bw || y < by || y > by + bh) {
            continue;
        }
        int node = findNodeByTag(s.tag);
        if (node < 0) {
            continue;
        }
        if (s.flags == 0) {
            return node;            // immediate hit
        }
        if (spriteBest < 0) {
            spriteBest = node;
        }
    }

    int best = -1;
    int32_t bestZ = 0;
    for (int node = 0; node < g_count; ++node) {
        // type==2 nodes are not hit-testable.
        if (recordInt(node, kType) == kTypeExcluded) {
            continue;
        }
        // Enabled byte (offset 0x11) must be 0.
        if (enabledByte(node) != 0) {
            continue;
        }
        // AABB containment (absolute, inclusive bounds).
        int32_t x1 = recordInt(node, kBboxX1);
        int32_t y1 = recordInt(node, kBboxY1);
        int32_t x2 = recordInt(node, kBboxX2);
        int32_t y2 = recordInt(node, kBboxY2);
        if (x < x1 || x > x2 || y < y1 || y > y2) {
            continue;
        }
        int32_t z = recordInt(node, kZPriority);
        if (best < 0 || z > bestZ) {
            best = node;
            bestZ = z;
        }
    }
    return (best >= 0) ? best : spriteBest;
}

int Area::cursorId(int node) {
    if (node < 0 || node >= g_count) {
        return -1;
    }
    return (int)(recordInt(node, kFlags) & 0xff);
}

int Area::verbHandler(int node, int verb) {
    if (node < 0 || node >= g_count) {
        return -1;
    }
    for (int k = 0; k < kVerbSlots; ++k) {
        if (recordInt(node, kVerb0 + k) == verb) {
            return (int)recordInt(node, kHandler0 + k);
        }
    }
    return -1;
}

int Area::count() {
    return g_count;
}

// Mirror of RunProg_Exec cases 0x7 (value 0) / 0x8 (value 1): walk every node,
// and for those tagged `tag` (p[5]) whose enabled byte (bits 8-15 of p[4])
// differs from `value`, rewrite that byte while preserving the rest of p[4]
// (the engine's CONCAT22(...high half..., ...) keeps the high 16 bits and the
// low cursor-id byte). The engine guards the write on the current byte != target
// (0x7: != 0, 0x8: != 1), so we only count a change when it really flips.
bool Area::setEnabledByteByTag(int tag, int value) {
    bool changed = false;
    const int32_t want = (int32_t)(value & 0xff);
    for (int node = 0; node < g_count; ++node) {
        if (recordInt(node, 5) != tag) {
            continue;
        }
        int32_t& flags = g_records[(size_t)node * kRecordInts + kFlags];
        if (((flags >> 8) & 0xff) == want) {
            continue;
        }
        flags = (flags & ~0x0000ff00) | (want << 8);
        changed = true;
    }
    return changed;
}

bool Area::nodeInfo(int node, NodeInfo& out) {
    if (node < 0 || node >= g_count) {
        return false;
    }
    out.x1 = recordInt(node, kBboxX1);
    out.y1 = recordInt(node, kBboxY1);
    out.x2 = recordInt(node, kBboxX2);
    out.y2 = recordInt(node, kBboxY2);
    out.flags = recordInt(node, kFlags);
    out.cursor = out.flags & 0xff;
    out.enabledByte = enabledByte(node);
    out.tag = recordInt(node, 5);
    out.type = recordInt(node, kType);
    out.z = recordInt(node, kZPriority);
    out.hittable = (out.type != kTypeExcluded) && (out.enabledByte == 0);
    return true;
}
