#include "halo2/desktop_session.hpp"
#include "halo2/menu_music.hpp"
#include <ctime>
#include <stdexcept>

namespace halo2 {
std::uint32_t DesktopSession::fresh_secondary_random() {
  const auto seconds=std::uint32_t(std::time(nullptr));
  const auto draw=source_crt_random(crt_random_state_);
  return source_secondary_random_seed(seconds,SDL_GetTicks(),draw);
}
std::uint32_t &DesktopSession::auxiliary_random() {
  if(!auxiliary_random_)auxiliary_random_=fresh_secondary_random();
  return *auxiliary_random_;
}
std::string DesktopSession::apply_video(unsigned width, unsigned height,
                                        bool fullscreen) {
  if (!window_ || width < 320 || height < 240 || width > 16384 ||
      height > 16384)
    return "Invalid native display mode.";
  int old_width{}, old_height{};
  SDL_GetWindowSize(window_, &old_width, &old_height);
  const auto old_flags =
      SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP;
  SDL_DisplayMode old_mode{};
  SDL_GetWindowDisplayMode(window_, &old_mode);
  auto rollback = [&] {
    SDL_SetWindowFullscreen(window_, 0);
    SDL_SetWindowDisplayMode(window_, &old_mode);
    SDL_SetWindowSize(window_, old_width, old_height);
    SDL_SetWindowFullscreen(window_, old_flags);
  };
  SDL_DisplayMode mode{};
  bool desktop_fullscreen=false;
  if (fullscreen) {
    SDL_DisplayMode wanted{};
    wanted.w = int(width);
    wanted.h = int(height);
    if (!SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(window_), &wanted,
                                   &mode) ||
        mode.w != int(width) || mode.h != int(height))
      desktop_fullscreen=true;
  }
  if (SDL_SetWindowFullscreen(window_, 0) != 0)
    return SDL_GetError();
  if (fullscreen && !desktop_fullscreen && SDL_SetWindowDisplayMode(window_, &mode) != 0) {
    const std::string error = SDL_GetError();
    rollback();
    return error;
  }
  SDL_SetWindowSize(window_, int(width), int(height));
  if (fullscreen &&
      SDL_SetWindowFullscreen(window_, desktop_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) != 0) {
    const std::string error = SDL_GetError();
    rollback();
    return error;
  }
  return {};
}
SDL_Window *DesktopSession::window(unsigned width, unsigned height,
                                   bool hidden) {
  // Preserve the user's size, position and fullscreen state on a handoff.
  if (window_)
    return window_;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS |
               SDL_INIT_TIMER) != 0)
    throw std::runtime_error(SDL_GetError());
  initialized_ = true;
  window_ = SDL_CreateWindow(
      "Halo 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, int(width),
      int(height),
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
          (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
  if (!window_)
    throw std::runtime_error(SDL_GetError());
  return window_;
}
DesktopSession::~DesktopSession() {
  // The bridge cancels/joins its graphics thread and releases retained Vulkan
  // devices before the platform window disappears.
  loading_.reset();
  if (window_)
    SDL_DestroyWindow(window_);
  if (initialized_)
    SDL_Quit();
}
} // namespace halo2
