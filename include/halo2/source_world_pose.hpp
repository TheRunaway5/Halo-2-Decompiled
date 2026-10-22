#pragma once
#include "halo2/source_shadow.hpp"
#include "halo2/source_attached_effect.hpp"
#include "halo2/source_model_regions.hpp"
#include <optional>
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
    // Authority's actual scene constructor tick, retained across late join.
    std::uint64_t created_tick{};
    std::vector<SourceAttachedEffect> attached_effects;
    // Completed source constructor/current choices, never a replica RNG roll.
    std::optional<SourceModelRegionBank> model_regions;
    // Actual successful entries in either original spatial pool, including
    // orphan entries. Sorted unique cluster IDs; empty means unregistered.
    std::vector<std::int16_t> render_clusters;
};
struct SourceWorldPose {
    std::uint32_t object{0xffffffffu};
    std::vector<Transform> nodes;
    ShadowCasterSphere sphere;
};
}
