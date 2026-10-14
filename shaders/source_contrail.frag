#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 color0;
layout(location=1) in vec4 color1;
layout(location=5) in vec4 texcoord0;
layout(location=9) flat in vec4 screen0;
layout(location=10) flat in vec4 screen1;
layout(location=11) flat in vec4 screen2;
layout(set=0,binding=0) uniform sampler2D texture0;
layout(set=0,binding=2,std140) uniform SourceContrail {
    vec4 constants[192];vec4 color;vec4 viewport;vec4 clipRange;vec4 depthParameters;uvec4 program[15];
} state;
#define H2_COMBINER_WORD(index) state.program[index/4u][index%4u]
#include "combiner.glsl"
#include "world_depth_fragment.glsl"
layout(location=0) out vec4 pixel;
void main(){
    float depth=worldWDepth(gl_FragCoord.xy,vec2(1),screen0,screen1,screen2,0,0,state.depthParameters.y);
    if(depth<state.clipRange.z || state.clipRange.w<depth)discard;
    vec4 samples[4];samples[0]=textureProj(texture0,texcoord0.xyw);samples[1]=samples[2]=samples[3]=vec4(0,0,0,1);
    pixel=runCombiners(samples,color0,color1);gl_FragDepth=worldD24Depth(depth);
}
