#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D depthImage;
layout(set=0,binding=1) uniform sampler2D gradientImage;
layout(set=0,binding=2,std140) uniform Fog {
    uvec4 program[15]; vec4 lookup[2];
} material;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
vec4 fogGradient(vec2 uv) {return texture(gradientImage,uv);}
#include "fog_color.glsl"
void main() {
    // Same depth sample and admission caller as unchanged production fog.frag.
    float depth=texelFetch(depthImage,ivec2(gl_FragCoord.xy),0).r;
    if(!titleFogVisible(depth))discard;
    pixel=vec4(1);
}
