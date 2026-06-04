// Display.h — SDL2 window that presents the 8-bit Framebuffer.
//
// Creates a 640x480 logical surface scaled to a resizable window, backed by a
// streaming ARGB8888 texture we update from Framebuffer::toRGBA each present().
// open() degrades gracefully when there's no display (headless), so the engine
// can still run the script/decoder path and dump frames to disk.
#pragma once
#include "Framebuffer.h"
#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

// Result of pumping the event queue during playback.
//   Continue — nothing notable; keep playing
//   Skip     — user pressed a key / clicked: abort the current cutscene (skippable)
//   Quit     — user closed the window: tear the whole program down
enum class PumpResult { Continue, Skip, Quit };

class Display {
public:
    bool open(const char* title, int scale = 1);
    void close();
    bool isOpen()     const { return window_ != nullptr; }
    bool isHeadless() const { return headless_; }
    // True for a real on-screen window (pace playback to the video's fps). False
    // when headless or running the SDL "dummy" driver — then run flat-out (turbo),
    // which is what we want for fast offscreen iteration/CI.
    bool isRealtime() const { return realtime_; }

    // Pump SDL events. Quit on window close; Skip on key/mouse press. Also
    // updates the mouse position and queues a click for takeClick().
    PumpResult pump();

    int  mouseX() const { return mouseX_; }
    int  mouseY() const { return mouseY_; }
    // If a mouse button was pressed since the last call, report it (and consume).
    // button: SDL_BUTTON_LEFT/RIGHT/MIDDLE. Returns false if no click pending.
    bool takeClick(int& x, int& y, int& button);

    // Push a framebuffer to the screen.
    void present(const Framebuffer& fb);

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  texture_  = nullptr;
    bool          headless_ = false;
    bool          realtime_ = false;   // real visible window → throttle to fps
    int           mouseX_ = 0, mouseY_ = 0;
    bool          clickPending_ = false;
    int           clickButton_ = 0;
    std::vector<uint32_t> rgba_;
};
