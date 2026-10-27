// Source-qualified native self-illumination expressions and independent stage
// images. Scene.pixels stores records and images, never executable instructions.
const uint traceEmissionRecordWords=48u;
int traceEmissionAddress(int value,int extent,uint mode) {
    if(mode==3u)return clamp(value,0,extent-1);
    if(mode==4u)return value<0 || value>=extent?-1:value;
    if(mode==5u)return min(value<0?-value-1:value,extent-1);
    int period=extent*(mode==2u?2:1),at=value%period;if(at<0)at+=period;
    return mode==2u && at>=extent?period-at-1:at;
}
vec4 traceEmissionColor(uint argb) {
    return vec4(float((argb>>16u)&255u),float((argb>>8u)&255u),float(argb&255u),float(argb>>24u))/255.0;
}
vec4 traceEmissionTexel(uint record,uint stage,ivec2 pixel) {
    uint at=record+8u+stage*10u,state=tracePixels[at+3u];
    ivec2 extent=ivec2(tracePixels[at+1u],tracePixels[at+2u]);
    pixel.x=traceEmissionAddress(pixel.x,extent.x,state&15u);
    pixel.y=traceEmissionAddress(pixel.y,extent.y,(state>>4u)&15u);
    if(any(lessThan(pixel,ivec2(0))))return traceEmissionColor(tracePixels[at+8u]);
    return unpackUnorm4x8(tracePixels[record+tracePixels[at]+uint(pixel.y)*uint(extent.x)+uint(pixel.x)]);
}
vec4 traceEmissionSample(uint record,uint stage,vec2 sourceUV) {
    uint at=record+8u+stage*10u;
    vec2 uv=sourceUV*vec2(uintBitsToFloat(tracePixels[at+4u]),uintBitsToFloat(tracePixels[at+5u]))+
        vec2(uintBitsToFloat(tracePixels[at+6u]),uintBitsToFloat(tracePixels[at+7u]));
    vec2 extent=vec2(tracePixels[at+1u],tracePixels[at+2u]);
    if(((tracePixels[at+3u]>>8u)&255u)==1u)return traceEmissionTexel(record,stage,ivec2(floor(uv*extent)));
    vec2 point=uv*extent-.5,weight=fract(point);ivec2 pixel=ivec2(floor(point));
    return mix(mix(traceEmissionTexel(record,stage,pixel),traceEmissionTexel(record,stage,pixel+ivec2(1,0)),weight.x),
        mix(traceEmissionTexel(record,stage,pixel+ivec2(0,1)),traceEmissionTexel(record,stage,pixel+ivec2(1,1)),weight.x),weight.y);
}
vec3 traceEmission(TraceMaterial material,vec2 uv,vec3 tint) {
    uint pointer=floatBitsToUint(material.emission.w);
    if(pointer==0u) {
        if(!any(greaterThan(material.emission.rgb,vec3(0))))return vec3(0);
        return material.emission.rgb*traceTexture(material,uv).rgb*tint;
    }
    uint header=pointer-1u;vec3 result=vec3(0);
    for(uint pass=0u;pass<tracePixels[header+1u];++pass) {
        uint record=header+2u+pass*traceEmissionRecordWords;
        // The exact admitted control identities use stage0, or stages0..2.
        // Named values avoid another dynamically indexed per-fragment array.
        vec4 firstSample=traceEmissionSample(record,0u,uv);
        if((tracePixels[record+17u]&4u)!=0u && firstSample.a==0.0)continue;
        uint family=tracePixels[record];vec3 value=vec3(0);
        if(family==1u)value=clamp(max(firstSample.rgb,vec3(0))*traceEmissionColor(tracePixels[record+2u]).rgb,0.0,1.0);
        else if(family==2u) {
            vec3 textureColor=max(firstSample.rgb,vec3(0));
            float first=clamp(dot(traceEmissionColor(tracePixels[record+2u]).rgb,textureColor),0.0,1.0);
            float second=clamp(dot(traceEmissionColor(tracePixels[record+3u]).rgb,textureColor),0.0,1.0);
            vec4 finalColor=traceEmissionColor(tracePixels[record+6u]);
            value=clamp(clamp(first*traceEmissionColor(tracePixels[record+4u]).rgb+
                second*traceEmissionColor(tracePixels[record+5u]).rgb,0.0,1.0)+
                clamp(firstSample.b,0.0,1.0)*(finalColor.rgb*finalColor.a),0.0,1.0);
        } else if(family==3u) {
            vec4 secondSample=traceEmissionSample(record,1u,uv),detailSample=traceEmissionSample(record,2u,uv);
            if(((tracePixels[record+27u]&4u)!=0u && secondSample.a==0.0) ||
               ((tracePixels[record+37u]&4u)!=0u && detailSample.a==0.0))continue;
            vec3 sum=clamp(max(firstSample.rgb,vec3(0))+max(secondSample.rgb,vec3(0)),0.0,1.0);
            value=clamp(clamp(sum*max(detailSample.rgb,vec3(0)),0.0,1.0)*traceEmissionColor(tracePixels[record+7u]).rgb,0.0,1.0);
        }
        result+=value;
    }
    return result;
}
