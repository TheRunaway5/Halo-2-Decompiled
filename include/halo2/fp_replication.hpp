#pragma once
#include "halo2/fp_registration.hpp"

namespace halo2 {
// Native LAN presentation data, not an Xbox network or dedicated-observer
// structure. Node maps and full object identities remain asset/binding owned.
struct FirstPersonChannelState {
  std::uint32_t graph=0xffffffffu, animation_identifier=0xffffffffu;
  std::uint8_t delay{}, flags{};
  std::uint16_t options{}, events{}, future_events{};
  float rate=1, frame{};
  std::uint32_t activity=0xffffffffu;
  std::int16_t fire_ticks{};
};
struct FirstPersonReplicatedClock {
  bool present{};
  // Actual post-action input and completed output of one source-clock step.
  // A complete pair lets a later snapshot recover without replaying lost events.
  FirstPersonChannelState origin, current;
};
FirstPersonChannelState first_person_channel_state(const FirstPersonHandBinding &);
bool same_first_person_channel(const FirstPersonChannelState &,const FirstPersonChannelState &);
// Apply only after the original registration has resolved current full identities
// and authored node maps. Validates the complete clock transition on a copy;
// never dispatches a gameplay, sound or effect callback.
bool apply_replicated_first_person_clock(FirstPersonViewBinding &origin,
    FirstPersonViewBinding &current,const FirstPersonReplicatedClock &,
    const FirstPersonBindingDefinition &);
}
