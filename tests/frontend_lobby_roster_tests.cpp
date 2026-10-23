#include "halo2/frontend_lobby_roster.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
std::uint32_t hex_word(const std::string &word) {
  return std::uint32_t(std::stoul(word, nullptr, 16));
}
Vec3 read_color(std::istringstream &row) {
  std::array<std::uint32_t, 3> words;
  for (auto &word : words) { std::string text; row >> text; word = hex_word(text); }
  return std::bit_cast<Vec3>(words);
}
bool equal_color(Vec3 a, Vec3 b) {
  return std::bit_cast<std::array<std::uint32_t, 3>>(a) ==
         std::bit_cast<std::array<std::uint32_t, 3>>(b);
}
}
int main(int argc, char **argv) { try {
  if (argc != 2) throw FormatError("lobby roster check needs the unchanged original fixture");
  if (!std::filesystem::exists(argv[1])) return 77;
  std::ifstream input(argv[1]);
  std::vector<Vec3> palette, teams;
  std::vector<FrontendLobbyRankRange> ranges;
  std::vector<FrontendLobbyRosterPlayer> players;
  FrontendLobbyRoster roster;
  unsigned cases = 0, rows = 0, hidden = 0;
  for (std::string line; std::getline(input, line);) {
    if (line.empty() || line.front() == '#') continue;
    std::istringstream row(line); std::string kind; row >> kind;
    if (kind == "palette" || kind == "team_palette") {
      unsigned index; row >> index;
      auto &values = kind == "palette" ? palette : teams;
      check(index == values.size(), "original palette ordering changed");
      values.push_back(read_color(row));
      if (kind == "team_palette")
        check(equal_color(values.back(), frontend_lobby_team_colors.at(index)),
              "frontend team palette differs from literal mulg universal words");
    } else if (kind == "range") {
      unsigned index; FrontendLobbyRankRange value; row >> index >> value.lower >> value.upper;
      check(index == ranges.size(), "original UI rank ranges are not ordered");
      ranges.push_back(value);
      check(value.lower == frontend_lobby_rank_ranges.at(index).lower &&
            value.upper == frontend_lobby_rank_ranges.at(index).upper,
            "frontend rank ranges differ from literal wigl bounds");
    } else if (kind == "case") {
      unsigned index, count; int mode, valid, voice, team_mode, local_team, rank, members, team;
      row >> index >> mode >> count >> valid >> voice >> team_mode >> local_team >> rank >> members >> team;
      check(index == cases && palette.size() == 18 && teams.size() == 9,
            "original roster fixture lost its literal palette inputs");
      players.clear();
      for (unsigned p = 0; p < count; ++p) {
        FrontendLobbyRosterPlayer value;
        value.session_index = int(p); value.name = "Source Player " + std::to_string(p + 1);
        value.appearance.colors = {8, 12, 9, 1}; value.appearance.foreground = 26;
        value.appearance.background = 9; value.team = team; value.rank_value = rank;
        value.member_count = members; value.voice_port_flag0_only = voice != 2;
        value.voice_query_53750 = voice == 0; players.push_back(value);
      }
      FrontendLobbyRosterInput presentation;
      presentation.pane = mode; presentation.valid_session = valid;
      presentation.session_present = true; presentation.team_mode = team_mode;
      presentation.players = players; presentation.appearance_palette = palette;
      presentation.team_palette = teams; presentation.rank_ranges = ranges;
      presentation.local_teams[0].session_index = 0;
      if (local_team != -999) {
        presentation.local_teams[0].valid = true;
        presentation.local_teams[0].team = local_team;
      }
      roster = frontend_lobby_roster(presentation);
      for (unsigned p = 0; p < roster.size(); ++p) {
        const auto &slot = roster[p];
        check(slot.group == p && slot.bitmap_a == 5 + p && slot.speaker_bitmap == 21 + p,
              "original roster bitmap/group ordinal mapping changed");
        check(slot.text_a == unsigned(mode == 1 ? 22 : 27) + p &&
              slot.text_b == unsigned(mode == 1 ? 6 : 11) + p,
              "original roster typed text ordinal lost its title/subtitle prefix");
        check(slot.visible == (valid != -1 && p < count), "original active roster size changed");
      }
      ++cases;
    } else if (kind == "hidden") {
      unsigned index, speaker; row >> index >> speaker;
      check(!roster.at(index).visible && roster.at(index).speaker_visible == bool(speaker),
            "unused original roster row or speaker remains visible");
      ++hidden;
    } else if (kind == "row") {
      unsigned index, no_team, voice, background_visible, background_frame, secondary,
          rank_visible, rank_frame, emblem;
      int team, rank; std::string flags, sid;
      row >> index >> flags >> team >> no_team >> rank >> sid >> voice >> background_visible >>
          background_frame >> secondary >> rank_visible >> rank_frame >> emblem;
      const auto name_color = read_color(row), primary_color = read_color(row);
      const auto &slot = roster.at(index);
      check(slot.visible && slot.speaker_visible, "occupied original row/speaker is hidden");
      check(slot.item_flags == hex_word(flags) && slot.team == team && slot.no_team == bool(no_team) &&
            slot.rank_range == rank, "original25042D player/appearance/team/rank flags differ");
      check(slot.name_string.value_or(0xffffffffu) == hex_word(sid),
            "local team edit does not bind the original localized name override");
      check(slot.name == players[index].name && slot.appearance == players[index].appearance,
            "original row lost its session-player name or appearance binding");
      check(slot.speaker_frame == voice && slot.background_visible == bool(background_visible) &&
            slot.background_frame == background_frame && slot.secondary_background_visible == bool(secondary) &&
            slot.rank_visible == bool(rank_visible) && slot.rank_frame == rank_frame &&
            slot.emblem_visible == bool(emblem), "original roster skin visibility/frame binding differs");
      check(equal_color(slot.name_color, name_color) && equal_color(slot.primary_color, primary_color),
            "original roster nameHSV/primary color differs at a float word");
      ++rows;
    } else throw FormatError("unknown original roster fixture row");
    check(bool(row), "original roster fixture row is incomplete");
  }
  check(cases == 144 && rows + hidden == cases * 16, "original roster coverage changed");
  FrontendLobbyRosterPlayer player;player.member_count=4;
  FrontendLobbyRosterInput tu15;tu15.valid_session=1;tu15.tu15=true;
  tu15.players=std::span{&player,1};tu15.appearance_palette=palette;
  check(frontend_lobby_roster(tu15)[0].background_frame==0,"TU1.5 four-player quantity was not normalized to one");
  player.appearance_available=false;
  const auto unknown=frontend_lobby_roster(tu15)[0];
  check(!unknown.emblem_visible && !unknown.background_visible && unknown.name==player.name,
        "Missing remote appearance fabricated profile artwork");
  std::cout << cases << " unchanged original roster cases; " << rows << " occupied and "
            << hidden << " hidden slots passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
