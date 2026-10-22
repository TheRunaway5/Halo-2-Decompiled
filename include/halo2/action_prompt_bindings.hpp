#pragma once
#include "halo2/prompt_device.hpp"
#include "halo2/controller_settings.hpp"
#include "halo2/pc_settings.hpp"
union SDL_Event;
namespace halo2 {
// Only input belonging to this game window may select keyboard/mouse prompts.
bool native_action_prompt_keyboard_activity(const SDL_Event &,std::uint32_t window);
// Resolve the actual native controller adapter, rather than assuming a
// particular button layout when choosing Vista texture glyphs.
std::array<std::uint32_t,2> native_action_prompt_controller_buttons(const ControllerSettings &);
std::array<std::string,2> native_action_prompt_keyboard_buttons(const PcKeyboardSettings &);
std::array<std::uint32_t,2> native_boarding_prompt_controller_buttons(const ControllerSettings &);
std::array<std::string,2> native_boarding_prompt_keyboard_buttons(const PcKeyboardSettings &);
}
