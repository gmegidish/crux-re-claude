// Log.h — lightweight logging for the Crux/Granny SDL2 reimplementation.
//
// Levels are printed with a tag and (optionally) source location. Set the
// global level with Log::setLevel(). Everything goes to stderr so it stays
// separate from any stdout the game itself might produce.
#pragma once
#include <cstdio>
#include <cstdarg>

namespace Log {

enum Level { TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, NONE = 5 };

inline Level& levelRef() { static Level lvl = INFO; return lvl; }
inline void setLevel(Level l) { levelRef() = l; }
inline const char* tag(Level l) {
    switch (l) {
        case TRACE: return "TRACE";
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO ";
        case WARN:  return "WARN ";
        case ERROR: return "ERROR";
        default:    return "?????";
    }
}

inline void vlog(Level l, const char* fmt, va_list ap) {
    if (l < levelRef()) return;
    std::fprintf(stderr, "[%s] ", tag(l));
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
}

#define LOG_FN(NAME, LVL) \
    inline void NAME(const char* fmt, ...) { \
        va_list ap; va_start(ap, fmt); vlog(LVL, fmt, ap); va_end(ap); }

LOG_FN(trace, TRACE)
LOG_FN(debug, DEBUG)
LOG_FN(info,  INFO)
LOG_FN(warn,  WARN)
LOG_FN(error, ERROR)
#undef LOG_FN

} // namespace Log
