#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_shadow.hpp"
#include <functional>

namespace halo2 {
struct SourceObjectLodInput {
    //133790's separate normalized camera-distance metric, before4BCC0.
    float draw_metric{};
    //Actual object+4 flag19 and object+10A bit2. Other bits are ignored.
    std::uint32_t object_flags{};
    std::uint16_t control_flags{};
};
struct SourceObjectViewLod {
    unsigned view_index{},local_players=1;
    bool source_4ba019{};
    //The source visibility collector and camera producer own these inputs.
    //The sphere is the actual retained bound, not a model-origin proxy.
    std::function<SourceObjectLodInput(std::size_t source_object,std::uint32_t full_handle,
                                     Vec3 sphere_center,float sphere_radius)> input;
    // Actual render-camera shader inputs. Geometry distance and collector
    // coverage are separate source producers. Absent only in legacy diagnostics.
    std::optional<SourceModelShaderView> shader;
    std::optional<SourceGeometryLodPolicy> detail_policy;
    // Necessary camera-frustum admission before requiring a machine's draw
    // resources. This does not replace portal/spatial source admission.
    std::function<bool(Vec3 sphere_center,float sphere_radius)> in_frustum;
    // Actual retained object+30/+3C from this full generation's completed
    // source snapshot. A node0 shadow bound is a separate producer.
    std::function<ShadowCasterSphere(std::uint32_t full_handle)> source_bound;
    // Original spatial membership and current portal-frustum admission.
    // Absent only for explicit diagnostic/legacy presentation callers.
    std::function<bool(std::uint32_t full_handle,ShadowCasterSphere)> source_visible;
    // Native receiver depth materializes its original prefix before upload;
    // a generic deferred skin must not replace that exact WORLD bank.
    bool require_cpu_world_positions{};
};
struct SourceObjectViewSelection {
    std::size_t source_object{};
    unsigned lod{};
    float transition{};
    std::size_t vertices{};
    float source_distance{}; //Unmodified133790 metric for first-view lighting.
};
}
