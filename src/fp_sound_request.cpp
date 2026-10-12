#include "halo2/fp_sound_request.hpp"
#include <algorithm>
#include <bit>
#include <unordered_set>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
template<class F> const F &required(const F &function,const char *message) {
  if(!function)throw FormatError(message);
  return function;
}
template<std::size_t N> void store(std::array<std::byte,N> &data,
                                  std::size_t offset,std::uint32_t value,unsigned size) {
  for(unsigned i=0;i<size;++i)data[offset+i]=std::byte(value>>(i*8));
}
bool related(std::uint32_t current,std::uint32_t target,
             const FirstPersonSoundServices &services) {
  std::uint32_t last=none;
  std::unordered_set<std::uint32_t> visited;
  while(current!=none && last!=target) {
    if(!visited.insert(current).second)
      throw FormatError("Cyclic original sound object parent chain");
    last=current;
    // BBEA0 reads this parent even when current already equals target.
    current=required(services.object_parent,"Original sound object parent owner unavailable")(current);
  }
  return target!=none && last==target;
}
}
bool source_sound_uses_weapon_callbacks(std::uint32_t sound,std::uint32_t owner,
                                        const FirstPersonSoundServices &services) {
  if(sound==none)return false;
  const auto definition=required(services.sound_definition,
      "Original sound definition provider unavailable")(sound);
  if(!(std::to_integer<unsigned>(definition[0])&0x40) || owner==none)return false;
  const auto type=required(services.object_header_type,
      "Original sound object header provider unavailable")(owner);
  return ((1u<<(type&31))&4)!=0;
}
std::uint8_t source_sound_audibility(std::uint32_t owner,std::uint8_t requested,
    const FirstPersonSoundServices &services,std::uint32_t *resolved_view) {
  if(resolved_view)*resolved_view=none;
  if(requested!=1)return requested;
  for(unsigned view=0;view<4;++view) {
    const auto player=required(services.local_player,
        "Original local-player sound provider unavailable")(view);
    if(player==none)continue;
    const auto mode=required(services.camera_mode,
        "Original sound camera mode provider unavailable")(view);
    if(!((1u<<(std::uint32_t(mode)&31))&3))continue;
    const auto unit=required(services.player_current_unit,
        "Original sound player/current-unit provider unavailable")(player);
    if(unit==none)continue;
    if(related(owner,unit,services) || related(unit,owner,services)) {
      if(resolved_view)*resolved_view=view;
      return 0;
    }
  }
  return requested;
}
void source_first_person_sound_marker(std::uint32_t sound,std::uint32_t owner,
    const FirstPersonSoundConstants &constants,std::array<std::byte,40> &marker,
    const FirstPersonSoundServices &services) {
  std::copy(constants.marker_position.begin(),constants.marker_position.end(),marker.begin()+4);
  std::copy(constants.marker_forward.begin(),constants.marker_forward.end(),marker.begin()+0x10);
  marker[0]=std::byte{}; // The actual FP caller supplies marker index NONE.
  store(marker,0x24,none,4);
  if(owner!=none && source_sound_uses_weapon_callbacks(sound,owner,services)) {
    // 18ACD0 repeats the original header classification before body reads.
    const auto type=required(services.object_header_type,
        "Original sound marker object header provider unavailable")(owner);
    if(((1u<<(type&31))&4)!=0) {
      const auto association=required(services.weapon_association,
          "Original sound weapon association provider unavailable")(owner);
      if((association.flags_12c&1) && association.object_154!=none)
        store(marker,0x24,association.object_154,4);
    }
  }
}
bool source_first_person_sound_event(FirstPersonSoundState &state,
    bool storage,unsigned view,const FirstPersonHandBinding &hand,
    const FirstPersonAnimationEvent &event,std::uint16_t current_bsp,
    const FirstPersonSoundConstants &constants,
    FirstPersonSoundBacking &backing,const FirstPersonSoundServices &services) {
  if(!storage || event.type!=1 || event.tag==none)return false;
  if(view>=4)throw FormatError("Unsupported original first-person sound view");
  const auto mode=required(services.camera_mode,
      "Original sound event camera provider unavailable")(view);
  if(mode!=0 && (event.reference_flags&8))return false;
  auto &marker=backing.marker;
  marker[1]=std::byte{}; // 18AA90 writes before invoking 18ACD0.
  source_first_person_sound_marker(event.tag,hand.weapon,constants,marker,services);
  const auto callbacks=source_sound_uses_weapon_callbacks(event.tag,hand.weapon,services)?
      constants.weapon_callbacks:constants.ordinary_callbacks;
  const auto audible=source_sound_audibility(hand.weapon,1,services);
  auto &location=backing.location;
  store(location,0,0,2);
  location[2]=std::byte{};
  location[3]=std::byte((unsigned(audible)<<4)|(audible&15));
  store(location,4,0x3f800000u,4); // Original callback FLD1/FSTP.
  store(location,8,0,4);
  store(location,0x28,none,4);
  store(location,0x2c,0xffff,2);
  store(location,0x2e,current_bsp,2); // [46FB64], the owning active BSP word.
  auto identifier=none;
  if(required(services.owner_request,"Original sound world/marker owner unavailable")(
      hand.weapon,event.tag,callbacks,marker,location)) {
    const auto definition=required(services.sound_definition,
        "Original sound play definition provider unavailable")(event.tag);
    const auto class_index=std::bit_cast<std::int8_t>(definition[2]);
    const auto sound_class=required(services.sound_class,
        "Original sound class provider unavailable")(class_index);
    SourceSoundPlayRequest request{event.tag,callbacks?0x25u:5u,
        (std::to_integer<unsigned>(location[3])&15)?3u:1u,
        hand.weapon,callbacks,none,location,marker,40};
    if(hand.weapon!=none)request.flags|=8;
    if((std::to_integer<unsigned>(sound_class[10])&0x80) &&
       !(std::to_integer<unsigned>(definition[1])&1))
      request.location[1]|=std::byte{8};
    identifier=required(services.submit,"Original sound admission/scheduler provider unavailable")(request);
    if(identifier!=none) {
      // The original rereads the class after submit returns.
      const auto after=services.sound_class(class_index);
      if((std::to_integer<unsigned>(after[8])&6)==6)
        required(services.successful_attention,
            "Original successful sound attention/time owner unavailable")(request,definition);
    }
  }
  state.identifier=identifier;
  state.activity=std::uint16_t(hand.activity);
  return true;
}
} // namespace halo2
