#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Literal scenario+118/11C rows. Channel0 supplies spline controls; channel1
// supplies height markers. Selector12 is deliberately not used by King.
struct SourceKingMarker {
  std::array<float,3> position{};
  float facing{};
  std::int16_t type{},selector12{},channel{};
  std::array<std::uint8_t,10> remainder{};
};
static_assert(sizeof(SourceKingMarker)==0x20);
struct SourceKingHills {
  std::int16_t count{};
  std::array<std::int16_t,8> indices{};
};
// Original engine globals+FC state, including untouched/stale fields. Keeping
// this layout allows whole-owner comparisons rather than approximate geometry.
struct SourceKingState {
  std::array<std::int32_t,16> marker_indices{};
  std::array<std::int32_t,8> segment_samples{};
  std::int32_t marker_count{};
  std::array<std::array<float,2>,32> polygon{};
  std::array<float,3> center{};
  float radius{},minimum_z{},maximum_z{},perimeter{};
  std::array<std::int32_t,8> team_ticks{};
  std::int32_t hill{},move_ticks{};
  std::uint16_t occupants{},reserved_1aa{};
  std::array<std::array<float,3>,4> default_points{};
};
static_assert(sizeof(SourceKingState)==0x1dc);
static_assert(offsetof(SourceKingState,marker_count)==0x60);
static_assert(offsetof(SourceKingState,polygon)==0x64);
static_assert(offsetof(SourceKingState,center)==0x164);
static_assert(offsetof(SourceKingState,team_ticks)==0x180);
static_assert(offsetof(SourceKingState,hill)==0x1a0);
static_assert(offsetof(SourceKingState,occupants)==0x1a8);
static_assert(offsetof(SourceKingState,default_points)==0x1ac);

// Registered slot6 creates category6/subtype0 targeted through event+8.
// Mode4 suppresses the callback and leaves caller storage untouched; the
// defined prefix is34 bytes and the trailing two stack bytes remain opaque.
bool source_king_activation_publication(std::uint8_t mode,std::uint32_t recipient,
    std::span<std::byte,0x24> caller_storage) noexcept;

// Retail2BD140 / TU1.1 2C34D0 / TU1.5 2C6F50. Unique authored groups11..18
// retain their first-occurrence order. Unused entries in the group bank survive.
void source_king_initialize(SourceKingState&,SourceKingHills&,
    std::span<const SourceKingMarker>);
// Whole round-start/build/spline chain; NONE clears only marker_count. A build
// with fewer than4 controls preserves the source's previous count/geometry path.
void source_king_build_hill(SourceKingState&,std::span<const SourceKingMarker>,std::int32_t hill);
void source_king_round_start(SourceKingState&,std::span<const SourceKingMarker>,
    std::int16_t move_seconds,std::int16_t tick_rate);
std::int32_t source_king_select_hill(const SourceKingHills&,std::int32_t current,
    std::uint32_t& random_seed);

struct SourceKingPlayer {
  std::uint32_t datum{0xffffffffu},current_unit{0xffffffffu};
  std::int8_t membership{-1};
  std::int16_t inside_ticks{};
};
// Original current-unit world-position owner is required only after the source
// datum/count/unit gates. Its result is an input to the literal polygon test.
using SourceKingPosition=std::function<std::array<float,3>(std::uint32_t current_unit)>;
bool source_king_contains(const SourceKingState&,const SourceKingPlayer&,const SourceKingPosition&);
void source_king_update_player(const SourceKingState&,SourceKingPlayer&,
    bool game_over,std::uint8_t raw_mode,const SourceKingPosition&);
// Slot35: strictly-positive timers select the hill flag, including false;
// other timers use the shared global trait bits. Unknown selectors returnfalse.
bool source_king_trait(std::int16_t inside_ticks,std::int32_t selector,
    std::uint32_t hill_flags,std::uint32_t global_flags);
// Slot30: retained stats28 then27, not kill-score or match-bank fields.
// NONE credit is represented by a null attacker; the fourth source argument
// has no effect. The consumer owns the original enabled/frozen/stat bounds.
void source_king_death_statistics(const SourceKingPlayer& victim,const SourceKingPlayer* attacker,
    std::uint8_t betrayal,const std::function<void(std::uint32_t player,int stat)>&);

struct SourceKingContext {
  bool engine_present{true},team_mode{},game_over{};
  std::uint8_t raw_mode{1};
  std::int16_t lifecycle{1},tick_rate{30},move_seconds{};
  std::int32_t status{1};
  std::uint32_t flags{}; // options+22C, original variant+F0
  std::uint32_t match_replication_entity{0xffffffffu};
};
struct SourceKingScoringServices {
  // Original getters return the team score in team play. Award delivery owns
  // the shared stat/score-limit/phase/replication paths, not a King-only score.
  std::function<std::int32_t(std::uint32_t player)> score;
  std::function<void(std::uint32_t player,std::int32_t delta)> award;
  std::function<void(std::uint32_t player,std::int32_t score)> score_changed;
  // Original FFA recording event1972A0/198DA0/19BCA0(4,NONE).
  // The real recorder, including disabled/frozen gates, belongs to this owner.
  std::function<void(std::uint32_t player,std::int32_t kind,std::uint32_t other)> record;
};
// Whole2BEEE0/2C5220/2C8CA0 control, clock and award-selection owner. The span
// is the occupied source iterator in ascending slot order, including unitNONE
// records (which the active iterator skips). No missing service gets success.
// The direct scorer does not own the upstream game-over/playback guards.
void source_king_score(SourceKingState&,const SourceKingContext&,
    std::span<SourceKingPlayer>,const SourceKingScoringServices&);
struct SourceKingNotice {
  // Original category6 event. All other defined event fields are NONE/zero.
  // Type1/5 acquired,2/6 contested (FFA/team); type4 is hill relocation.
  std::int32_t type{};
  std::uint32_t player{0xffffffffu};
  std::int32_t membership{-1};
};
struct SourceKingWorldServices {
  SourceKingScoringServices scoring;
  std::function<void(const SourceKingNotice&)> notification;
  std::function<void(std::uint32_t entity,std::uint32_t flags)> dirty_entity;
};
// Whole King slot18 control/notifications/scorer/movement chain, following the
// original per-player slot15 callbacks. Occupancy publication precedes scoring;
// movement follows it and consumes the retained shared RNG. Actual event/dirty
// services remain required when their source branches execute.
void source_king_world_update(SourceKingState&,const SourceKingHills&,const SourceKingContext&,
    std::span<const SourceKingMarker>,std::span<SourceKingPlayer>,std::uint32_t& random_seed,
    const SourceKingWorldServices&);
} // namespace halo2
