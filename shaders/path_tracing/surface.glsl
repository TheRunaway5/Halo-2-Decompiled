#ifndef H2_PATH_TRACING_SURFACE_GLSL
#define H2_PATH_TRACING_SURFACE_GLSL
// Fixed source-qualified unlit material expressions. The CPU admits complete
// native control/vertex identities; these records contain data, not programs.
layout(set=0,binding=0) uniform sampler2D traceSurfaceTexture0;
layout(set=0,binding=1) uniform sampler2D traceSurfaceTexture1;
layout(set=0,binding=2) uniform sampler2D traceSurfaceTexture2;
layout(set=0,binding=3) uniform sampler2D traceSurfaceTexture3;
layout(set=0,binding=4,std140) uniform TraceSurfaceSource {
    layout(offset=0) uvec4 program[15];
    layout(offset=304) vec4 forwardTransforms[8];
} traceSurfaceSource;
const uint traceSurfaceFlag=64u,traceSurfaceMagic=0x53525448u,traceSurfaceWords=52u;
vec4 traceSurfaceColor(uint argb) {
    return vec4(float((argb>>16u)&255u),float((argb>>8u)&255u),float(argb&255u),float(argb>>24u))/255.0;
}
bool traceSurfaceRecord(TraceMaterial material,out uint record) {
    uint pointer=floatBitsToUint(material.base.w);record=pointer-1u;
    uint length=uint(tracePixels.length());
    return pointer!=0u && record<length && traceSurfaceWords<=length-record && tracePixels[record]==traceSurfaceMagic;
}
vec4 traceSurfaceEvaluate(uint family,vec4 first,vec4 second,vec4 third,vec4 fourth,
                           vec4 color13,vec4 color21,vec4 color43,vec4 color44) {
    if(family==1u) {
        float mask=clamp(first.a,0.0,1.0);
        vec3 blend=(1.0-mask)*max(second.rgb,vec3(0))+mask*max(third.rgb,vec3(0));
        vec3 base=clamp(2.0*max(first.rgb,vec3(0))*blend,0.0,1.0);
        vec3 detail=clamp(2.0*base*max(fourth.rgb,vec3(0)),0.0,1.0);
        return vec4(clamp(detail*color43.rgb+color44.rgb,0.0,1.0),
                    clamp((1.0-mask)*max(second.a,0.0)+mask*max(third.a,0.0),0.0,1.0));
    }
    // Native no-bump foliage writes a binary alpha through its final mux.
    // Its source LESS comparison is applied to this output, never raw T0.a.
    float opaque=family==3u?color21.a:color13.a;
    float transparent=family==3u?color13.a:color21.a;
    return vec4(clamp(2.0*max(first.rgb,vec3(0))*max(second.rgb,vec3(0)),0.0,1.0),
                (family>=4u?third.a:first.a)>=.5?opaque:transparent);
}
bool traceSurfaceKill(uint record,uint stage,float alpha) {
    return (tracePixels[record+12u+stage*10u+9u]&4u)!=0u && alpha==0.0;
}
bool tracePrimarySurface(TraceMaterial material,vec2 rawUV,out vec4 sourceColor) {
    if((material.texture.w&traceSurfaceFlag)==0u) {
        sourceColor=texture(traceSurfaceTexture0,rawUV*material.transform.xy+material.transform.zw);return true;
    }
    uint record;if(!traceSurfaceRecord(material,record))return false;
    uint family=tracePixels[record+1u];if(family<1u || family>5u)return false;
    vec2 base=rawUV*traceSurfaceSource.forwardTransforms[0].xy+traceSurfaceSource.forwardTransforms[0].zw;
    vec4 first=texture(traceSurfaceTexture0,base);
    vec4 second=texture(traceSurfaceTexture1,base*traceSurfaceSource.forwardTransforms[1].xy+
                        traceSurfaceSource.forwardTransforms[1].zw);
    if(traceSurfaceKill(record,0u,first.a) || traceSurfaceKill(record,1u,second.a))return false;
    vec4 third=vec4(0),fourth=vec4(0);
    if(family==1u || family>=4u) {
        vec2 coordinates=base*traceSurfaceSource.forwardTransforms[2].xy;
        if(family>=4u)coordinates+=traceSurfaceSource.forwardTransforms[2].zw;
        third=texture(traceSurfaceTexture2,coordinates);
        if(traceSurfaceKill(record,2u,third.a))return false;
    }
    if(family==1u) {
        fourth=texture(traceSurfaceTexture3,base*traceSurfaceSource.forwardTransforms[2].zw);
        if(traceSurfaceKill(record,3u,fourth.a))return false;
    }
    // Literal dynamic source colors; families4/5 retain qualified coverage,
    // with the baked-light-dependent output alpha represented by its bounds.
    vec4 color0=traceSurfaceColor(traceSurfaceSource.program[3].y);
    vec4 color1=traceSurfaceColor(traceSurfaceSource.program[5].y);
    if(family==4u)color1=traceSurfaceColor(traceSurfaceSource.program[5].z);
    if(family==5u) {
        color0=traceSurfaceColor(traceSurfaceSource.program[3].w);
        color1=traceSurfaceColor(traceSurfaceSource.program[4].y);
    }
    sourceColor=traceSurfaceEvaluate(family,first,second,third,fourth,color0,color1,
        traceSurfaceColor(traceSurfaceSource.program[10].w),traceSurfaceColor(traceSurfaceSource.program[11].x));
    return true;
}
int traceSurfaceAddress(int value,int extent,uint mode) {
    if(mode==3u)return clamp(value,0,extent-1);
    if(mode==4u)return value<0 || value>=extent?-1:value;
    if(mode==5u)return min(value<0?-value-1:value,extent-1);
    int period=extent*(mode==2u?2:1),at=value%period;if(at<0)at+=period;
    return mode==2u && at>=extent?period-at-1:at;
}
vec4 traceSurfaceTexel(uint record,uint stage,ivec2 pixel) {
    uint at=record+12u+stage*10u,state=tracePixels[at+3u];
    ivec2 extent=ivec2(tracePixels[at+1u],tracePixels[at+2u]);
    pixel.x=traceSurfaceAddress(pixel.x,extent.x,state&15u);
    pixel.y=traceSurfaceAddress(pixel.y,extent.y,(state>>4u)&15u);
    if(any(lessThan(pixel,ivec2(0))))return traceSurfaceColor(tracePixels[at+8u]);
    return unpackUnorm4x8(tracePixels[record+tracePixels[at]+uint(pixel.y)*uint(extent.x)+uint(pixel.x)]);
}
vec4 traceSurfaceSample(uint record,uint stage,vec2 rawUV) {
    vec2 base=rawUV*vec2(uintBitsToFloat(tracePixels[record+8u]),uintBitsToFloat(tracePixels[record+9u]))+
        vec2(uintBitsToFloat(tracePixels[record+10u]),uintBitsToFloat(tracePixels[record+11u]));
    uint at=record+12u+stage*10u;
    vec2 uv=base*vec2(uintBitsToFloat(tracePixels[at+4u]),uintBitsToFloat(tracePixels[at+5u]))+
        vec2(uintBitsToFloat(tracePixels[at+6u]),uintBitsToFloat(tracePixels[at+7u]));
    vec2 extent=vec2(tracePixels[at+1u],tracePixels[at+2u]);
    bool point=((tracePixels[at+3u]>>8u)&255u)==1u;
    vec2 coordinate=uv*extent-(point?vec2(0):vec2(.5)),weight=fract(coordinate);
    ivec2 pixel=ivec2(floor(coordinate));int taps=point?1:2;
    vec4 upper=vec4(0),lower=vec4(0);
    [[dont_unroll]] for(int row=0;row<taps;++row) {
        vec4 left=vec4(0),right=vec4(0);
        [[dont_unroll]] for(int column=0;column<taps;++column) {
            vec4 value=traceSurfaceTexel(record,stage,pixel+ivec2(column,row));
            if(column==0)left=value;else right=value;
        }
        vec4 value=point?left:mix(left,right,weight.x);
        if(row==0)upper=value;else lower=value;
    }
    return point?upper:mix(upper,lower,weight.y);
}
// Coverage has no material RGB or lighting evaluation. In particular a BVH
// candidate never needs the terrain's complete four-stage albedo expression.
float traceSurfaceAlphaTexel(uint record,uint stage,ivec2 pixel) {
    uint at=record+12u+stage*10u,state=tracePixels[at+3u];
    ivec2 extent=ivec2(tracePixels[at+1u],tracePixels[at+2u]);
    pixel.x=traceSurfaceAddress(pixel.x,extent.x,state&15u);
    pixel.y=traceSurfaceAddress(pixel.y,extent.y,(state>>4u)&15u);
    if(any(lessThan(pixel,ivec2(0))))return float(tracePixels[at+8u]>>24u)/255.0;
    return float(tracePixels[record+tracePixels[at]+uint(pixel.y)*uint(extent.x)+uint(pixel.x)]>>24u)/255.0;
}
float traceSurfaceAlphaSample(uint record,uint stage,vec2 rawUV) {
    vec2 base=rawUV*vec2(uintBitsToFloat(tracePixels[record+8u]),uintBitsToFloat(tracePixels[record+9u]))+
        vec2(uintBitsToFloat(tracePixels[record+10u]),uintBitsToFloat(tracePixels[record+11u]));
    uint at=record+12u+stage*10u;
    vec2 uv=base*vec2(uintBitsToFloat(tracePixels[at+4u]),uintBitsToFloat(tracePixels[at+5u]))+
        vec2(uintBitsToFloat(tracePixels[at+6u]),uintBitsToFloat(tracePixels[at+7u]));
    vec2 extent=vec2(tracePixels[at+1u],tracePixels[at+2u]);
    bool point=((tracePixels[at+3u]>>8u)&255u)==1u;
    vec2 coordinate=uv*extent-(point?vec2(0):vec2(.5)),weight=fract(coordinate);
    ivec2 pixel=ivec2(floor(coordinate));int taps=point?1:2;
    float upper=0.0,lower=0.0;
    [[dont_unroll]] for(int row=0;row<taps;++row) {
        float left=0.0,right=0.0;
        [[dont_unroll]] for(int column=0;column<taps;++column) {
            float value=traceSurfaceAlphaTexel(record,stage,pixel+ivec2(column,row));
            if(column==0)left=value;else right=value;
        }
        float value=point?left:mix(left,right,weight.x);
        if(row==0)upper=value;else lower=value;
    }
    return point?upper:mix(upper,lower,weight.y);
}
bool traceSurfaceStageKill(uint record,uint stage) {
    return (tracePixels[record+12u+stage*10u+9u]&4u)!=0u;
}
float traceSurfaceOutputAlpha(uint family,float first,float second,float third,float color0,float color1) {
    if(family==1u)return clamp((1.0-clamp(first,0.0,1.0))*max(second,0.0)+clamp(first,0.0,1.0)*max(third,0.0),0.0,1.0);
    float opaque=family==3u?color1:color0,transparent=family==3u?color0:color1;
    return (family>=4u?third:first)>=.5?opaque:transparent;
}
bool tracePrimaryCoverage(TraceMaterial material,vec2 rawUV,out float alpha) {
    if((material.texture.w&traceSurfaceFlag)==0u) {
        alpha=texture(traceSurfaceTexture0,rawUV*material.transform.xy+material.transform.zw).a;return true;
    }
    uint record;if(!traceSurfaceRecord(material,record))return false;
    uint family=tracePixels[record+1u];if(family<1u || family>5u)return false;
    vec2 base=rawUV*traceSurfaceSource.forwardTransforms[0].xy+traceSurfaceSource.forwardTransforms[0].zw;
    float first=1.0,second=1.0,third=1.0;
    if(family<4u || traceSurfaceStageKill(record,0u))first=texture(traceSurfaceTexture0,base).a;
    if(family==1u || traceSurfaceStageKill(record,1u))second=texture(traceSurfaceTexture1,
        base*traceSurfaceSource.forwardTransforms[1].xy+traceSurfaceSource.forwardTransforms[1].zw).a;
    if(family==1u || family>=4u) {
        vec2 coordinates=base*traceSurfaceSource.forwardTransforms[2].xy;
        if(family>=4u)coordinates+=traceSurfaceSource.forwardTransforms[2].zw;
        third=texture(traceSurfaceTexture2,coordinates).a;
    }
    if(traceSurfaceKill(record,0u,first) || traceSurfaceKill(record,1u,second) ||
       ((family==1u || family>=4u) && traceSurfaceKill(record,2u,third)))return false;
    if(family==1u && traceSurfaceStageKill(record,3u) && texture(traceSurfaceTexture3,
        base*traceSurfaceSource.forwardTransforms[2].zw).a==0.0)return false;
    float color0=float(traceSurfaceSource.program[3].y>>24u)/255.0;
    float color1=float(traceSurfaceSource.program[5].y>>24u)/255.0;
    if(family==4u)color1=float(traceSurfaceSource.program[5].z>>24u)/255.0;
    if(family==5u) {
        color0=float(traceSurfaceSource.program[3].w>>24u)/255.0;
        color1=float(traceSurfaceSource.program[4].y>>24u)/255.0;
    }
    alpha=traceSurfaceOutputAlpha(family,first,second,third,color0,color1);return true;
}
bool traceAtlasCoverage(TraceMaterial material,vec2 rawUV,out float alpha) {
    if((material.texture.w&traceSurfaceFlag)==0u) {
        vec2 uv=rawUV*material.transform.xy+material.transform.zw;
        uv.x=(material.texture.w&2u)!=0u?clamp(uv.x,0.0,1.0):fract(uv.x);
        uv.y=(material.texture.w&4u)!=0u?clamp(uv.y,0.0,1.0):fract(uv.y);
        uvec2 extent=max(material.texture.yz,uvec2(1)),at=min(uvec2(uv*vec2(extent)),extent-1u);
        alpha=float(tracePixels[material.texture.x+at.y*extent.x+at.x]>>24u)/255.0;return true;
    }
    uint record;if(!traceSurfaceRecord(material,record))return false;
    uint family=tracePixels[record+1u];if(family<1u || family>5u)return false;
    float first=1.0,second=1.0,third=1.0;
    [[dont_unroll]] for(uint stage=0u;stage<4u;++stage) {
        bool kill=traceSurfaceStageKill(record,stage);
        bool needed=kill || (family==1u && stage<3u) || (family<4u && stage==0u) || (family>=4u && stage==2u);
        if(!needed)continue;
        float value=traceSurfaceAlphaSample(record,stage,rawUV);
        if(kill && value==0.0)return false;
        if(stage==0u)first=value;else if(stage==1u)second=value;else if(stage==2u)third=value;
    }
    alpha=traceSurfaceOutputAlpha(family,first,second,third,float(tracePixels[record+3u]>>24u)/255.0,
        float(tracePixels[record+4u]>>24u)/255.0);return true;
}
bool traceAtlasSurface(TraceMaterial material,vec2 rawUV,out vec4 sourceColor) {
    if((material.texture.w&traceSurfaceFlag)==0u) {
        vec2 uv=rawUV*material.transform.xy+material.transform.zw;
        uv.x=(material.texture.w&2u)!=0u?clamp(uv.x,0.0,1.0):fract(uv.x);
        uv.y=(material.texture.w&4u)!=0u?clamp(uv.y,0.0,1.0):fract(uv.y);
        uvec2 extent=max(material.texture.yz,uvec2(1)),at=min(uvec2(uv*vec2(extent)),extent-1u);
        sourceColor=unpackUnorm4x8(tracePixels[material.texture.x+at.y*extent.x+at.x]);return true;
    }
    uint record;if(!traceSurfaceRecord(material,record))return false;
    uint family=tracePixels[record+1u];if(family<1u || family>5u)return false;
    vec4 first=vec4(0),second=vec4(0),third=vec4(0),fourth=vec4(0);
    uint stages=tracePixels[record+7u];
    [[dont_unroll]] for(uint stage=0u;stage<4u;++stage) {
        if((stages&(1u<<stage))==0u)continue;
        vec4 value=traceSurfaceSample(record,stage,rawUV);
        if(traceSurfaceKill(record,stage,value.a))return false;
        if(stage==0u)first=value;else if(stage==1u)second=value;else if(stage==2u)third=value;else fourth=value;
    }
    sourceColor=traceSurfaceEvaluate(family,first,second,third,fourth,
        traceSurfaceColor(tracePixels[record+3u]),traceSurfaceColor(tracePixels[record+4u]),
        traceSurfaceColor(tracePixels[record+5u]),traceSurfaceColor(tracePixels[record+6u]));return true;
}
#endif
