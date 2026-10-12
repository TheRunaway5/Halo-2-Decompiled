#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=2) uniform sampler2D noiseImage;
layout(set=0,binding=3,std140) uniform Patchy {
    uvec4 program[15]; vec4 rows[8]; vec4 controls; vec4 lookup; vec4 combinerConstants[18];
} material;
layout(location=0) in vec4 noiseT0;
layout(location=1) in vec4 noiseT1;
layout(location=2) in vec4 noiseT2;
layout(location=3) in vec4 noiseT3;
layout(location=4) in vec2 colorAlpha;
layout(location=0) out vec4 pixel;
#include "patchy_constants.glsl"
#include "combiner.glsl"
void main() {
    vec4 samples[4]=vec4[4](textureProj(noiseImage,noiseT0.xyw),
        textureProj(noiseImage,noiseT1.xyw),textureProj(noiseImage,noiseT2.xyw),
        textureProj(noiseImage,noiseT3.xyw));
    pixel=runCombiners(samples,vec4(0,0,0,colorAlpha.x),vec4(0,0,0,colorAlpha.y));
}
