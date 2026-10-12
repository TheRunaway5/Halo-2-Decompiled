// Original Xemu Vulkan W-buffer reconstruction, captured in the title
// particle fragment stage. Preserve perspective barycentrics and the D24
// conversion's next representable float rather than interpolate Z depth.
float particleDeterminant(vec2 a,vec2 b) {
    float cd=a.y*b.x;
    float error=fma(-a.y,b.x,cd);
    return fma(a.x,b.y,-cd)+error;
}
float particleArea(vec2 a,vec2 b,vec2 c) {
    return particleDeterminant(b-a,c-a);
}
float particleWDepth(vec2 xy,vec4 p0,vec4 p1,vec4 p2,
    float slope,float factor,float offset) {
    float b0=particleArea(xy,p1.xy,p2.xy)/p0.w;
    float b1=particleArea(xy,p2.xy,p0.xy)/p1.w;
    float b2=particleArea(xy,p0.xy,p1.xy)/p2.w;
    float inverse=1.0/((b0+b1)+b2);
    if(isinf(inverse))inverse=0.0;
    b1*=inverse;b2*=inverse;
    float depth=p0.w+(b1*(p1.w-p0.w)+b2*(p2.w-p0.w));
    if(depth>0.0) {
        float slopeOffset=((factor*slope)*depth)*depth;
        depth+=offset;depth+=slopeOffset;
    }else depth=uintBitsToFloat(0x7f7fffffu);
    if(isnan(depth))depth=uintBitsToFloat(0x7f7fffffu);
    return depth;
}
float particleD24Depth(float depth) {
    return uintBitsToFloat(floatBitsToUint(floor(depth)/16777216.0)+1u);
}
