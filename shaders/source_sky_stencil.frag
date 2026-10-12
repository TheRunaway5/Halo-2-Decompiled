#version 450
layout(location=9) flat in vec4 vtxPos0;
layout(location=10) flat in vec4 vtxPos1;
layout(location=11) flat in vec4 vtxPos2;
layout(location=12) flat in float triMZ;
layout(set=0,binding=0,std140) uniform SourceDepthRegion {
    vec4 clipRange;
    ivec4 clipRegion[8];
    vec2 depthFactorOffset;
    ivec2 surfaceScale;
};
// Literal captured fragment determinant/area operation order.
float kahan_det(vec2 a,vec2 b) {
    precise float cd=a.y*b.x;
    precise float err=fma(-a.y,b.x,cd);
    precise float res=fma(a.x,b.y,-cd)+err;
    return res;
}
float area(vec2 a,vec2 b,vec2 c) {return kahan_det(b-a,c-a);}
void main() {
    bool clipContained=false;
    vec2 coord=gl_FragCoord.xy-0.5;
    for(int i=0;i<8;++i) {
        bool outside=any(bvec4(lessThan(coord,vec2(clipRegion[i].xy)),
                              greaterThanEqual(coord,vec2(clipRegion[i].zw))));
        if(!outside) {clipContained=true;break;}
    }
    // Captured modules demote here. With no color/texture/derivative/storage
    // effects in this diagnostic, kill and demote suppress the same depth/
    // stencil updates. Do not enable early_fragment_tests.
    if(!clipContained) discard;
    vec2 unscaled_xy=gl_FragCoord.xy/vec2(surfaceScale);
    precise float bc0=area(unscaled_xy,vtxPos1.xy,vtxPos2.xy);
    precise float bc1=area(unscaled_xy,vtxPos2.xy,vtxPos0.xy);
    precise float bc2=area(unscaled_xy,vtxPos0.xy,vtxPos1.xy);
    bc0/=vtxPos0.w;bc1/=vtxPos1.w;bc2/=vtxPos2.w;
    float inv_bcsum=1.0/(bc0+bc1+bc2);
    if(isinf(inv_bcsum)) inv_bcsum=0.0;
    bc1*=inv_bcsum;bc2*=inv_bcsum;
    precise float zvalue=vtxPos0.w+(bc1*(vtxPos1.w-vtxPos0.w)+bc2*(vtxPos2.w-vtxPos0.w));
    if(zvalue>0.0) {
        float zslopeofs=depthFactorOffset.x*triMZ*zvalue*zvalue;
        zvalue+=depthFactorOffset.y;zvalue+=zslopeofs;
    } else zvalue=uintBitsToFloat(0x7f7fffffu);
    if(isnan(zvalue)) zvalue=uintBitsToFloat(0x7f7fffffu);
    zvalue=clamp(zvalue,clipRange.z,clipRange.w);
    gl_FragDepth=uintBitsToFloat(floatBitsToUint(floor(zvalue)/16777216.0)+1u);
}
