#ifdef HALO2_WORLD_DEPTH_SOURCE_PRECISE
#include "world_depth_fragment.glsl"
#else
#include "particle_depth.glsl"
#endif
layout(location=9) flat in vec4 vtxPos0;
layout(location=10) flat in vec4 vtxPos1;
layout(location=11) flat in vec4 vtxPos2;
layout(location=12) flat in float triMZ;
layout(set=0,binding=1,std140) uniform SourceFragment {
    layout(offset=272) vec4 clipRange;
    layout(offset=288) ivec4 clipRegion[8];
    layout(offset=832) float depthFactor;
    layout(offset=836) float depthOffset;
    layout(offset=864) ivec2 surfaceScale;
} source;
layout(location=0) out vec4 pixel;
void main() {
    bool contained=false;
    vec2 coord=gl_FragCoord.xy-vec2(0.5);
    for(int i=0;i<8;++i) {
        if(all(greaterThanEqual(coord,vec2(source.clipRegion[i].xy))) &&
           all(lessThan(coord,vec2(source.clipRegion[i].zw)))) {contained=true;break;}
    }
    if(!contained)demote;
#ifdef HALO2_WORLD_DEPTH_SOURCE_PRECISE
    float depth=worldWDepth(gl_FragCoord.xy,vec2(source.surfaceScale),
        vtxPos0,vtxPos1,vtxPos2,triMZ,source.depthFactor,source.depthOffset);
#else
    float depth=particleWDepth(gl_FragCoord.xy/vec2(source.surfaceScale),
        vtxPos0,vtxPos1,vtxPos2,triMZ,source.depthFactor,source.depthOffset);
#endif
    if(depth<source.clipRange.z || depth>source.clipRange.w)demote;
#ifdef HALO2_WORLD_DEPTH_SOURCE_PRECISE
    gl_FragDepth=worldD24Depth(depth);
#else
    gl_FragDepth=particleD24Depth(depth);
#endif
    pixel=vec4(1);
}
