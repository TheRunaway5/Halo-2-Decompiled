#include "halo2/pc_settings.hpp"
#include <SDL.h>
namespace halo2 {
PcKeyboardSettings::PcKeyboardSettings()
    : bindings{SDL_SCANCODE_A,
               SDL_SCANCODE_D,
               SDL_SCANCODE_W,
               SDL_SCANCODE_S,
               SDL_SCANCODE_SPACE,
               SDL_SCANCODE_LSHIFT,
               pc_mouse_button(SDL_BUTTON_LEFT),
               SDL_SCANCODE_Q,
               SDL_SCANCODE_TAB,
               SDL_SCANCODE_F,
               SDL_SCANCODE_R,
               pc_mouse_button(SDL_BUTTON_RIGHT),
               SDL_SCANCODE_F3,
               SDL_SCANCODE_E,SDL_SCANCODE_X,SDL_SCANCODE_X,SDL_SCANCODE_G,
               pc_mouse_button(SDL_BUTTON_RIGHT),pc_mouse_button(SDL_BUTTON_LEFT)} {
  secondary[unsigned(PcAction::Crouch)] = SDL_SCANCODE_RSHIFT;
  secondary[unsigned(PcAction::SwitchWeapon)] = pc_wheel_up;
}
bool valid_pc_binding(int value) {
  return value == 0 ||
         (value > SDL_SCANCODE_UNKNOWN && value < SDL_NUM_SCANCODES &&
          value != SDL_SCANCODE_ESCAPE) ||
         (value >= pc_mouse_button(1) && value <= pc_mouse_button(5)) ||
         value == pc_wheel_up;
}
std::string pc_binding_name(int value) {
  if (!value)
    return "Unbound";
  if (value == pc_wheel_up)
    return "Mouse Wheel";
  if (value >= pc_mouse_button(1))
    return "Mouse " + std::to_string(value - 512);
  return SDL_GetScancodeName(SDL_Scancode(value));
}
} // namespace halo2
