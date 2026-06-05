// Area.h — area-node (clickable hotspot) module for the Crux/Granny SDL2 port.
//
// Area nodes are the scene's clickable regions. They are loaded from the .SCN
// file's area-node section: [u32 count] then count x 0xB0-byte records. Each
// record is read as 44 little-endian int32s (word index k = byte offset k*4):
//
//   p[0..3]      bbox x1,y1,x2,y2 (ABSOLUTE, inclusive). Hit = x1<=x<=x2 && y1<=y<=y2.
//   p[4]         flags. Byte at record offset 0x11 (bits 8-15) must be 0 to be
//                enabled/hit-testable. LOW byte of p[4] is the node's cursor id.
//   p[5]         tag (script-facing node id).
//   p[6..0x14]   handler-script-id array (15 ints, parallel to verb array; -1 = none).
//   p[0x15..0x23] verb-code array (15 ints, parallel). Resolve: for k in 0..14,
//                if p[0x15+k]==verb then handler = p[6+k].
//   p[0x24]      z/priority (higher wins in hit-test).
//   p[0x25]      type/kind. type==2 => excluded from hit-test.
//   p[0x26..0x2b] unused for hit-test.
//
// Verb codes: 0=look (left click), 1=use (right click), 2=middle.
#pragma once
#include <cstdint>

namespace Area {

// Copy/parse `count` 0xB0-byte records from `records`. Replaces any prior data.
// Pass a null pointer or non-positive count to load nothing.
void load(const uint8_t* records, int count);

// Drop all loaded nodes.
void clear();

// Hit-test screen point (x,y) in [0,639]x[0,479]. Among nodes with type != 2,
// enabled (byte 0x11 == 0), and AABB-containing (x,y), returns the index of the
// one with the highest z/priority (p[0x24]). Returns -1 if none match.
int hitTest(int x, int y);

// Low byte of p[4] for `node`, or -1 if `node` is out of range.
int cursorId(int node);

// Resolve clicked `node` + `verb` to a handler script id. Returns the handler
// (p[6+k] where p[0x15+k]==verb), or -1 if none / out of range.
int verbHandler(int node, int verb);

// Number of loaded nodes.
int count();

// Set the enabled byte (bits 8-15 of p[4]) to `value` (0 or 1) on every node
// whose tag (p[5]) == `tag`. Mirrors RunProg_Exec's AREA_NODE_DISABLE (0x7,
// value 0) / AREA_NODE_ENABLE (0x8, value 1). Note the engine's polarity: the
// hit-test treats byte 0x11 == 0 as hit-testable, so value 0 makes matching
// nodes clickable and value 1 makes them non-clickable. Returns true if any
// node's byte actually changed (the engine's "cursor dirty" condition).
bool setEnabledByteByTag(int tag, int value);

// Per-node fields for debugging/visualization.
struct NodeInfo {
    int x1, y1, x2, y2;   // absolute, inclusive bbox
    int flags;            // raw p[4]
    int cursor;           // low byte of flags
    int enabledByte;      // bits 8-15 of flags (0 => enabled)
    int tag;              // p[5] script-facing id
    int type;             // p[0x25] (2 => excluded from hit-test)
    int z;                // p[0x24] priority
    bool hittable;        // type != 2 && enabledByte == 0
};

// Fill `out` for `node`. Returns false if `node` is out of range.
bool nodeInfo(int node, NodeInfo& out);

} // namespace Area
