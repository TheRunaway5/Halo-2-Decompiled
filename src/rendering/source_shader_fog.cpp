#include "halo2/source_shader_fog.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_vertex_extern.hpp"
#include <cfenv>
#include <cmath>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
#include <xmmintrin.h>
#endif
#if defined(__FAST_MATH__)
#error "Original shader fog providers require strict binary32 arithmetic"
#endif

namespace halo2 {
namespace {
void finite(float value) {
    if(!std::isfinite(value))throw FormatError("Shader fog provider outside finite admission");
}
float sub(float a,float b) {const volatile float result=a-b;return result;}
float mul(float a,float b) {const volatile float result=a*b;return result;}
float div(float a,float b) {const volatile float result=a/b;return result;}
}
SourceShaderFogExterns source_shader_fog_externs(const SourceShaderFogContext &context) {
    if(std::fegetround()!=FE_TONEAREST)
        throw FormatError("Shader fog provider requires nearest-even arithmetic");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
    if((_mm_getcsr()&0xffc0u)!=0x1f80u)
        throw FormatError("Shader fog provider requires masked gradual binary32 arithmetic");
#endif
    finite(context.near_distance);finite(context.far_distance);
    for(const auto value:context.camera_forward)finite(value);
    for(const auto value:context.default_color)finite(value);
    SourceShaderFogExterns result{0.f,context.default_color};
    if(context.datum!=0xffffffffu && context.far_distance>context.near_distance) {
        const auto reciprocal=div(1.f,sub(context.far_distance,context.near_distance));
        result.scalar39=sub(0.f,mul(reciprocal,context.near_distance));
        for(unsigned axis=0;axis<3;++axis)
            result.color41[axis]=sub(0.f,mul(context.camera_forward[axis],reciprocal));
    }
    finite(result.scalar39);for(const auto value:result.color41)finite(value);
    return result;
}

void bind_camouflage_fog_pass(Material &material,const SourceShaderFogContext &context) {
    if(!material.forward_pass)return;
    if(material.forward_pass_index>=material.passes.size())
        throw FormatError("Camouflage fog pass outside material bank");
    auto &pass=material.passes[material.forward_pass_index];
    std::vector<ShaderRemap> mappings;
    for(const auto &mapping:pass.vertex_externs) {
        const auto component=(mapping.destination>>5)&31;
        if((mapping.source==39 && component<4) || (mapping.source==41 && component==4))
            mappings.push_back(mapping);
    }
    if(mappings.empty())return;
    const auto fog=source_shader_fog_externs(context);
    std::array<float,40> scalars{};scalars[39]=fog.scalar39;
    std::array<ShaderVector,42> colors{};
    for(unsigned axis=0;axis<3;++axis)colors[41][axis]=fog.color41[axis];
    (void)remap_source_vertex_externs(material.program_constants,scalars,colors,mappings);
    (void)remap_source_vertex_externs(pass.constants,scalars,colors,mappings);
}
}
