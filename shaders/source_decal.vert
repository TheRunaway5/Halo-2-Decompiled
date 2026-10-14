#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec3 position;
layout(location=1) in uint projection;
layout(set=0,binding=2,std140) uniform SourceDecal {
    vec4 constants[192];vec4 color;vec4 viewport;vec4 clipRange;vec4 depthParameters;uvec4 program[15];
} state;
vec4 decalAttributes[16];
#define particleConstants state.constants
#include "source_decal_program.glsl"
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=5) out vec4 texcoord0;
layout(location=9) flat out vec4 screen0;
float component(uint value,uint bits) {
    int v=int(value<<(32u-bits))>>(32u-bits);
    return float(v)/float((1u<<(bits-1u))-1u);
}
void main() {
    for(int i=0;i<16;++i)decalAttributes[i]=vec4(0,0,0,1);
    decalAttributes[0]=vec4(position,1);
    decalAttributes[1]=vec4(component(projection,11),component(projection>>11u,11),component(projection>>22u,10),1);
    decalAttributes[10]=state.color;decalAttributes[11]=vec4(0);
    decalProgram();vec4 screen=outv[0];screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=clampAwayZeroInf(screen.w);screen0=screen;
    vec2 ndc=(screen.xy-state.viewport.xy)*2.0/state.viewport.zw-1.0;
    gl_Position=vec4(ndc*screen.w,screen.z/state.clipRange.y*screen.w,screen.w);
    color0=clamp(outv[3],0,1);color1=clamp(outv[4],0,1);texcoord0=outv[9];
}
