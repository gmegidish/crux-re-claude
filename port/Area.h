// Area.h — area-node (clickable hotspot) module for the Crux/Granny SDL2 port.
//
// Area nodes are the scene's clickable regions. They are loaded from the .SCN
// file's area-node section: [u32 count] then count x 0xB0-byte records. Each
// record is read as 44 little-endian int32s (word index k = byte offset k*4):
//
//   p[0..3]      bbox x1,y1,x2,y2 (ABSOLUTE, inclusive). Hit = x1<=x<=x2 && y1<=y<=y2.
//   p[4]         flags. Byte at record offset 0x11 (bits 16-23) must be 0 to be
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

} // namespace Area
