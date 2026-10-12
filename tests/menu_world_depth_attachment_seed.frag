#version 450
#extension GL_GOOGLE_include_directive : require
#include "particle_depth.glsl"
layout(set=0,binding=0,std430) readonly buffer Prior { uint integerDepth[]; } prior;
layout(push_constant) uniform Size { uint width; } size;
void main() {
    uint n=prior.integerDepth[uint(gl_FragCoord.y)*size.width+uint(gl_FragCoord.x)]&0xffffffu;
    // Controlled canonical prior state, not previous native draw history.
    gl_FragDepth=particleD24Depth(float(n));
}
