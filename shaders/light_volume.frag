#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 color;
layout(location=1) in vec2 uv;
layout(location=2) flat in float wDepth;
layout(set=0,binding=0) uniform sampler2D glow;
layout(set=0,binding=4,std140) uniform Material { uvec4 program[15]; } material;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
#include "world_depth_fragment.glsl"
void main() {
    // Store the original integer W depth as the native D24 alias does.
    gl_FragDepth=worldD24Depth(clamp(wDepth,0.0,16777215.0));
    vec4 samples[4];samples[0]=texture(glow,uv);
    samples[1]=samples[2]=samples[3]=vec4(0);
    pixel=runCombiners(samples,color,vec4(0));
}
