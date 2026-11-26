#version 460
#extension GL_GOOGLE_include_directive : require
// An additive self-illumination layer (stencil_emissive: door strips, console
// and device lamps) drawn over a path-traced surface. The retail layer tests
// depth EQUAL against its base; drawn by its own retail program it would
// compute that depth with different arithmetic and vanish. Here it shares
// world.vert and world.frag's depth expression, so it lands exactly on the
// surface the path tracer drew, and it evaluates the layer's own combiner.
//
// The layer's material bank holds only its program's colour constants (they
// animate); the control words come from the path tracer's copy of the whole
// program (Scene::material_layer_programs), at the atlas offset the draw
// passes in viewport.z (Render::bind_glow_layer).
layout(location=1) in vec2 traceTexcoord;
layout(location=4) in vec2 titleDepth;
layout(location=5) flat in uint stippleLevel;
layout(location=8) flat in uint sourceDepthRaw;
layout(set=0,binding=0) uniform sampler2D layerTexture0;
layout(set=0,binding=1) uniform sampler2D layerTexture1;
layout(set=0,binding=2) uniform sampler2D layerTexture2;
layout(set=0,binding=3) uniform sampler2D layerTexture3;
// The layer's own material bank (WorldMaterialUniform; see surface.glsl).
layout(set=0,binding=4,std140) uniform LayerSource {
    layout(offset=0) uvec4 program[15];
    layout(offset=304) vec4 forwardTransforms[8];
    layout(offset=432) vec4 diffuseColor;
} layer;
layout(set=1,binding=4,std430) readonly restrict buffer TracePixels {uint tracePixels[];};
layout(push_constant) uniform Camera {mat4 viewProjection;vec4 eyeForward;vec4 eyePosition;vec4 viewport;} camera;
layout(location=0) out vec4 pixel;
#include "source_stipple.glsl"
#include "world_depth_fragment.glsl"
uint layerWord(uint index) {
    switch(index) {
        case 10u: return layer.program[2].z;
        case 11u: return layer.program[2].w;
        case 12u: return layer.program[3].x;
        case 13u: return layer.program[3].y;
        case 14u: return layer.program[3].z;
        case 15u: return layer.program[3].w;
        case 16u: return layer.program[4].x;
        case 17u: return layer.program[4].y;
        case 18u: return layer.program[4].z;
        case 19u: return layer.program[4].w;
        case 20u: return layer.program[5].x;
        case 21u: return layer.program[5].y;
        case 22u: return layer.program[5].z;
        case 23u: return layer.program[5].w;
        case 24u: return layer.program[6].x;
        case 25u: return layer.program[6].y;
        case 43u: return layer.program[10].w;
        case 44u: return layer.program[11].x;
        default: return tracePixels[floatBitsToUint(camera.viewport.z)+index];
    }
}
#define H2_COMBINER_WORD(index) layerWord(index)
#include "combiner.glsl"
void main() {
    if(stippleLevel<16u && !sourceStipplePass(stippleLevel,31u,uvec2(gl_FragCoord.xy)))discard;
    // world.frag's depth, expression for expression.
    gl_FragDepth=titleDepth.y!=0.0?worldD24Depth(clamp(sourceDepthRaw!=0u?
        titleDepth.x:titleDepth.x*16777215.0,0.0,16777215.0)):gl_FragCoord.z;
    // stencil_emissive texture coordinates: c18 transforms the base UV once,
    // then c19..c22 scale and offset it for each stage (emission_uv).
    vec2 base=traceTexcoord*layer.forwardTransforms[0].xy+layer.forwardTransforms[0].zw;
    vec4 samples[4];
    samples[0]=texture(layerTexture0,base*layer.forwardTransforms[1].xy+layer.forwardTransforms[1].zw);
    samples[1]=texture(layerTexture1,base*layer.forwardTransforms[2].xy+layer.forwardTransforms[2].zw);
    samples[2]=texture(layerTexture2,base*layer.forwardTransforms[3].xy+layer.forwardTransforms[3].zw);
    samples[3]=texture(layerTexture3,base*layer.forwardTransforms[4].xy+layer.forwardTransforms[4].zw);
    // Only plain 2D stages are fetched; others read zero, as on NV2A.
    for(uint i=0u;i<4u;++i)if(((word(54u)>>(5u*i))&31u)!=1u)samples[i]=vec4(0);
    pixel=vec4(runCombiners(samples,layer.diffuseColor,vec4(0,0,0,1)).rgb,0.0);
}
