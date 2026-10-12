#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D depthImage;
layout(location=0) out vec4 pixel;
struct FogMaterial { vec4 lookup[2]; } material;
uint word(uint index) {return 1u;}
vec4 fogGradient(vec2 coordinate) {return vec4(0);}
vec4 runCombiners(vec4 samples[4],vec4 d0,vec4 d1) {return samples[0];}
#include "fog_color.glsl"
void main() {
    // Actual depth attachment read in a separate fragment render pass.
    pixel=titleFogColor(texelFetch(depthImage,ivec2(gl_FragCoord.xy),0).r);
}
