#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D scene;
layout(set=0,binding=1,std140) uniform Copy {
    uvec4 program[15];vec4 sourceRectangle;
} material;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
void main() {
    vec2 uv=mix(material.sourceRectangle.xy,material.sourceRectangle.zw,gl_FragCoord.xy/64.0);
    vec4 samples[4];samples[0]=texture(scene,uv);
    samples[1]=samples[2]=samples[3]=vec4(0);
    pixel=runCombiners(samples,vec4(0),vec4(0));
}
