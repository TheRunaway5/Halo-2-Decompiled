#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D sourceImage;
layout(set=0,binding=1,std140) uniform Bloom {uvec4 program[15];vec4 controls;vec4 sourceRect;} material;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
vec4 bloomLookup(int stage,vec2 uv) {
    // The original source rectangle excludes presentation borders. Preserve
    // linear clamp at its first/last texel, including the threshold taps.
    vec2 halfTexel=.5/vec2(textureSize(sourceImage,0));
    vec2 p=material.sourceRect.xy+uv*material.sourceRect.zw;
    return texture(sourceImage,clamp(p,material.sourceRect.xy+halfTexel,
        material.sourceRect.xy+material.sourceRect.zw-halfTexel));
}
#include "bloom_color.glsl"
void main() {pixel=titleBloomColor(uv);}
