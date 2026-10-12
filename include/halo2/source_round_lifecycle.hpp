#pragma once
#include "halo2/source_result_statistics.hpp"
#include "halo2/inventory.hpp"
#include <functional>

namespace halo2 {
// Original engine fields. Unknown ranking DWORDs retain their literal bank.
struct SourceRoundLifecycle {
  std::uint32_t flags{};
  std::int16_t phase{},index{};
  std::uint32_t start_tick{};
  std::array<std::uint32_t,27> ranking{};
  std::int16_t clock_seconds{};
  std::int32_t countdown{},status=1;
};
struct SourceRoundPlayer {
  std::uint32_t datum{},replication_entity=0xffffffffu;
  std::uint16_t flags{};
  std::int16_t lives{};
  std::int32_t respawn_countdown{};
  std::uint8_t player_1a7{},player_1a8{},engine_respawn_flag{};
};
struct SourceRoundContext {
  bool engine_present=true,game_over{};
  std::uint8_t raw_mode{};
  std::int16_t tick_rate=30;
  std::uint32_t tick{},variant_flags{},match_replication_entity=0xffffffffu;
  std::int32_t time_limit_seconds{},life_limit{};
};
// Services perform actual owners, not substitute results. Missing services
// throw when reached. In particular reset_objects must implement the complete
// source15C840, including authored map reset when its bit is set. A caller
// cannot admit later rounds by installing an empty callback for a live world.
struct SourceRoundServices {
  std::function<void()> reset_objects,initialize_engine,begin_game_over;
  std::function<void(std::uint32_t)> unlink_netgame_object;
  //160680's session-state/entity-tracking branch; this is not B7E50.
  // The owner must apply state4 and tracking-flag4 before setting dirty bit1.
  std::function<void(std::uint32_t)> reset_respawn_replication;
  std::function<void(std::int32_t)> phase_changed;
  // Preparation uses dirty flag8 only for a non-NONE match entity. It is
  // the same required replication owner used by the final phase flag2.
  std::function<void(std::uint32_t,std::uint32_t)> dirty_entity;
};
// Whole15EFA0 control and15A560's shared stores. The span is the original
// occupied-player bitmap in slot order; flag2 excludes a participant.
// Engine initialization, world reset and notifications use required services.
bool source_round_apply_phase(SourceRoundLifecycle&,const SourceRoundContext&,
    SourceGameEndStats&,SourceResultRecording&,std::span<SourceRoundPlayer>,
    std::span<NetgameWeaponState>,const SourceRoundServices&);
//15F450's non-game-over branch only. At the index31 cap begin_game_over runs
// before pending-phase application; true reports that call. Final result/report
// ownership is separate. Already game-over input is deliberately not handled.
bool source_round_post_world(SourceRoundLifecycle&,const SourceRoundContext&,
    SourceGameEndStats&,SourceResultRecording&,std::span<SourceRoundPlayer>,
    std::span<NetgameWeaponState>,const SourceRoundServices&);
} // namespace halo2
