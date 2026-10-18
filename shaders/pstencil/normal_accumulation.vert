#version 450
#extension GL_GOOGLE_include_directive : require
#define H2_EXTENDED_VERTEX_BANK
#define main currentVertexMain
#include "world_vertex.glsl"
#undef main
layout(location=15) out vec3 normalRowX;
layout(location=16) out vec3 normalRowY;
layout(location=17) out vec3 normalRowZ;
void main() {
    currentVertexMain();
    // Original1B/15/16 consumes (V6,V5,V4). The posed-geometry adapter
    // supplies the same transformed binormal, tangent and normal inputs.
    normalRowX=vec3(binormal.x,tangent.x,normal.x);
    normalRowY=vec3(binormal.y,tangent.y,normal.y);
    normalRowZ=vec3(binormal.z,tangent.z,normal.z);
}
