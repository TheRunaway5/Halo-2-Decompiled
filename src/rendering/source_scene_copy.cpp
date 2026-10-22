#include "halo2/source_scene_copy.hpp"
#include "halo2/cache.hpp"
#include <cfenv>
#include <cmath>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
#include <xmmintrin.h>
#endif
#if defined(__FAST_MATH__)
#error "Original scene-copy geometry requires strict binary32 arithmetic"
#endif

namespace halo2 {
namespace {
float add(float a,float b) {const volatile float result=a+b;return result;}
float sub(float a,float b) {const volatile float result=a-b;return result;}
float mul(float a,float b) {const volatile float result=a*b;return result;}
float div(float a,float b) {const volatile float result=a/b;return result;}
void finite(float value) {
    if(!std::isfinite(value))throw FormatError("Scene copy outside finite arithmetic admission");
}
float depth_clamp(float value) {
    finite(value);
    if(value<0.f)return 0.f;
    if(value>16777215.f)return 16777215.f;
    return value;
}
}
PixelProgram source_scene_copy_program() {
    PixelProgram out;auto &w=out.words;
    w[0]=0x18201920;w[1]=0x1a201b20;w[2]=0x1c201d20;
    w[8]=0xc;w[9]=0x1c00;
    w[26]=0x30c00;w[27]=0x30d00;w[28]=0x30c00;
    w[34]=0x08200920;w[35]=0x0a200b20;w[36]=0x0c200d20;
    w[45]=0x30c00;w[46]=0x30d00;w[47]=0x30c00;
    w[53]=0x11004;w[54]=0x8421;
    return out;
}
SourceSceneCopyFrame source_scene_copy_frame(const SourceSceneCopyContext &context) {
    if(std::fegetround()!=FE_TONEAREST)
        throw FormatError("Scene copy requires nearest-even arithmetic");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
    if((_mm_getcsr()&0xffc0u)!=0x1f80u)
        throw FormatError("Scene copy requires masked gradual binary32 arithmetic");
#endif
    if(context.destination_width<=0 || context.destination_height<=0)
        throw FormatError("Scene-copy destination dimensions must be positive");
    finite(context.near_plane);finite(context.far_plane);
    const std::array<float,2> destination{float(context.destination_width),float(context.destination_height)};
    const std::array<float,2> origin{float(context.camera_rectangle[1]),float(context.camera_rectangle[0])};
    // Original CVTSI2SS, DIVSS1/destination, then MULSSspan*reciprocal.
    // Span/destination is not an interchangeable binary32 expression.
    const std::array<float,2> ratio{
        mul(float(int(context.camera_rectangle[3])-int(context.camera_rectangle[1])),div(1.f,destination[0])),
        mul(float(int(context.camera_rectangle[2])-int(context.camera_rectangle[0])),div(1.f,destination[1]))};
    const float q=div(context.far_plane,sub(context.far_plane,context.near_plane));
    const float z=depth_clamp(mul(div(sub(mul(q,1.f),mul(context.near_plane,q)),1.f),16777215.f));
    const float w=depth_clamp(mul(div(1.f,context.far_plane),16777215.f));
    SourceSceneCopyFrame result{source_scene_copy_program(),{}};
    constexpr std::array<std::array<float,2>,4> corners{{{0,0},{0,1},{1,1},{1,0}}};
    for(unsigned corner=0;corner<4;++corner) {
        auto &vertex=result.vertices[corner];
        vertex.corner={corners[corner][0],corners[corner][1],0,1};
        vertex.position={add(mul(destination[0],corners[corner][0]),0.f),
                         add(mul(destination[1],corners[corner][1]),0.f),z,w};
        for(unsigned tap=0;tap<4;++tap) {
            auto &coordinate=vertex.texture_coordinates[tap];coordinate={0,0,0,1};
            for(unsigned axis=0;axis<2;++axis) {
                const float sign=(tap&(1u<<axis))?.5f:-.5f;
                const float pixel=add(mul(.5f,sign),vertex.position[axis]);
                coordinate[axis]=add(origin[axis],mul(pixel,ratio[axis]));
                finite(coordinate[axis]);
            }
        }
    }
    return result;
}
SourceSceneCopyShaderUniform source_scene_copy_shader_uniform(
    const SourceSceneCopyContext &context,std::array<std::int32_t,2> extent) {
    const auto frame=source_scene_copy_frame(context);
    if(extent[0]<=0 || extent[1]<=0)
        throw FormatError("Scene-copy shader needs its actual positive source coordinate extent");
    const float x=float(context.camera_rectangle[1]),y=float(context.camera_rectangle[0]);
    const float width=float(extent[0]),height=float(extent[1]);
    SourceSceneCopyShaderUniform result{frame.program.words};
    result.source_rectangle={div(x,width),div(y,height),
        div(float(int(context.camera_rectangle[3])-int(context.camera_rectangle[1])),width),
        div(float(int(context.camera_rectangle[2])-int(context.camera_rectangle[0])),height)};
    // Preserve independently rounded original coordinates. Neither source
    // texel spacing nor symmetric offsets can replace the emitted attributes.
    const auto &t=frame.vertices[0].texture_coordinates;
    result.taps={div(sub(t[0][0],x),width),div(sub(t[0][1],y),height),
                 div(sub(t[1][0],x),width),div(sub(t[2][1],y),height)};
    result.destination={float(context.destination_width),float(context.destination_height),
        div(1.f,float(context.destination_width)),div(1.f,float(context.destination_height))};
    return result;
}
} // namespace halo2
