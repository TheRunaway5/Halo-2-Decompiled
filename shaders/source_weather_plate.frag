#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 t0;
layout(location=1) in vec4 t1;
layout(location=2) in vec4 t2;
layout(location=3) in vec4 t3;
layout(location=4) flat in float sourceW;
layout(set=0,binding=0,std140) uniform WeatherVertex {
    vec4 scroll[3];vec4 clipRange;vec4 surface;uvec4 flags;
    ivec4 clipRegion[8];
} weather;
layout(set=0,binding=1,std140) uniform WeatherPixel {
    vec4 constants[18];
} material;
layout(set=0,binding=2) uniform sampler2D aliasDepth;
layout(set=0,binding=3) uniform sampler2D layer1;
layout(set=0,binding=4) uniform sampler2D layer2;
layout(set=0,binding=5) uniform sampler2D layer3;
layout(location=0) out vec4 pixel;
#include "native-programs/weather/plate-fragment.glsl"
#include "particle_depth.glsl"
void main() {
    vec2 coordinate=gl_FragCoord.xy-0.5;
    bool inside=false;
    for(int i=0;i<8;++i) {
        bool outside=any(bvec4(lessThan(coordinate,vec2(weather.clipRegion[i].xy)),
            greaterThanEqual(coordinate,vec2(weather.clipRegion[i].zw))));
        if(!outside) {inside=true;break;}
    }
    if(!inside)discard;
    // Original37A60 emits equal W at all four vertices. Original geometry
    // barycentrics therefore reconstruct this same constant W (zero offsets).
    float w=sourceW;
    if(weather.flags.x!=0u) {
        if(w<weather.clipRange.z || weather.clipRange.w<w)discard;
    }else w=clamp(w,weather.clipRange.z,weather.clipRange.w);
    vec4 stages[4];
    stages[0]=textureProj(aliasDepth,vec3(t0.xy/vec2(textureSize(aliasDepth,0)),t0.w));
    stages[1]=textureProj(layer1,t1.xyw);
    stages[2]=textureProj(layer2,t2.xyw);
    stages[3]=textureProj(layer3,t3.xyw);
    pixel=runCombiners(stages,vec4(0,0,0,1),vec4(0,0,0,1));
    if(!(int(round(pixel.a*255.0))>0))discard;
    gl_FragDepth=particleD24Depth(w);
}
