#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D scene;
layout(set=0,binding=2,std140) uniform Copy {
    uvec4 program[15]; vec4 sourceRectangle; vec4 taps;
} material;
layout(location=0) out vec4 pixel;
#include "native-programs/world/water-copy-combiner.glsl"
void main() {
    vec2 p=gl_FragCoord.xy/vec2(512,256);
    vec2 uv=material.sourceRectangle.xy+p*material.sourceRectangle.zw;
    vec4 samples[4];
    samples[0]=texture(scene,uv+material.taps.xy);samples[1]=texture(scene,uv+material.taps.zy);
    samples[2]=texture(scene,uv+material.taps.xw);samples[3]=texture(scene,uv+material.taps.zw);
    pixel=runCombiners(samples,vec4(0),vec4(0));
}
