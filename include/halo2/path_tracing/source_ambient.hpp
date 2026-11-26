#pragma once
#include "halo2/path_tracing/surface.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/world_material_uniform.hpp"

namespace halo2::path_tracing {
inline constexpr unsigned source_ambient_flag=4096u;
inline constexpr unsigned source_flat_ambient_flag=16777216u;
inline constexpr std::uint32_t source_default_ambient_kind=4u;
inline std::uint32_t source_default_ambient_rgb(const std::array<std::uint32_t,60>& program,
                                               SurfaceFamily family=SurfaceFamily::OpaqueDetail) {
    return program[family==SurfaceFamily::DynamicOneChangeColor?15u:14u]&0xffffffu;
}
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
// The Prophet robes retain vertex alpha at stage 5. The RGB lighting
// controls and ambient inputs are the same as the other accumulated pass.
inline constexpr auto dynamic_accumulation_vertex_alpha=[] {
    auto words=dynamic_accumulation;words[5]=0x14200000u;return words;
}();
inline constexpr std::array<std::uint32_t,60> fixed_accumulation{
    0,0,0,0x0d180000,0,0,0,0,0x0000000c,0x00001480,
    0,0x00807d78,0,0x00101010,0,0,0,0,0,0x005f5a55,0xc0000000,0,0,0,0,0,
    0,0,0,0x00010040,0,0,0,0,
    0x4a6b4a45,0x0c010d02,0x0c120000,0x0c080108,0,0,0,0,0,0,0,
    0x000030cd,0x00000c00,0x000020d0,0x00010c00,0,0,0,0,0x00011104,0x00018461,0,0,0,0,0};
inline constexpr auto fixed_combined=[] {
    auto words=fixed_accumulation;
    words[8]=0x140f000c;words[9]=0x09110080;
    return words;
}();
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
inline constexpr std::array<std::uint32_t,113> combined_vertex{
    0x1c2078,0,0x60e01a,0x7c001468,0x2e500000,0,0xc0601b,0x8367800,0x20b01800,
    0,0xc0001b,0x8361800,0x20b08800,0,0x6c0201b,0x8363bff,0x10b14800,0,0xc0401b,0x8365800,
    0x20b02800,0,0x2a0081a,0x543493ff,0x18a0184c,0,0xa0061a,0x54347000,0x24a00000,
    0,0xa0041a,0x54345000,0x22a00000,0,0xa0041a,0x2834a800,0x21a00000,0,0xa0001a,
    0xa4354800,0x21b00000,0,0x958215,0x182b9aff,0x3c900000,0,0x8400458,0x29ff4bfe,
    0xd7b80000,0,0x824015,0x942a5afc,0xbc900000,0,0x40001a,0xa4002800,0x2ea00000,
    0,0xa3691a,0xbc349000,0x21800000,0,0xa3671a,0xbc347000,0x28800000,0,0xa3651a,0xbc345000,
    0x22800000,0,0xe1001b,0xc4371800,0x20b08848,0,0xe1201b,0xc4373800,0x20904848,
    0,0x41401a,0xc4355800,0x20a0e800,0,0x80c0da,0xb5fedc69,0x50b0e850,0,0x828015,
    0x942a9afd,0x30b0c858,0,0xa3491a,0xac349000,0x20b08860,0,0xa3471a,0xac347000,
    0x20b04860,0,0xa3451a,0xac345000,0x20b02860,0,0x80a01a,0xa5febbfd,0x70a0e818,
    0,0x80a0ca,0x85febbfd,0x70a0e820,0,0x81601a,0xc5fe286a,0xf0b0e801};
inline bool accumulated_family(SurfaceFamily family) {
    return family==SurfaceFamily::TextureDetail || family==SurfaceFamily::TextureTwoChangeColor ||
        family==SurfaceFamily::TextureFourChangeColor || family==SurfaceFamily::TextureHonorGuard;
}
}
// Only source-qualified opaque roots borrow the separate lighting owner's
// current per-view/per-object ambient. Native descriptor copies stay intact.
inline const Material* source_ambient_owner(const Material& material) {
    const auto source=qualified_surface(material);if(!source || source->default_object_ambient)return nullptr;
    const bool detail=source->family==SurfaceFamily::DynamicDetail || source->family==SurfaceFamily::DynamicOneChangeColor;
    if(!detail && !source_ambient_detail::accumulated_family(source->family))return nullptr;
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
    if(pass.name=="shaders\\shader_passes\\lightmap\\lightmap_env" &&
       pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic_env" &&
       unlit_uv_detail::exact_vertex(child->vertex_program,source_ambient_detail::combined_vertex) &&
       unlit_uv_detail::exact_vertex(pass.vertex_code,source_ambient_detail::combined_vertex) &&
       pstencil::supports_combined_environment_program(*child) &&
       std::ranges::any_of(pass.pixel_externs,[](const auto& value) {
           return value.destination==2 && value.component==4 && value.source==24;
       }))return child;
    if(pass.name!="shaders\\shader_passes\\lightmap\\old_lightmap_tangentspace" ||
       pass.vertex_shader!="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic" ||
       !unlit_uv_detail::exact_vertex(child->vertex_program,source_ambient_detail::dynamic_vertex) ||
       !unlit_uv_detail::exact_vertex(pass.vertex_code,source_ambient_detail::dynamic_vertex) ||
       (!surface_detail::controls(child->forward_program,source_ambient_detail::dynamic_accumulation) &&
        !surface_detail::controls(child->forward_program,source_ambient_detail::dynamic_accumulation_vertex_alpha)))return nullptr;
    return child;
}
// Layer 3 implementation 3 puts the fixed authored ambient in C0 stage 3.
// Its RGB remains independent of the accumulated base texture's alpha.
inline std::optional<std::uint32_t> source_fixed_accumulated_ambient(const Material& root,SurfaceFamily family) {
    using namespace source_ambient_detail;
    const auto* child=root.environment_pass.get();
    if(!accumulated_family(family) || !child || !child->forward_pass || child->forward_vertex!=6 ||
       !child->accumulation_texture || child->forward_blend || child->forward_pass_index>=child->passes.size())return {};
    const auto& pass=child->passes[child->forward_pass_index];
    if(pass.implementation!=3 || !pass.pixel_externs.empty())return {};
    // The Guard carries an overlay in its shader tag, but the fixed layer
    // has no pixel animation remaps. Its constant stays valid across frames.
    const auto& animation=child->animation;
    if(!animation.overlays.empty()) {
        if(pass.template_pass>=animation.passes.size())return {};
        const auto range=animation.passes[pass.template_pass];
        const auto implementation=range.first()+pass.implementation;
        if(pass.implementation>=range.count() || implementation>=animation.implementations.size() ||
           animation.implementations[implementation].pixel_constants.count())return {};
    }
    const bool combined=pass.name=="shaders\\shader_passes\\lightmap\\lightmap_env" &&
        pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic_env" &&
        unlit_uv_detail::exact_vertex(child->vertex_program,combined_vertex) &&
        unlit_uv_detail::exact_vertex(pass.vertex_code,combined_vertex) &&
        surface_detail::controls(child->forward_program,fixed_combined);
    const bool split=pass.name=="shaders\\shader_passes\\lightmap\\old_lightmap_tangentspace" &&
        pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic" &&
        unlit_uv_detail::exact_vertex(child->vertex_program,dynamic_vertex) &&
        unlit_uv_detail::exact_vertex(pass.vertex_code,dynamic_vertex) &&
        surface_detail::controls(child->forward_program,fixed_accumulation);
    if(!combined && !split)return {};
    return child->forward_program.words[13]&0xffffffu;
}
// The selected, qualified one-change-color lighting pass receives the same
// native ambient extern as DynamicDetail through the alternate C1 slot.
inline unsigned source_ambient_color_word(const Material& lighting) {
    if(lighting.forward_pass_index>=lighting.passes.size())return 15u;
    const auto& name=lighting.passes[lighting.forward_pass_index].name;
    if(name==surface_detail::one_change_color_pass)return 23u;
    if(name=="shaders\\shader_passes\\lightmap\\lightmap_env")return 16u;
    return 15u;
}
inline ShaderVector source_ambient_irradiance(const std::array<std::uint32_t,60>& program,unsigned color_word=15u) {
    const auto color=program[color_word];
    return {2.f*float((color>>16)&255u)/255.f,2.f*float((color>>8)&255u)/255.f,
        2.f*float(color&255u)/255.f,1.f};
}
inline WorldMaterialUniform source_ambient_uniform(WorldMaterialUniform primary,const WorldMaterialUniform& lighting,
                                                     unsigned color_word=15u) {
    primary.specular=source_ambient_irradiance(lighting.program,color_word);
    primary.transforms[0]=lighting.forward_transforms[0];
    primary.transforms[1]=lighting.forward_transforms[2];
    return primary;
}
}
