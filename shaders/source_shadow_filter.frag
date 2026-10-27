#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D sourceImage;
layout(set=0,binding=1,std140) uniform Filter {
    vec4 taps[4];
} material;
layout(location=0) out vec4 pixel;
#include "native-programs/weather/shadow-filter-fragment.glsl"
void main() {
    vec4 samples[4];
    samples[0]=texture(sourceImage,(gl_FragCoord.xy+material.taps[0].xy)/128.0);
    samples[1]=texture(sourceImage,(gl_FragCoord.xy+material.taps[1].xy)/128.0);
    samples[2]=texture(sourceImage,(gl_FragCoord.xy+material.taps[2].xy)/128.0);
    samples[3]=texture(sourceImage,(gl_FragCoord.xy+material.taps[3].xy)/128.0);
    pixel=runCombiners(samples,vec4(0),vec4(0));
}
