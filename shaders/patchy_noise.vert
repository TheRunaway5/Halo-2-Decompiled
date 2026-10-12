#version 450
// Original 34420 quad split (1,2,0), (2,3,0). The 26CA0 noise or 27520 direct
// callback has completed the float4 attributes before interpolation.
layout(set=0,binding=4,std140) uniform PatchyQuad {
    vec4 attributes[20];vec4 clipRange;vec4 surface;
} quad;
layout(location=0) out vec4 noiseT0;
layout(location=1) out vec4 noiseT1;
layout(location=2) out vec4 noiseT2;
layout(location=3) out vec4 noiseT3;
layout(location=4) out vec2 colorAlpha;
void main() {
    const int corners[6]=int[6](1,2,0,2,3,0);
    int i=corners[gl_VertexIndex];
    vec4 screen=quad.attributes[i];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    vec4 clip=screen;
    clip.xy=(clip.xy*2.0-quad.surface.xy)/quad.surface.xy;
    clip.z/=quad.clipRange.y;
    clip.xyz*=clip.w;
    gl_Position=clip;
    noiseT0=quad.attributes[4+i];noiseT1=quad.attributes[8+i];
    noiseT2=quad.attributes[12+i];noiseT3=quad.attributes[16+i];
    colorAlpha=quad.surface.zw;
}
