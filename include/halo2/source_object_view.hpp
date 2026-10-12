#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_geometry_lod.hpp"
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
};
struct SourceObjectViewSelection {
    std::size_t source_object{};
    unsigned lod{};
    float transition{};
    std::size_t vertices{};
    float source_distance{}; //Unmodified133790 metric for first-view lighting.
};
}
