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
struct SourceWorldPoseAttachment {
    std::uint32_t parent{0xffffffffu};
    std::uint8_t parent_node{}; // Actual OBJECT+18 byte, including unparented values.
    // The exact frame anchoring this completed child bank. Full native
    // publication copies the selected node from its final published parent
    // bank. Transport selects each root refresh and its fresh native frame
    // atomically; a held pose retains its corresponding earlier frame.
    std::optional<Transform> parent_frame;
};
struct SourceWorldPoseModel {
    // Actual object definition and16D660 permutation bytes, copied beside
    // this owner's completed WORLD hierarchy. Secondary rays resolve these
    // choices through4C2B0 at their explicit LOD4 policy; this is not a claim
    // that a camera-dependent native collector selected that LOD.
    std::uint32_t definition{0xffffffffu};
    std::vector<std::uint8_t> permutations;
    bool operator==(const SourceWorldPoseModel&)const=default;
};
inline std::vector<unsigned> source_world_pose_secondary_sections(const SourceWorldPoseModel &model,
    std::uint32_t expected_definition,const SourceInitialModelDefinition &definition) {
    if(model.definition!=expected_definition)
        throw FormatError("Secondary WORLD model lost its accepted full definition identity");
    SourceInitialModelSelection selection;selection.permutations=model.permutations;
    return source_initial_model_sections(definition,selection,4);
}
struct SourceWorldPose {
    std::uint32_t object{0xffffffffu};
    std::vector<Transform> nodes;
    ShadowCasterSphere sphere;
    // Unavailable on legacy wire formats, distinct from known unparented.
    std::optional<SourceWorldPoseAttachment> attachment;
    std::optional<SourceWorldPoseModel> model;
};
}
