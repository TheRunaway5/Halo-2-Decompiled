#pragma once
#include "halo2/source_result_statistics.hpp"
#include "halo2/source_player_profile.hpp"

namespace halo2 {
using SourceResultPlayerIdentity = SourcePlayerIdentity;
using SourceResultMachineIdentity = SourceMachineIdentity;
struct SourceResultPlayerProperties {
  std::uint8_t occupied{};
  std::int8_t machine{};
  SourceResultPlayerIdentity identity{};
  std::array<std::uint8_t,2> reserved{};
  // Exact144-byte source player+44 configuration. Field producers remain
  // separate; remapping must not reconstruct names/appearance from guesses.
  SourcePlayerConfiguration configuration{};
  std::int8_t rank{};
  std::uint8_t rank_reserved{};
  std::int16_t score{};
};
static_assert(sizeof(SourceResultPlayerProperties)==0xa4);
static_assert(offsetof(SourceResultPlayerProperties,identity)==2);
static_assert(offsetof(SourceResultPlayerProperties,configuration)==0x10);
static_assert(offsetof(SourceResultPlayerProperties,rank)==0xa0);
static_assert(offsetof(SourceResultPlayerProperties,score)==0xa2);
struct SourceResultMachine {
  SourceResultMachineIdentity identity{};
  std::uint8_t occupied{};
  std::array<std::uint8_t,4> remaining{};
};
static_assert(sizeof(SourceResultMachine)==11);
// Complete original51DFB0 retained context. Unrecovered fields are raw and
// preserved. Header+1 is completion, independent of recording's frozen byte.
struct SourceRetainedResults {
  std::array<std::uint8_t,0x384> header{};
  std::array<SourceResultPlayerProperties,16> players{};
  std::array<std::uint8_t,0x180> team_properties{};
  SourceResultStatistics statistics;
  std::array<std::array<std::uint8_t,36>,1000> events{};
  std::array<SourceResultMachine,16> machines{};
  std::array<std::uint8_t,0x1a4> remaining{};
};
static_assert(offsetof(SourceRetainedResults,players)==0x384);
static_assert(offsetof(SourceRetainedResults,statistics)==0xf44);
static_assert(offsetof(SourceRetainedResults,events)==0x4f84);
static_assert(offsetof(SourceRetainedResults,machines)==0xdc24);
static_assert(sizeof(SourceRetainedResults)==0xde78);
struct SourceResultLivePlayer {
  std::uint16_t salt{}; // Original slot occupancy word, not a fabricated ID.
  SourceResultPlayerIdentity identity{};
  std::int16_t machine = -1;
  SourcePlayerConfiguration configuration{};
};
using SourceResultLivePlayers = std::array<SourceResultLivePlayer,16>;
using SourceResultMachineRoster = std::array<SourceResultMachineIdentity,16>;
enum class SourceResultMachinePolicy { PruneUnreferenced, Keep };
// Whole19DD10 (U5) or199310/19AE10 (retail/TU1.1) mutation. Earlier owners
// prune machine records not referenced by retained occupied properties.
void source_result_bind_machines(SourceRetainedResults&,const SourceResultLivePlayers&,
    std::int32_t high_water,const SourceResultMachineRoster&,SourceResultMachinePolicy);
// Whole196470/197F70/19AE70 retained mutation. Caller owns real source
// identities/configuration, machine roster and publication lifecycle. No
// native platform identity, valid/complete result or PCR is manufactured.
void source_result_remap(SourceRetainedResults&,SourceResultRecording,
    const SourceResultLivePlayers&,std::int32_t high_water,
    const SourceResultMachineRoster&,SourceResultMachinePolicy);
} // namespace halo2
