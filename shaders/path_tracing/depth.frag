#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
// Match the shared vertex stage interface, including fields unused by this
// depth/coverage role, so pipeline validation sees the complete producer ABI.
layout(location=0) in vec3 traceWorld;
layout(location=1) in vec2 traceTexcoord;
layout(location=2) in vec3 traceWorldNormal;
layout(location=3) flat in uint traceMaterialIndex;
layout(location=4) in vec2 titleDepth;
layout(location=5) flat in uint stippleLevel;
layout(location=6) in vec3 traceTint;
layout(location=8) flat in uint sourceDepthRaw;
layout(location=9) in vec2 traceLightmapUV;
layout(location=10) flat in uint traceLightmapIndex;
layout(location=11) in vec3 traceVertexRadiance;
// Exact existing material ABI. The cheap visibility phase needs no BVH,
// emission evaluator, or ray transport calls.
struct TraceMaterial {vec4 base,emission,response;uvec4 texture;vec4 transform;uvec4 colorGuide;};
layout(set=1,binding=2,std430) readonly buffer TraceMaterials {TraceMaterial traceMaterials[];};
layout(set=1,binding=4,std430) readonly buffer TracePixels {uint tracePixels[];};
#include "source_alpha_test.glsl"
#include "source_stipple.glsl"
#include "world_depth_fragment.glsl"
#include "path_tracing/surface.glsl"
void main() {
    if(stippleLevel<16u && !sourceStipplePass(stippleLevel,31u,uvec2(gl_FragCoord.xy)))discard;
    TraceMaterial material=traceMaterials[traceMaterialIndex];
    float sourceAlpha;if(!tracePrimaryCoverage(material,traceTexcoord,sourceAlpha))discard;
    if((material.texture.w&64u)==0u && (material.texture.w&8u)!=0u && sourceAlpha==0.0)discard;
    if((material.texture.w&1u)!=0u && !sourceAlphaTestPass(sourceAlpha,
        0x200u+((material.texture.w>>8u)&7u),uint(round(material.response.w*255.0))))discard;
    // D32 stores this exact source-W result. The ray phase compares the same
    // expression before tracing, then still writes it to real source D24S8.
    gl_FragDepth=titleDepth.y!=0.0?worldD24Depth(clamp(sourceDepthRaw!=0u?
        titleDepth.x:titleDepth.x*16777215.0,0.0,16777215.0)):gl_FragCoord.z;
}
