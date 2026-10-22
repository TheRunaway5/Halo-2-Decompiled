#include "halo2/native_action_prompt_hud.hpp"
namespace halo2 {
namespace {
HudContent messages(const Cache &cache,std::uint32_t unit) {
  auto content=cache.load_hud(unit,null_datum);
  content.widgets.clear();
  return content;
}
}
NativeActionPromptHud::NativeActionPromptHud(const Cache &cache,std::uint32_t unit,
    const std::filesystem::path &fonts,Mesh &mesh,const std::filesystem::path &assets)
    :hud_(messages(cache,unit),fonts,mesh,1,{},std::nullopt,assets) {}
NativeActionPromptFrame NativeActionPromptHud::frame(NativeActionPrompt prompt,
    const PcKeyboardSettings &keyboard,const ControllerSettings &controller,bool gamepad,
    unsigned width,unsigned height) {
  return frame(prompt,keyboard,controller,gamepad?PromptDevice::Xbox:PromptDevice::KeyboardMouse,width,height);
}
NativeActionPromptFrame NativeActionPromptHud::frame(NativeActionPrompt prompt,
    const PcKeyboardSettings &keyboard,const ControllerSettings &controller,PromptDevice device,
    unsigned width,unsigned height) {
  MultiplayerState state;state.players[0].connected=true;state.players[0].action_prompt=prompt;
  hud_.prompt_device(0,device);
  hud_.prompt_boarding_buttons(0,keyboard,controller);
  hud_.prompt_keyboard_buttons(0,native_action_prompt_keyboard_buttons(keyboard));
  hud_.prompt_gamepad_buttons(0,native_action_prompt_controller_buttons(controller));
  return {hud_.action_text(state,0,0),hud_.draw(state,0,0,1,width,height,0,0)};
}
}
