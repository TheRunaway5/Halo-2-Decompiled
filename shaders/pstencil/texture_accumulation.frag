#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#define H2_FIXED_STAGE_UNROLL [[unroll]]
uint textureAccumulationWord(uint index);
#define H2_COMBINER_WORD(index) textureAccumulationWord(index)
#include "pstencil/light_surface.glsl"
#undef H2_COMBINER_WORD
#include "world_combiner_specialization.glsl"
#include "pstencil/texture_accumulation_2003_program.glsl"
bool originalPhase=false;
uint textureAccumulationWord(uint index) {
    return originalPhase?originalTextureProgram[index]:h2CombinerWord(index);
}
#define stageCoordinates currentCoordinates
vec4 fetchStage2D(uint stage,vec2 p) {
    if(stage==0u)return texture(diffuse,p);
    if(stage==1u)return texture(detail,p);
    if(stage==2u)return texture(bump,p);
    return texture(lightmap,p);
}
vec4 fetchStageCube(uint stage,vec3 p) {return vec4(0);}
vec4 fetchStage3D(uint stage,vec3 p) {return vec4(0);}
#define TEXTURE_REJECT() discard
#define TEXTURE_COLOR_SIGN(i) (material.textureEffects[i].y>>1u)
#define TEXTURE_BUMP_MATRIX(i) material.bumpMatrices[i]
#include "texture_program.glsl"
void main() {
    lightSurfaceDepth();
    vec4 samples[4];runTextureStages(samples);
    // Admitted current color-change subgraphs are independent of base/detail
    // RGB until their final product. Unit product inputs recover that exact
    // authored color factor, including multi-region Elite/Chief rank colors.
    // Preserve all sampled alpha inputs; no faction/material presets are used.
    vec4 colorInputs[4];for(uint s=0u;s<4u;++s)colorInputs[s]=samples[s];
    colorInputs[0].rgb=vec3(1);colorInputs[1].rgb=vec3(.5);
    vec4 colorFactor=runCombiners(colorInputs,material.diffuseColor,
        material.flags.w!=0u?lightingDirection:vec4(0,0,0,1));
    originalPhase=true;
    // Current-asset color adaptation: the original mask lerp with a white
    // mask accepts the already-completed current color factor as its V10.
    // Original builtin3 then owns base/detail RGB and specular-mask alpha.
    samples[3]=vec4(1);
    pixel=runCombiners(samples,colorFactor,vec4(0,0,0,1));
}
