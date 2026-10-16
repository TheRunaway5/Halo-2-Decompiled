#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/adaptation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2::pstencil {
namespace {
struct Reference {std::string_view shader;EnvironmentParameters parameters;};
constexpr Reference references[]{
#include "reference_environment.inc"
};
}
const EnvironmentParameters* reference_environment_parameters(std::string_view shader) {
    const auto found=std::lower_bound(std::begin(references),std::end(references),shader,
        [](const auto& entry,std::string_view key){return entry.shader<key;});
    return found!=std::end(references) && found->shader==shader?&found->parameters:nullptr;
}
void apply_reference_environment(const EnvironmentParameters& reference,PixelProgram& program,
    std::array<ShaderVector,16>& attributes) {
    attributes[10]=reference.facing;attributes[11]=reference.glancing;
    program.words[13]=reference.midrange_factor_argb&0xff000000u;
    program.words[14]=reference.midrange_color_argb&0xffffffu;
}
bool supports_environment_response(const Material& material) {
    if(!material.forward_pass || material.forward_vertex!=4 || material.forward_blend!=8 ||
       material.forward_pass_index>=material.passes.size())return false;
    constexpr std::array<std::uint32_t,60> supported{{
        0x00000000u,0x0c200000u,0x1c113c11u,0x3c151c14u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x0b0f0000u,0x0c1c0080u,0x00000000u,0x00000000u,
        0x00000000u,0x00787387u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x000000c0u,0x00014d00u,0x00000c00u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x6a410000u,0x00000000u,
        0x3c051c04u,0x3d0c1d01u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x000020c0u,0x00000000u,0x00000c00u,
        0x00000c00u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00011104u,
        0x00062e21u,0x00000111u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    }};
    for(unsigned n=0;n<supported.size();++n) {
        if((n>=10 && n<=25) || n==43 || n==44)continue;
        if(material.forward_program.words[n]!=supported[n])return false;
    }
    const auto& pass=material.passes[material.forward_pass_index];
    return pass.implementation==0 &&
        pass.name.ends_with("\\default_environment_map_tangent_space") &&
        pass.vertex_shader.ends_with("\\stencil_env_bumped_tangentspace") &&
        material.forward_program.words[54]==0x62e21u && material.forward_program.words[55]==0x111u;
}
bool supports_combined_environment_strength(const Material& material) {
    if(!material.forward_pass || material.forward_vertex!=6 || material.forward_blend!=0 ||
       material.forward_pass_index>=material.passes.size())return false;
    const auto& pass=material.passes[material.forward_pass_index];
    if(pass.implementation!=2 || !pass.name.ends_with("\\lightmap_env") ||
       !pass.vertex_shader.ends_with("\\lightmap_dynamic_env"))return false;
    constexpr std::array<std::uint32_t,60> supported{{
        0x11200000u,0x0c200000u,0xcacc80cdu,0x1d1d0000u,0x14140000u,0x1d1d0000u,
        0x1d180d18u,0x1c113c12u,0x0f1c000eu,0x04140080u,0xc0000000u,0x00000000u,
        0x00000000u,0x00ffe8bau,0x00000000u,0xc0ffba59u,0xc0000000u,0xe5000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00ffd68au,0x00000000u,0x00dbdba6u,
        0x00000000u,0xff000000u,0x000000b0u,0x000000c0u,0x00010d00u,0x00000040u,
        0x000000d0u,0x000000d0u,0x000100d4u,0x00000c00u,0x4a444b44u,0x4a4b4a45u,
        0x0a010b02u,0x1c013c02u,0x12200501u,0x1c013c02u,0x0c080108u,0x0409051du,
        0x00000000u,0x00000000u,0x00000000u,0x000030cdu,0x000030abu,0x00000c00u,
        0x00000500u,0x000000d5u,0x00030400u,0x00010c00u,0x00010045u,0x00011108u,
        0x00018461u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    }};
    for(unsigned n=0;n<supported.size();++n) {
        if((n>=10 && n<=25) || n==43 || n==44)continue;
        if(material.forward_program.words[n]!=supported[n])return false;
    }
    return true;
}
void adapt_combined_environment_strength(PixelProgram& program,const EnvironmentParameters* reference,bool polished) {
    const auto pack=[](float value) {
        return std::uint32_t(std::nearbyint(double(std::clamp(value,0.f,1.f))*255.))<<24;
    };
    if(reference) {
        program.words[17]=(program.words[17]&0xffffffu)|pack(reference->facing[3]);
        program.words[25]=(program.words[25]&0xffffffu)|pack(reference->glancing[3]);
    }
    if(polished)for(const auto slot:{17u,25u}) {
        const auto value=float(program.words[slot]>>24)/255.f;
        program.words[slot]=(program.words[slot]&0xffffffu)|pack(value*arbiter_environment_gain);
    }
}
PixelProgram adapt_environment_response(const PixelProgram& current) {
    // Actual resident custom-index13 program from the default layered 2003
    // pipeline, ec_interior_test.map default_environment_map_tangent_space.
    // Preserve the current pass's completed authored/view/animated inputs.
    PixelProgram program{{
        0x00000000u,0x2c2c0000u,0x6020fc1cu,0x1c113c11u,0x3c151c14u,0x00000000u,
        0x00000000u,0x00000000u,0x0c0f0000u,0x201c0080u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x000000c0u,0x00000c00u,0x00014d00u,
        0x00000c00u,0x00000000u,0x00000000u,0x00000000u,0x68410b0bu,0x0d0d0000u,
        0x0d0d0000u,0x3c051c04u,0x3d0c1d01u,0x2c0d0c0bu,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x000020cdu,0x000000d0u,0x000000d0u,
        0x00000c00u,0x00000c00u,0x00000c00u,0x00000000u,0x00000000u,0x00011106u,
        0x00064621u,0x00000111u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    },{}};
    program.words[10]=current.words[10]; // Encoded positive world view forward.
    program.words[13]=current.words[12]&0xff000000u; // Midrange factor.
    program.words[14]=current.words[13]&0x00ffffffu; // Midrange RGB tint.
    return program;
}
PixelProgram fallback_environment_response(Vec3 window_direction) {
    // Exact unchanged May2003 001CF17A..001CF309 construction export.
    // analysis/pstencil/2003-presentation/fallback-environment.json owns the
    // instruction hashes and original CPU fixtures. Only word10 is dynamic.
    PixelProgram program{{
        0x00000000u,0x00000000u,0x00000000u,0x2c150c14u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x0c1c0000u,0x00000000u,0x00808000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000c00u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x4a410b0bu,0x0c0c0d0du,
        0x0c0c0d0du,0x2c050c04u,0x2c0d0c0bu,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x000020cdu,0x000000cdu,0x0000000du,
        0x00000c00u,0x00000c00u,0x00000000u,0x00000000u,0x00000000u,0x00011005u,
        0x00062e21u,0x00000111u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    },{}};
    const auto byte=[](float direction) {
        if(!std::isfinite(direction))throw FormatError("pStencil environment direction is nonfinite");
        // Original x87 PC53 expression spills to float before RGB packing.
        const auto value=std::clamp(float(.5-double(direction)*.5),0.f,1.f);
        return std::uint32_t(std::nearbyint(double(value)*255.));
    };
    program.words[10]=(byte(window_direction.x)<<16)|(byte(window_direction.y)<<8)|byte(window_direction.z);
    return program;
}
}
