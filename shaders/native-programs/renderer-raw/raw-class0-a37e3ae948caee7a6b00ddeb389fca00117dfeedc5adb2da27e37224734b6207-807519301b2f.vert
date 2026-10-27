#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in int v2_cmp;
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
    vpInput[10]=source.attributes[10];
    vpInput[11]=source.attributes[11];
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
    vec4 h2RendererA0=vpInput[0].xyzw;
    vec4 h2RendererB0=vpConstant(13u).xyzw;
    vec4 h2RendererC0=vec4(0.0);
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    vec4 h2RendererProduct0=h2MenuMultiply(h2RendererA0,h2RendererB0);
    precise float h2RendererDot0=(h2RendererProduct0.x+h2RendererProduct0.y)+h2RendererProduct0.z;
    h2RendererDot0+=h2RendererB0.w;
    h2RendererM0=vec4(h2RendererDot0);
    h2MenuTemp8.x=h2RendererM0.x;
    // renderer instruction 1
    vec4 h2RendererA1=vpInput[0].xyzw;
    vec4 h2RendererB1=vpConstant(14u).xyzw;
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    vec4 h2RendererProduct1=h2MenuMultiply(h2RendererA1,h2RendererB1);
    precise float h2RendererDot1=(h2RendererProduct1.x+h2RendererProduct1.y)+h2RendererProduct1.z;
    h2RendererDot1+=h2RendererB1.w;
    h2RendererM1=vec4(h2RendererDot1);
    h2MenuTemp8.y=h2RendererM1.y;
    // renderer instruction 2
    vec4 h2RendererA2=vpInput[0].xyzw;
    vec4 h2RendererB2=vpConstant(12u).xyzw;
    vec4 h2RendererC2=vec4(0.0);
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    vec4 h2RendererProduct2=h2MenuMultiply(h2RendererA2,h2RendererB2);
    precise float h2RendererDot2=(h2RendererProduct2.x+h2RendererProduct2.y)+h2RendererProduct2.z;
    h2RendererDot2+=h2RendererB2.w;
    h2RendererM2=vec4(h2RendererDot2);
    h2MenuTemp8.z=h2RendererM2.z;
    // renderer instruction 3
    vec4 h2RendererA3=vpConstant(4u).wwww;
    vec4 h2RendererB3=vec4(0.0);
    vec4 h2RendererC3=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    h2RendererM3=h2RendererA3+h2RendererC3;
    h2MenuTemp8.xyz=h2RendererM3.xyz;
    // renderer instruction 4
    vec4 h2RendererA4=vpConstant(7u).xyzz;
    vec4 h2RendererB4=vec4(0.0);
    vec4 h2RendererC4=-vpInput[0].xyzz;
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    h2RendererM4=h2RendererA4+h2RendererC4;
    h2MenuTemp5.xyz=h2RendererM4.xyz;
    // renderer instruction 5
    vec4 h2RendererA5=h2MenuTemp8.xyzz;
    vec4 h2RendererB5=vpConstant(15u).yyyy;
    vec4 h2RendererC5=vec4(0.0);
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    h2RendererM5=max(h2RendererA5,h2RendererB5);
    h2MenuTemp8.xyz=h2RendererM5.xyz;
    // renderer instruction 6
    vec4 h2RendererA6=h2MenuTemp8.xyzz;
    vec4 h2RendererB6=h2MenuTemp8.xyzz;
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    h2RendererM6=h2MenuMultiply(h2RendererA6,h2RendererB6);
    h2MenuTemp8.xyz=h2RendererM6.xyz;
    // renderer instruction 7
    vec4 h2RendererA7=h2MenuTemp8.xyzz;
    vec4 h2RendererB7=vpConstant(4u).wwww;
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    h2RendererM7=min(h2RendererA7,h2RendererB7);
    h2MenuTemp8.xyz=h2RendererM7.xyz;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp8.xxxx;
    vec4 h2RendererB8=vec4(0.0);
    vec4 h2RendererC8=h2MenuTemp8.yyyy;
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    h2RendererM8=h2RendererA8+h2RendererC8;
    h2MenuTemp8.x=h2RendererM8.x;
    // renderer instruction 9
    vec4 h2RendererA9=h2MenuTemp5.xyzz;
    vec4 h2RendererB9=h2MenuTemp5.xyzz;
    vec4 h2RendererC9=vec4(0.0);
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    vec4 h2RendererProduct9=h2MenuMultiply(h2RendererA9,h2RendererB9);
    precise float h2RendererDot9=(h2RendererProduct9.x+h2RendererProduct9.y)+h2RendererProduct9.z;
    h2RendererM9=vec4(h2RendererDot9);
    h2MenuTemp5.w=h2RendererM9.w;
    // renderer instruction 10
    vec4 h2RendererA10=h2MenuTemp8.xxxx;
    vec4 h2RendererB10=vpConstant(4u).wwww;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    h2RendererM10=min(h2RendererA10,h2RendererB10);
    h2MenuTemp8.x=h2RendererM10.x;
    // renderer instruction 11
    vec4 h2RendererA11=vpConstant(4u).wwww;
    vec4 h2RendererB11=vec4(0.0);
    vec4 h2RendererC11=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    h2RendererM11=h2RendererA11+h2RendererC11;
    h2MenuTemp8.xyz=h2RendererM11.xyz;
    // renderer instruction 12
    vec4 h2RendererA12=vpInput[1].xyyy;
    vec4 h2RendererB12=vpConstant(172u).xyyy;
    vec4 h2RendererC12=vpConstant(172u).zwww;
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    h2RendererM12=h2MenuMultiply(h2RendererA12,h2RendererB12)+h2RendererC12;
    h2MenuTemp9.xy=h2RendererM12.xy;
    // renderer instruction 13
    vec4 h2RendererA13=vpConstant(15u).yyyy;
    vec4 h2RendererB13=vec4(0.0);
    vec4 h2RendererC13=h2MenuTemp5.wwww;
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    h2RendererM13=h2RendererA13;
    h2RendererL13=vec4(inversesqrt(abs(h2RendererC13.x)));
    h2MenuTemp9.z=h2RendererM13.z;
    h2MenuTemp1.w=h2RendererL13.w;
    // renderer instruction 14
    vec4 h2RendererA14=h2MenuTemp8.xyzz;
    vec4 h2RendererB14=h2MenuTemp8.xyzz;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    h2RendererM14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    h2MenuTemp8.xyz=h2RendererM14.xyz;
    // renderer instruction 15
    vec4 h2RendererA15=h2MenuTemp5.xyzz;
    vec4 h2RendererB15=h2MenuTemp1.wwww;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    h2RendererM15=h2MenuMultiply(h2RendererA15,h2RendererB15);
    h2MenuTemp5.xyz=h2RendererM15.xyz;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp8.yyyy;
    vec4 h2RendererB16=vec4(0.0);
    vec4 h2RendererC16=-h2MenuTemp8.xxxx;
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=h2RendererA16+h2RendererC16;
    h2MenuTemp8.y=h2RendererM16.y;
    // renderer instruction 17
    vec4 h2RendererA17=vpInput[2].xyzz;
    vec4 h2RendererB17=h2MenuTemp5.xyzz;
    vec4 h2RendererC17=vec4(0.0);
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    vec4 h2RendererProduct17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    precise float h2RendererDot17=(h2RendererProduct17.x+h2RendererProduct17.y)+h2RendererProduct17.z;
    h2RendererM17=vec4(h2RendererDot17);
    h2MenuTemp11.x=h2RendererM17.x;
    // renderer instruction 18
    vec4 h2RendererA18=vpConstant(15u).xxxx;
    vec4 h2RendererB18=h2MenuTemp8.yyyy;
    vec4 h2RendererC18=h2MenuTemp8.xxxx;
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    h2RendererM18=h2MenuMultiply(h2RendererA18,h2RendererB18)+h2RendererC18;
    h2MenuTemp8.w=h2RendererM18.w;
    // renderer instruction 19
    vec4 h2RendererA19=-vpInput[0].xyzz;
    vec4 h2RendererB19=vec4(0.0);
    vec4 h2RendererC19=vpConstant(7u).xyzz;
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2RendererA19+h2RendererC19;
    h2MenuTemp5.xyz=h2RendererM19.xyz;
    // renderer instruction 20
    vec4 h2RendererA20=h2MenuTemp11.xxxx;
    vec4 h2RendererB20=vpConstant(15u).yyyy;
    vec4 h2RendererC20=vec4(0.0);
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    h2RendererM20=max(h2RendererA20,h2RendererB20);
    h2MenuTemp11.x=h2RendererM20.x;
    // renderer instruction 21
    vec4 h2RendererA21=vpInput[0].xyzw;
    vec4 h2RendererB21=vpConstant(0u).xyzw;
    vec4 h2RendererC21=vec4(0.0);
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    vec4 h2RendererProduct21=h2MenuMultiply(h2RendererA21,h2RendererB21);
    precise float h2RendererDot21=(h2RendererProduct21.x+h2RendererProduct21.y)+h2RendererProduct21.z;
    h2RendererDot21+=h2RendererB21.w;
    h2RendererM21=vec4(h2RendererDot21);
    vpOut[0].x=h2RendererM21.x;
    // renderer instruction 22
    vec4 h2RendererA22=vpInput[0].xyzw;
    vec4 h2RendererB22=vpConstant(1u).xyzw;
    vec4 h2RendererC22=vec4(0.0);
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    vec4 h2RendererProduct22=h2MenuMultiply(h2RendererA22,h2RendererB22);
    precise float h2RendererDot22=(h2RendererProduct22.x+h2RendererProduct22.y)+h2RendererProduct22.z;
    h2RendererDot22+=h2RendererB22.w;
    h2RendererM22=vec4(h2RendererDot22);
    vpOut[0].y=h2RendererM22.y;
    // renderer instruction 23
    vec4 h2RendererA23=vpInput[0].xyzw;
    vec4 h2RendererB23=vpConstant(2u).xyzw;
    vec4 h2RendererC23=vec4(0.0);
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    vec4 h2RendererProduct23=h2MenuMultiply(h2RendererA23,h2RendererB23);
    precise float h2RendererDot23=(h2RendererProduct23.x+h2RendererProduct23.y)+h2RendererProduct23.z;
    h2RendererDot23+=h2RendererB23.w;
    h2RendererM23=vec4(h2RendererDot23);
    vpOut[0].z=h2RendererM23.z;
    // renderer instruction 24
    vec4 h2RendererA24=vpInput[0].xyzw;
    vec4 h2RendererB24=vpConstant(3u).xyzw;
    vec4 h2RendererC24=vec4(0.0);
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    vec4 h2RendererProduct24=h2MenuMultiply(h2RendererA24,h2RendererB24);
    precise float h2RendererDot24=(h2RendererProduct24.x+h2RendererProduct24.y)+h2RendererProduct24.z;
    h2RendererDot24+=h2RendererB24.w;
    h2RendererM24=vec4(h2RendererDot24);
    vpOut[0].w=h2RendererM24.w;
    // renderer instruction 25
    vec4 h2RendererA25=h2MenuTemp8.zzzw;
    vec4 h2RendererB25=vpConstant(15u).zzzw;
    vec4 h2RendererC25=vec4(0.0);
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererM25=h2MenuMultiply(h2RendererA25,h2RendererB25);
    h2MenuTemp8.zw=h2RendererM25.zw;
    // renderer instruction 26
    vec4 h2RendererA26=h2MenuTemp5.xyzz;
    vec4 h2RendererB26=vpInput[2].xyzz;
    vec4 h2RendererC26=vpOut[0].wwww;
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    vec4 h2RendererProduct26=h2MenuMultiply(h2RendererA26,h2RendererB26);
    precise float h2RendererDot26=(h2RendererProduct26.x+h2RendererProduct26.y)+h2RendererProduct26.z;
    h2RendererM26=vec4(h2RendererDot26);
    float h2RendererReciprocal26=1.0/h2RendererC26.x;
    if(h2RendererReciprocal26>0.0 || floatBitsToUint(h2RendererReciprocal26)==0u)
        h2RendererReciprocal26=clamp(h2RendererReciprocal26,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal26)) h2RendererReciprocal26=clamp(h2RendererReciprocal26,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL26=vec4(h2RendererReciprocal26);
    h2MenuTemp10.w=h2RendererM26.w;
    h2MenuTemp1.w=h2RendererL26.w;
    // renderer instruction 27
    vec4 h2RendererA27=-h2MenuTemp11.xxxx;
    vec4 h2RendererB27=vec4(0.0);
    vec4 h2RendererC27=vpConstant(4u).wwww;
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2RendererA27+h2RendererC27;
    h2MenuTemp11.y=h2RendererM27.y;
    // renderer instruction 28
    vec4 h2RendererA28=-h2MenuTemp8.zzzw;
    vec4 h2RendererB28=vec4(0.0);
    vec4 h2RendererC28=vpConstant(4u).wwww;
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2RendererA28+h2RendererC28;
    h2MenuTemp8.zw=h2RendererM28.zw;
    // renderer instruction 29
    vec4 h2RendererA29=h2MenuTemp10.wwww;
    vec4 h2RendererB29=vpInput[2].xyzz;
    vec4 h2RendererC29=vec4(0.0);
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29);
    h2MenuTemp10.xyz=h2RendererM29.xyz;
    // renderer instruction 30
    vec4 h2RendererA30=vpInput[11].xyzw;
    vec4 h2RendererB30=h2MenuTemp11.yyyy;
    vec4 h2RendererC30=vec4(0.0);
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=h2MenuMultiply(h2RendererA30,h2RendererB30);
    h2MenuTemp7.xyzw=h2RendererM30.xyzw;
    // renderer instruction 31
    vec4 h2RendererA31=vpOut[0].xyzz;
    vec4 h2RendererB31=vpConstant(10u).xyzz;
    vec4 h2RendererC31=vec4(0.0);
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    h2RendererM31=h2MenuMultiply(h2RendererA31,h2RendererB31);
    vpOut[0].xyz=h2RendererM31.xyz;
    // renderer instruction 32
    vec4 h2RendererA32=h2MenuTemp8.zzzz;
    vec4 h2RendererB32=h2MenuTemp8.wwww;
    vec4 h2RendererC32=vec4(0.0);
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    h2RendererM32=h2MenuMultiply(h2RendererA32,h2RendererB32);
    vpOut[4].w=h2RendererM32.w;
    // renderer instruction 33
    vec4 h2RendererA33=h2MenuTemp10.xyzz;
    vec4 h2RendererB33=vpConstant(6u).wwww;
    vec4 h2RendererC33=-h2MenuTemp5.xyzz;
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33)+h2RendererC33;
    vpOut[9].xyz=h2RendererM33.xyz;
    // renderer instruction 34
    vec4 h2RendererA34=h2MenuTemp9.xyzw;
    vec4 h2RendererB34=vpConstant(20u).xyzw;
    vec4 h2RendererC34=vec4(0.0);
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    vec4 h2RendererProduct34=h2MenuMultiply(h2RendererA34,h2RendererB34);
    precise float h2RendererDot34=(h2RendererProduct34.x+h2RendererProduct34.y)+h2RendererProduct34.z;
    h2RendererDot34+=h2RendererB34.w;
    h2RendererM34=vec4(h2RendererDot34);
    vpOut[10].x=h2RendererM34.x;
    // renderer instruction 35
    vec4 h2RendererA35=h2MenuTemp9.xyzw;
    vec4 h2RendererB35=vpConstant(21u).xyzw;
    vec4 h2RendererC35=vec4(0.0);
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    vec4 h2RendererProduct35=h2MenuMultiply(h2RendererA35,h2RendererB35);
    precise float h2RendererDot35=(h2RendererProduct35.x+h2RendererProduct35.y)+h2RendererProduct35.z;
    h2RendererDot35+=h2RendererB35.w;
    h2RendererM35=vec4(h2RendererDot35);
    vpOut[10].y=h2RendererM35.y;
    // renderer instruction 36
    vec4 h2RendererA36=h2MenuTemp9.xyzw;
    vec4 h2RendererB36=vpConstant(22u).xyzw;
    vec4 h2RendererC36=vec4(0.0);
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    vec4 h2RendererProduct36=h2MenuMultiply(h2RendererA36,h2RendererB36);
    precise float h2RendererDot36=(h2RendererProduct36.x+h2RendererProduct36.y)+h2RendererProduct36.z;
    h2RendererDot36+=h2RendererB36.w;
    h2RendererM36=vec4(h2RendererDot36);
    vpOut[11].x=h2RendererM36.x;
    // renderer instruction 37
    vec4 h2RendererA37=h2MenuTemp9.xyzw;
    vec4 h2RendererB37=vpConstant(23u).xyzw;
    vec4 h2RendererC37=vec4(0.0);
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    vec4 h2RendererProduct37=h2MenuMultiply(h2RendererA37,h2RendererB37);
    precise float h2RendererDot37=(h2RendererProduct37.x+h2RendererProduct37.y)+h2RendererProduct37.z;
    h2RendererDot37+=h2RendererB37.w;
    h2RendererM37=vec4(h2RendererDot37);
    vpOut[11].y=h2RendererM37.y;
    // renderer instruction 38
    vec4 h2RendererA38=h2MenuTemp9.xyzw;
    vec4 h2RendererB38=vpConstant(24u).xyzw;
    vec4 h2RendererC38=vec4(0.0);
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    vec4 h2RendererProduct38=h2MenuMultiply(h2RendererA38,h2RendererB38);
    precise float h2RendererDot38=(h2RendererProduct38.x+h2RendererProduct38.y)+h2RendererProduct38.z;
    h2RendererDot38+=h2RendererB38.w;
    h2RendererM38=vec4(h2RendererDot38);
    vpOut[12].x=h2RendererM38.x;
    // renderer instruction 39
    vec4 h2RendererA39=h2MenuTemp9.xyzw;
    vec4 h2RendererB39=vpConstant(25u).xyzw;
    vec4 h2RendererC39=vec4(0.0);
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    vec4 h2RendererProduct39=h2MenuMultiply(h2RendererA39,h2RendererB39);
    precise float h2RendererDot39=(h2RendererProduct39.x+h2RendererProduct39.y)+h2RendererProduct39.z;
    h2RendererDot39+=h2RendererB39.w;
    h2RendererM39=vec4(h2RendererDot39);
    vpOut[12].y=h2RendererM39.y;
    // renderer instruction 40
    vec4 h2RendererA40=vpInput[10].xyzw;
    vec4 h2RendererB40=h2MenuTemp11.xxxx;
    vec4 h2RendererC40=h2MenuTemp7.xyzw;
    precise vec4 h2RendererM40=vec4(0.0),h2RendererL40=vec4(0.0);
    h2RendererM40=h2MenuMultiply(h2RendererA40,h2RendererB40)+h2RendererC40;
    vpOut[3].xyzw=h2RendererM40.xyzw;
    // renderer instruction 41
    vec4 h2RendererA41=vpOut[0].xyzz;
    vec4 h2RendererB41=h2MenuTemp1.wwww;
    vec4 h2RendererC41=vpConstant(11u).xyzz;
    precise vec4 h2RendererM41=vec4(0.0),h2RendererL41=vec4(0.0);
    h2RendererM41=h2MenuMultiply(h2RendererA41,h2RendererB41)+h2RendererC41;
    vpOut[0].xyz=h2RendererM41.xyz;
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
