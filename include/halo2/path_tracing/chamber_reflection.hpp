#pragma once
#include "halo2/path_tracing/surface.hpp"
#include <algorithm>

namespace halo2::path_tracing {

struct ChamberReflectionParameters {
    ShaderVector facing, glancing;
    float baked_weight;
};

inline ChamberReflectionParameters chamber_reflection_parameters(const std::array<std::uint32_t,60>& words) {
    // The static lightmap_env graph interpolates tint in RGB stage 2 and
    // strength in alpha stage 3. RGB stage 4 weights the incident light.
    return {surface_detail::argb((words[12]&0xffffffu)|(words[13]&0xff000000u)),
        surface_detail::argb((words[20]&0xffffffu)|(words[21]&0xff000000u)),
        float(words[22]>>24)/255.f};
}

namespace chamber_reflection_detail {
inline constexpr std::string_view owner="scenarios\\shaders\\covenant\\military\\metals\\";
inline constexpr std::array<std::string_view,14> leaves{
    "cov_blue","cov_dark_blue","cov_dark_purple","panel_f_blue","panel_f_grey","panel_f_purple","panel_g_purple",
    "scallop","strip_a_blue","strip_a_blue_lit","strip_a_grey","strip_a_purple","strip_a_purple_lit","strip_b"};
// Assembled retail lightmap_env, implementation 0, for the chamber's
// cov_dark_blue. The per-vertex path uses the normalization cube in T3.
inline constexpr std::array<std::uint32_t,60> pixel_controls{
    0x45205518,0x15ccf5dc,0xccca80cd,0x0a112a12,0x1d1d1119,0x15150000,0x1d1d0000,0x0d180000,
    0x040f000e,0x141c0080,0,0,0x949494,0x40000000,0x80000000,0x1f2b45,0,0,0,0,
    0x6b6b6b,0x33000000,0xc0000000,0,0,0,0xc9,0x500,0x10d00,0xc00,0x59,0xd0,0xd0,0x10040,
    0x4a454a44,0x45440000,0x0a012a02,0x0b200bd5,0x0c120409,0x0c08010b,0x0b190000,0x1d050000,
    0,0,0,0x30ca,0x20d0,0x400,0xc00,0x20d4,0x100cb,0x10050,0x10050,0x11108,0x8461,0,0,0,0,0};
inline constexpr auto vertex_controls=[] {
    auto words=pixel_controls;
    words[0]=0x4b205518;
    words[34]=0x4a4b4a44;
    words[35]=0x4b440000;
    words[37]=0x052005d5;
    words[39]=0x0c080105;
    words[40]=0x05190000;
    words[50]=0x100c5;
    words[54]=0x18461;
    return words;
}();
inline bool controls_match(const Material& pass,bool alternate_highlight) {
    auto controls=pass.per_vertex_lighting?vertex_controls:pixel_controls;
    if(alternate_highlight) {
        // Bare grey trim uses a different diffuse/highlight operand in
        // stage 6. The stage 2/3 tint and masked cube graph are identical.
        controls[6]=0xc0200000;
        controls[40]=0xc0200000;
        controls[51]=0x50;
    }
    const auto& words=pass.forward_program.words;
    for(std::size_t i=0;i<words.size();++i) {
        std::uint32_t fixed=0xffffffffu;
        switch(i) {
            case 12:case 15:case 20:fixed=0xff000000u;break;
            case 13:case 14:case 21:fixed=0x00ffffffu;break;
        }
        if((words[i]&fixed)!=(controls[i]&fixed))return false;
    }
    return true;
}
}

inline const Material* chamber_reflection_pass(const Material& root) {
    using namespace chamber_reflection_detail;
    const std::string_view name=root.name;
    if(!name.starts_with(owner) || !root.world_pass || !root.forward_pass ||
       root.forward_blend || root.forward_raster.alpha_test)return nullptr;
    const auto leaf=name.substr(owner.size());
    if(std::ranges::find(leaves,leaf)==leaves.end())return nullptr;
    const auto* pass=root.environment_pass?root.environment_pass.get():&root;
    if(!pass->world_pass || !pass->forward_pass || pass->forward_vertex!=6 || pass->forward_blend ||
       pass->forward_raster.alpha_test || pass->name!=root.name || pass->source_shader_datum!=root.source_shader_datum ||
       pass->forward_pass_index>=pass->passes.size() || pass->detail.layers!=6 || pass->detail.depth ||
       pass->bump.layers!=1 || pass->bump.depth)return nullptr;

    const auto& source=pass->passes[pass->forward_pass_index];
    const bool accumulated=pass!=&root;
    const bool per_vertex=pass->per_vertex_lighting;
    constexpr std::string_view lightmap="shaders\\shader_passes\\lightmap\\lightmap_env";
    constexpr std::string_view textured="shaders\\shader_passes\\lightmap\\lightmap_tex_env";
    constexpr std::string_view vertex_owner="rasterizer\\vertex_shaders_xbox\\";
    const std::string_view vertex=accumulated?(per_vertex?"lightmap_pervertex_env":"lightmap_perpixel_env"):
        (per_vertex?"lightmap_pervertex_tex_env":"lightmap_perpixel_tex_env");
    const std::string_view vertex_shader=source.vertex_shader;
    if(pass->accumulation_texture!=accumulated || pass->baked_lightmap_texture==per_vertex ||
       source.name!=(accumulated?lightmap:textured) || source.implementation!=(per_vertex?1u:0u) ||
       !vertex_shader.starts_with(vertex_owner) || vertex_shader.substr(vertex_owner.size())!=vertex ||
       source.textures.size()!=4)return nullptr;

    const std::array<ShaderTextureBinding,4> bindings{{
        {std::uint8_t(accumulated?255:1),std::uint8_t(accumulated?3:0),0,0},
        {2,0,1,0},{0,0,2,0},{255,std::uint8_t(per_vertex?1:11),3,0}}};
    for(std::size_t i=0;i<bindings.size();++i) {
        const auto& texture=source.textures[i];
        const auto& expected=bindings[i];
        if(texture.stage!=expected.stage || texture.parameter!=expected.parameter ||
           texture.external!=expected.external || texture.flags!=expected.flags)
            return nullptr;
    }
    const auto cube=std::ranges::find_if(source.bitmap_remaps,[](const auto& remap) {
        return (remap.destination&255u)==2u && remap.source==4u;
    });
    if(cube==source.bitmap_remaps.end() || !controls_match(*pass,leaf=="strip_a_grey"))return nullptr;
    if(accumulated) {
        // T0 alpha is the accumulation pass's authored texture/detail
        // product. Admit only that exact, already supported surface.
        const auto base=qualified_surface(root);
        if(!base || base->family!=SurfaceFamily::TextureDetail)return nullptr;
    }
    return pass;
}
}
