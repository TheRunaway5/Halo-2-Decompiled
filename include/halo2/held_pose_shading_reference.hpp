#pragma once
#include "halo2/model.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/render_view.hpp"
#include "halo2/source_geometry_lod.hpp"

namespace halo2 {
// Offline renderer comparison inputs captured at original13A2E0/13A357.
// This service is linked only into the held-pose shading capture executable.
// It supplies a retained render frame, never a gameplay/animation acceptance.
struct HeldPoseShadingReference {
    std::uint32_t definition{},render_model{};
    unsigned lod{};
    std::vector<unsigned> sections;
    std::vector<Transform> nodes;
    ObjectLightingState lighting;
    ShadowCasterSphere sphere;
    std::optional<ObjectColors> colors;
};
const HeldPoseShadingReference* held_pose_shading_reference(std::uint32_t definition);
double held_pose_shading_reference_time();
RenderViewport held_pose_shading_reference_safe_rectangle();
RenderViewport held_pose_shading_reference_outer_rectangle();
SourceGeometryLodPolicy held_pose_shading_reference_policy();
// Explicit retained-image storage input for isolated sampler experiments.
bool held_pose_shading_reference_texture(Texture &);
}
