#include "halo2/source_player_damage.hpp"
#include "halo2/damage_amount.hpp"
#include <stdexcept>

namespace halo2 {
float source_player_damage_scale(bool engine,std::uint32_t flags,
    std::uint32_t attacker,std::uint32_t victim,const SourcePlayerDamageServices& services) {
  constexpr std::uint32_t none=0xffffffffu;
  if(!engine || attacker==victim || attacker==none)return 1.f;
  if(!services.player)throw std::runtime_error("Player damage requires its actual player fields");
  const auto from=services.player(attacker);
  float scale=damage_strength_scale(from.strength);
  if(victim==none)return scale;
  if(!services.trait)throw std::runtime_error("Player damage requires its selected-engine trait query");
  if(services.trait(attacker,2))scale=scale*1.5f;
  if(services.trait(victim,3))scale=scale*.5f;
  const auto to=services.player(victim);
  if(to.field_194>0)scale=scale*.1f;
  if(!(flags&0x80u)) {
    if(!services.enemies)throw std::runtime_error("Player damage requires its selected-engine membership query");
    if(!services.enemies(from.membership,to.membership))return 0.f;
  }
  return scale;
}
}
