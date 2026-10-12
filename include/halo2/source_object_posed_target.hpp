#pragma once
#include "halo2/posed_collision.hpp"

namespace halo2 {
//168B40's exact world-plane arithmetic; nullptr retains the source plane.
CollisionPlane source_collision_world_plane(const Transform*,CollisionPlane,bool reversed);
// Complete retail 1EFF40 and its 168B40 surface conversion, after actual
// 1EFB40 object/model/collision lookup succeeded. The source queries the whole
// segment independently of the outer169510 packet's current fraction.
// Current WORLD matrices and active bytes are explicit live inputs. Missing
// active pointer means1EFBD0's zero-selection branch; an empty present bank
// is distinct and is rejected when the source consumes it.
std::optional<PosedCollisionHit> source_object_posed_target(
    const ModelCollision&,std::span<const Transform> current_world_nodes,
    std::optional<std::span<const std::uint8_t>> active_permutations,
    Vec3 origin,Vec3 delta,std::uint32_t lower_flags);
}
