// Sentence.h — SENTENCE.BIN loader: the game's speech/subtitle text table.
//
// SENTENCE.BIN (a standalone file beside the game data) holds the Hebrew subtitle
// text for every spoken line, keyed by an ASCII sentence id (the same number that
// names the speech audio + lipsync resources). Strings are Windows CP1255
// (Hebrew). Format (Speech_Init @ 0x004750d0):
//   [u32 0][u32 version][u32 count]            (leading 0 sentinel -> re-read)
//   count x { u32 keyLen, key; (version>0:) u32 midLen, speaker; u32 txtLen, hebrew }
// The script's START_SPEECH op looks a line up by its key (string compare).
#pragma once
#include <string>
#include <cstdint>

namespace Sentence {

// Parse SENTENCE.BIN at `path`. Returns false if missing/malformed.
bool load(const std::string& path);
bool loaded();

// The Hebrew (CP1255) bytes for sentence `key`, or nullptr if not found.
const std::string* lookup(const char* key);

// CP1255 byte -> Unicode codepoint (for glyph rendering).
uint32_t cp1255Codepoint(uint8_t b);

// Decode a CP1255 string to UTF-8 (for logging/terminal display).
std::string cp1255ToUtf8(const std::string& s);

}  // namespace Sentence
