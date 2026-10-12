#pragma once
#include "halo2/source_game_end.hpp"

namespace halo2 {
// Immutable original16-byte general-stat descriptors. Only the first two
// words have recovered writer semantics; retain the remaining bytes verbatim.
struct SourceResultStatDescriptor {
  std::uint16_t minimum{}, maximum{};
  std::array<std::uint8_t,12> remaining{};
};
using SourceResultStatDescriptors = std::array<SourceResultStatDescriptor,45>;
static_assert(sizeof(SourceResultStatDescriptor)==16);
// Original contiguous51EEF4..522F34 store (U5). General stats occupy the
// first45 words of each437-word player record. Other player fields and the
// intervening PvP records remain raw until their own producers are recovered.
struct SourceResultStatistics {
  std::array<std::array<std::uint16_t,437>,16> players{};
  std::array<std::array<std::array<std::uint16_t,2>,16>,16> pvp{};
  std::array<std::array<std::uint16_t,45>,16> teams{};
};
static_assert(offsetof(SourceResultStatistics,pvp)==0x36a0);
static_assert(offsetof(SourceResultStatistics,teams)==0x3aa0);
static_assert(sizeof(SourceResultStatistics)==0x4040);
// Source51DFA0/+1 are independent raw bytes. This writer view does not imply
// valid/complete retained player properties, names, identities or report rows.
struct SourceResultRecording { std::uint8_t enabled{}, frozen{}; };
struct SourceResultPublication {
  SourceResultStatistics& statistics;
  const SourceResultRecording& recording;
  const SourceResultStatDescriptors& descriptors;
};
// Whole original19B2B0 and19B1D0. DestinationNONE=-1; both may be selected.
// Add wraps32 bits before signed clamping. Every write preserves bit15.
void source_result_stat_set(SourceResultPublication&,int stat,int player,int team,std::int32_t value);
void source_result_stat_add(SourceResultPublication&,int stat,int player,int team,std::int32_t delta);
// Whole1F1FC0/1F20D0 stat mutation and retained publication. Native source
// adapters own the selected-engine, live-pool and replication service gates.
// A supplied team is a validated live membership, and requires an admitted
// selected team engine. No native networking codec is implied by these writes.
std::int16_t source_game_stat_player_add(SourceGameEndStats&,unsigned player,unsigned field,
    std::int16_t delta,int result_stat,SourceResultPublication* = nullptr,int team = -1);
std::int16_t source_game_stat_team_add(SourceGameEndStats&,unsigned team,unsigned field,
    std::int16_t delta,int result_stat,SourceResultPublication* = nullptr);
} // namespace halo2
