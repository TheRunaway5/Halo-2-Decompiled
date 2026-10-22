#pragma once
#include "halo2/source_hud_score.hpp"
#include <array>
#include <cstdint>
namespace halo2 {
struct SourceHudTerritory {
  std::uint32_t color{};
  float fraction{};
  auto operator<=>(const SourceHudTerritory &) const = default;
};
// Defined objective portion of the original per-view HUD status. The same
// charge meter is used for an Assault bomb and a local Territories capture.
struct SourceHudObjectives {
  bool charge_visible{};
  float charge_fraction{};
  std::uint32_t territory_count{};
  std::array<SourceHudTerritory,8> territories{};
  auto operator<=>(const SourceHudObjectives &) const = default;
};
struct SourceHudObjectivePlayer {
  std::uint32_t datum=0xffffffffu,unit=0xffffffffu;
  std::int8_t membership=-1,capturing=-1;
  std::uint8_t capture{};
};
struct SourceHudObjectiveInput {
  bool engine_present=true;
  std::int32_t engine=2;
  std::uint32_t player=0xffffffffu;
  std::uint16_t active_players{};
  std::array<SourceHudObjectivePlayer,16> players;
  std::int16_t territory_count{};
  std::array<std::int16_t,8> territory_indices{-1,-1,-1,-1,-1,-1,-1,-1};
  std::array<std::uint32_t,8> territory_owners{0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,
      0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
  bool assault_bomb{},bomb_armed{};
  std::int16_t bomb_index=-1,bomb_ticks{},bomb_seconds{};
  float tick_seconds=1.f/30.f;
};
// Disc15F3A0/TU1.5 161E20 objective query and24B430 bomb fraction.
SourceHudObjectives source_hud_objectives(const SourceHudObjectiveInput &,
    std::span<const SourceHudScoreColor> team_colors);
class SourceRuntimeMemory;
// Read the actual retained TU1.5 options/engine/player/object/tag owners.
// Ordinary Slayer/King frames have no objective meter payload.
SourceHudObjectives source_hud_objectives_tu15(const SourceRuntimeMemory &,std::uint32_t player);
} // namespace halo2
