#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#define H2_FIXED_STAGE_UNROLL [[unroll]]
#include "pstencil/light_surface.glsl"
vec4 sourceCoordinates[4];
#define stageCoordinates sourceCoordinates
vec4 fetchStage2D(uint stage,vec2 p) {return texture(bump,p);}
vec4 fetchStageCube(uint stage,vec3 p) {return texture(cubeTextures[1],p);}
vec4 fetchStage3D(uint stage,vec3 p) {return texture(volumeTextures[3],p);}
#define TEXTURE_REJECT() discard
#define TEXTURE_VOLUME(i) ((i)==3u)
#define TEXTURE_COLOR_SIGN(i) (material.textureEffects[(i)==0u?2u:1u].y>>1u)
#define TEXTURE_BUMP_MATRIX(i) material.bumpMatrices[i]
#include "texture_program.glsl"
void main() {
    lightSurfaceDepth();
    // Original36's UV/light/eye/falloff inputs mapped from the actual current
    // tangent sphere program. C35.w is the verified original -0.5/radius.
    sourceCoordinates[0]=currentCoordinates[2];
    sourceCoordinates[1]=currentCoordinates[1];
    sourceCoordinates[2]=currentCoordinates[3];
    if(textureMode(3u)==4u) {
        // Original2F: DPH supplies Q; DP3 plus original C20.z supplies
        // the three falloff components. Retail C36 has another meaning.
        float distance=dot(lightConstant(32u).xyz-world,lightConstant(35u).xyz);
        sourceCoordinates[3]=vec4(vec3(distance+lightConstant(36u).z),distance+lightConstant(35u).w);
    }else sourceCoordinates[3]=vec4(vec3(.5)-(lightConstant(31u).xyz-world)*lightConstant(35u).w,1);
    vec4 samples[4];runTextureStages(samples);
    pixel=runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
