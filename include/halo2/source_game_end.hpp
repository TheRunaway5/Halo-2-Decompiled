#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
struct SourceResultPublication;
// Original engine+304 bank: header,16 player records and8 team records.
// Retain unidentified shorts rather than inventing their gameplay meanings.
struct SourceGameEndPlayerStats {
  std::array<std::int16_t,12> fields{};
  std::uint32_t last_kill_tick{};
};
struct SourceGameEndTeamStats { std::array<std::int16_t,9> fields{}; };
struct SourceGameEndStats {
  std::uint32_t header{};
  std::array<SourceGameEndPlayerStats,16> players;
  std::array<SourceGameEndTeamStats,8> teams;
};
static_assert(sizeof(SourceGameEndPlayerStats)==0x1c);
static_assert(sizeof(SourceGameEndTeamStats)==0x12);
static_assert(offsetof(SourceGameEndStats,players)==4);
static_assert(offsetof(SourceGameEndStats,teams)==0x1c4);
static_assert(sizeof(SourceGameEndStats)==0x254);
struct SourceGameEndParticipant {
  std::uint32_t datum;
  std::int8_t membership;
  std::uint16_t flags=1;
};
// U5 15DDC0 / TU1.1 15C9E0 win-stat branch. The span is the occupied
// original player bitmap. FFA validates the full winning datum; teams use the
// original1A8E50 iterator (exclude flag2), awarding matching members and the team.
// Returns the winner's post-increment field7. NONE/absent FFA returns0.
std::int16_t source_game_end_award(SourceGameEndStats&,
    std::span<const SourceGameEndParticipant>,bool team,std::uint32_t winner,
    SourceResultPublication* publication=nullptr);
// Same owner's literal round selector and signed-index/first-to-X thresholds.
// This leaf does not perform a round transition or admit native round settings.
bool source_round_finishes(std::int32_t kind,std::int16_t index,
    std::uint32_t winner,std::int16_t wins,bool force);
// U5 1F1A60 / TU1.1 1ED140, all slots including disconnected players and
// unretained teams. Field1 total wraps i16; field0 score stays on game-over.
// Player fields9,10 and last-kill clock clear; other fields remain unchanged.
void source_game_end_accumulate(SourceGameEndStats&,bool game_over);
} // namespace halo2
