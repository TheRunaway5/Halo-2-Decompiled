#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_geometry.glsl"
// One instance is an ordered assembled triangle. These are bit copies of
// retained VSOut, never CPU-reconstructed planes or expected stencil pixels.
layout(location=0) in vec4 clip0;
layout(location=1) in vec4 window0;
layout(location=2) in vec4 clip1;
layout(location=3) in vec4 window1;
layout(location=4) in vec4 clip2;
layout(location=5) in vec4 window2;
layout(location=9) flat out vec4 vtxPos0;
layout(location=10) flat out vec4 vtxPos1;
layout(location=11) flat out vec4 vtxPos2;
layout(location=12) flat out float triMZ;
void main() {
    mat4 triangle=world_depth_triangle(window0,window1,window2);
    gl_Position=gl_VertexIndex==0?clip0:gl_VertexIndex==1?clip1:clip2;
    vtxPos0=triangle[0];vtxPos1=triangle[1];vtxPos2=triangle[2];
    triMZ=world_depth_finite_slope(triangle);
}
