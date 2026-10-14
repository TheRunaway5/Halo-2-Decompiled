#pragma once
#include "halo2/source_world_pose.hpp"
#include "halo2/source_model_regions.hpp"

namespace halo2 {
// Completed original object state for source-created equipment/debris/crates
// without an authored placement. No parallel body, local birth time or model
// animation is invented. WORLD nodes travel in the shared source pose bank.
struct SourceDynamicObject {
    std::uint32_t object{},definition{},parent=0xffffffffu;
    std::uint8_t type{};
    Transform transform;
    Vec3 velocity;
    ShadowCasterSphere sphere;
    std::uint32_t runtime_flags{};
    std::uint16_t control_flags{};
    bool visible{},frozen{},has_world_nodes{};
    SourceModelRegionBank regions;
    std::vector<Vec3> colors;
};
void validate_source_dynamic_objects(std::span<const SourceDynamicObject>);
}
