#version 450
#extension GL_GOOGLE_include_directive : require
#define H2_EXTENDED_VERTEX_BANK
#define main currentVertexMain
#include "world_vertex.glsl"
#undef main
void main() {
    currentVertexMain();
    // Original44/3D world-space light and eye vectors. C15 maps to the
    // completed current C31; clip, bump UV and W depth stay source-authored.
    precise vec3 lightProduct=-position*vpConstant(31u).w;
    precise vec3 lightVector=lightProduct+vpConstant(31u).xyz;
    stageCoordinates[1]=vec4(lightVector,1);
    stageCoordinates[3]=vec4(camera.eyePosition.xyz-position,1);
}
