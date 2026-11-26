// The default object pass's ambient term is a packed source RGB independent
// of the bump map. The two hit paths share this decode.
const uint traceDefaultAmbientKind=4u;
const uint traceFlatAmbientFlag=16777216u;
vec3 traceDefaultSourceAmbient(TraceMaterial material) {
    return 2.0*traceSurfaceColor(material.colorGuide.z).rgb;
}
#ifndef H2_PATH_TRACING_COMPUTE
struct TraceObjectSurface {
    vec3 ambient;
    vec3 bump;
    bool hasBump;
};
vec3 traceObjectBump(vec2 rawUV,vec4 baseTransform,vec4 bumpTransform) {
    vec2 base=rawUV*baseTransform.xy+baseTransform.zw;
    return texture(traceSurfaceTexture2,base*bumpTransform.xy+bumpTransform.zw).rgb;
}
TraceObjectSurface tracePrimaryObjectSurface(TraceMaterial material,vec2 rawUV) {
    if(material.colorGuide.x==traceDefaultAmbientKind) {
        if((material.texture.w&traceFlatAmbientFlag)!=0u)
            return TraceObjectSurface(traceDefaultSourceAmbient(material),vec3(0),false);
        vec3 bump=traceObjectBump(rawUV,traceSurfaceSource.forwardTransforms[0],traceSurfaceSource.forwardTransforms[2]);
        return TraceObjectSurface(traceDefaultSourceAmbient(material),bump,true);
    }
    if((material.texture.w&4096u)==0u || traceSurfaceSource.primaryAmbient.w<=0.0)
        return TraceObjectSurface(vec3(0),vec3(0),false);
    // The renderer binds this owner's current lighting-pass image and its
    // completed c18/c20, including independent bump scale and animation.
    vec3 bump=traceObjectBump(rawUV,traceSurfaceSource.ambientBumpBase,traceSurfaceSource.ambientBumpDetail);
    float bumpZ=(material.texture.w&traceFlatAmbientFlag)!=0u?1.0:max(2.0*bump.b-1.0,0.0);
    return TraceObjectSurface(max(traceSurfaceSource.primaryAmbient.rgb,vec3(0))*bumpZ,bump,true);
}
#endif
// Compact per-owner model ambient records borrow the source bump texels.
// Their native quantized RGB, ordered c18/c20 UVs and sampler are independent
// of albedo and of the static BSP lightmap guide.
vec4 traceAmbientTexel(uint record,ivec2 point) {
    ivec2 extent=ivec2(tracePixels[record+1u],tracePixels[record+2u]);
    uint samplerState=tracePixels[record+3u];
    point.x=traceSurfaceAddress(point.x,extent.x,samplerState&15u);
    point.y=traceSurfaceAddress(point.y,extent.y,(samplerState>>4u)&15u);
    if(any(lessThan(point,ivec2(0))))return traceSurfaceColor(tracePixels[record+4u]);
    return unpackUnorm4x8(tracePixels[tracePixels[record]+uint(point.y)*uint(extent.x)+uint(point.x)]);
}
vec3 traceSecondarySourceAmbient(TraceMaterial material,vec2 rawUV) {
    if(material.colorGuide.x==traceDefaultAmbientKind)return traceDefaultSourceAmbient(material);
    if(material.colorGuide.x!=3u)return vec3(0);
    vec3 irradiance=2.0*traceSurfaceColor(material.colorGuide.z).rgb;
    if((material.texture.w&traceFlatAmbientFlag)!=0u)return irradiance;
    uint record=material.colorGuide.y,length=uint(tracePixels.length());
    if(record>=length || length-record<16u)return vec3(0);
    uvec2 size=uvec2(tracePixels[record+1u],tracePixels[record+2u]);
    if(any(equal(size,uvec2(0))))return vec3(0);
    vec4 base=vec4(uintBitsToFloat(tracePixels[record+5u]),uintBitsToFloat(tracePixels[record+6u]),
                  uintBitsToFloat(tracePixels[record+7u]),uintBitsToFloat(tracePixels[record+8u]));
    vec4 bump=vec4(uintBitsToFloat(tracePixels[record+9u]),uintBitsToFloat(tracePixels[record+10u]),
                  uintBitsToFloat(tracePixels[record+11u]),uintBitsToFloat(tracePixels[record+12u]));
    vec2 uv=(rawUV*base.xy+base.zw)*bump.xy+bump.zw;
    bool point=((tracePixels[record+3u]>>8u)&255u)==1u;
    vec2 coordinate=uv*vec2(size)-(point?vec2(0):vec2(.5));ivec2 at=ivec2(floor(coordinate));
    vec4 sampleColor=traceAmbientTexel(record,at);
    if(!point) {
        vec2 weight=fract(coordinate);
        sampleColor=mix(mix(sampleColor,traceAmbientTexel(record,at+ivec2(1,0)),weight.x),
            mix(traceAmbientTexel(record,at+ivec2(0,1)),traceAmbientTexel(record,at+ivec2(1,1)),weight.x),weight.y);
    }
    return irradiance*max(2.0*sampleColor.b-1.0,0.0);
}
