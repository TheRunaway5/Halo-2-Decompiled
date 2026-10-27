#pragma once
#include "halo2/full_scoreboard_assets.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace halo2 {
struct SourcePlayerScoreboardGateTu15 {
  std::int32_t tick{},respawn_seconds{},life_limit{};
  std::uint32_t current_unit=0xffffffffu;
  std::uint16_t player_flags{};
  std::int16_t lives_remaining{},lifecycle{};
  std::uint8_t options_state=4;
  std::int32_t engine_status=1;
  bool death_observer_excluded{};
  bool operator==(const SourcePlayerScoreboardGateTu15&)const=default;
};
struct SourcePlayerStatusStateTu15 {
  std::int32_t status=-1;
  std::int16_t lifecycle{};
  bool special_mode{};
  std::uint8_t options_state=4;
  std::int32_t engine_status=1;
  std::optional<SourcePlayerScoreboardGateTu15> scoreboard_gate;
  bool operator==(const SourcePlayerStatusStateTu15&)const=default;
};
struct SourcePlayerScoreboardEventTu15 {
  std::uint32_t player=0xffffffffu;
  SourcePlayerScoreboardGateTu15 gate;
};
struct SourcePlayerStatusEventTu15 {
  std::uint32_t player=0xffffffffu;
  SourcePlayerStatusStateTu15 state;
};
struct SourcePlayerStatusPhaseTu15 {
  std::uint32_t tick{};
  std::vector<SourcePlayerStatusEventTu15> players;
};
// Complete TU1.5 1618B0 selection. Engine slot37 and15BE80 ranking results
// are explicit owner inputs; this component does not invent either producer.
struct SourcePlayerStatusInput {
  std::int32_t engine_status=-1;
  bool engine_override{},special_mode{},leading{},tied{};
  std::uint32_t current_unit=0xffffffffu;
  std::int8_t membership=-1,alive_ticks{};
  std::uint16_t player_flags{};
  bool exhausted{};
  std::int32_t respawn_ticks=-1,life_limit{},score_limit{},tick{},round_start_tick{};
  std::int16_t lives_remaining{},rate=30,round_countdown{};
};
std::int32_t source_player_status(const SourcePlayerStatusInput&);
// Inputs of161BA0 and the supported1A7C10 tokens. Scores are already delivered
// by15BB50; respawn_seconds is player+170, distinct from player+164 ticks.
struct SourcePlayerStatusTextInput {
  bool engine_present=true,team_mode{},special_mode{},suppress_playing{};
  std::int16_t lifecycle=1;
  std::int32_t status=-1,respawn_seconds{},lives_remaining{};
  std::u16string player_score,team_score,score_to_win;
};
// No value means the original gate, absent locale, or an unbound token.
std::optional<std::u16string> source_player_status_text(
    const FullScoreboardAssets&,const SourcePlayerStatusTextInput&,std::size_t capacity=256);
}
