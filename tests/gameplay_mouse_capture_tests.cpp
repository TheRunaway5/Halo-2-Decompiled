#define SDL_MAIN_HANDLED
#include "halo2/gameplay_mouse_capture.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
void captured(SDL_Window *window, halo2::GameplayMouseCapture &owner, bool relative = true) {
  check(relative ? owner.capture() : SDL_SetRelativeMouseMode(SDL_FALSE) == 0,
        SDL_GetError());
  SDL_SetWindowMouseGrab(window, SDL_TRUE);
  check(SDL_CaptureMouse(SDL_TRUE) == 0, SDL_GetError());
  SDL_ShowCursor(SDL_DISABLE);
  check(bool(SDL_GetRelativeMouseMode()) == relative,
        "Gameplay did not acquire the requested relative mouse mode");
  check(SDL_GetWindowMouseGrab(window), "Gameplay did not acquire the mouse grab");
  // SDL intentionally ignores explicit capture while relative mode owns the
  // pointer. Exercise explicit capture independently in the other mode.
  if (!relative)
    check(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_CAPTURE,
          "Gameplay did not acquire mouse capture");
  check(SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE, "Gameplay cursor was not hidden");
}
void released(SDL_Window *window) {
  check(!SDL_GetRelativeMouseMode(), "Gameplay exit left relative mouse mode enabled");
  check(!SDL_GetWindowMouseGrab(window), "Gameplay exit left the window mouse grabbed");
  check(!(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_CAPTURE),
        "Gameplay exit left mouse capture enabled");
  check(SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE,
        "Gameplay exit left the menu cursor hidden");
}
void finish_game(SDL_Window *window, bool disconnected, bool failure, bool relative) {
  halo2::GameplayMouseCapture owner(window);
  captured(window, owner, relative);
  if (disconnected) return;
  if (failure) throw std::runtime_error("Owned gameplay failure");
}
}
int main() try {
  SDL_SetMainReady();
  check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0, SDL_GetError());
  auto *window = SDL_CreateWindow("Gameplay cursor lifetime regression", 0, 0,
                                 320, 240, SDL_WINDOW_SHOWN);
  check(window, SDL_GetError());
  SDL_RaiseWindow(window);
  check(SDL_SetWindowInputFocus(window) == 0, SDL_GetError());
  SDL_PumpEvents();
  // All cases reuse the same SDL window, just as DesktopSession does when a
  // completed match, ONLINE failure or an exception returns to the menu.
  for (bool relative : {true, false}) for (unsigned repeat = 0; repeat < 3; ++repeat) {
    finish_game(window, false, false, relative);
    released(window);
    finish_game(window, true, false, relative);
    released(window);
    bool failed{};
    try { finish_game(window, false, true, relative); }
    catch (const std::runtime_error &error) {
      check(std::string(error.what()) == "Owned gameplay failure", error.what());
      failed = true;
    }
    check(failed, "Gameplay failure did not unwind the capture owner");
    released(window);
    {
      halo2::GameplayMouseCapture owner(window);
      captured(window, owner, relative);
      owner.release(); // Release before handing the retained window to menus.
      released(window);
      owner.release(); // Explicit release and scope teardown are idempotent.
      released(window);
      captured(window, owner); // A subsequent game can capture the same window.
    }
    released(window);
  }
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::cout << "PASS actual SDL gameplay capture cleanup: normal return, ONLINE "
               "failure, exception, explicit menu handoff and window reuse\n";
} catch (const std::exception &error) {
  SDL_Quit();
  std::cerr << error.what() << '\n';
  return 1;
}
