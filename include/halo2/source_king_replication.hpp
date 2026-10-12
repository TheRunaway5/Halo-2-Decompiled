#pragma once
#include "halo2/source_king.hpp"

namespace halo2 {
// Shared engine slots41..43 use these fields of the original MP globals.
// Preserve the unidentified +06..+21 words literally, including the nine
// sentinel words at +10..+21. They are not native player/score identifiers.
struct SourceEngineReplicationState {
  std::array<std::uint16_t,14> words_06_21{};
  std::int16_t phase{};                  // MP globals+6C
  std::uint16_t round{},clock_seconds{}; // +6E / +E0
  std::uint8_t game_over{};              // +C08, raw source byte
};
struct SourceEngineReplicationBank {
  std::array<std::uint16_t,14> words{};
  std::uint8_t phase{},game_over{};
  std::uint16_t round{},clock_seconds{},reserved_22{};
};
static_assert(sizeof(SourceEngineReplicationBank)==0x24);
static_assert(offsetof(SourceEngineReplicationBank,phase)==0x1c);
static_assert(offsetof(SourceEngineReplicationBank,reserved_22)==0x22);
struct SourceKingReplicationBank {
  SourceEngineReplicationBank shared;
  std::int16_t hill{};
  std::uint16_t occupants{};
};
static_assert(sizeof(SourceKingReplicationBank)==0x28);
static_assert(offsetof(SourceKingReplicationBank,hill)==0x24);

// Apply invokes these owners in source order after/before the corresponding
// stores. The callbacks must implement their actual world/recording effects.
// No missing owner is silently treated as success, even for a false bank flag.
struct SourceEngineReplicationServices {
  std::function<void()> refresh_player_properties,refresh_recording;
  std::function<void(std::uint8_t phase)> phase_changed;
  std::function<void()> begin_game_over;
};
void source_engine_replication_initialize(SourceEngineReplicationBank&) noexcept;
void source_engine_replication_difference(const SourceEngineReplicationState&,
    std::uint32_t requested,std::uint32_t& changed,SourceEngineReplicationBank&) noexcept;
bool source_engine_replication_apply(SourceEngineReplicationState&,
    std::uint32_t mask,const SourceEngineReplicationBank&,const SourceEngineReplicationServices&);

// Actual King slots44..46, retail2BDB40/80/2BDC00, TU1.1 2C3E80/C0/2C3F40,
// TU1.5 2C7900/40/2C79C0. These are engine-bank callbacks, not a wire codec or
// a native session adapter. The original unused callback argument has no effect.
void source_king_replication_initialize(SourceKingReplicationBank&) noexcept;
void source_king_replication_difference(const SourceEngineReplicationState&,
    const SourceKingState&,std::uint32_t& requested_and_changed,SourceKingReplicationBank&) noexcept;
bool source_king_replication_apply(SourceEngineReplicationState&,SourceKingState&,
    std::span<const SourceKingMarker>,std::uint32_t mask,const SourceKingReplicationBank&,
    const SourceEngineReplicationServices&);
} // namespace halo2
