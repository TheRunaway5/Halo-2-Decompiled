#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D sourceImage;
layout(set=0,binding=1,std140) uniform Bloom {uvec4 program[15];vec4 controls;vec4 sourceRect;} material;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
// One texel of the original-resolution source image: the exact area average
// of the physical texels it covers. ratio is physical texels per source texel.
vec4 bloomSourceTexel(ivec2 texel,vec2 ratio,ivec2 size) {
    vec2 low=vec2(texel)*ratio,high=low+ratio;
    ivec2 first=ivec2(floor(low)),last=ivec2(ceil(high))-1;
    vec4 sum=vec4(0);
    for(int y=first.y;y<=last.y;++y) {
        float wy=min(high.y,float(y+1))-max(low.y,float(y));
        for(int x=first.x;x<=last.x;++x) {
            float wx=min(high.x,float(x+1))-max(low.x,float(x));
            sum+=texelFetch(sourceImage,clamp(ivec2(x,y),ivec2(0),size-1),0)*(wx*wy);
        }
    }
    return sum/(ratio.x*ratio.y);
}
vec4 bloomLookup(int stage,vec2 uv) {
    // The original source rectangle excludes presentation borders. Preserve
    // linear clamp at its first/last texel, including the threshold taps.
    vec2 halfTexel=.5/vec2(textureSize(sourceImage,0));
    vec2 p=material.sourceRect.xy+uv*material.sourceRect.zw;
    if(material.controls.w>=1.0) {
        // The threshold taps of a view larger than its original raster read
        // that raster: controls.w packs its width*4096+height. Each tap is the
        // original linear clamp lookup of the area-averaged image, so the
        // filter chain receives what the original 640x480 frame supplied.
        ivec2 size=textureSize(sourceImage,0);
        vec2 source=vec2(floor(material.controls.w/4096.0),mod(material.controls.w,4096.0));
        vec2 ratio=vec2(size)/source;
        vec2 at=clamp(uv,.5/source,1.0-.5/source)*source-.5;
        ivec2 base=ivec2(floor(at));vec2 f=at-vec2(base);
        ivec2 limit=ivec2(source)-1;
        vec4 a=bloomSourceTexel(clamp(base,ivec2(0),limit),ratio,size);
        vec4 b=bloomSourceTexel(clamp(base+ivec2(1,0),ivec2(0),limit),ratio,size);
        vec4 c=bloomSourceTexel(clamp(base+ivec2(0,1),ivec2(0),limit),ratio,size);
        vec4 d=bloomSourceTexel(clamp(base+ivec2(1,1),ivec2(0),limit),ratio,size);
        return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
    }
    return texture(sourceImage,clamp(p,material.sourceRect.xy+halfTexel,
        material.sourceRect.xy+material.sourceRect.zw-halfTexel));
}
#include "bloom_color.glsl"
void main() {pixel=titleBloomColor(uv);}
