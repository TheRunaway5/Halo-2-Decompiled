#pragma once
#include "halo2/death_camera_binding.hpp"
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// Complete23FCA0: another occupied player on the same team enables the
// non-multiplayer restriction, regardless of that player's current unit.
bool source_death_camera_team_restriction_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t player);
// Complete23FE30/BBDA0: last occupied type6 object with no parent and a valid
// cluster. Reads the actual object pool; no substitute camera anchor is made.
std::uint32_t source_death_camera_fallback_1_1(
    const SourcePhysicsContactMemory &);
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
    const SourcePhysicsContactRecordObserver & = {});
// Complete23FB00: raw retained per-view camera packet, actual observer/RNG
// owners, current/previous unit selection, conditional participant replacement.
void source_construct_death_camera_1_1(
    SourcePhysicsContactMemory &, std::uint32_t camera, std::int32_t view,
    std::int32_t watch_mode, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1573B0 death/normal-camera selection. Reads real player ownership
// and source frame time; construction and respawn restoration compose natively.
void source_select_death_camera_1_1(
    SourcePhysicsContactMemory &, std::int32_t view, bool reset,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
