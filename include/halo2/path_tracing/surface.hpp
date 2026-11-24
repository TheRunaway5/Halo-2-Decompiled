#pragma once
#include "halo2/path_tracing/unlit_uv.hpp"
#include <algorithm>

namespace halo2::path_tracing {
enum class SurfaceFamily : std::uint32_t {None=0,TerrainBlend=1,LeafDetail=2,LeafDetailPerVertex=3,
    BumpLeafCoverage=4,BumpLeafCoveragePerVertex=5,OpaqueDetail=6,TerrainBlendThreeStage=7,
    TextureDetail=8,TextureTwoChangeColor=9,TextureFourChangeColor=10,DynamicDetail=11};
inline constexpr std::uint32_t surface_flag=64u,surface_magic=0x53525448u,surface_record_words=52u;
// Primary-surface detail flags (GpuMaterial::texture[3]); see world.frag.
inline constexpr std::uint32_t surface_detail_stage1_flag=8192u,surface_bump_stage2_flag=16384u,
    surface_bump_stage1_flag=32768u,surface_forward_combiner_flag=65536u;
// Light-only emitter (glow_material): emits, never occludes shadow rays.
inline constexpr std::uint32_t light_only_flag=131072u;
// The surface emits GpuMaterial::emission evenly over its face: the average
// colour of its self-illumination layers, or the power it gave the bake.
inline constexpr std::uint32_t uniform_emission_flag=262144u;
// That emission is the light the surface gave the map's lightmap bake, so
// lightmapped receivers already hold it.
inline constexpr std::uint32_t baked_emission_flag=524288u;
// The authored environment reflection is masked by the base map's alpha.
inline constexpr std::uint32_t environment_mask_flag=1048576u;
struct SurfaceSource {
    SurfaceFamily family=SurfaceFamily::None;
    UnlitUv uv;
    std::uint32_t stages{};
};
namespace surface_detail {
inline constexpr std::array<std::uint32_t,60> terrain{
    0x3819181a,0,0,0,0,0,0,0,0x0c010002,0x00001c80,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00000c00,0,0,0,0,0,0,0,
    0x3809180a,0x0c080000,0x0c0b0000,0,0,0,0,0,0,0x00ffffff,0,
    0x00000c00,0x000100c0,0x000100c0,0,0,0,0,0,0x00011103,0x00008421,0,0,0,0,0};
inline constexpr std::array<std::uint32_t,60> leaf{
    0x45200000,0x15ccf5d9,0x18200000,0x12201120,0,0,0,0,0x0000000c,0x00001880,
    0x008080ff,0,0,0,0,0,0,0,0,0,0,0xff000000,0,0,0,0,
    0x00000090,0x00000500,0x000000c0,0x00004800,0,0,0,0,
    0x41450000,0x45440809,0x0b200bd5,0x0c080000,0,0,0,0,0,0,0,
    0x000020c0,0x000120d8,0x00000c00,0x000100c0,0,0,0,0,0x00011104,0x00009021,0,0,0,0,0};
inline constexpr std::array<std::uint32_t,60> leaf_per_vertex{
    0x4b200000,0x15ccf5dc,0x18200000,0x11201220,0,0,0,0,0x0000000c,0x00001880,
    0x008080ff,0,0,0xff000000,0,0,0,0,0,0,0,0,0,0,0,0,
    0x000000c0,0x00000500,0x000000c0,0x00004800,0,0,0,0,
    0x414b4144,0x4b440809,0x052005d5,0x0c080000,0,0,0,0,0,0,0,
    0x000030ca,0x000120d8,0x00000c00,0x000100c0,0,0,0,0,0x00011104,0x00019021,0,0,0,0,0};
inline constexpr std::array<std::uint32_t,60> bump_leaf{
    0x45205518,0x15ccf5d9,0xccca80cd,0x1a20111b,0x1d1d111c,0x15150000,0xc0200000,0,0x0c200005,0x00000d80,
    0,0,0,0xfe000000,0xff000000,0x00ffffff,0,0,0,0,0,0,0xff000000,0,0,0,
    0x0000009b,0x00000500,0x00010d00,0x000000cb,0x00000059,0x000000d0,0x000000d0,0,
    0x4a454a44,0x45440000,0x08090000,0x0b200bd5,0x12201b20,0x0c08010b,0xc0200000,0x1d050000,0,0,0,
    0x000030ca,0x000020d0,0x00010080,0x00000c00,0x00004d00,0x000100cb,0x00000050,0x00010050,
    0x00011108,0x00008421,0,0,0,0,0};
inline constexpr std::array<std::uint32_t,60> bump_leaf_per_vertex{
    0x4b205518,0x15ccf5dc,0xccca80cd,0x1a200000,0x1d1d1119,0x15151911,0xc0200000,0x11201920,0x0520000c,0x00001a80,
    0,0,0,0,0xff000000,0xfeffffff,0,0xff000000,0,0,0,0,0xc0000000,0,0,0,
    0x000000c9,0x00000500,0x00010d00,0x000000c0,0x00000059,0x000000d9,0x000000d0,0x00004a00,
    0x4a4b4a44,0x4b440000,0x08090000,0x052005d5,0x0c120000,0x0c080105,0xc0200000,0x1d050000,0,0,0,
    0x000030ca,0x000020d0,0x00010080,0x00000c00,0x000020d0,0x000100c5,0x00000050,0x00010050,
    0x00011108,0x00018421,0,0,0,0,0};
// Authenticated opaque lightmap_tex_detail body. RGB constant15 belongs to
// its separate baked specular branch, removed from the unlit diffuse color.
// The full-program source oracle proves every color word10..25/43/44 is
// independent of this unlit diffuse subgraph once baked specular is removed.
// Keep every control word pinned; color constants remain authored data.
inline constexpr std::array<std::uint32_t,60> opaque_detail{
    0x45205518,0x15ccf5dc,0xccca80cd,0x19180000,0x1d1d1119,0x15150000,0x1d1d0000,0x0d180000,
    0x0c200005,0x00001480,0,0,0,0,0x33000000,0x007d7db0,
    0,0,0,0,0,0,0xc0000000,0,0,0,
    0x000000c9,0x00000500,0x00010d00,0x00000090,0x00000059,0x000000d0,0x000000d0,0x00010040,
    0x4a454a44,0x45440000,0x08090000,0x0b200bd5,0x0c120000,0x0c08010b,0x0b190000,0x1d050000,
    0,0,0,0x000030ca,0x000020d0,0x00010080,0x00000c00,0x000020d0,0x000100cb,0x00010050,
    0x00010050,0x00011108,0x00008421,0,0,0,0,0};
// frozen_panels keeps the identical diffuse RGB subgraph but uses another
// opaque output-alpha calculation. Admit that complete identity explicitly;
// neither alpha calculation is eligible for raster testing or blending.
inline constexpr auto opaque_detail_panel=[] {
    auto words=opaque_detail;words[6]=0x1d1d8020;words[32]=0x00010d00;return words;
}();
// Exact lightmap_dynamic_tex_detail body used by the capital cruiser. Its
// separate lighting/specular RGB branches leave2*T0*T1 as diffuse albedo.
// Final alpha is2*C24.a*T0.a*T1.a, independent of those lighting branches;
// the following DST_ALPHA environment pass consumes that source mask.
inline constexpr std::array<std::uint32_t,60> dynamic_detail{
    0x4a201120,0x0c201819,0xcacc80cd,0x1d1d6a20,0x14145920,0x1d1d0000,0x1d181218,0x59200000,
    0x0539000c,0x00001480,0xc0000000,0,0,0x0075f5ff,0,0xc0000000,0xc0000000,0,
    0,0,0,0x00fcd1fc,0,0,0,0,0x000000ab,0x000000c8,0x00010d00,0x00000049,
    0x000000d9,0x000000d0,0x000100d4,0x00000090,0x4a444b44,0x4a4b4a45,
    0x0a010b02,0x1c013c02,0x05010809,0x1a010000,0x0c080a08,0x051d0000,0,0,0,
    0x000030cd,0x000030ab,0x00000c00,0x00000500,0x00010058,0x000000a0,0x00010c00,
    0x00010050,0x00011108,0x00018421,0,0,0,0,0};
// The three-stage detail_blend_texture source blends T1/T2 with T0.a, then
// multiplies T0.rgb. This body has no fourth detail/color stage.
inline constexpr std::array<std::uint32_t,60> terrain_three_stage{
    0x3819181a,0,0,0,0,0,0,0,0x0c010002,0x00001c80,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00000c00,0,0,0,0,0,0,0,
    0x3809180a,0x0c080000,0,0,0,0,0,0,0,0x00ffffff,0,
    0x00000c00,0x000100c0,0,0,0,0,0,0,0x00011102,0x00000421,0,0,0,0,0};
inline bool controls(const PixelProgram& program,const std::array<std::uint32_t,60>& expected) {
    for(unsigned n=0;n<60;++n) {
        if((n>=10 && n<=25) || n==43 || n==44)continue;
        if(program.words[n]!=expected[n])return false;
    }
    return true;
}
inline std::array<std::uint32_t,60> texture_controls(unsigned stages) {
    std::array<std::uint32_t,60> w{};
    w[8]=stages==1?0x0c010000u:0x0c0f0000u;w[9]=stages==1?0x00001c80u:0x04051c80u;
    w[53]=0x11100u|stages;w[54]=stages==1?0x21u:0x8021u;
    constexpr std::array<std::uint32_t,8> rgb1{0x08090000},out1{0x100c0};
    constexpr std::array<std::uint32_t,8> rgb4{0x010b020b,0x2c200c01,0x2d200d01,0x08090000},out4{0x30cd,0x400,0x500,0x100c0};
    constexpr std::array<std::uint32_t,8> rgb7{0x010b020b,0x2c200c01,0x2d200d01,0x3c201c01,0x3b201b01,0x04050c0d,0x08090000},out7{0x30cd,0x400,0x500,0xc00,0xd00,0x45,0x100c0};
    for(unsigned i=0;i<8;++i) {
        w[34+i]=(stages==1?rgb1:stages==4?rgb4:rgb7)[i];
        w[45+i]=(stages==1?out1:stages==4?out4:out7)[i];
        w[i]=i+1==stages?0x18190000u:stages==7 && i==0?0x0b200000u:0u;
        w[26+i]=(i+1==stages || (stages==7 && i==0))?0xc0u:0u;
    }
    return w;
}
inline ShaderVector argb(std::uint32_t c) {
    return {float((c>>16)&255u)/255.f,float((c>>8)&255u)/255.f,float(c&255u)/255.f,float(c>>24)/255.f};
}
}
inline bool qualified_opaque_detail(const Material& material) {
    if(!material.forward_pass || material.forward_blend || material.forward_raster.alpha_test ||
       material.per_vertex_lighting || material.forward_pass_index>=material.passes.size())return false;
    const auto uv=qualified_unlit_uv(material);
    if(!uv || uv->family!=UnlitUvFamily::LightmapDetail)return false;
    const auto& pass=material.passes[material.forward_pass_index];
    if(pass.name!="shaders\\shader_passes\\lightmap\\lightmap_tex_detail" ||
       pass.vertex_shader!="rasterizer\\vertex_shaders_xbox\\lightmap_perpixel_tex_detail")return false;
    return surface_detail::controls(material.forward_program,surface_detail::opaque_detail) ||
        surface_detail::controls(material.forward_program,surface_detail::opaque_detail_panel);
}
inline std::array<std::uint32_t,4> surface_constants(const std::array<std::uint32_t,60>& w,SurfaceFamily family) {
    // Family6 uses no color constants in diffuse RGB or representative alpha.
    // Keep its resident record and image key invariant under color animation.
    if(family==SurfaceFamily::OpaqueDetail)return {};
    if(family==SurfaceFamily::DynamicDetail)return {w[24],0,0,0};
    if(family==SurfaceFamily::TextureDetail)return {w[43],0,0,0};
    if(family==SurfaceFamily::TextureTwoChangeColor)return {w[11],w[12],0,0};
    if(family==SurfaceFamily::TextureFourChangeColor)return {w[11],w[12],w[13],w[14]};
    if(family==SurfaceFamily::BumpLeafCoverage)return {w[13],w[22],w[43],w[44]};
    if(family==SurfaceFamily::BumpLeafCoveragePerVertex)return {w[15],w[17],w[43],w[44]};
    return {w[13],w[21],w[43],w[44]};
}
inline std::array<std::uint32_t,4> surface_constants(const Material& material,SurfaceFamily family) {
    return surface_constants(material.forward_program.words,family);
}
inline std::optional<SurfaceSource> qualified_surface(const Material& material) {
    if(!material.forward_pass || material.forward_blend || material.forward_pass_index>=material.passes.size())return {};
    const auto uv=qualified_unlit_uv(material);if(!uv)return {};
    const auto& pass=material.passes[material.forward_pass_index];SurfaceSource out;out.uv=*uv;
    const auto texture_family=[&](std::string_view name,unsigned stages) {
        constexpr std::string_view prefix="shaders\\shader_passes\\texture_mapping\\";
        std::string_view path=pass.name;
        if(!path.starts_with(prefix))return false;
        path.remove_prefix(prefix.size());
        if(path.ends_with("_zwrite"))path.remove_suffix(7);
        return uv->family==UnlitUvFamily::Texture &&
            path==name &&
            surface_detail::controls(material.forward_program,surface_detail::texture_controls(stages)) &&
            (stages==1 || ((material.forward_program.words[10]&0xffffffu)==0xff0000u &&
                          (material.forward_program.words[18]&0xffffffu)==0x00ff00u));
    };
    if(texture_family("default_texture",1)) {
        out.family=SurfaceFamily::TextureDetail;out.stages=3;
    } else if(texture_family("default_texture_two_change_color",4)) {
        out.family=SurfaceFamily::TextureTwoChangeColor;out.stages=11;
    } else if(texture_family("default_texture_four_change_color",7)) {
        out.family=SurfaceFamily::TextureFourChangeColor;out.stages=11;
    } else if(qualified_opaque_detail(material)) {
        out.family=SurfaceFamily::OpaqueDetail;out.stages=3;
    } else if(uv->family==UnlitUvFamily::DynamicDetail &&
        pass.name=="shaders\\shader_passes\\lightmap\\lightmap_tex_detail" &&
        surface_detail::controls(material.forward_program,surface_detail::dynamic_detail)) {
        out.family=SurfaceFamily::DynamicDetail;out.stages=3;
    } else if(uv->family==UnlitUvFamily::Texture &&
       (pass.name=="shaders\\shader_passes\\texture_mapping\\detail_blend_texture_zwrite" ||
        pass.name=="shaders\\shader_passes\\texture_mapping\\detail_blend_texture") &&
       surface_detail::controls(material.forward_program,surface_detail::terrain_three_stage)) {
        out.family=SurfaceFamily::TerrainBlendThreeStage;out.stages=7;
    } else if(uv->family==UnlitUvFamily::Texture &&
       (pass.name=="shaders\\shader_passes\\texture_mapping\\detail_blend_detail_texture_zwrite" ||
        pass.name=="shaders\\shader_passes\\texture_mapping\\detail_blend_detail_texture") &&
       surface_detail::controls(material.forward_program,surface_detail::terrain)) {
        out.family=SurfaceFamily::TerrainBlend;out.stages=15;
    } else if(uv->family==UnlitUvFamily::LightmapDetail &&
        pass.name=="shaders\\shader_passes\\lightmap\\lightmap_tex_detail_alpha_test_no_bump" &&
        surface_detail::controls(material.forward_program,surface_detail::leaf)) {
        out.family=SurfaceFamily::LeafDetail;out.stages=3;
    } else if(uv->family==UnlitUvFamily::LightmapDetail &&
        pass.name=="shaders\\shader_passes\\lightmap\\lightmap_tex_detail_alpha_test_no_bump" &&
        surface_detail::controls(material.forward_program,surface_detail::leaf_per_vertex)) {
        out.family=SurfaceFamily::LeafDetailPerVertex;out.stages=3;
    } else if(uv->family==UnlitUvFamily::LightmapDetail &&
        pass.name=="shaders\\shader_passes\\lightmap\\lightmap_tex_detail_alpha_test" &&
        material.forward_raster.alpha_test && material.forward_raster.alpha_function==0x201 &&
        material.forward_raster.alpha_reference==255) {
        // world_vertex.glsl clamps native oD1 to [0,1] before interpolation.
        // Under these exact constants, all source opaque results round below
        //255 and every hole is255. Retain equivalent LESS255 coverage while
        // removing the baked-light factor from RGB; this is not full RGBA
        // equivalence and is never admitted for another alpha/blend policy.
        const auto& w=material.forward_program.words;
        if(pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_perpixel_tex_detail" &&
           surface_detail::controls(material.forward_program,surface_detail::bump_leaf) &&
           (w[13]>>24)<255 && (w[22]>>24)==255)out.family=SurfaceFamily::BumpLeafCoverage;
        else if(pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\lightmap_pervertex_tex_detail" &&
            surface_detail::controls(material.forward_program,surface_detail::bump_leaf_per_vertex) &&
            (w[14]>>24)==255 && (w[15]>>24)<255 && (w[17]>>24)==255)
            out.family=SurfaceFamily::BumpLeafCoveragePerVertex;
        else return {};
        out.stages=7;
    } else return {};
    const Texture* images[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
    for(unsigned stage=0;stage<4;++stage) {
        const auto& sampler=material.texture_states[stage];
        if(!(out.stages&(1u<<stage))) {
            if(sampler.alpha_kill&4u)return {}; // Unqualified lightmap/procedural alpha owner.
            continue;
        }
        const auto& frames=material.animated_textures[stage];
        if(!frames.empty() && material.texture_frames[stage]>=frames.size())return {};
        const auto& image=frames.empty()?*images[stage]:frames[material.texture_frames[stage]];
        if(!image.width || !image.height || image.pixels.empty() || image.layers!=1 || image.depth ||
           (image.format!=11 && (image.format<14 || image.format>16)) ||
           sampler.color_sign || sampler.color_key_op || sampler.address_u<1 || sampler.address_u>5 ||
           sampler.address_v<1 || sampler.address_v>5 || sampler.mag_filter<1 || sampler.mag_filter>3 ||
           sampler.min_filter<1 || sampler.min_filter>3 || sampler.anisotropy>4)return {};
    }
    return out;
}
// Native unlit RGB/alpha subgraphs. Lighting inputs are deliberately absent;
// all authored texture blends and output-alpha constants remain independent.
inline ShaderVector surface_color(SurfaceFamily family,const std::array<std::uint32_t,4>& colors,
                                  const std::array<ShaderVector,4>& samples) {
    const auto sat=[](float value){return std::clamp(value,0.f,1.f);};ShaderVector result{};
    if(family==SurfaceFamily::TextureDetail || family==SurfaceFamily::TextureTwoChangeColor ||
       family==SurfaceFamily::TextureFourChangeColor) {
        const auto first=surface_detail::argb(colors[0]),second=surface_detail::argb(colors[1]);
        const auto third=surface_detail::argb(colors[2]),fourth=surface_detail::argb(colors[3]);
        for(unsigned c=0;c<3;++c) {
            const float base=sat(2.f*std::max(samples[0][c],0.f)*std::max(samples[1][c],0.f));
            float tint=first[c];
            if(family!=SurfaceFamily::TextureDetail) {
                const float a=(1.f-sat(samples[3][0]))+sat(samples[3][0])*first[c];
                const float b=(1.f-sat(samples[3][1]))+sat(samples[3][1])*second[c];
                tint=a*b;
                if(family==SurfaceFamily::TextureFourChangeColor) {
                    const float d=(1.f-sat(samples[3][2]))+sat(samples[3][2])*third[c];
                    const float e=(1.f-sat(samples[3][3]))+sat(samples[3][3])*fourth[c];
                    tint*=d*e;
                }
            }
            result[c]=sat(base*tint);
        }
        result[3]=sat(std::max(samples[0][3],0.f)*std::max(samples[1][3],0.f));
    } else if(family==SurfaceFamily::TerrainBlend || family==SurfaceFamily::TerrainBlendThreeStage) {
        const auto gain=surface_detail::argb(colors[2]),bias=surface_detail::argb(colors[3]);
        const float mask=sat(samples[0][3]);
        for(unsigned c=0;c<3;++c) {
            const float blend=(1.f-mask)*std::max(samples[1][c],0.f)+mask*std::max(samples[2][c],0.f);
            const float base=sat(2.f*std::max(samples[0][c],0.f)*blend);
            const float detail=family==SurfaceFamily::TerrainBlend? sat(2.f*base*std::max(samples[3][c],0.f)):base;
            result[c]=sat(detail*gain[c]+bias[c]);
        }
        result[3]=sat((1.f-mask)*std::max(samples[1][3],0.f)+mask*std::max(samples[2][3],0.f));
    } else if(family==SurfaceFamily::LeafDetail || family==SurfaceFamily::LeafDetailPerVertex) {
        for(unsigned c=0;c<3;++c)result[c]=sat(2.f*std::max(samples[0][c],0.f)*std::max(samples[1][c],0.f));
        const unsigned opaque=family==SurfaceFamily::LeafDetail?0:1;
        result[3]=surface_detail::argb(colors[samples[0][3]>=.5f?opaque:1-opaque])[3];
    } else if(family==SurfaceFamily::BumpLeafCoverage || family==SurfaceFamily::BumpLeafCoveragePerVertex) {
        for(unsigned c=0;c<3;++c)result[c]=sat(2.f*std::max(samples[0][c],0.f)*std::max(samples[1][c],0.f));
        result[3]=surface_detail::argb(colors[samples[2][3]>=.5f?0:1])[3];
    } else if(family==SurfaceFamily::DynamicDetail) {
        for(unsigned c=0;c<3;++c)result[c]=sat(2.f*std::max(samples[0][c],0.f)*std::max(samples[1][c],0.f));
        result[3]=sat(2.f*surface_detail::argb(colors[0])[3]*std::max(samples[0][3],0.f)*std::max(samples[1][3],0.f));
    } else if(family==SurfaceFamily::OpaqueDetail) {
        for(unsigned c=0;c<3;++c)result[c]=sat(2.f*std::max(samples[0][c],0.f)*std::max(samples[1][c],0.f));
        // This family has no raster alpha test or blending. Retain the
        // existing T0 alpha representative; native baked-light/specular alpha
        // is not claimed as unlit RGBA. Raw used-stage kills remain separate.
        result[3]=sat(samples[0][3]);
    } else throw FormatError("Path Tracing unlit surface has an unknown native family");
    return result;
}
}
