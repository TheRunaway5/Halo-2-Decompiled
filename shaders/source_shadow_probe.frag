#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec2 uv;
layout(location=0) out vec4 pixel;
layout(set=0,binding=0) uniform sampler2D sourceTexture0;
layout(set=0,binding=1) uniform sampler2D sourceTexture1;
layout(set=0,binding=2,std140) uniform AuthoredPass {
    uvec4 program[15];vec4 D0;vec4 D1;vec4 coordinates;uvec4 controls;
} material;
vec4 stageCoordinates[4];
#include "combiner.glsl"
vec4 fetchStage2D(uint stage,vec2 p) {
    // Original external8 is the linear target10: its projected UV is in pixels.
    if(stage==0u) {
        if(material.controls.y!=0u)p/=vec2(textureSize(sourceTexture0,0));
        return texture(sourceTexture0,p);
    }
    return texture(sourceTexture1,p);
}
vec4 fetchStageCube(uint stage,vec3 p){return vec4(0);}
#define TEXTURE_REJECT() discard
#define TEXTURE_COLOR_SIGN(i) 0u
#define TEXTURE_BUMP_MATRIX(i) vec4(0)
#include "texture_program.glsl"
void main() {
    stageCoordinates[0]=vec4(uv*material.coordinates.xy+material.coordinates.zw,0,1);
    stageCoordinates[1]=vec4(uv,0,1);stageCoordinates[2]=stageCoordinates[3]=vec4(0,0,0,1);
    vec4 samples[4];runTextureStages(samples);
    if((material.controls.x&4u)!=0u && samples[0].a==0.0)discard;
    pixel=runCombiners(samples,material.D0,material.D1);
}
