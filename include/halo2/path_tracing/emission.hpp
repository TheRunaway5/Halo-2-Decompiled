#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace halo2::path_tracing {
enum class EmissionFamily { None=0, Default=1, Multichannel=2, Detail=3 };

namespace emission_detail {
// Literal stencil_emissive world program from the authenticated Xbox TU1.5
// pass bank, also retained by the prepared MCC native shader closure. Exact
// comparison includes its header and all unused instruction bits.
inline constexpr std::array<std::uint32_t,49> vertex_words{
    0xc2078u,0u,0xc0001bu,0x836186cu,0x20708800u,
    0u,0xc0201bu,0x836186cu,0x20704800u,
    0u,0xc0401bu,0x836186cu,0x20702800u,
    0u,0xc0601bu,0x836186cu,0x20701800u,
    0u,0x95821bu,0x82a1afcu,0x3c900ff8u,
    0u,0x641401bu,0xc4361bffu,0x1011e800u,
    0u,0x82401bu,0x942a1afcu,0x3c900ff8u,
    0u,0x81601bu,0xc5fe2868u,0x3070e800u,
    0u,0x82601bu,0x942a1afcu,0x3070c848u,
    0u,0x82801bu,0x942a1afcu,0x3070c850u,
    0u,0x82a01bu,0x942a1afcu,0x3070c858u,
    0u,0x82c01bu,0x942a1afcu,0x3070c861u};

inline bool exact_vertex(std::span<const std::byte> bytes) {
    if(bytes.size()!=vertex_words.size()*4)return false;
    for(std::size_t word=0;word<vertex_words.size();++word)
        for(unsigned byte=0;byte<4;++byte)
            if(std::to_integer<unsigned>(bytes[word*4+byte])!=
               ((vertex_words[word]>>(byte*8))&255))return false;
    return true;
}

// Fixed control identities for the three source RGB expressions below.
// Only D3DPIXELSHADERDEF's packed constant words may vary with authored
// color/animation. Every other word, including unused stages, must match.
inline constexpr auto default_control=[] {
    std::array<std::uint32_t,60> w{};
    w[8]=0xcu;w[9]=0xd80u;w[34]=0x08010000u;w[35]=0x0c0c0000u;
    w[45]=0xc0u;w[46]=0x20d0u;w[53]=0x11102u;w[54]=1;
    return w;
}();
inline constexpr auto multichannel_control=[] {
    std::array<std::uint32_t,60> w{};
    w[0]=0x08200000u;w[8]=0x1c0f000cu;w[9]=0x01112080u;w[26]=0xc0u;
    w[34]=0x01080208u;w[35]=0x0c010d02u;w[45]=0x30cdu;w[46]=0xc00u;
    w[53]=0x11102u;w[54]=1;
    return w;
}();
inline constexpr auto detail_control=[] {
    std::array<std::uint32_t,60> w{};
    w[8]=0xcu;w[9]=0xd80u;w[34]=0x08200920u;w[35]=0x0c0a0000u;
    w[36]=0x0c010000u;w[37]=0x0c0c0000u;
    w[45]=0xc00u;w[46]=0xc0u;w[47]=0xc0u;w[48]=0x20d0u;
    w[53]=0x11104u;w[54]=0x421u;
    return w;
}();
inline bool exact_control(const PixelProgram& program,
                          const std::array<std::uint32_t,60>& expected) {
    for(unsigned n=0;n<60;++n)
        if(!(n>=10 && n<=25) && n!=43 && n!=44 &&
           program.words[n]!=expected[n])return false;
    return true;
}
inline ShaderVector color(std::uint32_t argb) {
    return {float((argb>>16)&255)/255,float((argb>>8)&255)/255,
            float(argb&255)/255,float(argb>>24)/255};
}
inline float saturate(float value){return std::clamp(value,0.f,1.f);}
inline float positive(float value){return std::max(0.f,value);}
inline float dot_rgb(const ShaderVector& a,const ShaderVector& b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
}

inline EmissionFamily emission_family(const Material& material) {
    if(!material.forward_pass || material.forward_vertex!=2 ||
       (material.forward_blend!=0 && material.forward_blend!=3) ||
       material.forward_pass_index>=material.passes.size())return EmissionFamily::None;
    const auto& pass=material.passes[material.forward_pass_index];
    if(pass.vertex_shader!="rasterizer\\vertex_shaders_xbox\\stencil_emissive" ||
       !emission_detail::exact_vertex(pass.vertex_code) ||
       !emission_detail::exact_vertex(material.vertex_program))return EmissionFamily::None;
    EmissionFamily family=EmissionFamily::None;
    if(pass.name=="shaders\\shader_passes\\self_illumination\\default_self_illumination")family=EmissionFamily::Default;
    else if(pass.name=="shaders\\shader_passes\\self_illumination\\multichannel_self_illumination" ||
            pass.name=="shaders\\shader_passes\\self_illumination\\multichannel_self_illumination_zwrite")family=EmissionFamily::Multichannel;
    else if(pass.name=="shaders\\shader_passes\\self_illumination\\self_illumination_single_pass_detail_zwrite")family=EmissionFamily::Detail;
    const auto* control=family==EmissionFamily::Default?&emission_detail::default_control:
        family==EmissionFamily::Multichannel?&emission_detail::multichannel_control:
        family==EmissionFamily::Detail?&emission_detail::detail_control:nullptr;
    if(!control || !emission_detail::exact_control(material.forward_program,*control))return EmissionFamily::None;
    const Texture* textures[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
    for(unsigned stage=0;stage<4;++stage) {
        if(((material.forward_program.words[54]>>(stage*5))&31)==0)continue;
        if(material.texture_states[stage].color_sign)return EmissionFamily::None;
        const auto& frames=material.animated_textures[stage];
        if(!frames.empty()) {
            if(material.texture_frames[stage]>=frames.size())return EmissionFamily::None;
            textures[stage]=&frames[material.texture_frames[stage]];
        }
        if(textures[stage]->layers!=1 || textures[stage]->depth)return EmissionFamily::None;
    }
    return family;
}

inline bool supports_emission(const Material& material) {
    return emission_family(material)!=EmissionFamily::None;
}

// Original stencil_emissive oT0..3: c18 transforms base UV once, then
// c19..22 apply an independent scale/offset to each actual texture stage.
inline std::array<Vec2,4> emission_uv(const Material& material,Vec2 uv) {
    const auto& base=material.forward_transforms[0];
    const Vec2 transformed{uv.x*base[0]+base[2],uv.y*base[1]+base[3]};
    std::array<Vec2,4> result{};
    for(unsigned stage=0;stage<4;++stage) {
        const auto& c=material.forward_transforms[stage+1];
        result[stage]={transformed.x*c[0]+c[2],transformed.y*c[1]+c[3]};
    }
    return result;
}

// Native expressions for the authenticated fixed source programs. Samples
// are the four independently sampled authored stages, after source texture
// addressing/sign conversion. There is no instruction interpreter here.
// RGB is emission radiance; alpha retains the source program's output.
inline ShaderVector emission_rgb(const Material& material,
                                const std::array<ShaderVector,4>& samples) {
    using namespace emission_detail;
    const auto family=emission_family(material);
    if(family==EmissionFamily::None)throw FormatError("Unsupported native path tracing emissive program");
    const auto& w=material.forward_program.words;
    ShaderVector result{};
    if(family==EmissionFamily::Default) {
        const auto tint=color(w[10]);
        for(unsigned c=0;c<3;++c)result[c]=saturate(positive(samples[0][c])*tint[c]);
    } else if(family==EmissionFamily::Multichannel) {
        ShaderVector texture{};
        for(unsigned c=0;c<3;++c)texture[c]=positive(samples[0][c]);
        const float first=saturate(dot_rgb(color(w[10]),texture));
        const float second=saturate(dot_rgb(color(w[18]),texture));
        const float blue=saturate(samples[0][2]);
        const auto a=color(w[11]),b=color(w[19]),final=color(w[43]);
        for(unsigned c=0;c<3;++c)
            result[c]=saturate(saturate(first*a[c]+second*b[c])+blue*(final[c]*final[3]));
        result[3]=1;
        return result;
    } else {
        const auto tint=color(w[12]);
        for(unsigned c=0;c<3;++c) {
            const float sum=saturate(positive(samples[0][c])+positive(samples[1][c]));
            const float detail=saturate(sum*positive(samples[2][c]));
            result[c]=saturate(detail*tint[c]);
        }
    }
    result[3]=saturate(dot_rgb(result,result));
    return result;
}
}
