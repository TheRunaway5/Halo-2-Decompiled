#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#define H2_FIXED_STAGE_UNROLL [[unroll]]
#include "pstencil/light_surface.glsl"
layout(location=15) in vec3 normalRowX;
layout(location=16) in vec3 normalRowY;
layout(location=17) in vec3 normalRowZ;
vec4 sourceCoordinates[4];
#define stageCoordinates sourceCoordinates
vec4 fetchStage2D(uint stage,vec2 p) {return texture(bump,p);}
vec4 fetchStageCube(uint stage,vec3 p) {return texture(cubeTextures[1],p);}
vec4 fetchStage3D(uint stage,vec3 p) {return vec4(0);}
#define TEXTURE_REJECT() discard
#define TEXTURE_COLOR_SIGN(i) (material.textureEffects[(i)==0u?2u:1u].y>>1u)
#define TEXTURE_BUMP_MATRIX(i) material.bumpMatrices[i]
#include "texture_program.glsl"
void main() {
    lightSurfaceDepth();
    sourceCoordinates[0]=currentCoordinates[2];
    sourceCoordinates[1]=vec4(normalRowX,1);
    sourceCoordinates[2]=vec4(normalRowY,1);
    sourceCoordinates[3]=vec4(normalRowZ,1);
    vec4 samples[4];runTextureStages(samples);
    pixel=runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
