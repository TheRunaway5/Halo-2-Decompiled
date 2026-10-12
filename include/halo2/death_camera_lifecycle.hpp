#pragma once
#include "halo2/death_camera_binding.hpp"
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
struct SourceDeathCameraLifecycleServices {
  // Only the non-multiplayer23FCA0 restriction owner remains external.
  // Multiplayer reads its actual engine table and team-game flag directly.
  std::function<bool(std::uint32_t player)> non_multiplayer_team_restriction;
};
struct SourceUnitCameraMode {
  bool seat_camera{};
  std::uint16_t mode{};
};
// Complete156F60, including salted BBCB0 validation, actual vehicle seat
// flags,110DB0 animation state and C83D0's source state list.
SourceUnitCameraMode
source_unit_camera_mode_1_1(const SourcePhysicsContactMemory &,
                            std::uint32_t unit);
// Complete157520 mode/transition setter, preserving a zero transition timer.
void source_set_camera_mode_1_1(
    SourcePhysicsContactMemory &, std::int32_t view, std::uint32_t mode_entry,
    bool transition, const SourcePhysicsContactRecordObserver & = {});
// Complete157070 normal/seat-camera restoration and conditional initialization.
void source_restore_unit_camera_1_1(
    SourcePhysicsContactMemory &, std::int32_t view, bool force,
    const SourcePhysicsContactRecordObserver & = {});
// Complete23FEA0 over actual occupied current player slots.
void source_replace_death_camera_target_1_1(
    SourcePhysicsContactMemory &, std::uint32_t camera,
    const SourceDeathCameraLifecycleServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete23FB00: raw retained per-view camera packet, actual observer/RNG
// owners, current/previous unit selection, conditional participant replacement.
void source_construct_death_camera_1_1(
    SourcePhysicsContactMemory &, std::uint32_t camera, std::int32_t view,
    std::int32_t watch_mode, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathCameraLifecycleServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete1573B0 death/normal-camera selection. Reads real player ownership
// and source frame time; construction and respawn restoration compose natively.
void source_select_death_camera_1_1(
    SourcePhysicsContactMemory &, std::int32_t view, bool reset,
    SourcePhysicsFactoryFloatingPoint,
    const SourceDeathCameraLifecycleServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
