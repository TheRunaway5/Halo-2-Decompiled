#pragma once
#include "halo2/full_scoreboard_providers.hpp"
#include <array>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace halo2 {
// Data delivered from the owned map's actual matg/mulg/unic records. This
// adapter imports locale bytes directly; it does not assign Xbox ready flags.
struct FullScoreboardAssets {
  bool scenario_present{};
  std::vector<FullScoreboardColorBits> palette;
  std::vector<FullScoreboardColorBits> team_palette;
  std::array<std::string,9> team_names;
  std::string name_label,wins_label,score_label;
  struct PlayerStatus {
    std::int16_t status{};
    std::uint32_t string_id{},team_string_id{};
    std::string text,team_text;
  };
  std::vector<PlayerStatus> player_status;
  std::string game_over_label,round_over_label,waiting_for_next_round_label;
  std::uint32_t emblem_shader=0xffffffffu, emblem_bitmap=0xffffffffu;
  std::uint32_t dead_shader=0xffffffffu, dead_bitmap=0xffffffffu;
};
// Setup-derived Vista palette and paint words; positions and roster stay with
// full_scoreboard_layout. A selected Vista pack must contain every image bank.
std::array<std::uint32_t,5> load_full_scoreboard_vista_style(
    FullScoreboardAssets &, const std::filesystem::path &);
}
