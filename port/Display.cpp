#include "Display.h"
#include "Log.h"
#include <SDL.h>

bool Display::open(const char* title, int scale) {
    rgba_.resize(Framebuffer::W * Framebuffer::H);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        Log::warn("Display: SDL_Init(VIDEO) failed (%s) — running headless", SDL_GetError());
        headless_ = true;
        return true;  // headless is a valid mode
    }

    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               Framebuffer::W * scale, Framebuffer::H * scale,
                               SDL_WINDOW_RESIZABLE);
    if (!window_) {
        Log::warn("Display: CreateWindow failed (%s) — running headless", SDL_GetError());
        headless_ = true;
        return true;
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, 0);
    if (!renderer_) {
        Log::warn("Display: CreateRenderer failed (%s) — running headless", SDL_GetError());
        SDL_DestroyWindow(window_); window_ = nullptr; headless_ = true; return true;
    }
    SDL_RenderSetLogicalSize(renderer_, Framebuffer::W, Framebuffer::H);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, Framebuffer::W, Framebuffer::H);
    if (!texture_) {
        Log::error("Display: CreateTexture failed (%s)", SDL_GetError());
        close(); headless_ = true; return true;
    }
    // The "dummy" video driver "succeeds" without a real window — treat it like
    // headless for pacing so offscreen runs go full speed (turbo) instead of fps.
    const char* drv = SDL_GetCurrentVideoDriver();
    realtime_ = !(drv && SDL_strcmp(drv, "dummy") == 0);
    Log::info("Display: %dx%d window open (scale %d, driver=%s, %s)",
              Framebuffer::W, Framebuffer::H, scale, drv ? drv : "?",
              realtime_ ? "realtime" : "turbo");
    return true;
}

void Display::close() {
    if (texture_)  { SDL_DestroyTexture(texture_);   texture_  = nullptr; }
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_)   { SDL_DestroyWindow(window_);      window_   = nullptr; }
    if (SDL_WasInit(SDL_INIT_VIDEO)) SDL_Quit();
}

PumpResult Display::pump() {
    if (headless_) { return PumpResult::Continue; }
    SDL_Event ev;
    PumpResult r = PumpResult::Continue;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { return PumpResult::Quit; }   // window close → quit
        if (ev.type == SDL_MOUSEMOTION) {
            mouseX_ = ev.motion.x;
            mouseY_ = ev.motion.y;
        } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
            mouseX_ = ev.button.x;
            mouseY_ = ev.button.y;
            clickPending_ = true;
            clickButton_ = ev.button.button;
            r = PumpResult::Skip;
        } else if (ev.type == SDL_KEYDOWN) {
            r = PumpResult::Skip;                               // key → skip cutscene
        }
    }
    return r;
}

bool Display::takeClick(int& x, int& y, int& button) {
    if (!clickPending_) { return false; }
    x = mouseX_;
    y = mouseY_;
    button = clickButton_;
    clickPending_ = false;
    return true;
}

void Display::present(const Framebuffer& fb) {
    fb.toRGBA(rgba_.data());
    if (headless_ || !texture_) return;
    SDL_UpdateTexture(texture_, nullptr, rgba_.data(), Framebuffer::W * sizeof(uint32_t));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}
