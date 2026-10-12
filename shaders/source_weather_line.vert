#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 sourcePosition;
layout(location=1) in uint sourceColor;
layout(set=0,binding=0,std140) uniform WeatherVertex {
    uvec4 instructions[136];vec4 camera[12];uvec4 flags;
    vec4 clipRange;vec4 surface;vec4 depth;
} weather;
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=2) flat out vec4 screenPosition;
#include "source_weather_point_program.glsl"
void main() {
    sourceWeatherPointProgram(sourcePosition,sourceColor);
    vec4 screen=vpOut[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    // The pinned original wrapper clamps W without changing its sign.
    if(screen.w>0.0 || floatBitsToUint(screen.w)==0u)
        screen.w=clamp(screen.w,uintBitsToFloat(0x1f800000u),uintBitsToFloat(0x5f800000u));
    else screen.w=clamp(screen.w,uintBitsToFloat(0xdf800000u),uintBitsToFloat(0x9f800000u));
    screenPosition=screen;
    vec4 clip=screen;
    clip.xy=(2.0*clip.xy-weather.surface.xy)/weather.surface.xy;
    clip.z/=weather.clipRange.y;
    clip.xyz*=clip.w;
    gl_Position=clip;
    color0=clamp(mix(vpOut[3],vec4(1),isnan(vpOut[3])),0.0,1.0);
    color1=clamp(mix(vpOut[4],vec4(1),isnan(vpOut[4])),0.0,1.0);
}
