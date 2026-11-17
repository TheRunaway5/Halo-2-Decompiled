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
    if(material.colorGuide.x!=3u)return vec3(0);
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
    return 2.0*traceSurfaceColor(material.colorGuide.z).rgb*max(2.0*sampleColor.b-1.0,0.0);
}
