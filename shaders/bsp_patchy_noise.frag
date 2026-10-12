#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=2) uniform sampler2D noiseImages[4];
layout(set=0,binding=3,std140) uniform Patchy {
    uvec4 program[15]; vec4 rows[8]; vec4 controls; vec4 lookup;
} material;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
vec4 patchyNoiseLookup(int layer,vec2 coordinates) {return texture(noiseImages[layer],coordinates);}
vec4 patchyGradientLookup(vec2 coordinates) {return vec4(0);}
vec4 patchyTargetLookup(vec2 coordinates) {return vec4(0);}
#include "patchy_color.glsl"
void main() {pixel=titlePatchyNoise(uv);}
