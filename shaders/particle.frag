#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 color0;
layout(location=1) in vec4 color1;
layout(location=5) in vec4 texcoord0;
layout(location=6) in vec4 texcoord1;
layout(location=9) flat in vec4 screen0;
layout(location=10) flat in vec4 screen1;
layout(location=11) flat in vec4 screen2;
layout(location=12) flat in float depthSlope;
layout(set=0,binding=0) uniform sampler2D texture0;
layout(set=0,binding=1) uniform sampler2D texture1;
layout(set=0,binding=4,std140) uniform Material {
    uvec4 program[15];uvec4 alphaTest;vec4 bumpMatrix;vec4 coordinateScale;
} material;
layout(set=0,binding=5,std140) uniform ParticleDepth {
    vec4 clipRange;
    vec4 depthParameters; // slope factor, offset, X/Y surface scale.
} depthState;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
#include "particle_depth.glsl"
#include "source_alpha_test.glsl"
float signedBumpByte(float value) {
    float byteValue=value*255.0;
    return (byteValue>=128.0?byteValue-256.0:byteValue)/127.0;
}
void main() {
    float depth=particleWDepth(gl_FragCoord.xy/depthState.depthParameters.zw,
        screen0,screen1,screen2,depthSlope,depthState.depthParameters.x,
        depthState.depthParameters.y);
    if(depth<depthState.clipRange.z || depthState.clipRange.w<depth)discard;
    vec4 sampled[4];
    sampled[0]=(word(54u)&31u)!=0u?textureProj(texture0,texcoord0.xyw):vec4(0,0,0,1);
    uint mode1=(word(54u)>>5u)&31u;
    sampled[1]=vec4(0,0,0,1);
    if(mode1==1u)sampled[1]=textureProj(texture1,texcoord1.xyw);
    if(mode1==6u) {
        // Stage1's dependency is always stage0. Signed texture channels
        // have already been decoded before filtering by the upload path.
        uint sign=material.alphaTest.w;
        vec2 delta=vec2((sign&2u)!=0u?sampled[0].b:signedBumpByte(sampled[0].b),
                       (sign&4u)!=0u?sampled[0].g:signedBumpByte(sampled[0].g));
        delta=mat2(material.bumpMatrix.xy,material.bumpMatrix.zw)*delta;
        sampled[1]=texture(texture1,(texcoord1.xy+delta)*material.coordinateScale.xy);
    }
    sampled[2]=sampled[3]=vec4(0,0,0,1);
    // Same authored NV_PGRAPH_TEXCTL0_ALPHAKILLEN admission as world.frag.
    // Each fetched stage is tested before combiners and final ALPHATEST.
    for(uint stage=0u;stage<2u;++stage) {
        uint mode=(word(54u)>>(stage*5u))&31u;
        if(((material.alphaTest.w>>(16u+stage*4u))&4u)!=0u &&
           (mode==1u || mode==2u || mode==3u || mode==9u || mode==11u || mode==12u ||
            mode==14u || mode==15u || mode==16u) && sampled[stage].a==0.0)discard;
    }
    pixel=runCombiners(sampled,color0,color1);
    if(material.alphaTest.x!=0u && !sourceAlphaTestPass(pixel.a,material.alphaTest.y,material.alphaTest.z))discard;
    gl_FragDepth=particleD24Depth(depth);
}
