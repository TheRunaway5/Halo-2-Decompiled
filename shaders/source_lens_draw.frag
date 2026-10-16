#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec2 uv;
layout(location=1) in vec4 primary;
layout(location=2) in float secondaryAlpha;
layout(location=3) flat in float sourceW;
layout(set=0,binding=0) uniform sampler2D bitmap;
layout(location=0) out vec4 pixel;
#include "source_lens_program.glsl"
#define H2_COMBINER_WORD(index) lensProgram[index]
#include "combiner.glsl"
#include "particle_depth.glsl"
void main() {
    vec4 sampled[4];sampled[0]=texture(bitmap,uv);
    sampled[1]=sampled[2]=sampled[3]=vec4(0,0,0,1);
    pixel=runCombiners(sampled,primary,vec4(0,0,0,secondaryAlpha));
    gl_FragDepth=particleD24Depth(sourceW);
}
