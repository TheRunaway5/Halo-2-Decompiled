#include "halo2/frontend_model_shader.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/model_selection.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
SourceShaderCoverageCamera frontend_model_coverage_camera() {
    // 3763B0 initializes5234DC from literal45E628. 2F800's null-observer
    // branch converts this horizontal angle without the observer's .785 factor.
    const double horizontal=std::bit_cast<float>(0x3f9c61aau);
    const float vertical=float(2.*std::atan(std::tan(horizontal*.5)*.75));
    SourceFrustumProjectionInput input;
    input.projection_angle=vertical;
    input.outer=input.safe={0,0,480,640};
    input.use_projection_bounds=false; // 2C8A0 calls2FD90 with EDX=0.
    input.layout_state_present=true;
    input.layout_state_flag=1;
    input.layout_state_index=0;
    input.layout_state_mode=3;
    const auto projection=source_frustum_projection(input).projection_program;
    // Source literal vectors are position0, forward+X, up+Z. The original
    // 2FD90 inverse Z row is [-1,0,0,0]; viewport metric is (p11*p5)*.5.
    return {{-1,0,0,0},(projection[11]*projection[5])*.5f};
}
FrontendModelGeometryLod frontend_model_geometry_lod(
    const SourceGeometryLodDefinition&definition,Vec3 owner,
    std::uint32_t flags,std::uint8_t auxiliary,std::int16_t scenario_type,
    std::int8_t previous) {
    const float distance=source_geometry_lod_distance(owner,{},1.f);
    float metric=distance;
    if(source_geometry_lod_view_modifier(false,false,true,scenario_type))
        metric*=std::bit_cast<float>(0x3e99999au); // Original44AE90.
    if(flags&0x80000u)return {distance,metric,5}; //4BE1F precedes auxiliary scale.
    if(auxiliary&4u)metric*=std::bit_cast<float>(0x3fd9999au); // Original45E384 (1.7).
    const auto choice=source_geometry_lod_choice(definition,metric,previous);
    return {distance,metric,source_geometry_lod_cap(choice,
        source_geometry_lod_policy(1),flags,false)};
}
RenderModel load_frontend_model_geometry(const Cache&cache,std::uint32_t object,
    std::uint32_t variant,Vec3 owner,std::uint32_t flags,std::uint8_t auxiliary,
    std::int8_t previous) {
    const auto choice=frontend_model_geometry_lod(
        cache.load_source_geometry_lod_definition(object),owner,flags,auxiliary,
        cache.load_source_geometry_scenario_type(),previous);
    const auto definition=cache.load_initial_model_definition(object);
    const auto selection=source_deterministic_initial_model_selection(
        definition,variant?variant:definition.default_variant);
    return cache.load_model(object,true,variant,selection,unsigned(choice.level));
}
}
