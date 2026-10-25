#pragma once
#include <SDL.h>

namespace halo2 {
// The DesktopSession retains its window when gameplay returns to the menu.
// Keep this scope inside that window's lifetime, including exceptional exits.
class GameplayMouseCapture {
public:
  explicit GameplayMouseCapture(SDL_Window *window) noexcept : window_(window) {}
  ~GameplayMouseCapture() noexcept { release(); }
  GameplayMouseCapture(const GameplayMouseCapture &) = delete;
  GameplayMouseCapture &operator=(const GameplayMouseCapture &) = delete;

  bool capture() noexcept { return SDL_SetRelativeMouseMode(SDL_TRUE) == 0; }
  void release() noexcept {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_SetWindowMouseGrab(window_, SDL_FALSE);
    SDL_CaptureMouse(SDL_FALSE);
    // Xwayland locks the host pointer while an X11 client hides its cursor
    // under a grab, and drops that lock only on a hidden->visible cursor
    // transition. Leaving relative mode can warp while still hidden, so issue
    // an explicit transition after the grab is gone; no confinement remains
    // for the brief hide to re-lock against.
    SDL_ShowCursor(SDL_DISABLE);
    SDL_ShowCursor(SDL_ENABLE);
  }

private:
  SDL_Window *window_{};
};
} // namespace halo2
