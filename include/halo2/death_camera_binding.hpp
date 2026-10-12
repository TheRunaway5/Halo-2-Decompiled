#pragma once
#include "halo2/death_camera.hpp"
#include "halo2/physics_contact_record.hpp"
#include <vector>

namespace halo2 {
// The pool addresses are current runtime pointers, not image-relative globals.
// BBCB0 validates the complete salt before 23FF50 consumes WORLD node zero.
// A valid object with unavailable node backing is an error in the memory
// provider; it must not turn into a missing target or a root-position proxy.
std::optional<DeathCameraTarget>
source_death_camera_target(const SourcePhysicsContactMemory &,
                           std::uint32_t object_pool, std::uint32_t datum);

// Actual occupied source-player slots in iterator order, retaining current and
// previous object ownership. No host player index is substituted for a datum.
std::vector<DeathCameraPlayer>
source_death_camera_players(const SourcePhysicsContactMemory &,
                            std::uint32_t player_pool);
} // namespace halo2
