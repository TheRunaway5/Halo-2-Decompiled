#pragma once
#include "halo2/source_player_profile.hpp"
#include <bit>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Original screen14's four-byte teams[controller] records: valid at+0,
// signed team at+2. The unused byte has no native state or decision meaning.
struct FrontendLobbyTeamEdit {
  bool valid{};
  std::int16_t team = -1;
};
inline bool frontend_lobby_team_available(int session_enum, std::uint32_t flags) {
  //251364 ->199967/19989D/199951: a team variant in a custom-game session.
  return (flags & 1) && (session_enum == 1 || session_enum == 3 || session_enum == 5);
}
inline unsigned frontend_lobby_team_count(std::span<const std::uint8_t> counts,
                                          int source_type, bool valid_settings) {
  //251139 ->2510E1 ->19C5F0: literal matg multiplayer record+C54.
  return valid_settings && source_type > 0 && source_type < 10 &&
      std::size_t(source_type) < counts.size() && counts[std::size_t(source_type)] > 1
      ? counts[std::size_t(source_type)] : 1;
}
inline void frontend_lobby_team_begin(FrontendLobbyTeamEdit &edit, bool available,
                                      std::optional<std::int8_t> member) {
  //250CDA(update=true). An unavailable session retains both preview fields.
  if (available) {
    edit.team = member.value_or(-1);
    edit.valid = member.has_value();
  }
}
inline bool frontend_lobby_team_commit(FrontendLobbyTeamEdit &edit, bool available,
                                       std::int32_t &committed) {
  //250CDA(update=false): clear validity even when delivery is unavailable.
  edit.valid = false;
  if (available) committed = edit.team;
  return available; // Caller owns18FE9E's session profile/update delivery.
}
inline void frontend_lobby_team_cycle(FrontendLobbyTeamEdit &edit, int delta,
                                      unsigned count, bool allow_none) {
  if (!edit.valid) return;
  //250D5C uses a wrapping32-bit add, signed comparisons, then a short store.
  auto value = std::bit_cast<std::int32_t>(std::uint32_t(std::int32_t(edit.team)) +
                                          std::uint32_t(delta));
  const int minimum = allow_none ? -1 : 0;
  if (value < minimum) value = std::int32_t(count - 1);
  else if (value >= std::int32_t(count)) value = minimum;
  edit.team = std::bit_cast<std::int16_t>(std::uint16_t(value));
}
struct FrontendLobbyTeamEvent {
  int type{}, param{};
  bool unready = true, channel_busy{}, signed_in = true, available{};
  std::optional<std::int8_t> member;
  unsigned team_count = 1;
  bool allow_none{};
};
struct FrontendLobbyTeamEventResult {
  bool consumed{}, update_requested{};
};
// The idle250B1E team-edit branches. Countdown cancellation, unsigned-controller
// sign-in and non-edit Leave Party actions belong to their separate producers.
inline FrontendLobbyTeamEventResult frontend_lobby_team_event(
    FrontendLobbyTeamEdit &edit, std::int32_t &committed,
    const FrontendLobbyTeamEvent &input) {
  if (!input.unready || input.channel_busy || !input.signed_in)
    return {true, false};
  if ((input.type == 2 || input.type == 4) && edit.valid) {
    frontend_lobby_team_cycle(edit, input.type == 2 ? -1 : 1,
                              input.team_count, input.allow_none);
    return {true, false};
  }
  if (input.type != 5) return {};
  if (input.param == 2 || ((input.param == 0 || input.param == 12) && edit.valid)) {
    if (edit.valid)
      return {true, frontend_lobby_team_commit(edit, input.available, committed)};
    frontend_lobby_team_begin(edit, input.available, input.member);
    return {true, false};
  }
  if ((input.param == 1 || input.param == 13) && edit.valid) {
    edit.valid = false;
    edit.team = -1;
    return {true, false};
  }
  return {};
}
} // namespace halo2
