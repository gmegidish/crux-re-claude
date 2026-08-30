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
// Verb codes (from Adv_CursorHandler @0x0040eec0): 0=left click, 1=right click,
// 2=middle click, 4=mouse-ENTER hotspot, 5=hover-hold, 6=mouse-LEAVE hotspot,
// 8=idle/timer, 10=hotspot-changed-while-held, 0xb=keyboard shortcut. Verbs 4/6 are
// fired by Adv_UpdateHotspot on hotspot change, NOT by a click. Edge/corner EXIT
// nodes carry ONLY a verb-4 handler (no verb-0): they trigger on cursor-enter, which
// walks the character to the screen edge and transitions rooms. The current port only
// dispatches verb 0/1 on click, so those exits aren't reachable by their real path.
#pragma once
#include <cstdint>

namespace Area {

// Copy/parse `count` 0xB0-byte records from `records`. Replaces any prior data.
// Pass a null pointer or non-positive count to load nothing.
void load(const uint8_t* records, int count);

// Load the area-cache hit-strip records (count x 0x14 = {x1,y1,x2,y2,nodeId}). These are
// per-scanline strips tracing each node's painted clickable shape (the engine's secondary
// hit list, Ani32). Replaces any prior set; load nothing for a null/zero count.
void loadCacheRecords(const uint8_t* records, int count);

// The node id of the cache strip covering (x,y) — only if that node is currently
// hit-testable (type != 2 && enabled). -1 if no strip covers the point. This is how
// degenerate-bbox nodes (e.g. menu flowers) get hit by their painted shape.
int  cacheRecordAt(int x, int y);

// A representative interior point of `node` (first cache strip's centre, else the node bbox
// centre) — for headless hover/click simulation. Returns false if the node has no geometry.
bool nodeAnchor(int node, int& x, int& y);

// Drop all loaded nodes.
void clear();

// Hit-test screen point (x,y) in [0,639]x[0,479]. Among nodes with type != 2,
// enabled (byte 0x11 == 0), and AABB-containing (x,y), returns the index of the
// one with the highest z/priority (p[0x24]). Returns -1 if none match.
int hitTest(int x, int y);

// Node index whose tag (p[5]) == `tag`, or -1 (engine Area_FindNodeByTag).
int findNodeByTag(int tag);

// LINKFULL (RunProg op 0x169 flags=0 / 0x2c3 flags=1): register the anim in slot
// `animSlot` as a dynamic clickable hotspot whose hit-rect is the anim's painted
// frame bbox (Anim::frameBounds), resolving to the node whose tag == `tag` at hit
// time. Mirrors the engine's g_anAreaSpriteList; hitTest() checks these before the
// static nodes. Capped at 20 ("too many moving areas").
void linkFull(int animSlot, int tag, int flags);

// Drop all LINKFULL links (call on area change).
void clearSprites();

// REMOVE_AREA_SPRITE (RunProg op 0x41): remove the sprite link registered for the node
// tagged `tag`. Engine: node=Area_FindNodeByTag(a0); locate the sprite whose areaId==node;
// Area_RemoveSpriteAt(idx) (compacts g_anAreaSpriteList). The port stores the tag on the
// link, so we remove the first link whose tag==`tag`. Returns true if one was removed.
bool removeSprite(int tag);

// Drop the sprite link registered for anim slot `animSlot`, if any. Anim_MarkForDump
// (@0x00405810) does this before queueing a slot: an anim about to be dumped must stop
// being a clickable hotspot. Returns true if one was removed.
bool removeSpriteBySlot(int animSlot);

// LINKFULL sprite-hotspot introspection (for the debug overlay). spriteInfo fills
// the link's anim slot, its resolved node (findNodeByTag, or -1), and flags.
int  spriteCount();
bool spriteInfo(int i, int& animSlot, int& node, int& flags);

// Selection list (engine g_anAreaList + cursor): a cursor-indexed int list the script
// builds/walks via RunProg ops 0x15e-0x165 to accumulate area-query results. Ports the
// src/AREAS.cpp Area_*List family. resetList clears it (call on area change).
void resetList();                 // 0x15e: count=0, cursor=0
void rewindList();                // 0x15f: cursor=0
void seekListEnd();               // 0x160: cursor=count-1
int  listNext();                  // 0x161: advance cursor; 0 ok / -1 at end (clamped)
int  listPrev();                  // 0x162: retreat cursor; 0 ok / -1 at start (clamped)
int  listGet();                   // 0x163: value at cursor
void listSet(int value);          // 0x164: write value at cursor
void listAppend();                // 0x165: append a 0 slot, cursor -> it

// Low byte of p[4] for `node`, or -1 if `node` is out of range.
int cursorId(int node);

// Resolve clicked `node` + `verb` to a handler script id. Returns the handler
// (p[6+k] where p[0x15+k]==verb), or -1 if none / out of range.
int verbHandler(int node, int verb);

// Enumerate a node's verb table directly: fills `verb`/`handler` for slot `k`
// (0..14) and returns false when the slot is empty. Lets a caller ask "which verbs
// does this node actually define?" rather than probing one verb at a time.
bool verbSlot(int node, int k, int& verb, int& handler);

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
