// Authored lighting RGB steers only diffuse sky/sun hue. Magnitude is removed.
layout(set=1,binding=8,std430) readonly restrict buffer TraceColorGuidePixels {uint traceColorGuidePixels[];};
float traceSourceBrightness(vec3 c){return (c.r*.299+c.g*.587)+c.b*.114;}
vec4 traceValidColorGuide(vec3 c){return vec4(c,traceSourceBrightness(c)>0.0?1.0:0.0);}
int traceColorGuideAddress(int at,int extent,uint mode){
    if(mode==3u)return clamp(at,0,extent-1);
    if(mode==4u)return at<0 || at>=extent?-1:at;
    if(mode==5u)return min(at<0?-at-1:at,extent-1);
    int period=extent*(mode==2u?2:1),value=at%period;if(value<0)value+=period;
    return mode==2u && value>=extent?period-value-1:value;
}
vec3 traceColorGuideTexel(uint record,ivec2 at,uvec4 source){
    ivec2 extent=ivec2(traceColorGuidePixels[record+1u],traceColorGuidePixels[record+2u]);
    at.x=traceColorGuideAddress(at.x,extent.x,source.y&15u);at.y=traceColorGuideAddress(at.y,extent.y,(source.y>>4u)&15u);
    if(any(lessThan(at,ivec2(0))))return vec3((source.z>>16u)&255u,(source.z>>8u)&255u,source.z&255u)/255.0;
    return unpackUnorm4x8(traceColorGuidePixels[traceColorGuidePixels[record]+uint(at.y)*uint(extent.x)+uint(at.x)]).rgb;
}
vec4 tracePrimaryColorGuide(TraceMaterial material,uint image,vec2 uv,vec3 radiance){
    uvec4 source=material.colorGuide;
    if(source.x==2u)return traceValidColorGuide(radiance);
    if(source.x!=1u || image==0u || image>traceColorGuidePixels[0])return vec4(0);
    uint record=image*4u;vec2 extent=vec2(traceColorGuidePixels[record+1u],traceColorGuidePixels[record+2u]);
    if(((source.y>>8u)&255u)==1u)return traceValidColorGuide(traceColorGuideTexel(record,ivec2(floor(uv*extent)),source));
    vec2 point=uv*extent-.5,weight=fract(point);ivec2 at=ivec2(floor(point));
    return traceValidColorGuide(mix(mix(traceColorGuideTexel(record,at,source),traceColorGuideTexel(record,at+ivec2(1,0),source),weight.x),
        mix(traceColorGuideTexel(record,at+ivec2(0,1),source),traceColorGuideTexel(record,at+ivec2(1,1),source),weight.x),weight.y));
}
vec4 traceTriangleColorGuide(TraceTriangle triangle,vec3 barycentric,TraceMaterial material){
    uint image=floatBitsToUint(triangle.colorGuide.w);if(image==0u)return vec4(0);
    if(image==0xffffffffu){
        vec3 color=unpackUnorm4x8(floatBitsToUint(triangle.colorGuide.x)).rgb*barycentric.x+
            unpackUnorm4x8(floatBitsToUint(triangle.colorGuide.y)).rgb*barycentric.y+
            unpackUnorm4x8(floatBitsToUint(triangle.colorGuide.z)).rgb*barycentric.z;
        return tracePrimaryColorGuide(material,0u,vec2(0),color);
    }
    vec2 uv=unpackSnorm2x16(floatBitsToUint(triangle.colorGuide.x))*barycentric.x+
        unpackSnorm2x16(floatBitsToUint(triangle.colorGuide.y))*barycentric.y+
        unpackSnorm2x16(floatBitsToUint(triangle.colorGuide.z))*barycentric.z;
    return tracePrimaryColorGuide(material,image,uv,vec3(0));
}
vec3 traceStaticColor(vec3 incoming,vec4 guide){
    float brightness=traceSourceBrightness(guide.rgb);
    return guide.a>0.0 && brightness>0.0?guide.rgb*(traceSourceBrightness(incoming)/brightness):incoming;
}
