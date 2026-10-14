#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D depthImage;
layout(set=0,binding=1) uniform sampler2D gradientImage;
layout(set=0,binding=2) uniform sampler2D noiseImages[4];
layout(set=0,binding=3,std140) uniform Patchy {
    uvec4 program[15]; vec4 rows[8]; vec4 controls; vec4 lookup;
} material;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
void main() {
    float depth=texelFetch(depthImage,ivec2(gl_FragCoord.xy),0).r;
    uint encoded=uint(round(clamp(depth,0.0,1.0)*16777215.0));
    // The original direct quad uses non-writing integer W LESSEQUAL.
    // This color-only pass samples the same depth attachment, so retain
    // that comparison before the range combiner can wrap below its start.
    if(encoded<uint(floor(material.controls.w)))discard;
    vec4 samples[4];
    samples[0]=vec4((encoded>>8u)&255u,encoded&255u,0,encoded>>16u)/255.0;
    vec4 noise=texture(noiseImages[0],uv);
    samples[1]=word(54u)==0x21u?noise:vec4(0);
    vec3 hilo=vec3(float(encoded>>8u)/65535.0,float((encoded&255u)<<8u)/65535.0,1);
    samples[2]=texture(gradientImage,vec2(dot(material.lookup.xyz,hilo)));
    samples[3]=noise;
    pixel=runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
