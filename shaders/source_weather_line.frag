#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 color0;
layout(location=1) in vec4 color1;
layout(location=2) flat in vec4 position0;
layout(location=3) flat in vec4 position1;
layout(location=4) flat in vec4 position2;
layout(set=0,binding=0,std140) uniform WeatherVertex {
    uvec4 instructions[136];vec4 camera[12];uvec4 flags;
    vec4 clipRange;vec4 surface;vec4 depth;
} weather;
layout(set=0,binding=1,std140) uniform WeatherPixel {uvec4 program[15];} material;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
#include "particle_depth.glsl"
void main() {
    float w=particleWDepth(gl_FragCoord.xy,position0,position1,position2,
        0.0,weather.depth.x,weather.depth.y);
    if(weather.flags.x!=0u) {
        if(w<weather.clipRange.z || weather.clipRange.w<w)discard;
    } else w=clamp(w,weather.clipRange.z,weather.clipRange.w);
    //39780 rain does not bind or sample a sprite bitmap.
    vec4 stages[4];for(int i=0;i<4;++i)stages[i]=vec4(0,0,0,1);
    pixel=runCombiners(stages,color0,color1);
    int fragAlpha=int(round(pixel.a*255.0));
    if(!(fragAlpha>=0))discard;
    gl_FragDepth=particleD24Depth(w);
}
