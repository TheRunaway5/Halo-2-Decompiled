#pragma once
#include "halo2/source_king.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace halo2 {
using SourceKingColor = std::array<std::uint32_t,3>;
struct SourceKingVisualPlayer {
  std::uint32_t current_unit=0xffffffffu;
  std::int8_t membership{};
  std::uint8_t appearance_type{}; // Original player+88, independent of primary color.
};
struct SourceKingColorInput {
  bool engine_present=true,team_mode{};
  std::uint16_t occupied_players{},hill_occupants{};
  std::uint32_t query=0xffffffffu,local_player=0xffffffffu;
  std::int32_t render_view=-1;
  // Raw pool slots are deliberately distinct from the occupied bitmap. The
  // original query/local lookups use low16 without checking datum salt.
  std::array<SourceKingVisualPlayer,16> players;
};
// Whole retail2BE3B0 / TU1.1 2C46F0 / TU1.5 2C8170 and actual7F720 palette
// lookup. Active players skip current-unit NONE; flags bit1 is not a gate.
SourceKingColor source_king_hill_color(const SourceKingColorInput&,
    std::span<const SourceKingColor> authored_team_colors);
// Original2BE6D0 marker producer, ending at its actual24E59F handoff. The
// marker view may differ from the current render view used by color fallback.
// Invalid hill counts leave all caller bytes untouched. Valid records define
// bytes[0,2) and[4,6C); projection/arrow/icon services remain external.
bool source_king_hill_marker(const SourceKingState&,std::int32_t marker_view,
    std::uint32_t marker_player,const SourceKingColorInput&,
    std::span<const SourceKingColor> authored_team_colors,const SourceKingPosition&,
    std::span<std::byte,0x90> caller_storage);
} // namespace halo2
