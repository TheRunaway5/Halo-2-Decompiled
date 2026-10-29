#ifndef H2_PATH_TRACING_EMITTER_SAMPLING_GLSL
#define H2_PATH_TRACING_EMITTER_SAMPLING_GLSL
// CPU emitter_sampling.cpp appends this optional proposal-only table after
// the ordinary emissive triangle index list. No additional rays or resources
// are used, and sampled radiance always comes from the actual closest hit.
const uint traceEmitterGroupMagic=0x45525447u,traceEmitterGroupWords=12u;
uint traceEmitterGroupCandidate(uint table,uint count,float total,inout uint state) {
    float choice=traceRandom(state)*total;uint low=0u,high=count;
    [[dont_unroll]] for(uint step=0u;step<16u && low<high;++step) {
        uint middle=low+(high-low)/2u;
        float cumulative=uintBitsToFloat(tracePixels[table+middle*traceEmitterGroupWords+11u]);
        if(cumulative<choice)low=middle+1u;else high=middle;
    }
    return min(low,count-1u);
}
vec3 traceDiffuseContinuation(vec3 position,vec3 normal,inout uint state,out float weight) {
    weight=1.0;uint header=floatBitsToUint(traceScene.environment.w)+traceScene.counts.w;
    uint length=uint(tracePixels.length());
    if(traceScene.counts.w==0u || header>=length || length-header<4u ||
       tracePixels[header]!=traceEmitterGroupMagic)return traceHemisphere(normal,state);
    uint count=tracePixels[header+1u],table=header+4u;
    float total=uintBitsToFloat(tracePixels[header+2u]);
    if(count==0u || count>65536u || count>(length-table)/traceEmitterGroupWords || !(total>0.0))
        return traceHemisphere(normal,state);
    vec3 selectedCenter=vec3(0);float selectedRadius=0.0,maximumScore=0.0;
    // Small source banks are inspected completely. Large banks retain a
    // fixed eight flux-weighted candidates, independent of map size.
    [[dont_unroll]] for(uint candidate=0u;candidate<min(count,8u);++candidate) {
        uint group=count<=8u?candidate:traceEmitterGroupCandidate(table,count,total,state);
        uint record=table+group*traceEmitterGroupWords;
        vec3 center=vec3(uintBitsToFloat(tracePixels[record+4u]),uintBitsToFloat(tracePixels[record+5u]),
                         uintBitsToFloat(tracePixels[record+6u]));
        float radius=uintBitsToFloat(tracePixels[record+7u]);
        vec3 power=vec3(uintBitsToFloat(tracePixels[record+8u]),uintBitsToFloat(tracePixels[record+9u]),
                        uintBitsToFloat(tracePixels[record+10u]));
        vec3 delta=center-position;float squared=dot(delta,delta),distance=sqrt(squared);
        float facing=distance>traceEpsilon?clamp((dot(normal,delta)+radius)/distance,0.0,1.0):1.0;
        float score=dot(power,vec3(.2126,.7152,.0722))*facing/max(max(squared,radius*radius),1.e-12);
        if(score>maximumScore){maximumScore=score;selectedCenter=center;selectedRadius=radius;}
    }
    vec3 delta=selectedCenter-position;float squared=dot(delta,delta);
    // An enclosing green room already occupies the BSDF hemisphere. A cone
    // cannot help from inside its sphere, so retain the original sampler.
    if(!(maximumScore>0.0) || selectedRadius<=traceEpsilon || squared<=selectedRadius*selectedRadius)
        return traceHemisphere(normal,state);
    vec3 axis=delta*inversesqrt(squared);
    float cosineMaximum=sqrt(max(0.0,1.0-selectedRadius*selectedRadius/squared));
    if(cosineMaximum>=1.0)return traceHemisphere(normal,state);
    vec3 direction;
    if(traceRandom(state)<.5) {
        float cosine=mix(cosineMaximum,1.0,traceRandom(state)),phi=2.0*tracePi*traceRandom(state);
        float sine=sqrt(max(0.0,1.0-cosine*cosine));
        vec3 tangent=traceNormalize(cross(abs(axis.z)<.999?vec3(0,0,1):vec3(0,1,0),axis),vec3(1,0,0));
        direction=tangent*(cos(phi)*sine)+cross(axis,tangent)*(sin(phi)*sine)+axis*cosine;
    }else direction=traceHemisphere(normal,state);
    // Conditioned on the selected source sphere this is an exact mixture:
    // cosine-hemisphere PDF plus uniform solid-angle cone PDF. It needs no
    // emitter-triangle PDF sum and keeps full support for every other source,
    // sky miss and indirect surface. Visibility is the existing closest ray.
    float bsdf=max(dot(normal,direction),0.0)/tracePi;
    float cone=dot(axis,direction)>=cosineMaximum?1.0/(2.0*tracePi*(1.0-cosineMaximum)):0.0;
    float pdf=.5*(bsdf+cone);weight=pdf>0.0?bsdf/pdf:0.0;return direction;
}
#endif
