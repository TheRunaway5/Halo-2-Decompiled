#pragma once
#include "halo2/transform.hpp"
#include <cstdint>
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
class SceneObjects;
// Read-only presentation of a live source object's root. TU1.5 BF5C3..BF620
// supplies position/+64, forward/+70, up/+7C, scale/+A0 and flag10 to BFC70.
// Attached objects consume the actual parent's WORLD node and handedness.
// Both identities are checked against the retained full salted object pool.
Transform source_scene_physics_transform(const SourceRuntimeMemory &,
    std::uint32_t object_pool,std::uint32_t full_object);
std::vector<Transform> source_scene_physics_nodes(const SourceRuntimeMemory &,
    std::uint32_t object_pool,std::uint32_t full_object);
// Publish completed source state for scene/render interpolation. This neither
// advances physics nor writes to the source bank or its placement metadata.
void publish_source_scene_physics(SceneObjects &,const SourceRuntimeMemory &,
    std::uint32_t object_pool);
}
