#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D image;
layout(set=0,binding=1) uniform sampler2D detail;
// Match the renderer's MaterialUniform through the authored texture states.
layout(set=0,binding=4,std140) uniform Material {
    uvec4 program[15]; vec4 transforms[3]; uvec4 flags;
    vec4 forwardTransforms[8]; vec4 diffuseColor; vec4 specularColor;
    uvec4 vertexInstructions[136]; vec4 vertexConstants[16]; vec4 vertexAttributes[16];
    uvec4 alphaTest; uvec4 textureEffects[4];
} material;
layout(location=0) in vec2 uv;
layout(location=1) in vec4 color;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
void main() {
    // HUD/font bitmap externs 16/15 are bound by the widget owner. The
    // retail HUD passes use stage 0 and their original register combiners.
    vec4 samples[4];samples[0]=texture(image,uv);
    // Retail 189A0 applies D3DTSS_ALPHAKILL before combiners. The shield
    // pass sets TEXCTL0_ALPHAKILLEN (bit 2), rejecting zero-alpha samples.
    // Its ONE/SRC_ALPHA blend otherwise darkens the transparent padding.
    if((material.textureEffects[0].x&4u)!=0u &&
       (word(54u)&31u)==1u && samples[0].a==0.0)discard;
    for(int i=1;i<4;++i)samples[i]=vec4(0);
    // 22A664 forces vertex slot20 (screen_hud), passing the same v3 UV
    // unchanged to both authored waypoint_emblem texture stages.
    if(material.flags.z==11u)samples[1]=texture(detail,uv);
    pixel=runCombiners(samples,color,vec4(0));
}
