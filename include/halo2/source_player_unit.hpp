#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class SourcePlayerUnitVersion { Retail, TitleUpdate };
struct SourcePlayerUnitBindings {
  std::uint32_t players_slot,player_globals_slot,objects_slot;
  std::uint32_t game_globals_slot,engine_globals_slot,engine_table;
  SourcePlayerUnitVersion version;
};
// Reached services are required. Their implementations own the original
// view/controller, activation, network-dirty and engine-parameter work; this owner
// does not replace those operations with successful returns.
struct SourcePlayerUnitServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  std::function<void(std::int16_t,std::uint32_t)> bind_view;
  std::function<void(std::int16_t,std::uint32_t,std::int32_t)> bind_control;
  std::function<void(std::int16_t)> reset_local_state,refresh_user;
  std::function<void(std::uint32_t,std::uint32_t)> mark_network_dirty;
  std::function<void(std::uint32_t,bool)> activate_unit;
  std::function<std::uint32_t(std::uint32_t,std::int32_t)> parameter_bits;
  std::function<void(std::uint32_t)> forget_view_object;
};
struct SourcePlayerUnitStore {
  std::uint32_t address,before,after;
  unsigned bytes;
  friend bool operator==(const SourcePlayerUnitStore &,const SourcePlayerUnitStore &)=default;
};
using SourcePlayerUnitObserver=std::function<void(const SourcePlayerUnitStore &)>;
// Complete original14E240 assignment and153A90 roster-status owner.
void bind_source_player_unit(const SourcePlayerUnitBindings &,std::uint32_t player,
    std::uint32_t unit,const SourcePlayerUnitServices &,const SourcePlayerUnitObserver & = {});
// Complete original154440 object callback. This releases player references
// during deactivation; it does not delete an object or retire its storage.
void release_source_object_player_references(const SourcePlayerUnitBindings &,
    std::uint32_t object,const SourcePlayerUnitServices &,const SourcePlayerUnitObserver & = {});
} // namespace halo2
