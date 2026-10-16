#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) flat in float sourceW;
#include "particle_depth.glsl"
void main() {
    // Original2DD10 selects W depth, LEQUAL, no color/depth writes.
    gl_FragDepth=particleD24Depth(sourceW);
}
