#include "halo2/action_prompt_bindings.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
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
