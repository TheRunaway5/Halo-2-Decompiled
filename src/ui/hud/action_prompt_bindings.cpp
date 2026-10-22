#include "halo2/action_prompt_bindings.hpp"
#include "halo2/cache.hpp"
#include <SDL.h>
#include <cmath>
namespace halo2 {
PromptDevice controller_prompt_device(SDL_GameController *pad) {
  if(!pad || !SDL_GameControllerGetAttached(pad))return PromptDevice::KeyboardMouse;
#if SDL_VERSION_ATLEAST(2,0,12)
  const auto type=SDL_GameControllerGetType(pad);
  if(type==SDL_CONTROLLER_TYPE_PS3 || type==SDL_CONTROLLER_TYPE_PS4
#if SDL_VERSION_ATLEAST(2,0,14)
      || type==SDL_CONTROLLER_TYPE_PS5
#endif
  )return PromptDevice::PlayStation;
#endif
#if SDL_VERSION_ATLEAST(2,0,6)
  if(SDL_GameControllerGetVendor(pad)==0x054c)return PromptDevice::PlayStation;
#endif
  return PromptDevice::Xbox;
}
void PlayerPromptDevices::bind(unsigned view,SDL_GameController *pad) {
  const auto device=controller_prompt_device(pad);
  bind(view,device==PromptDevice::KeyboardMouse?-1:SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)),device);
}
void PlayerPromptDevices::bind(unsigned view,std::int32_t instance,PromptDevice device) {
  if(instance<0)device=PromptDevice::KeyboardMouse;
  if(instances_.at(view)==instance && controllers_.at(view)==device)return;
  instances_[view]=instance;controllers_[view]=device;devices_[view]=device;
}
void PlayerPromptDevices::event(const SDL_Event &e,std::uint32_t window,unsigned keyboard_view) {
  if(native_action_prompt_keyboard_activity(e,window) && keyboard_view<devices_.size())
    devices_[keyboard_view]=PromptDevice::KeyboardMouse;
  if(e.type==SDL_CONTROLLERDEVICEREMOVED) {
    for(unsigned i=0;i<instances_.size();++i)if(instances_[i]==e.cdevice.which)bind(i,-1,PromptDevice::KeyboardMouse);
  }
  // Match sample_controller's strict gameplay dead zone. Button releases,
  // trigger releases and noise must not claim a player's prompt selection.
  if(e.type!=SDL_CONTROLLERBUTTONDOWN &&
      !(e.type==SDL_CONTROLLERAXISMOTION && std::abs(float(e.caxis.value)/32767.f)>.15f))return;
  const auto instance=e.type==SDL_CONTROLLERBUTTONDOWN?e.cbutton.which:e.caxis.which;
  for(unsigned i=0;i<instances_.size();++i)if(instance>=0 && instances_[i]==instance)
    devices_[i]=controllers_[i];
}
bool native_action_prompt_keyboard_activity(const SDL_Event &event,std::uint32_t window) {
  switch(event.type) {
  case SDL_KEYDOWN:return event.key.windowID==window;
  case SDL_MOUSEBUTTONDOWN:return event.button.windowID==window;
  case SDL_MOUSEMOTION:return event.motion.windowID==window && (event.motion.xrel || event.motion.yrel);
  case SDL_MOUSEWHEEL:
    if(event.wheel.windowID!=window)return false;
    if(event.wheel.x || event.wheel.y)return true;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    return event.wheel.preciseX!=0 || event.wheel.preciseY!=0;
#else
    return false;
#endif
  default:return false;
  }
}
std::array<std::string,2> native_action_prompt_keyboard_buttons(const PcKeyboardSettings &settings) {
  std::array<std::string,2> result;
  constexpr std::array actions{PcAction::Pickup,PcAction::DualPickup};
  for(unsigned action=0;action<2;++action) {
    const auto i=unsigned(actions[action]);
    result[action]=pc_binding_name(settings.bindings[i]?settings.bindings[i]:settings.secondary[i]);
  }
  return result;
}
std::array<std::uint32_t,2> native_action_prompt_controller_buttons(const ControllerSettings &settings) {
  // Source hardware ordinals from ControllerSample, Vista glyph ordinals
  // from the supplied Conduit fonts. Zero entries have no action binding.
  constexpr std::array<std::uint32_t,16> glyphs{
    0xe106,0xe107,0,0,0,0,0,0,0,0,0xe104,0xe105,0xe100,0xe101,0xe102,0xe103};
  std::array<std::uint32_t,2> result{};
  for(unsigned hardware=0;hardware<glyphs.size();++hardware) {
    ControllerSample sample;sample.hardware[hardware]=1;
    const auto mapped=controller_input(sample,settings);
    for(unsigned action=0;action<2;++action)if(action?mapped.dual_wield:mapped.pickup) {
      if(!glyphs[hardware] || result[action])throw FormatError("Ambiguous or unsupported native action button binding");
      result[action]=glyphs[hardware];
    }
  }
  if(!result[0] || !result[1])throw FormatError("Native action has no controller button binding");
  return result;
}
std::array<std::string,2> native_boarding_prompt_keyboard_buttons(const PcKeyboardSettings &settings) {
  std::array<std::string,2> result;constexpr std::array actions{PcAction::Grenade,PcAction::Melee};
  for(unsigned a=0;a<2;++a){const auto i=unsigned(actions[a]);result[a]=pc_binding_name(settings.bindings[i]?settings.bindings[i]:settings.secondary[i]);}
  return result;
}
std::array<std::uint32_t,2> native_boarding_prompt_controller_buttons(const ControllerSettings &settings) {
  constexpr std::array<std::uint32_t,16> glyphs{0xe106,0xe107,0xe108,0xe109,0xe10a,0xe10b,0xe10c,0xe10d,0xe10e,0xe10f,0xe104,0xe105,0xe100,0xe101,0xe102,0xe103};
  std::array<std::uint32_t,2> result{};
  for(unsigned h=0;h<16;++h){ControllerSample sample;sample.hardware[h]=1;const auto mapped=controller_input(sample,settings);
    if(mapped.grenade)result[0]=glyphs[h];if(mapped.melee)result[1]=glyphs[h];}
  if(!result[0] || !result[1])throw FormatError("Boarding action has no controller binding");return result;
}

}
