#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 sourceCoordinates[4];
layout(location=0) out vec4 pixel;
layout(set=0,binding=0) uniform sampler2D sourceTexture0;
layout(set=0,binding=1) uniform sampler2D sourceTexture1;
layout(set=0,binding=2,std140) uniform AuthoredPass {
    uvec4 program[15];vec4 D0;vec4 D1;vec4 coordinates;uvec4 controls;
} material;
vec4 stageCoordinates[4];
#include "combiner.glsl"
vec4 fetchStage2D(uint stage,vec2 p){return stage==0u?texture(sourceTexture0,p):texture(sourceTexture1,p);}
vec4 fetchStageCube(uint stage,vec3 p){return vec4(0);}
#define TEXTURE_REJECT() discard
#define TEXTURE_COLOR_SIGN(i) 0u
#define TEXTURE_BUMP_MATRIX(i) vec4(0)
#include "texture_program.glsl"
void main() {
    for(uint i=0u;i<4u;++i)stageCoordinates[i]=sourceCoordinates[i];
    vec4 samples[4];runTextureStages(samples);
    if((material.controls.x&4u)!=0u && samples[0].a==0.0)discard;
    pixel=runCombiners(samples,material.D0,material.D1);
}
