#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec3 position;
layout(location=1) in vec2 packedUv;
layout(location=3) in vec4 packedColor;
layout(set=0,binding=2,std140) uniform SourceDecoratorDecal {
    // The shared prefix matches the independently compiled decal fragment ABI.
    vec4 constants[192];vec4 color;vec4 viewport;vec4 clipRange;vec4 depthParameters;uvec4 program[15];
    vec4 attributes[16];
} state;
vec4 decoratorAttributes[16];
#define particleConstants state.constants
#include "source_decorator_program.glsl"
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=5) out vec4 texcoord0;
layout(location=9) flat out vec4 screen0;
void main() {
    for(int i=0;i<16;++i)decoratorAttributes[i]=state.attributes[i];
    decoratorAttributes[0]=vec4(position,1);
    decoratorAttributes[1]=vec4(packedUv,0,1);
    // Stream53 leaves v2 disabled. It reads UB_D3D4 at v3: RGBA8 fetch
    // followed by B/R exchange, as in the authenticated Xemu backend.
    decoratorAttributes[3]=packedColor.bgra;
    decoratorProgram();vec4 screen=outv[0];screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=clampAwayZeroInf(screen.w);screen0=screen;
    vec2 ndc=(screen.xy-state.viewport.xy)*2.0/state.viewport.zw-1.0;
    gl_Position=vec4(ndc*screen.w,screen.z/state.clipRange.y*screen.w,screen.w);
    color0=clamp(outv[3],0,1);color1=clamp(outv[4],0,1);texcoord0=outv[9];
}
