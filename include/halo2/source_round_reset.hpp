#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SourceRoundResetBindings {
  std::uint32_t game_globals_slot,players_slot,objects_slot;
  std::uint32_t tags_slot,mask_slot,engine_globals_slot,engine_table;
};
struct SourceRoundResetServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Must execute complete live deactivation/deletion, including player/view,
  // world/physics/model ownership and storage retirement. A handle clear or
  // empty callback cannot admit a later native round.
  std::function<void(std::uint32_t,bool)> deactivate_object,delete_object;
  std::function<void()> reset_decals,reset_contrails,reset_effects;
  std::function<void()> reset_registry,reset_lights,reset_sound,place_scenario;
  std::function<void(std::uint32_t)> reset_scenario,scenario_callback;
};
struct SourceRoundResetStore {
  std::uint32_t address,before,after;
  unsigned bytes;
  friend bool operator==(const SourceRoundResetStore &,const SourceRoundResetStore &)=default;
};
using SourceRoundResetObserver=std::function<void(const SourceRoundResetStore &)>;
// Complete15B460 control/data owner with original1A2C60/BBDA0 iteration.
// Uses original variant flags, object ancestry/type/category and actual raw
// pools. Reached world/scenario/platform services are required, with no
// successful defaults. This does not initialize their storage or enable modes.
void source_round_reset(const SourceRoundResetBindings &,
    const SourceRoundResetServices &,const SourceRoundResetObserver & = {});
} // namespace halo2
