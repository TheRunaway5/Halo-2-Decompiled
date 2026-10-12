#include <SDL.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <array>
#include <vulkan/vulkan.h>

// Observe the actual application window lifetime. The injected events are the
// test user's title -> saved-profile sign-in -> Split Screen -> Start Game
// key presses. A saved active profile is not itself a signed-in local player.
namespace {
unsigned windows{}, injected{};
std::array<SDL_JoystickID,2> gamepads{-1,-1};
void record(const char *kind, SDL_Window *window) {
  const char *path = std::getenv("HALO2_WINDOW_PROBE_LOG");
  if (!path) return;
  if (auto *file = std::fopen(path, "a")) {
    int width{}, height{};
    SDL_GetWindowSize(window, &width, &height);
    std::fprintf(file, "%s %u %d %d\n", kind, SDL_GetWindowID(window), width, height);
    std::fclose(file);
  }
}
}
extern "C" SDL_Window *SDL_CreateWindow(const char *title, int x, int y,
                                         int w, int h, Uint32 flags) {
  const auto original = reinterpret_cast<decltype(&SDL_CreateWindow)>(
      dlsym(RTLD_NEXT, "SDL_CreateWindow"));
  auto *window = original(title, x, y, w, h, flags);
  if (window) {
    ++windows;
    record("create", window);
    if(windows==1 && std::getenv("HALO2_WINDOW_PROBE_TWO_PLAYERS")) {
      SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
      for(auto& id:gamepads) {
        const auto index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        if(index<0 || !SDL_IsGameController(index))std::abort();
        id=SDL_JoystickGetDeviceInstanceID(index);
      }
    }
    // An explicit test resize before the menu accepts its input. A handoff
    // must preserve this geometry rather than reset it to game defaults.
    if (windows == 1 && std::getenv("HALO2_WINDOW_PROBE_RESIZE"))
      SDL_SetWindowSize(window, 1100, 650);
  }
  return window;
}
extern "C" void SDL_DestroyWindow(SDL_Window *window) {
  record("destroy", window);
  const auto original = reinterpret_cast<decltype(&SDL_DestroyWindow)>(
      dlsym(RTLD_NEXT, "SDL_DestroyWindow"));
  original(window);
}
extern "C" int SDL_PollEvent(SDL_Event *event) {
  const auto original = reinterpret_cast<decltype(&SDL_PollEvent)>(
      dlsym(RTLD_NEXT, "SDL_PollEvent"));
  if(windows==1 && event && std::getenv("HALO2_WINDOW_PROBE_TWO_PLAYERS")) {
    struct Press {unsigned pad;SDL_GameControllerButton button;};
    constexpr Press presses[]{{0,SDL_CONTROLLER_BUTTON_A},{0,SDL_CONTROLLER_BUTTON_A},
      {0,SDL_CONTROLLER_BUTTON_DPAD_DOWN},{0,SDL_CONTROLLER_BUTTON_DPAD_DOWN},{0,SDL_CONTROLLER_BUTTON_A},
      {1,SDL_CONTROLLER_BUTTON_START},{1,SDL_CONTROLLER_BUTTON_DPAD_DOWN},{1,SDL_CONTROLLER_BUTTON_A},
      {0,SDL_CONTROLLER_BUTTON_A},{0,SDL_CONTROLLER_BUTTON_A}};
    if(injected<std::size(presses)) {
      const auto press=presses[injected++];*event={};event->type=SDL_CONTROLLERBUTTONDOWN;
      event->cbutton.which=gamepads[press.pad];event->cbutton.button=press.button;event->cbutton.state=SDL_PRESSED;
      return 1;
    }
    return original(event);
  }
  constexpr SDL_Keycode keys[]{SDLK_RETURN, SDLK_RETURN,
                               SDLK_DOWN, SDLK_DOWN, SDLK_RETURN,
                               SDLK_RETURN, SDLK_RETURN};
  if (windows == 1 && event && injected < std::size(keys)) {
    *event = {};
    event->type = SDL_KEYDOWN;
    event->key.keysym.sym = keys[injected++];
    return 1;
  }
  return original(event);
}

// Optional platform fault injection: a format enumeration can legitimately
// change between count and fill during a renderer/loading handoff.
extern "C" VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice device, VkSurfaceKHR surface, std::uint32_t *count,
    VkSurfaceFormatKHR *formats) {
  const auto original = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
      dlsym(RTLD_NEXT, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
  const auto result = original(device, surface, count, formats);
  static bool injected_incomplete{};
  if (formats && result == VK_SUCCESS && !injected_incomplete &&
      std::getenv("HALO2_WINDOW_PROBE_FORMAT_RETRY")) {
    injected_incomplete = true;
    if (const auto *path = std::getenv("HALO2_WINDOW_PROBE_LOG"))
      if (auto *file = std::fopen(path, "a")) {
        std::fputs("incomplete_formats\n", file);
        std::fclose(file);
      }
    return VK_INCOMPLETE;
  }
  return result;
}
