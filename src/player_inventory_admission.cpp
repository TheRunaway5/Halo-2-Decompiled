#include "halo2/player_inventory_admission.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
void source_restrict_player_inventory(std::array<std::uint8_t,4> &out,
                                     const SourcePlayerCarryAdmission &in) {
  constexpr auto none=0xffffffffu;
  const auto flags=[&](std::uint32_t datum) {
    const auto item=std::find_if(in.slots.begin(),in.slots.end(),
        [&](const auto &s){return s.datum==datum;});
    if(item==in.slots.end())throw std::invalid_argument("Current source hand is absent from its inventory");
    return item->definition_flags;
  };
  for(unsigned hand=0;hand<2;++hand)
    if(in.current[hand]!=none && (flags(in.current[hand])&8u))
      out[hand]=out[hand+2]=0;
  if(in.parent!=none)out.fill(0);
  unsigned count=0;
  for(const auto &item:in.slots) {
    if(item.datum==none || (item.definition_flags&16u))continue;
    if((in.target_definition_flags&0xc00000u) &&
        item.datum==in.current[1] && item.datum==in.requested[1])continue;
    ++count;
  }
  if(count<2)return;
  if(!(in.target_definition_flags&16u)) {
    out[0]=0;
    if(in.current[0]!=none && (flags(in.current[0])&16u))out[2]=0;
  }
  if(in.current[1]!=none && !(flags(in.current[1])&16u))out[1]=0;
}
} // namespace halo2
