#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D sourceImage;
layout(set=0,binding=1,std140) uniform Filter {
    uvec4 program[15];vec4 taps[4];
} material;
layout(location=0) out vec4 pixel;
vec4 stageCoordinates[4];
#include "combiner.glsl"
vec4 fetchStage2D(uint stage,vec2 p){return texture(sourceImage,p);}
vec4 fetchStageCube(uint stage,vec3 p){return vec4(0);}
#define TEXTURE_REJECT() discard
#define TEXTURE_COLOR_SIGN(i) 0u
#define TEXTURE_BUMP_MATRIX(i) vec4(0)
#include "texture_program.glsl"
void main() {
    for(uint stage=0u;stage<4u;++stage)
        stageCoordinates[stage]=vec4((gl_FragCoord.xy+material.taps[stage].xy)/128.0,0,1);
    vec4 samples[4];runTextureStages(samples);
    pixel=runCombiners(samples,vec4(0),vec4(0));
}
