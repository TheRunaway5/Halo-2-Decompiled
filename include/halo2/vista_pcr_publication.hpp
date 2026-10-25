#pragma once
#include "halo2/source_game_variant.hpp"
#include "halo2/source_result_statistics.hpp"
#include "halo2/vista_pcr_results.hpp"

namespace halo2 {
// Supplied PC image33119398...09f109. The PC45B33D initializer, all112
// descriptors and63A8C2/63AA54 writes/reads independently agree with the native
// source representation for default Slayer and its admitted23 parameters.
// Reconstruct and compare all304 bytes so unknown Xbox variant fields cannot
// silently enter this PC bridge. Localized title production remains external.
struct VistaPcrPcVariant { std::array<std::byte,0x130> bytes{}; };
VistaPcrPcVariant vista_pcr_slayer_variant(const SourceGameVariant&);
// The PC session/PCR stores the same verified304-byte option schema. This
// bridge preserves the original CTF/Assault mode union as well as common
// settings; it never converts gameplay to a PC engine.
VistaPcrPcVariant vista_pcr_objective_variant(const SourceGameVariant&);
// Native network reports use the same original option union as local PCR.
// Keep each family's validation; unsupported original families still reject.
VistaPcrPcVariant vista_pcr_multiplayer_variant(const SourceGameVariant&);

struct VistaPcrPublicationPlayer {
  std::uint8_t occupied{};
  std::array<std::byte,8> identity{};       // Actual reconciled PC XUID.
  std::array<std::byte,0x84> configuration{}; // Actual final PC payload.
  std::int32_t score{},placement{};
};
struct VistaPcrPublicationTeam {
  std::uint8_t occupied{};
  std::int32_t score{},placement{};
};
struct VistaPcrPublicationInput {
  VistaPcrPcVariant variant;
  std::array<VistaPcrPublicationPlayer,16> players{};
  std::array<VistaPcrPublicationTeam,16> teams{};
  const SourceResultStatistics* statistics{};
  SourceResultRecording recording{};
  // Actual source lifecycle/engine owner results; no presence is inferred.
  std::uint8_t current{},finalized{};
  bool engine_present{};
};
struct VistaPcrConsumedSnapshot {
  std::array<std::byte,vista_pcr_source_size> source{};
  // This adapter defines only the PCR-consumed retained ranges. It is not a
  // complete469DFD/46962F publication: event, machine and other header owners
  // are unresolved here. Zero storage outside those ranges is not metadata.
  static constexpr bool complete_retained_metadata=false;
};

// Literal466E93/466ED4 writes and guards. The PC player writer acceptsNONE=-1;
// other indices must refer to caller-owned16 slots. The team writer admits0..7
// only. Player score narrows to16 bits; team score clamps signed32 to16 bits.
void vista_pcr_write_player_result(std::span<std::byte> source,int player,
                                  std::int32_t score,std::int32_t placement);
void vista_pcr_write_team_result(std::span<std::byte> source,int team,
                                std::int32_t score,std::int32_t placement);

// Literal469DFD clear prefix: +10..DC78 zeroed, +4 ring counter cleared,
// +8 receives the actual437E5E clock result. The other first16 bytes survive.
void vista_pcr_clear_publication(std::span<std::byte> source,std::uint32_t clock);

// Explicit PC identity/config copies at469EAD..469EC4 and numeric4040 copy
// at469DAF..469DC0. Result writes happen before the supplied finalization byte,
// matching the write-then-finalize lifecycle. No slot/XUID join is invented;
// the caller must supply reconciled native-slot -> actual PC identity mappings.
VistaPcrConsumedSnapshot vista_pcr_publish_consumed_snapshot(const VistaPcrPublicationInput&);

// PC4712E9's FFA branch passes field1 to4D023A, then sign-extends its AX
// result before466E93. PC game-end player records have stride20; native source
// records have stride1C. Read the actual accumulated field explicitly.
std::int16_t vista_pcr_accumulated_player_score(const SourceGameEndStats&,
                                               std::uint32_t player);
// Literal4D25DF, reading the actual source round owner flags.
bool vista_pcr_ranking_available(std::uint32_t actual_round_flags) noexcept;

// Restricted original4D3045 ->4D2D0A result: a sole enumerated FFA participant
// skips every comparison, yields encoded rank0, then469470..46948F converts it to0.
// Requires the actual source pool enumeration and selected native datum.
// Multiple/absent participants andNONE remain unsupported, rather than ranked
// by a new native policy. Score is an independent actual result owner value.
std::int8_t vista_pcr_single_player_placement(
    std::span<const std::uint32_t> actual_participants,std::uint32_t player,
    std::uint32_t actual_round_flags);
} // namespace halo2
