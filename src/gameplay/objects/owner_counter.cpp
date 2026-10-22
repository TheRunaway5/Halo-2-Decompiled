#include "halo2/owner_counter.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
std::optional<std::size_t> incoming_owner_counter(const OwnerCounterInput &input,
    std::span<const OwnerCounterPlayer> players, std::span<OwnerCounterObject> objects) {
  if (!input.lethal || input.incoming_designator == -1 ||
      !(input.victim_membership == -1 || input.victim_membership != input.incoming_designator))
    return {};
  auto indexed = [&](std::uint32_t datum) -> std::optional<std::size_t> {
    if (datum == 0xffffffffu) return {};
    for (std::size_t i=0;i<objects.size();++i)
      if ((objects[i].datum & 0xffffu) == (datum & 0xffffu)) return i;
    return {};
  };
  std::optional<std::size_t> selected;
  if (input.incoming_player != 0xffffffffu && input.incoming_player >> 16) {
    const auto player=std::find_if(players.begin(),players.end(),[&](const auto &p) {
      return p.datum==input.incoming_player;
    });
    if (player!=players.end()) {
      selected=indexed(player->current_unit);
      if(selected && !objects[*selected].body_present) selected.reset();
    }
  }
  if (!selected) {
    selected=indexed(input.incoming_object);
    if (selected) {
      const auto &object=objects[*selected];
      if(object.datum!=input.incoming_object || !(object.datum >> 16) ||
          !object.body_present || !(3u & (1u << (object.type & 31u)))) selected.reset();
    }
  }
  if (!selected) return {};
  const auto &primary=objects[*selected];
  const auto forwarded=input.damage_class==9 ? primary.forward_9 : primary.forward_other;
  if (forwarded!=0xffffffffu) selected=indexed(forwarded);
  if (!selected || !objects[*selected].body_present || objects[*selected].dead) return {};
  auto &object=objects[*selected];auto &state=object.counter;
  if (state.last_tick==0xffffffffu) state.count=0;
  else {
    const auto delta=std::bit_cast<std::int32_t>(input.tick-state.last_tick);
    const float converted=float(delta);
    const float elapsed=converted*input.seconds_per_tick;
    if(elapsed>4.f) state.count=0;
  }
  state.count=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(state.count)+1u));
  state.last_tick=input.tick;
  if(input.enabled_cap && state.count >= (object.player==0xffffffffu ? 3 : 5)) state.count=0;
  return selected;
}
}
