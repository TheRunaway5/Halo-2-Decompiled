#version 450
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in vec4 v2;
layout(location=3) in vec4 v3;
layout(location=4) in vec4 v4;
layout(set=0,binding=0,std140) uniform WeatherVertex {
    vec4 scroll[3];vec4 clipRange;vec4 surface;uvec4 flags;
    ivec4 clipRegion[8];
} weather;
layout(location=0) out vec4 t0;
layout(location=1) out vec4 t1;
layout(location=2) out vec4 t2;
layout(location=3) out vec4 t3;
layout(location=4) flat out float sourceW;
void main() {
    // All five literal VS21 operations, with its masked ADD outputs.
    vec4 screen=v0;
    t0=v1;
    t1=vec4(v2.xy+weather.scroll[0].xy,0,1);
    t2=vec4(v3.xy+weather.scroll[1].xy,0,1);
    t3=vec4(v4.xy+weather.scroll[2].xy,0,1);
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=clamp(screen.w,uintBitsToFloat(0x1f800000u),uintBitsToFloat(0x5f800000u));
    sourceW=screen.w;
    screen.xy=(2.0*screen.xy-weather.surface.xy)/weather.surface.xy;
    screen.z/=weather.clipRange.y;
    screen.xyz*=screen.w;
    gl_Position=screen;
}
