#pragma once
#include <cstdint>
#include <span>

namespace halo2 {
struct SourceRoundEndParticipant {
  std::uint32_t datum;
  std::uint16_t flags;
  std::int8_t membership;
  std::uint32_t current_unit;
  std::int16_t lives;
};
struct SourceRoundEndInput {
  bool engine_present=true,team_mode{};
  std::int32_t life_limit{};
  std::uint16_t retained_team_mask{}; // Engine+0E; count only original teams0..7.
  std::int32_t source_engine=2,variant_240{};
  std::uint32_t engine_2f0=0xffffffffu; // Literal CTF query field, separate from Slayer.
  std::int16_t lifecycle=1;
  std::int32_t status=1;
  std::uint8_t raw_mode{};
};
struct SourceRoundEndDecision {
  bool end{},force{};
  std::uint32_t winner=0xffffffffu;
};
// Original158D20/15A360/15B740 and its count/collection owners. The span
// is the occupied source bitmap (capacity16), in ascending slot order.
// Winner preserves the caller's input when the original owner does not store.
SourceRoundEndDecision source_round_end_query(const SourceRoundEndInput&,
    std::span<const SourceRoundEndParticipant>,std::uint32_t initial_winner=0xffffffffu);
// Original15BA20/15D060/15E440 active-state gate around that query. This
// decision does not itself award wins, change phase, reset a round or emit HUD.
SourceRoundEndDecision source_round_end_check(const SourceRoundEndInput&,
    std::span<const SourceRoundEndParticipant>);
} // namespace halo2
