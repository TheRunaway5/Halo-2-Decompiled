#include "halo2/unit_zoom.hpp"
#include <bit>

namespace halo2 {
namespace {
bool magazine_active(const ZoomWeaponView &weapon) {
  for(const auto mode:weapon.magazine_modes)if(mode>=1 && mode<=6)return true;
  return false;
}
std::int16_t word(std::uint16_t bits) {return std::bit_cast<std::int16_t>(bits);}
// Separate source SSE operations must retain their individual float rounding.
float rounded(float value) {volatile float result=value;return result;}
}
bool source_zoom_admitted(const UnitZoomContext &context) {
  if(context.parent!=0xffffffffu || (context.action_word&0x0c440d03u))return false;
  for(const auto *weapon:context.equipped)if(weapon && magazine_active(*weapon))return false;
  return true;
}
std::int16_t source_zoom_next(std::int16_t current,const ZoomWeaponView *primary,
                             const BinocularZoomDefinition &binocular) {
  if(primary) {
    const auto count=primary->definition.levels;
    if(count>0) {
      if(magazine_active(*primary))return current;
      if(current>=0 && int(current)<int(count)-1)return word(std::uint16_t(current)+1);
      return int(current)==int(count)-1 ? -1 : 0;
    }
    if(primary->definition.flags&0x00010000u)return -1;
  }
  if(binocular.levels>0 && int(current)+1<binocular.levels)
    return word(std::uint16_t(current)+1);
  return -1;
}
void source_zoom_request(UnitZoomState &state,std::int16_t incoming) {
  state.requested=std::bit_cast<std::int8_t>(std::uint8_t(std::uint16_t(incoming)));
}
bool source_zoom_update(UnitZoomState &state,const UnitZoomContext &context,bool enabled) {
  if(!enabled)return false;
  const auto selected=source_zoom_admitted(context) ? state.requested : std::int8_t(-1);
  if(selected==state.current)return false;
  state.current=selected;
  if(selected==-1)state.motion=0;
  return true;
}
void source_zoom_reset(UnitZoomState &state) {state.current=-1;state.requested=-1;state.motion=0;}
std::uint16_t source_zoom_weapon_input(std::uint16_t flags,const UnitZoomState &state) {
  return state.current!=-1 ? std::uint16_t(flags|0x40u) : flags;
}
std::optional<float> source_zoom_magnification(std::int16_t current,
    const ZoomWeaponView *primary,const BinocularZoomDefinition &binocular) {
  if(primary) {
    if(!magazine_active(*primary) && primary->definition.levels!=0) {
      const auto count=primary->definition.levels;
      if(current<0 || current>=count)return 1.f;
      if(count!=1)return std::nullopt;
      // Original positive-bound fallback and power(...,0)*lower endpoint.
      return primary->definition.minimum>0.f ? primary->definition.minimum : 1.f;
    }
    if(primary->definition.flags&0x00010000u)return 1.f;
  }
  if(current==-1)return 1.f;
  float fraction=1.f;
  if(binocular.levels!=1) {
    const auto denominator=std::bit_cast<std::int32_t>(std::uint32_t(binocular.levels)-1u);
    fraction=rounded(float(current)/float(denominator));
  }
  const float difference=rounded(binocular.maximum-binocular.minimum);
  const float product=rounded(difference*fraction);
  return rounded(product+binocular.minimum);
}
ZoomHudStatus source_zoom_hud_status(std::int8_t current,const ZoomWeaponView *primary) {
  const bool weapon_zoom=primary && primary->definition.levels!=0 && !magazine_active(*primary);
  return {current,std::uint8_t(current!=-1 && primary && !weapon_zoom)};
}
std::uint16_t source_zoom_hud_mask(const ZoomHudStatus &status) {
  std::uint16_t bits=0;
  if(status.current==-1)bits|=0x40;
  if(status.current==0)bits|=0x80;
  if(status.current==1)bits|=0x100;
  if(status.binocular)bits|=0x400;
  return bits;
}
} // namespace halo2
