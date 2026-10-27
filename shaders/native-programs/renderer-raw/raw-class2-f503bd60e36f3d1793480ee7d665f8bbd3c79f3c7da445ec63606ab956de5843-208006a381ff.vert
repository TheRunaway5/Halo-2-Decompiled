#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in int v2_cmp;
layout(location=3) in int v3_cmp;
layout(location=4) in int v4_cmp;
layout(set=1,binding=0,std140) uniform MenuWorldVertexUniforms {
    vec4 c[192];
    vec4 clipRange;
    vec2 fogParam;
    vec2 surfaceSize;
    vec4 attributes[16];
} source;
layout(location=0) out vec4 v_vtxD0;
layout(location=1) out vec4 v_vtxD1;
layout(location=2) out vec4 v_vtxB0;
layout(location=3) out vec4 v_vtxB1;
layout(location=4) out float v_vtxFog;
layout(location=5) out vec4 v_vtxT0;
layout(location=6) out vec4 v_vtxT1;
layout(location=7) out vec4 v_vtxT2;
layout(location=8) out vec4 v_vtxT3;
layout(location=9) flat out vec4 v_vtxPos0;
layout(location=10) flat out vec4 v_vtxPos1;
layout(location=11) flat out vec4 v_vtxPos2;
layout(location=12) flat out float v_triMZ;
vec4 vpInput[16],vpOut[16];
vec4 vpConstant(uint index) {return source.c[index];}
vec4 h2MenuMultiply(vec4 left,vec4 right) {
    vec4 product=left*right;
    return vec4((left.x==0.0 || right.x==0.0)?0.0:product.x,
                (left.y==0.0 || right.y==0.0)?0.0:product.y,
                (left.z==0.0 || right.z==0.0)?0.0:product.z,
                (left.w==0.0 || right.w==0.0)?0.0:product.w);
}
vec4 h2MenuAdd(vec4 left,vec4 right) {return left+right;}
vec4 h2MenuMultiplyAdd(vec4 left,vec4 right,vec4 addend) {
    return h2MenuMultiply(left,right)+addend;
}
vec4 h2MenuDot3(vec4 left,vec4 right) {return vec4(dot(left.xyz,right.xyz));}
vec4 h2MenuHomogeneousDot(vec4 left,vec4 right) {
    return vec4(dot(vec4(left.xyz,1.0),right));
}
vec4 h2MenuDot4(vec4 left,vec4 right) {return vec4(dot(left,right));}
vec4 h2MenuMove(vec4 value) {return value;}
vec4 h2MenuReciprocalClamp(vec4 value) {
    float reciprocal=1.0/value.x;
    float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
    uint signedMagnitude=(floatBitsToUint(reciprocal)&0x80000000u)|(floatBitsToUint(magnitude)&0x7fffffffu);
    return vec4(uintBitsToFloat(signedMagnitude));
}
vec4 h2MenuReciprocalSqrt(vec4 value) {return vec4(inversesqrt(abs(value.x)));}
float sourceClampW(float t) {
    return t>0.0 || floatBitsToUint(t)==0u
        ? clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u))
        : clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
vec4 sourcePackedNormal(int p) {
    return vec4(float(bitfieldExtract(p,0,11))/1023.0,
                float(bitfieldExtract(p,11,11))/1023.0,
                float(bitfieldExtract(p,22,10))/511.0,1.0);
}
vec4 sourceColor(vec4 value) {
    return clamp(mix(value,vec4(1.0),isnan(value)),0.0,1.0);
}
void main() {
    vpInput[0]=v0;
    vpInput[1]=v1;
    vpInput[2]=sourcePackedNormal(v2_cmp);
    int h2MenuAddress=0;
    vec4 h2MenuTemp0=vec4(0.0);
    vec4 h2MenuTemp1=vec4(0.0);
    vec4 h2MenuTemp2=vec4(0.0);
    vec4 h2MenuTemp3=vec4(0.0);
    vec4 h2MenuTemp4=vec4(0.0);
    vec4 h2MenuTemp5=vec4(0.0);
    vec4 h2MenuTemp6=vec4(0.0);
    vec4 h2MenuTemp7=vec4(0.0);
    vec4 h2MenuTemp8=vec4(0.0);
    vec4 h2MenuTemp9=vec4(0.0);
    vec4 h2MenuTemp10=vec4(0.0);
    vec4 h2MenuTemp11=vec4(0.0);
    vpOut[0]=vec4(0.0,0.0,0.0,1.0);
    vpOut[1]=vec4(0.0,0.0,0.0,1.0);
    vpOut[2]=vec4(0.0,0.0,0.0,1.0);
    vpOut[3]=vec4(0.0,0.0,0.0,1.0);
    vpOut[4]=vec4(0.0,0.0,0.0,1.0);
    vpOut[5]=vec4(0.0,0.0,0.0,1.0);
    vpOut[6]=vec4(0.0,0.0,0.0,1.0);
    vpOut[7]=vec4(0.0,0.0,0.0,1.0);
    vpOut[8]=vec4(0.0,0.0,0.0,1.0);
    vpOut[9]=vec4(0.0,0.0,0.0,1.0);
    vpOut[10]=vec4(0.0,0.0,0.0,1.0);
    vpOut[11]=vec4(0.0,0.0,0.0,1.0);
    vpOut[12]=vec4(0.0,0.0,0.0,1.0);
    vpOut[13]=vec4(0.0,0.0,0.0,1.0);
    vpOut[14]=vec4(0.0,0.0,0.0,1.0);
    vpOut[15]=vec4(0.0,0.0,0.0,1.0);
    // renderer instruction 0
    vec4 h2RendererA0=vpInput[1].xxxx;
    vec4 h2RendererB0=vpConstant(7u).wwww;
    vec4 h2RendererC0=vec4(0.0);
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    h2RendererM0=h2MenuMultiply(h2RendererA0,h2RendererB0);
    h2MenuTemp0.x=h2RendererM0.x;
    // renderer instruction 1
    vec4 h2RendererA1=vpInput[0].xyzz;
    vec4 h2RendererB1=vpConstant(170u).xyzz;
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    h2RendererM1=h2MenuMultiply(h2RendererA1,h2RendererB1);
    h2MenuTemp10.xyz=h2RendererM1.xyz;
    // renderer instruction 2
    vec4 h2RendererA2=h2MenuTemp0.xxxx;
    vec4 h2RendererB2=vec4(0.0);
    vec4 h2RendererC2=vpConstant(15u).yyyy;
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    h2RendererL2=h2RendererC2;
    h2MenuAddress=int(floor(h2RendererA2.x));
    vpOut[10].xy=h2RendererL2.xy;
    // renderer instruction 3
    vec4 h2RendererA3=h2MenuTemp10.xyzz;
    vec4 h2RendererB3=vec4(0.0);
    vec4 h2RendererC3=vpConstant(171u).xyzz;
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    h2RendererM3=h2RendererA3+h2RendererC3;
    h2MenuTemp10.xyz=h2RendererM3.xyz;
    // renderer instruction 4
    vec4 h2RendererA4=vpConstant(uint(50+h2MenuAddress)).xyzw;
    vec4 h2RendererB4=vec4(0.0);
    vec4 h2RendererC4=vec4(0.0);
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    h2RendererM4=h2RendererA4;
    h2MenuTemp4.xyzw=h2RendererM4.xyzw;
    // renderer instruction 5
    vec4 h2RendererA5=vpConstant(uint(51+h2MenuAddress)).xyzw;
    vec4 h2RendererB5=vec4(0.0);
    vec4 h2RendererC5=vec4(0.0);
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    h2RendererM5=h2RendererA5;
    h2MenuTemp5.xyzw=h2RendererM5.xyzw;
    // renderer instruction 6
    vec4 h2RendererA6=vpConstant(uint(52+h2MenuAddress)).xyzw;
    vec4 h2RendererB6=vec4(0.0);
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    h2RendererM6=h2RendererA6;
    h2MenuTemp6.xyzw=h2RendererM6.xyzw;
    // renderer instruction 7
    vec4 h2RendererA7=h2MenuTemp10.xyzw;
    vec4 h2RendererB7=h2MenuTemp4.xyzw;
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    vec4 h2RendererProduct7=h2MenuMultiply(h2RendererA7,h2RendererB7);
    precise float h2RendererDot7=(h2RendererProduct7.x+h2RendererProduct7.y)+h2RendererProduct7.z;
    h2RendererDot7+=h2RendererB7.w;
    h2RendererM7=vec4(h2RendererDot7);
    h2MenuTemp0.x=h2RendererM7.x;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp10.xyzw;
    vec4 h2RendererB8=h2MenuTemp5.xyzw;
    vec4 h2RendererC8=vec4(0.0);
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    vec4 h2RendererProduct8=h2MenuMultiply(h2RendererA8,h2RendererB8);
    precise float h2RendererDot8=(h2RendererProduct8.x+h2RendererProduct8.y)+h2RendererProduct8.z;
    h2RendererDot8+=h2RendererB8.w;
    h2RendererM8=vec4(h2RendererDot8);
    h2MenuTemp0.y=h2RendererM8.y;
    // renderer instruction 9
    vec4 h2RendererA9=h2MenuTemp10.xyzw;
    vec4 h2RendererB9=h2MenuTemp6.xyzw;
    vec4 h2RendererC9=vec4(0.0);
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    vec4 h2RendererProduct9=h2MenuMultiply(h2RendererA9,h2RendererB9);
    precise float h2RendererDot9=(h2RendererProduct9.x+h2RendererProduct9.y)+h2RendererProduct9.z;
    h2RendererDot9+=h2RendererB9.w;
    h2RendererM9=vec4(h2RendererDot9);
    h2MenuTemp0.z=h2RendererM9.z;
    // renderer instruction 10
    vec4 h2RendererA10=vpInput[2].xyzz;
    vec4 h2RendererB10=h2MenuTemp4.xyzz;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererM10=vec4(h2RendererDot10);
    h2MenuTemp11.x=h2RendererM10.x;
    // renderer instruction 11
    vec4 h2RendererA11=h2MenuTemp0.xyzw;
    vec4 h2RendererB11=vpConstant(0u).xyzw;
    vec4 h2RendererC11=vec4(0.0);
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererDot11+=h2RendererB11.w;
    h2RendererM11=vec4(h2RendererDot11);
    vpOut[0].x=h2RendererM11.x;
    // renderer instruction 12
    vec4 h2RendererA12=h2MenuTemp0.xyzw;
    vec4 h2RendererB12=vpConstant(1u).xyzw;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererB12.w;
    h2RendererM12=vec4(h2RendererDot12);
    vpOut[0].y=h2RendererM12.y;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp0.xyzw;
    vec4 h2RendererB13=vpConstant(2u).xyzw;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    vec4 h2RendererProduct13=h2MenuMultiply(h2RendererA13,h2RendererB13);
    precise float h2RendererDot13=(h2RendererProduct13.x+h2RendererProduct13.y)+h2RendererProduct13.z;
    h2RendererDot13+=h2RendererB13.w;
    h2RendererM13=vec4(h2RendererDot13);
    vpOut[0].z=h2RendererM13.z;
    // renderer instruction 14
    vec4 h2RendererA14=h2MenuTemp0.xyzw;
    vec4 h2RendererB14=vpConstant(3u).xyzw;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    vec4 h2RendererProduct14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    precise float h2RendererDot14=(h2RendererProduct14.x+h2RendererProduct14.y)+h2RendererProduct14.z;
    h2RendererDot14+=h2RendererB14.w;
    h2RendererM14=vec4(h2RendererDot14);
    vpOut[0].w=h2RendererM14.w;
    // renderer instruction 15
    vec4 h2RendererA15=vpInput[2].xyzz;
    vec4 h2RendererB15=h2MenuTemp5.xyzz;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    vec4 h2RendererProduct15=h2MenuMultiply(h2RendererA15,h2RendererB15);
    precise float h2RendererDot15=(h2RendererProduct15.x+h2RendererProduct15.y)+h2RendererProduct15.z;
    h2RendererM15=vec4(h2RendererDot15);
    h2MenuTemp11.y=h2RendererM15.y;
    // renderer instruction 16
    vec4 h2RendererA16=vpInput[2].xyzz;
    vec4 h2RendererB16=h2MenuTemp6.xyzz;
    vec4 h2RendererC16=vpOut[0].wwww;
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    vec4 h2RendererProduct16=h2MenuMultiply(h2RendererA16,h2RendererB16);
    precise float h2RendererDot16=(h2RendererProduct16.x+h2RendererProduct16.y)+h2RendererProduct16.z;
    h2RendererM16=vec4(h2RendererDot16);
    float h2RendererReciprocal16=1.0/h2RendererC16.x;
    if(h2RendererReciprocal16>0.0 || floatBitsToUint(h2RendererReciprocal16)==0u)
        h2RendererReciprocal16=clamp(h2RendererReciprocal16,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal16)) h2RendererReciprocal16=clamp(h2RendererReciprocal16,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL16=vec4(h2RendererReciprocal16);
    h2MenuTemp11.z=h2RendererM16.z;
    h2MenuTemp1.w=h2RendererL16.w;
    // renderer instruction 17
    vec4 h2RendererA17=vpOut[0].xyzz;
    vec4 h2RendererB17=vpConstant(10u).xyzz;
    vec4 h2RendererC17=vec4(0.0);
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    h2RendererM17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    vpOut[0].xyz=h2RendererM17.xyz;
    // renderer instruction 18
    vec4 h2RendererA18=h2MenuTemp11.xyzz;
    vec4 h2RendererB18=vpConstant(5u).wwww;
    vec4 h2RendererC18=vpConstant(5u).wwww;
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    h2RendererM18=h2MenuMultiply(h2RendererA18,h2RendererB18)+h2RendererC18;
    vpOut[3].xyz=h2RendererM18.xyz;
    // renderer instruction 19
    vec4 h2RendererA19=vpOut[0].xyzz;
    vec4 h2RendererB19=h2MenuTemp1.wwww;
    vec4 h2RendererC19=vpConstant(11u).xyzz;
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2MenuMultiply(h2RendererA19,h2RendererB19)+h2RendererC19;
    vpOut[0].xyz=h2RendererM19.xyz;
    vec4 screen=vpOut[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=sourceClampW(screen.w);
    vec4 clip=screen;
    clip.xy=(2.0*clip.xy-source.surfaceSize)/source.surfaceSize;
    clip.z=clip.z/source.clipRange.y;
    clip.xyz=clip.xyz*clip.w;
    gl_Position=clip;
    gl_PointSize=1.0;
    v_vtxD0=sourceColor(vpOut[3]);v_vtxD1=sourceColor(vpOut[4]);
    v_vtxB0=sourceColor(vpOut[7]);v_vtxB1=sourceColor(vpOut[8]);
    float distance=vpOut[5].x;
    float fog=source.fogParam.x+distance*source.fogParam.y;
    fog=fog-1.0;
    v_vtxFog=isinf(distance)?1.0:clamp(isnan(fog)?1.0:fog,-uintBitsToFloat(2139095039u),uintBitsToFloat(2139095039u));
    v_vtxT0=vpOut[9];v_vtxT1=vpOut[10];v_vtxT2=vpOut[11];v_vtxT3=vpOut[12];
    v_vtxPos0=screen;v_vtxPos1=screen;v_vtxPos2=screen;v_triMZ=0.0;
}
