// Ordinary world W reconstruction from the retained Xemu Vulkan psh.c
// (fc24584ce88f0915ad7f04775bb7712c2e3f49ee), lines915..926 and1007..1042.
// Precision is part of the source contract: the retained original fragment
// modules prohibit contraction for the determinant and W reconstruction.
// Keep clip demotion and the source depth/stencil policy in the caller.
float worldDepthDeterminant(vec2 a,vec2 b) {
    precise float cd=a.y*b.x;
    precise float error=fma(-a.y,b.x,cd);
    precise float result=fma(a.x,b.y,-cd)+error;
    return result;
}
float worldDepthArea(vec2 a,vec2 b,vec2 c) {
    return worldDepthDeterminant(b-a,c-a);
}
// A light's shadow target is a Z buffer, not the W buffer of the main view:
// the retained psh.c/geom path without z_perspective. Z is linear across the
// screen triangle, and the polygon offset is the bias plus the factor times
// the larger screen-space Z slope.
float worldZDepth(vec2 fragmentCoordinate,vec2 surfaceScale,
    vec4 p0,vec4 p1,vec4 p2,float factor,float offset) {
    vec2 xy=fragmentCoordinate/surfaceScale;
    precise float b0=worldDepthArea(xy,p1.xy,p2.xy);
    precise float b1=worldDepthArea(xy,p2.xy,p0.xy);
    precise float b2=worldDepthArea(xy,p0.xy,p1.xy);
    float inverse=1.0/((b0+b1)+b2);
    if(isinf(inverse))inverse=0.0;
    b1*=inverse;
    b2*=inverse;
    precise float depth=p0.z+(b1*(p1.z-p0.z)+b2*(p2.z-p0.z));
    vec2 first=p1.xy-p0.xy,second=p2.xy-p0.xy;
    vec2 difference=vec2(p0.z-p1.z,p0.z-p2.z);
    float determinant=worldDepthDeterminant(first,second);
    float dx=worldDepthDeterminant(vec2(difference.x,first.y),vec2(difference.y,second.y))/determinant;
    float dy=worldDepthDeterminant(vec2(first.x,difference.x),vec2(second.x,difference.y))/determinant;
    float slope=max(abs(dx),abs(dy));
    if(isnan(slope) || isinf(slope))slope=0.0;
    depth+=offset;
    depth+=factor*slope;
    if(isnan(depth))depth=uintBitsToFloat(0x7f7fffffu);
    return depth;
}
float worldWDepth(vec2 fragmentCoordinate,vec2 surfaceScale,
    vec4 p0,vec4 p1,vec4 p2,float slope,float factor,float offset) {
    vec2 xy=fragmentCoordinate/surfaceScale;
    precise float b0=worldDepthArea(xy,p1.xy,p2.xy);
    precise float b1=worldDepthArea(xy,p2.xy,p0.xy);
    precise float b2=worldDepthArea(xy,p0.xy,p1.xy);
    b0/=p0.w;
    b1/=p1.w;
    b2/=p2.w;
    float inverse=1.0/((b0+b1)+b2);
    if(isinf(inverse))inverse=0.0;
    b1*=inverse;
    b2*=inverse;
    precise float depth=p0.w+(b1*(p1.w-p0.w)+b2*(p2.w-p0.w));
    if(depth>0.0) {
        float slopeOffset=((factor*slope)*depth)*depth;
        depth+=offset;
        depth+=slopeOffset;
    } else {
        depth=uintBitsToFloat(0x7f7fffffu);
    }
    if(isnan(depth))depth=uintBitsToFloat(0x7f7fffffu);
    return depth;
}
float worldD24Depth(float depth) {
    return uintBitsToFloat(floatBitsToUint(floor(depth)/16777216.0)+1u);
}
