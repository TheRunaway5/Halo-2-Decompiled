// Execute authored NV2A transforms, including screen-space position. Geometry
// is decoded/skinned on the CPU before it enters the original program.
#include "render_view.glsl"
vec4 vpTemp[13], vpOut[16], vpInput[16];
#if defined(H2_SOURCE_VERTEX_DECLARATION) && !defined(H2_VERTEX_REFERENCE_INPUTS)
#include "source_vertex_inputs.glsl"
#endif
#ifdef H2_EXTENDED_VERTEX_BANK
int vpAddress;
#endif
vec4 vpProduct(vec4 a,vec4 b) {
    vec4 result;
    // NV_vertex_program arithmetic: zero * infinity/NaN is zero.
    for(uint i=0u;i<4u;++i)result[i]=(a[i]==0.0 || b[i]==0.0)?0.0:a[i]*b[i];
    return result;
}
#ifndef H2_VERTEX_REFERENCE_CONSTANTS
vec4 vpConstant(uint index) {
#ifdef H2_EXTENDED_VERTEX_BANK
    if(index<12u && vertexBank.range.w!=0u)
        return vertexBank.constants[index];
#endif
#ifdef H2_SOURCE_VIEW_SHADER_CONSTANTS
    if(index<7u && material.sourceCameraControl.x!=0u) {
        if(camera.eyePosition.w==3.0)return material.sourceCameraReflection[index];
        if(index<4u && camera.eyePosition.w==2.0)return material.sourceCameraForeground[index];
        return material.sourceCameraWorld[index];
    }
#endif
    // Original clip Y points up; Vulkan's negative projection Y is applied
    // only to gl_Position. Retail c8..11 perform their own pixel conversion.
    if(index<4u)return transpose(camera.viewProjection)[index]*(index==1u?-1.0:1.0);
    if(index==4u)return vec4(normalize(vec3(camera.viewProjection[0][0],camera.viewProjection[1][0],camera.viewProjection[2][0])),1);
    if(index==5u)return vec4(-normalize(vec3(camera.viewProjection[0][1],camera.viewProjection[1][1],camera.viewProjection[2][1])),0.5);
    // 2FD90 stores the normalized NEGATED camera forward at +0x50; 12FA0
    // uploads it as c6. Reversing this flips the authored face/edge gel colors.
    if(index==6u)return vec4(-camera.eyeForward.xyz,2);
    if(index==7u)return vec4(camera.eyePosition.xyz,767.8125);
    if(index>=8u && index<12u)return retailViewportConstant(index,camera.viewport,1.0,false,false,0u);
    if(index>=12u && index<18u && camera.eyeForward.w!=0.0)return material.fogConstants[index-12u];
    if(index>=18u && index<34u)return material.vertexConstants[index-18u];
    if(index==172u)return vec4(1,1,0,0); // UVs already decompressed by the cache reader.
#ifdef H2_EXTENDED_VERTEX_BANK
    if(index>=vertexBank.range.x && index-vertexBank.range.x<vertexBank.range.y)
        return vertexBank.constants[index-vertexBank.range.x];
#endif
    return vec4(0);
}
#endif
vec4 vpSource(uvec4 t,uint which) {
    uint mux,reg,negative;uvec4 swizzle;
    if(which==0u){mux=(t.z>>26u)&3u;reg=t.z>>28u;negative=(t.y>>8u)&1u;swizzle=uvec4(t.y>>6u,t.y>>4u,t.y>>2u,t.y)&3u;}
    else if(which==1u){mux=(t.z>>11u)&3u;reg=(t.z>>13u)&15u;negative=(t.z>>25u)&1u;swizzle=uvec4(t.z>>23u,t.z>>21u,t.z>>19u,t.z>>17u)&3u;}
    else{mux=(t.w>>28u)&3u;reg=(t.z&3u)*4u+(t.w>>30u);negative=(t.z>>10u)&1u;swizzle=uvec4(t.z>>8u,t.z>>6u,t.z>>4u,t.z>>2u)&3u;}
    vec4 value=vec4(0);
    if(mux==1u)value=reg==12u?vpOut[0]:vpTemp[min(reg,12u)];
    if(mux==2u)value=vpInput[(t.y>>9u)&15u];
    if(mux==3u) {
#ifdef H2_EXTENDED_VERTEX_BANK
        int constantIndex=int((t.y>>13u)&255u)+((t.w&2u)!=0u?vpAddress:0);
        if(constantIndex>=0 && constantIndex<192)value=vpConstant(uint(constantIndex));
#else
        value=vpConstant((t.y>>13u)&255u);
#endif
    }
    return (negative!=0u?-1.0:1.0)*vec4(value[swizzle.x],value[swizzle.y],value[swizzle.z],value[swizzle.w]);
}
void vpWrite(inout vec4 target,vec4 value,uint mask) {
    for(uint axis=0u;axis<4u;++axis)if((mask&(8u>>axis))!=0u)target[axis]=value[axis];
}
void runVertexProgram() {
#ifdef H2_EXTENDED_VERTEX_BANK
    vpAddress=0;
#endif
    for(uint i=0u;i<13u;++i)vpTemp[i]=vec4(0);
    for(uint i=0u;i<16u;++i){vpOut[i]=vec4(0,0,0,1);vpInput[i]=material.vertexAttributes[i];}
#ifndef H2_VERTEX_REFERENCE_INPUTS
#ifdef H2_SOURCE_VERTEX_DECLARATION
    if(!loadSourceWorldVertexInputs()) {
#endif
    vpInput[0]=vec4(position,1);vpInput[1]=vec4(texcoord,0,1);
    vpInput[2]=vec4(normal,1);vpInput[3]=vec4(tangent,1);vpInput[4]=vec4(binormal,1);
    vpInput[5]=vec4(incident,1);vpInput[6]=vec4(lightmapCoord,0,1);
#ifdef H2_SOURCE_VIEW_SHADER_CONSTANTS
    // Original shadow world0 declarations differ by phase: generation v1
    // is UV, application v1 is the decoded NORMPACKED3 surface normal.
    // The explicit queued shadow phase supplies this marker; normal world
    // material declarations and generation's UV input stay unchanged.
    if(material.flags.z==10u && material.sourceCameraControl.w==1u)
        vpInput[1]=vec4(normal,1);
#endif
#ifdef H2_EXTENDED_VERTEX_BANK
    if(vertexBank.range.z!=0u) {
        vpInput[5]=vec4(prtCluster,0,0,1);
        vpInput[6]=prtWeights;
    }
#endif
    // lightmap_pervertex_env reads v6 as incident radiance/color. Its stream
    // declaration differs from the per-pixel programs, which read v6 as UVs.
#ifdef RETAIL_VERTEX_RADIANCE
    if(material.flags.z==4u)vpInput[6]=vec4(RETAIL_VERTEX_RADIANCE,1);
#endif
#ifdef H2_SOURCE_VERTEX_DECLARATION
    }
#endif
#endif
#ifdef H2_SOURCE_IMMEDIATE_COLOR11
    if(material.flags.z==12u)vpInput[11]=vec4(RETAIL_VERTEX_RADIANCE,sourceColorAlpha);
#endif
    for(uint i=0u;i<material.flags.w;++i) {
        uvec4 t=material.vertexInstructions[i];uint mac=(t.y>>21u)&15u,ilu=(t.y>>25u)&7u;
        vec4 a=vpSource(t,0u),b=vpSource(t,1u),c=vpSource(t,2u);
#ifdef H2_VERTEX_XEMU_ARITHMETIC
        // The captured particle program uses OpDot and does not decorate
        // arithmetic with NoContraction. Preserve that shader contract for
        // this explicitly selected path; other programs keep their ordering.
        vec4 m=vec4(0),l=vec4(0);
#else
        precise vec4 m=vec4(0),l=vec4(0);
#endif
        if(mac==1u)m=a;
        if(mac==2u)m=vpProduct(a,b);
        if(mac==3u)m=a+c;
        if(mac==4u)m=vpProduct(a,b)+c;
        if(mac>=5u && mac<=7u) {
#ifdef H2_VERTEX_XEMU_ARITHMETIC
            float value=mac==5u?dot(a.xyz,b.xyz):
                (mac==6u?dot(vec4(a.xyz,1.0),b):dot(a,b));
#else
            vec4 product=vpProduct(a,b);
            precise float value=(product.x+product.y)+product.z;
            if(mac==6u)value+=b.w;
            if(mac==7u)value+=product.w;
#endif
            m=vec4(value);
        }
        if(mac==8u)m=vec4(1,vpProduct(a,b).y,a.z,b.w);
        if(mac==9u)m=min(a,b);
        if(mac==10u)m=max(a,b);
        if(mac==11u)m=vec4(lessThan(a,b));
        if(mac==12u)m=vec4(greaterThanEqual(a,b));
        if(ilu==1u)l=c;
        if(ilu==2u)l=vec4(1.0/c.x);
        if(ilu==3u) {
            // RCC clamps the reciprocal's signed value, including +/-zero
            // operands and the signed zero produced by +/-infinity. sign(c.x)
            // would collapse a camera-plane vertex instead of retaining RCC.
            float reciprocal=1.0/c.x;
            if(reciprocal>0.0 || floatBitsToUint(reciprocal)==0u)
                reciprocal=clamp(reciprocal,5.421010862427522e-20,1.8446744073709552e19);
            else if(!isnan(reciprocal))
                reciprocal=clamp(reciprocal,-1.8446744073709552e19,-5.421010862427522e-20);
            l=vec4(reciprocal);
        }
        if(ilu==4u)l=vec4(inversesqrt(abs(c.x)));
        if(ilu==5u)l=vec4(exp2(floor(c.x)),fract(c.x),exp2(c.x),1);
        if(ilu==6u){float v=log2(abs(c.x));l=vec4(floor(v),abs(c.x)/exp2(floor(v)),v,1);}
        if(ilu==7u)l=vec4(1,max(c.x,0),c.x>0.0?pow(max(c.y,0),clamp(c.w,-128.0,128.0)):0,1);
#ifdef H2_EXTENDED_VERTEX_BANK
        if(mac==13u) {
#ifdef H2_VERTEX_XEMU_ARITHMETIC
            // Captured Xemu Vulkan _ARL adds this bias before Floor.
            vpAddress=int(floor(a.x+0.001));
#else
            vpAddress=int(floor(a.x));
#endif
        }
#endif
        uint reg=(t.w>>20u)&15u,mm=(t.w>>24u)&15u,im=(t.w>>16u)&15u,om=(t.w>>12u)&15u;
        if(mac!=0u && mac!=13u && mm!=0u && !(ilu!=0u && reg==1u)){
            if(reg==12u)vpWrite(vpOut[0],m,mm);else vpWrite(vpTemp[reg],m,mm);}
        if(ilu!=0u && im!=0u){uint ir=mac!=0u?1u:reg;
            if(ir==12u)vpWrite(vpOut[0],l,im);else vpWrite(vpTemp[ir],l,im);}
        if(om!=0u)vpWrite(vpOut[(t.w>>3u)&15u],(t.w&4u)!=0u?l:m,om);
    }
}
