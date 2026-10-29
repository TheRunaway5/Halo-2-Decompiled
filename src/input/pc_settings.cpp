#include "halo2/pc_settings.hpp"
#include <SDL.h>
namespace halo2 {
PcKeyboardSettings::PcKeyboardSettings()
    // MCC Data/settings/default.xml; see the authenticated reference in
    // tests/fixtures/mcc_default_controls.json. Halo 2 has two grenade types,
    // so MCC's Previous Grenade3 shares the native switch action with2.
    : bindings{SDL_SCANCODE_A,
               SDL_SCANCODE_D,
               SDL_SCANCODE_W,
               SDL_SCANCODE_S,
               SDL_SCANCODE_SPACE,
               SDL_SCANCODE_LCTRL,
               pc_mouse_button(SDL_BUTTON_LEFT),
               SDL_SCANCODE_F,
               SDL_SCANCODE_1,
               SDL_SCANCODE_Q,
               SDL_SCANCODE_R,
               pc_mouse_button(SDL_BUTTON_RIGHT),
               SDL_SCANCODE_TAB,
               SDL_SCANCODE_E,SDL_SCANCODE_C,SDL_SCANCODE_4,SDL_SCANCODE_2,
               pc_mouse_button(SDL_BUTTON_RIGHT),pc_mouse_button(SDL_BUTTON_LEFT)} {
  secondary[unsigned(PcAction::SwitchGrenade)]=SDL_SCANCODE_3;
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
