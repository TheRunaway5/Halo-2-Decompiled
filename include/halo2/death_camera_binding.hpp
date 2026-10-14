#pragma once
#include "halo2/death_camera.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/native_runtime_address.hpp"
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
// Native callers retain the owned host address. Only the original ABI adapter
// resolves its packed pool links back to source tokens.
std::optional<DeathCameraTarget>
source_death_camera_target(const SourceRuntimeMemory &, NativeRuntimeAddress,
                          std::uint32_t datum);
std::vector<DeathCameraPlayer>
source_death_camera_players(const SourceRuntimeMemory &, NativeRuntimeAddress);
std::uint32_t source_death_camera_fallback(const SourceRuntimeMemory &,
                                         NativeRuntimeAddress);
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
//23FE30's BBDA0 traversal over the caller's actual current object pool.
// Used by both authenticated image layouts; pool/body fields are identical.
std::uint32_t source_death_camera_fallback(const SourcePhysicsContactMemory &,
                                          std::uint32_t object_pool);
} // namespace halo2
