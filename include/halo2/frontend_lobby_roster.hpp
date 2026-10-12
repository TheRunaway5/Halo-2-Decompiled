#pragma once
#include "halo2/frontend_appearance.hpp"
#include "halo2/source_object_colors.hpp"
#include <array>
#include <bit>
#include <optional>
#include <span>
#include <string>

namespace halo2 {
struct FrontendLobbyRankRange { int lower{}, upper{}; };
// Literal mainmenu matg+16C ->mulg.universal+10 team-color block (9 entries),
// and wigl+158 UI-rank block (50 entries). The fixture checks every word;
// these are frontend asset bindings, not an invented multiplayer palette.
inline constexpr std::array<Vec3, 9> frontend_lobby_team_colors{{
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f41c1c6, 0x3e78f8e8, 0x3e78f8e8}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3e58d8ed, 0x3e64e4c9, 0x3f49c9c5}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f49c9c5, 0x3f3ababf, 0x3e58d8ed}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3e008073, 0x3f0a8a83, 0x3e008073}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f35b5b7, 0x3e70f0e9, 0x3f0a8a83}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f5fdfe3, 0x3f17979a, 0x3e54d4cc}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3ee8e8ea, 0x3e909093, 0x3e048494}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f6bebf2, 0x3efcfd09, 0x3f45c5c5}),
  std::bit_cast<Vec3>(std::array<std::uint32_t, 3>{0x3f52d2cc, 0x3f52d2cc, 0x3f52d2cc}),
}};
inline constexpr auto frontend_lobby_rank_ranges = [] {
  std::array<FrontendLobbyRankRange, 50> ranges;
  for (int i = 0; i < 49; ++i) ranges[unsigned(i)] = {i, i};
  ranges[49] = {49, 127};
  return ranges;
}();
// A compact active player from25042D's supplied players[] order, resolved by
//19AAA5. Transport, active-player enumeration and team sorting are caller-owned.
struct FrontendLobbyRosterPlayer {
  int session_index{};
  std::string name;
  FrontendAppearance appearance;
  int team = -1, rank_value = -1, member_count = -1;
  bool voice_port_flag0_only{}, voice_query_53750{};
};
//24FD74 supplies19AA17's controller mapping. Only matching valid local team
// edits override a player's session team, as250503..25059C proves.
struct FrontendLobbyLocalTeam {
  int session_index = -1;
  bool valid{};
  int team = -1;
};
struct FrontendLobbyRosterInput {
  int pane{}, valid_session = -1;
  bool session_present{}, team_mode{}, matchmaking{}, session_enum_is_six{};
  std::span<const FrontendLobbyRosterPlayer> players;
  std::array<FrontendLobbyLocalTeam, 4> local_teams{};
  std::span<const Vec3> appearance_palette;
  std::span<const Vec3> team_palette = frontend_lobby_team_colors;
  std::span<const FrontendLobbyRankRange> rank_ranges = frontend_lobby_rank_ranges;
};
struct FrontendLobbyRosterSlot {
  bool visible{}, speaker_visible{};
  // Original direct type6 lookup counts title/subtitle before pane texts.
  unsigned text_a{}, text_b{}, bitmap_a{}, speaker_bitmap{}, group{};
  int session_index = -1;
  std::string name;
  std::optional<std::uint32_t> name_string;
  FrontendAppearance appearance;
  std::uint32_t item_flags{};
  int team = -1, rank_range = -1;
  bool no_team{};
  unsigned speaker_frame{}, background_frame{}, rank_frame{};
  bool background_visible{}, secondary_background_visible{}, rank_visible{}, emblem_visible{};
  Vec3 primary_color{1, 1, 1}, name_color{1, 1, 1};
};
using FrontendLobbyRoster = std::array<FrontendLobbyRosterSlot, 16>;

// Original25042D ->22F042 ->2AFEAE, restricted to the literal screen14
//player_skin_default. Row count comes from compact active session players;
//E425's retained party-count parser is a separate original producer.
inline FrontendLobbyRoster frontend_lobby_roster(const FrontendLobbyRosterInput &input) {
  if (input.pane < 0 || input.pane > 2 || input.players.size() > 16)
    throw FormatError("frontend lobby roster input escapes the authored16-slot panes");
  FrontendLobbyRoster output;
  for (unsigned i = 0; i < output.size(); ++i) {
    auto &slot = output[i];
    slot.text_a = (input.pane == 1 ? 24u : 29u) + i - 2;
    slot.text_b = (input.pane == 1 ? 8u : 13u) + i - 2;
    slot.bitmap_a = 5 + i;
    slot.speaker_bitmap = 21 + i;
    slot.group = i;
    if (input.valid_session == -1 || i >= input.players.size())
      continue;
    const auto &player = input.players[i];
    slot.visible = slot.speaker_visible = true;
    slot.session_index = player.session_index;
    slot.name = player.name;
    slot.appearance = player.appearance;
    slot.item_flags = 0x103; // Name+appearance+value64, original250484..2504BD.
    for (unsigned range = 0; range < input.rank_ranges.size(); ++range)
      if (player.rank_value >= input.rank_ranges[range].lower &&
          player.rank_value <= input.rank_ranges[range].upper) {
        slot.rank_range = int(range);
        slot.item_flags |= 0x40;
        break;
      }
    if (!input.session_enum_is_six && !input.matchmaking && input.session_present &&
        input.team_mode && player.team >= -1 && player.team < 8) {
      slot.team = player.team;
      slot.no_team = player.team == -1;
      slot.item_flags |= 0x84;
    }
    for (const auto &local : input.local_teams)
      if (local.session_index == player.session_index) {
        if (local.valid) {
          slot.team = local.team;
          slot.no_team = local.team == -1;
          slot.item_flags |= 0x8c;
          slot.name_string = local.team == -1 ? 0x090001c6u
              : local.team >= 0 && local.team < 8 ? 0x0500020eu + unsigned(local.team) : 0;
        }
        break;
      }
    const auto colors = frontend_appearance_colors(player.appearance, input.appearance_palette);
    slot.primary_color = colors[0];
    if (slot.no_team)
      slot.primary_color = {1, 1, 1}; // Literal g468710,7F790(use_default=true).
    else if (slot.team != -1)
      slot.primary_color = slot.team >= 0 && std::size_t(slot.team) < input.team_palette.size()
          ? input.team_palette[std::size_t(slot.team)]
          : Vec3{1, 0.41f, 0.7f}; // Literal g468734 fallback,7F720.
    slot.name_color = source_profile_name_color(slot.primary_color);
    // Literal default skin contains name0/B000698, primary bitmap0/1400069A,
    // conditional bitmap1/E00069B, rank bitmap2/900069C, emblem0.
    slot.background_visible = true;
    const int background = player.member_count > 1 ? player.member_count - 1 : 0;
    if (background < 7) // Actual item_background contains7 pages;2B0A14 clamps.
      slot.background_frame = unsigned(background);
    slot.secondary_background_visible = false; //25042D never sets item bit5.
    slot.rank_visible = slot.rank_range != -1;
    if (slot.rank_range >= 0 && slot.rank_range < 50)
      slot.rank_frame = unsigned(slot.rank_range); // Literal rank_icons_sm50pages.
    slot.emblem_visible = true; // Literal emblem flags1;2B12CA tests flags2.
    slot.speaker_frame = player.voice_port_flag0_only ? !player.voice_query_53750 : 2;
  }
  return output;
}
} // namespace halo2
