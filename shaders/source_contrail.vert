#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec3 position;
layout(location=1) in vec2 uv;
layout(location=2) in uint packedColor;
layout(set=0,binding=2,std140) uniform SourceContrail {
    vec4 constants[192];vec4 color;vec4 viewport;vec4 clipRange;vec4 depthParameters;uvec4 program[15];
} state;
vec4 contrailAttributes[16];
#define particleConstants state.constants
#include "source_contrail_program.glsl"
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=5) out vec4 texcoord0;
layout(location=9) flat out vec4 screen0;
void main() {
    for(int i=0;i<16;++i)contrailAttributes[i]=vec4(0,0,0,1);
    contrailAttributes[0]=vec4(position,1);
    contrailAttributes[1]=vec4(uv,0,1);
    contrailAttributes[2]=vec4(0,0,0,1);
    contrailAttributes[3]=unpackUnorm4x8(packedColor).bgra;
    contrailProgram();vec4 screen=outv[0];screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=clampAwayZeroInf(screen.w);screen0=screen;
    vec2 ndc=(screen.xy-state.viewport.xy)*2.0/state.viewport.zw-1.0;
    gl_Position=vec4(ndc*screen.w,screen.z/state.clipRange.y*screen.w,screen.w);
    color0=clamp(outv[3],0,1);color1=clamp(outv[4],0,1);texcoord0=outv[9];
}
