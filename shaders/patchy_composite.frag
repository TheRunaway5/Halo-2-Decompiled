#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D depthImage;
layout(set=0,binding=1) uniform sampler2D gradientImage;
layout(set=0,binding=2) uniform sampler2D noiseImage;
layout(set=0,binding=3,std140) uniform Patchy {
    uvec4 program[15]; vec4 rows[8]; vec4 controls; vec4 lookup; vec4 combinerConstants[18];
} material;
layout(location=0) in vec4 depthT0;
layout(location=1) in vec4 gradientT1;
layout(location=2) in vec4 gradientT2;
layout(location=3) in vec4 targetT3;
layout(location=4) in vec2 colorAlpha;
layout(location=0) out vec4 pixel;
#include "patchy_constants.glsl"
#include "combiner.glsl"
void main() {
    float depth=texelFetch(depthImage,ivec2(gl_FragCoord.xy),0).r;
    uint encoded=uint(round(clamp(depth,0.0,1.0)*16777215.0));
    vec3 hilo=vec3(float(encoded>>8u)/65535.0,float((encoded&255u)<<8u)/65535.0,1);
    vec4 samples[4];
    samples[0]=vec4((encoded>>8u)&255u,encoded&255u,0,encoded>>16u)/255.0;
    samples[1]=vec4(0);
    samples[2]=texture(gradientImage,vec2(dot(gradientT1.xyz,hilo),dot(gradientT2.xyz,hilo)));
    samples[3]=textureProj(noiseImage,vec3(targetT3.xy/vec2(textureSize(noiseImage,0)),targetT3.w));
    pixel=runCombiners(samples,vec4(0,0,0,colorAlpha.x),vec4(0,0,0,colorAlpha.y));
}
