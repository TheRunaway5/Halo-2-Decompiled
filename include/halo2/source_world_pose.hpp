#pragma once
#include "halo2/source_shadow.hpp"
#include <vector>

namespace halo2 {
// Read-only completed source body presentation. WORLD nodes and the retained
// object+30/+3C sphere are copied from the same full salted source owner.
// Authored placement identity and completed source root. Non-model objects
// retain an actual empty WORLD bank; they still participate in retirement.
struct SourceSceneRoot {
    std::uint32_t object{0xffffffffu},definition{0xffffffffu},placement{};
    Transform transform;
    bool visible{},has_world_nodes{};
    Vec3 velocity;
    std::uint32_t runtime_flags{};
    bool frozen{};
};
struct SourceWorldPose {
    std::uint32_t object{0xffffffffu};
    std::vector<Transform> nodes;
    ShadowCasterSphere sphere;
};
}
