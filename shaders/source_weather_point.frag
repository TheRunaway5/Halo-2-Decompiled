#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 color0;
layout(location=1) in vec4 color1;
layout(location=2) flat in vec4 screenPosition;
layout(set=0,binding=0,std140) uniform WeatherVertex {
    vec4 camera[12];uvec4 flags;
    vec4 clipRange;vec4 surface;vec4 depth;
} weather;
layout(set=0,binding=2) uniform sampler2D sprite;
layout(location=0) out vec4 pixel;
#include "native-programs/weather/point-fragment.glsl"
#include "particle_depth.glsl"
void main() {
    float w=particleWDepth(gl_FragCoord.xy,screenPosition,screenPosition,
        screenPosition,0.0,weather.depth.x,weather.depth.y);
    if(weather.flags.x!=0u) {
        if(w<weather.clipRange.z || weather.clipRange.w<w)discard;
    } else w=clamp(w,weather.clipRange.z,weather.clipRange.w);
    vec4 stages[4];
    stages[0]=stages[1]=stages[2]=vec4(0,0,0,1);
    // Original GL raw rowY grows upward, while positive Vulkan viewport rowY
    // grows downward. With an upper-left point origin, the same numeric row
    // samples t_GL=.5-(row-center)/size and t_VK=.5+(row-center)/size.
    // Preserve original rows and sprite bytes; convert only this API coordinate.
    vec2 pointCoordinates=gl_PointCoord;
    if(weather.flags.y!=0u)pointCoordinates.y=1.0-pointCoordinates.y;
    stages[3]=texture(sprite,pointCoordinates);
    pixel=runCombiners(stages,color0,color1);
    // Original391F0 packet: method033C=0x206 (GEQUAL), method0340=0.
    // Pinned source fragment compares the rounded eight-bit integer alpha.
    int fragAlpha=int(round(pixel.a*255.0));
    if(!(fragAlpha>=0))discard;
    gl_FragDepth=particleD24Depth(w);
}
