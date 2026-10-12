#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_demote_to_helper_invocation : require
#define H2_FIXED_STAGE_UNROLL [[unroll]]
layout(location=0) in vec4 v_vtxD0;
layout(location=1) in vec4 v_vtxD1;
layout(location=2) in vec4 v_vtxB0;
layout(location=3) in vec4 v_vtxB1;
layout(location=4) in float v_vtxFog;
layout(location=5) in vec4 v_vtxT0;
layout(location=6) in vec4 v_vtxT1;
layout(location=7) in vec4 v_vtxT2;
layout(location=8) in vec4 v_vtxT3;
layout(location=9) flat in vec4 v_vtxPos0;
layout(location=10) flat in vec4 v_vtxPos1;
layout(location=11) flat in vec4 v_vtxPos2;
layout(location=12) flat in float v_triMZ;
layout(location=0) out vec4 pixel;
layout(set=0,binding=0) uniform sampler2D diffuse;
layout(set=0,binding=1) uniform sampler2D detail;
layout(set=0,binding=2) uniform sampler2D bump;
layout(set=0,binding=3) uniform sampler2D lightmap;
layout(set=0,binding=5) uniform samplerCube cubeTextures[4];
layout(set=0,binding=6) uniform sampler3D volumeTextures[4];
// Existing native Material prefix/offsets are unchanged. This shader consumes
// only the final program, alpha and texture effects; it does not execute the
// native world UV, lighting or vertex reconstruction a second time.
layout(set=0,binding=4,std140) uniform Material {
    uvec4 program[15];vec4 transforms[3];uvec4 flags;
    vec4 forwardTransforms[8];vec4 diffuseColor;vec4 specularColor;
    uvec4 vertexInstructions[136];vec4 vertexConstants[16];vec4 vertexAttributes[16];
    uvec4 alphaTest;uvec4 textureEffects[4];vec4 fogConstants[6];vec4 bumpMatrices[4];
    vec4 sourceCameraWorld[7];vec4 sourceCameraReflection[7];
    vec4 sourceCameraForeground[4];uvec4 sourceCameraControl;
} material;
layout(set=1,binding=1,std140) uniform MenuWorldFragmentUniforms {
    vec4 clipRange;
    ivec4 clipRegion[8];
    float depthFactor;
    float depthOffset;
    ivec2 surfaceScale;
    vec4 combinerConstants[18];
} menuWorldDepth;
vec4 stageCoordinates[4];
vec4 fetchStage2D(uint stage,vec2 p) {
    if(stage==0u)return texture(diffuse,p);
    if(stage==1u)return texture(detail,p);
    if(stage==2u)return texture(bump,p);
    return texture(lightmap,p);
}
vec4 fetchStageProjected2D(uint stage,vec4 p) {
    if(stage==0u) {
        if((material.textureEffects[0].y&1u)!=0u)p.xy/=vec2(textureSize(diffuse,0));
        return textureProj(diffuse,p.xyw);
    }
    if(stage==1u)return textureProj(detail,p.xyw);
    if(stage==2u)return textureProj(bump,p.xyw);
    return textureProj(lightmap,p.xyw);
}
vec4 fetchStageCube(uint stage,vec3 p) {
    if(stage==0u)return texture(cubeTextures[0],p);
    if(stage==1u)return texture(cubeTextures[1],p);
    if(stage==2u)return texture(cubeTextures[2],p);
    return texture(cubeTextures[3],p);
}
vec4 fetchStage3D(uint stage,vec3 p) {
    // PROJECT3D retains the original textureProj operation and Q. The
    // PROJECT2D also projects in the texture instruction, retaining Q.
    uint mode=(material.program[13][2]>>(5u*stage))&31u;
    if(mode==1u || mode==2u) {
        vec4 q=stageCoordinates[stage];
        if(mode==1u)q.z=0.0;
        if(stage==0u)return textureProj(volumeTextures[0],q);
        if(stage==1u)return textureProj(volumeTextures[1],q);
        if(stage==2u)return textureProj(volumeTextures[2],q);
        return textureProj(volumeTextures[3],q);
    }
    if(stage==0u)return texture(volumeTextures[0],p);
    if(stage==1u)return texture(volumeTextures[1],p);
    if(stage==2u)return texture(volumeTextures[2],p);
    return texture(volumeTextures[3],p);
}
#define MENU_WORLD_PROGRAM_WORD(i) material.program[(i)/4u][(i)%4u]
#define MENU_WORLD_CLIP_RANGE() menuWorldDepth.clipRange
#define MENU_WORLD_CLIP_REGION(i) menuWorldDepth.clipRegion[i]
#define MENU_WORLD_DEPTH_FACTOR() menuWorldDepth.depthFactor
#define MENU_WORLD_DEPTH_OFFSET() menuWorldDepth.depthOffset
#define MENU_WORLD_SURFACE_SCALE() menuWorldDepth.surfaceScale
#define MENU_WORLD_ALPHA_STATE() material.alphaTest.xyz
#define MENU_WORLD_TEXTURE_EFFECTS(i) material.textureEffects[i]
#define MENU_WORLD_TEXTURE_VOLUME(i) ((material.textureEffects[i].z&1u)!=0u)
#define MENU_WORLD_BUMP_MATRIX(i) material.bumpMatrices[i]
// Original float bank, expanded on the CPU after all completed-program
// producers. GPU packed-byte division was measured one ULP wrong in13 lanes.
#define H2_COMBINER_CONSTANT(i) menuWorldDepth.combinerConstants[(i)==43u?16u:(i)==44u?17u:(i)>=18u?2u*((i)-18u)+1u:2u*((i)-10u)]
#include "menu_world_fragment.glsl"
void main() {
    stageCoordinates[0]=v_vtxT0;stageCoordinates[1]=v_vtxT1;
    stageCoordinates[2]=v_vtxT2;stageCoordinates[3]=v_vtxT3;
    pixel=menuWorldFragment(v_vtxD0,v_vtxD1,v_vtxPos0,v_vtxPos1,v_vtxPos2,v_triMZ);
}
