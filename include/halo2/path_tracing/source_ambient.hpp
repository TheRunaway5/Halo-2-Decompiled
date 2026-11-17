#pragma once
#include "halo2/path_tracing/surface.hpp"
#include "halo2/world_material_uniform.hpp"

namespace halo2::path_tracing {
inline constexpr unsigned source_ambient_flag=4096u;
namespace source_ambient_detail {
// Complete old_lightmap_tangentspace dynamic controls. The accumulated T0
// albedo multiplies twice extern24 RGB (word15) and signed bump-map Z,
// independently of directional/specular lighting. Color words stay dynamic.
inline constexpr std::array<std::uint32_t,60> dynamic_accumulation{
    1243615520,203423744,3402399949,488466976,336877856,488439808,488115480,1495269376,
    87621644,5248,3221225472,0,0,16437247,0,3224587622,3221225472,0,0,0,0,12372,
    1207959552,0,0,0,171,192,68864,73,217,208,65748,144,1246317385,1246448197,
    167840514,469842946,304088321,436273152,201853448,85786624,0,0,0,12493,12459,
    3072,1280,213,160,68608,65616,69896,99425,0,0,0,0,0};
inline constexpr std::array<std::uint32_t,89> dynamic_vertex{
    0x162078u,0x0u,0xc0001bu,0x836186cu,0x20708800u,0x0u,
    0xc0201bu,0x836186cu,0x20704800u,0x0u,0xc0401bu,0x836186cu,
    0x20702800u,0x0u,0xc0601bu,0x836186cu,0x20701800u,0x0u,
    0x60e01bu,0xc36146cu,0x2e500ff8u,0x0u,0xe1001bu,0xc436186cu,
    0x20708848u,0x0u,0xe1201bu,0xc436186cu,0x20704848u,0x0u,
    0x2a3691bu,0xc36106fu,0x18a0184cu,0x0u,0xa0081bu,0x5436106cu,
    0x20708850u,0x0u,0xa0061bu,0x5436106cu,0x20704850u,0x0u,
    0xa0041bu,0x5436106cu,0x20702850u,0x0u,0x95821bu,0x82a1afcu,
    0x3c900ff8u,0x0u,0xa3491bu,0xc36106cu,0x20708860u,0x0u,
    0x82401bu,0x942a1afcu,0x3c900ff8u,0x0u,0xa3471bu,0xc36106cu,
    0x20704860u,0x0u,0x82801bu,0x942a1afcu,0x3070c858u,0x0u,
    0xa3451bu,0xc36106cu,0x20702860u,0x0u,0xa3671bu,0xc36106cu,
    0x24a00ff8u,0x0u,0xa3651bu,0xc36106cu,0x22a00ff8u,0x0u,
    0x641401bu,0xc4361bffu,0x1011e800u,0x0u,0x80a01bu,0xa5fe1bfcu,
    0x3070e820u,0x0u,0x81601bu,0xc5fe2868u,0x3070e801u};
}
// Only source-qualified opaque roots borrow the separate lighting owner's
// current per-view/per-object ambient. Native descriptor copies stay intact.
inline const Material* source_ambient_owner(const Material& material) {
    const auto source=qualified_surface(material);if(!source)return nullptr;
    const bool detail=source->family==SurfaceFamily::DynamicDetail;
    if(!detail && (unsigned(source->family)<8 || unsigned(source->family)>10))return nullptr;
    const auto* child=detail?&material:material.environment_pass.get();
    if(!child)return nullptr;
    const auto& frames=child->animated_textures[2];
    if(!frames.empty() && child->texture_frames[2]>=frames.size())return nullptr;
    const auto& image=frames.empty()?child->bump:frames[child->texture_frames[2]];
    const auto& sampler=child->texture_states[2];
    if(!image.width || !image.height || image.pixels.empty() || image.layers!=1 || image.depth ||
       (image.format!=11 && (image.format<14 || image.format>16)) ||
       sampler.color_sign || sampler.color_key_op || (sampler.alpha_kill&4u) ||
       sampler.address_u<1 || sampler.address_u>5 || sampler.address_v<1 || sampler.address_v>5 ||
       sampler.mag_filter<1 || sampler.mag_filter>3 || sampler.min_filter<1 || sampler.min_filter>3 ||
       sampler.anisotropy>4)return nullptr;
    if(child->vertex_declaration && (child->vertex_declaration->semantics.size()<2 ||
        child->vertex_declaration->semantics[1]!=3))return nullptr;
    if(detail)return child;
    if(!child || !child->forward_pass || child->forward_vertex!=6 || !child->accumulation_texture ||
       child->forward_pass_index>=child->passes.size() || child->forward_blend)return nullptr;
    const auto& pass=child->passes[child->forward_pass_index];
    if(pass.name!="shaders\\shader_passes\\lightmap\\old_lightmap_tangentspace" ||
       pass.vertex_shader!="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic" ||
       !unlit_uv_detail::exact_vertex(child->vertex_program,source_ambient_detail::dynamic_vertex) ||
       !unlit_uv_detail::exact_vertex(pass.vertex_code,source_ambient_detail::dynamic_vertex) ||
       !surface_detail::controls(child->forward_program,source_ambient_detail::dynamic_accumulation))return nullptr;
    return child;
}
inline ShaderVector source_ambient_irradiance(const std::array<std::uint32_t,60>& program) {
    const auto color=program[15];
    return {2.f*float((color>>16)&255u)/255.f,2.f*float((color>>8)&255u)/255.f,
        2.f*float(color&255u)/255.f,1.f};
}
inline WorldMaterialUniform source_ambient_uniform(WorldMaterialUniform primary,const WorldMaterialUniform& lighting) {
    primary.specular=source_ambient_irradiance(lighting.program);
    primary.transforms[0]=lighting.forward_transforms[0];
    primary.transforms[1]=lighting.forward_transforms[2];
    return primary;
}
}
