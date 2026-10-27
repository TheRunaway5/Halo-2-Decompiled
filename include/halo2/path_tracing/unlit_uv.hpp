#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cmath>
#include <optional>
#include <span>

namespace halo2::path_tracing {
enum class UnlitUvFamily { None=0, Texture=1, LightmapDetail=2 };
struct UnlitUv {
    UnlitUvFamily family=UnlitUvFamily::None;
    ShaderVector base{};
    std::array<ShaderVector,4> stages{};
};
namespace unlit_uv_detail {
// Exact world-class native bodies from the admitted original/MCC material
// banks. Include the header and unused instruction bits in qualification.
// SHA-256 46ff53294ae335c47103b1c5ec0b8b84cde853490d2e761df919f8e2589e8083
inline constexpr std::array<std::uint32_t,49> texture_words{
    0x000c2078u,0x00000000u,0x00c0001bu,0x0836186cu,
    0x20708800u,0x00000000u,0x00c0201bu,0x0836186cu,
    0x20704800u,0x00000000u,0x00c0401bu,0x0836186cu,
    0x20702800u,0x00000000u,0x00c0601bu,0x0836186cu,
    0x20701800u,0x00000000u,0x0095821bu,0x082a1afcu,
    0x3c900ff8u,0x00000000u,0x0020141bu,0x0836106cu,
    0x2070f818u,0x00000000u,0x0082401bu,0x942a1afcu,
    0x3c90c848u,0x00000000u,0x0641401bu,0xc4361bffu,
    0x1011e800u,0x00000000u,0x0082601bu,0x942a1afcu,
    0x3070c850u,0x00000000u,0x0042801bu,0x942a186cu,
    0x2070c858u,0x00000000u,0x0042801bu,0x957e186cu,
    0x2070c860u,0x00000000u,0x0081601bu,0xc5fe2868u,
    0x3070e801u};

// SHA-256 ee2742c030b1dcbd0ff0f5ad76643f304513d2fbae16ed419ff5f03db2d44ffd
inline constexpr std::array<std::uint32_t,89> perpixel_detail_words{
    0x00162078u,0x00000000u,0x00200a1au,0x58001000u,
    0x2eb00000u,0x00000000u,0x00958215u,0x182b9affu,
    0x3c900000u,0x00000000u,0x00a0081au,0xb4349000u,
    0x28a00000u,0x00000000u,0x00a0061au,0xb4347000u,
    0x24a00000u,0x00000000u,0x00a0041au,0xb4345000u,
    0x22a00000u,0x00000000u,0x00c0001bu,0x08361800u,
    0x20b08800u,0x00000000u,0x02a00c1au,0xa4354855u,
    0xa1a0c864u,0x00000000u,0x00c0201bu,0x08363800u,
    0x20b04800u,0x00000000u,0x08c0401bu,0x08365bfeu,
    0x90b12800u,0x00000000u,0x00c0601bu,0x08367800u,
    0x20a01800u,0x00000000u,0x00824015u,0x942a5afcu,
    0xbc90c848u,0x00000000u,0x06a00a1au,0x5834b3ffu,
    0x11b80000u,0x00000000u,0x0040001au,0xa5fe2800u,
    0x2ea00000u,0x00000000u,0x08a0c81au,0x6c3493feu,
    0xd1b10000u,0x00000000u,0x00a0c61au,0x6c347000u,
    0x24b00000u,0x00000000u,0x04a0c41au,0x6c3453fcu,
    0x52b01824u,0x00000000u,0x0041401au,0xc4355800u,
    0x20b0e800u,0x00000000u,0x00826015u,0x942a7afcu,
    0xf0b0c850u,0x00000000u,0x0080a01au,0xa5febbfdu,
    0x70a0e820u,0x00000000u,0x00828015u,0x942a9afdu,
    0x30a0c858u,0x00000000u,0x0080a0dau,0xb5febbfdu,
    0x70a0e818u,0x00000000u,0x0081601au,0xc400286au,
    0xf0b0e801u};

// SHA-256 8957bc85acbc060af5e15fbb07736d6e8a152f579224db7953a6a648e5baada3
inline constexpr std::array<std::uint32_t,77> pervertex_detail_words{
    0x00132078u,0x00000000u,0x00c0001bu,0x0836186cu,
    0x20708800u,0x00000000u,0x00c0201bu,0x0836186cu,
    0x20704800u,0x00000000u,0x00c0401bu,0x0836186cu,
    0x20702800u,0x00000000u,0x00c0601bu,0x0836186cu,
    0x20701800u,0x00000000u,0x0095821bu,0x082a1afcu,
    0x3c200ff8u,0x00000000u,0x00200a1bu,0x0836106cu,
    0x2e400ff8u,0x00000000u,0x0082401bu,0x242a1afcu,
    0x3c30c848u,0x00000000u,0x02a00c1bu,0x4436886cu,
    0x2150e824u,0x00000000u,0x0082601bu,0x342a1afcu,
    0x3070c850u,0x00000000u,0x0082801bu,0x342a1afcu,
    0x3070c858u,0x00000000u,0x08a0081bu,0x443613fdu,
    0x50118860u,0x00000000u,0x00a0061bu,0x4436106cu,
    0x20704860u,0x00000000u,0x04a0c81bu,0x0c3613fcu,
    0x58601824u,0x00000000u,0x00a0041bu,0x4436106cu,
    0x20702860u,0x00000000u,0x00a0c61bu,0x0c36106cu,
    0x24600ff8u,0x00000000u,0x00a0c41bu,0x0c36106cu,
    0x22600ff8u,0x00000000u,0x0641401bu,0xc4361bffu,
    0x1011e800u,0x00000000u,0x0080a01bu,0x65fe1bfcu,
    0x3070e818u,0x00000000u,0x0081601bu,0xc5fe2868u,
    0x3070e801u};
template<std::size_t N> inline bool exact_vertex(std::span<const std::byte> bytes,
                                                    const std::array<std::uint32_t,N>& expected) {
    if(bytes.size()!=N*4)return false;
    for(std::size_t word=0;word<N;++word)for(unsigned byte=0;byte<4;++byte)
        if(std::to_integer<unsigned>(bytes[word*4+byte])!=((expected[word]>>(byte*8))&255u))return false;
    return true;
}
}
// c18 transforms raw mesh UV once. All following stage transforms consume
// that result, retaining source MUL/ADD order rather than flattening affines.
// The per-pixel detail body's oT3 is the separate lightmap stream; its
// per-vertex sibling does not supply an unlit2D oT3. Neither is a raw-UV stage3
// and neither may be sampled by this unlit surface adapter.
inline std::optional<UnlitUv> qualified_unlit_uv(const Material& material) {
    if(!material.forward_pass || material.forward_pass_index>=material.passes.size())return {};
    if(material.vertex_declaration && (material.vertex_declaration->semantics.size()<2 ||
        material.vertex_declaration->semantics[1]!=3))return {};
    const auto& pass=material.passes[material.forward_pass_index];
    const auto matches=[&](const auto& words) {
        return unlit_uv_detail::exact_vertex(material.vertex_program,words) &&
            unlit_uv_detail::exact_vertex(pass.vertex_code,words);
    };
    UnlitUv result;
    if(material.forward_vertex==3 && pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\stencil_texture" &&
       matches(unlit_uv_detail::texture_words))result.family=UnlitUvFamily::Texture;
    else if(material.forward_vertex==6 &&
        ((pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_perpixel_tex_detail" &&
          matches(unlit_uv_detail::perpixel_detail_words)) ||
         (pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_pervertex_tex_detail" &&
          matches(unlit_uv_detail::pervertex_detail_words))))result.family=UnlitUvFamily::LightmapDetail;
    else return {};
    result.base=material.forward_transforms[0];result.stages[0]={1,1,0,0};
    result.stages[1]=material.forward_transforms[1];
    const auto& c20=material.forward_transforms[2];
    if(result.family==UnlitUvFamily::Texture) {
        result.stages[2]={c20[0],c20[1],0,0};result.stages[3]={c20[2],c20[3],0,0};
    }else {
        result.stages[2]=c20;result.stages[3]={0,0,0,0};
    }
    for(float c:result.base)if(!std::isfinite(c))return {};
    for(const auto& stage:result.stages)for(float c:stage)if(!std::isfinite(c))return {};
    return result;
}
inline std::array<Vec2,4> unlit_uv(const UnlitUv& model,Vec2 raw) {
    const Vec2 base{raw.x*model.base[0]+model.base[2],raw.y*model.base[1]+model.base[3]};
    std::array<Vec2,4> result{};
    result[0]=base;
    for(unsigned stage=1;stage<4;++stage) {
        const auto& c=model.stages[stage];
        result[stage]={base.x*c[0]+c[2],base.y*c[1]+c[3]};
    }
    return result;
}
// Source model.cpp fills Vertex.color only from the model radiance stream
// (or white when absent). These exact native bodies either ignore that
// stream, or consume it as the per-vertex lightmap factor that the unlit
// surface expression removes. Keep arbitrary/procedural colored materials
// and unqualified declarations outside this rule. Uniform attribute10 and
// authored change-color constants are separate and are never replaced here.
inline bool qualified_unlit_white_tint(const Material& material) {
    if(!qualified_unlit_uv(material))return false;
    const bool per_vertex=unlit_uv_detail::exact_vertex(material.vertex_program,
        unlit_uv_detail::pervertex_detail_words);
    if(material.vertex_declaration)
        for(std::size_t reg=0;reg<material.vertex_declaration->semantics.size();++reg)
            if(material.vertex_declaration->semantics[reg]==14 && !(per_vertex && reg==6))return false;
    return true;
}
} // namespace halo2::path_tracing
