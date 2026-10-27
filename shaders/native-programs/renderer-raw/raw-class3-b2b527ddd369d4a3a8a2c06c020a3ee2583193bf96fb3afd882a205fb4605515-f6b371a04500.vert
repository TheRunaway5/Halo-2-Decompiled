#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in vec4 v2;
layout(location=3) in int v3_cmp;
layout(location=4) in int v4_cmp;
layout(location=5) in int v5_cmp;
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
    vpInput[2]=v2;
    vpInput[3]=sourcePackedNormal(v3_cmp);
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
    vec4 h2RendererC2=vec4(0.0);
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    h2MenuAddress=int(floor(h2RendererA2.x));
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
    vec4 h2RendererA10=vpInput[3].xyzz;
    vec4 h2RendererB10=h2MenuTemp4.xyzz;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererM10=vec4(h2RendererDot10);
    h2MenuTemp11.x=h2RendererM10.x;
    // renderer instruction 11
    vec4 h2RendererA11=h2MenuTemp0.xyzw;
    vec4 h2RendererB11=vpConstant(13u).xyzw;
    vec4 h2RendererC11=vec4(0.0);
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererDot11+=h2RendererB11.w;
    h2RendererM11=vec4(h2RendererDot11);
    h2MenuTemp8.x=h2RendererM11.x;
    // renderer instruction 12
    vec4 h2RendererA12=h2MenuTemp0.xyzw;
    vec4 h2RendererB12=vpConstant(14u).xyzw;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererB12.w;
    h2RendererM12=vec4(h2RendererDot12);
    h2MenuTemp8.y=h2RendererM12.y;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp0.xyzw;
    vec4 h2RendererB13=vpConstant(12u).xyzw;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    vec4 h2RendererProduct13=h2MenuMultiply(h2RendererA13,h2RendererB13);
    precise float h2RendererDot13=(h2RendererProduct13.x+h2RendererProduct13.y)+h2RendererProduct13.z;
    h2RendererDot13+=h2RendererB13.w;
    h2RendererM13=vec4(h2RendererDot13);
    h2MenuTemp8.z=h2RendererM13.z;
    // renderer instruction 14
    vec4 h2RendererA14=vpConstant(4u).wwww;
    vec4 h2RendererB14=vec4(0.0);
    vec4 h2RendererC14=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    h2RendererM14=h2RendererA14+h2RendererC14;
    h2MenuTemp8.xyz=h2RendererM14.xyz;
    // renderer instruction 15
    vec4 h2RendererA15=vpInput[3].xyzz;
    vec4 h2RendererB15=h2MenuTemp5.xyzz;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    vec4 h2RendererProduct15=h2MenuMultiply(h2RendererA15,h2RendererB15);
    precise float h2RendererDot15=(h2RendererProduct15.x+h2RendererProduct15.y)+h2RendererProduct15.z;
    h2RendererM15=vec4(h2RendererDot15);
    h2MenuTemp11.y=h2RendererM15.y;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp8.xyzz;
    vec4 h2RendererB16=vpConstant(15u).yyyy;
    vec4 h2RendererC16=vec4(0.0);
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=max(h2RendererA16,h2RendererB16);
    h2MenuTemp8.xyz=h2RendererM16.xyz;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp8.xyzz;
    vec4 h2RendererB17=h2MenuTemp8.xyzz;
    vec4 h2RendererC17=vec4(0.0);
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    h2RendererM17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    h2MenuTemp8.xyz=h2RendererM17.xyz;
    // renderer instruction 18
    vec4 h2RendererA18=h2MenuTemp8.xyzz;
    vec4 h2RendererB18=vpConstant(4u).wwww;
    vec4 h2RendererC18=vec4(0.0);
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    h2RendererM18=min(h2RendererA18,h2RendererB18);
    h2MenuTemp8.xyz=h2RendererM18.xyz;
    // renderer instruction 19
    vec4 h2RendererA19=vpInput[3].xyzz;
    vec4 h2RendererB19=h2MenuTemp6.xyzz;
    vec4 h2RendererC19=vec4(0.0);
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    vec4 h2RendererProduct19=h2MenuMultiply(h2RendererA19,h2RendererB19);
    precise float h2RendererDot19=(h2RendererProduct19.x+h2RendererProduct19.y)+h2RendererProduct19.z;
    h2RendererM19=vec4(h2RendererDot19);
    h2MenuTemp11.z=h2RendererM19.z;
    // renderer instruction 20
    vec4 h2RendererA20=h2MenuTemp8.xxxx;
    vec4 h2RendererB20=vec4(0.0);
    vec4 h2RendererC20=h2MenuTemp8.yyyy;
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    h2RendererM20=h2RendererA20+h2RendererC20;
    h2MenuTemp8.x=h2RendererM20.x;
    // renderer instruction 21
    vec4 h2RendererA21=vpInput[2].xyyy;
    vec4 h2RendererB21=vpConstant(172u).xyyy;
    vec4 h2RendererC21=vpConstant(172u).zwww;
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    h2RendererM21=h2MenuMultiply(h2RendererA21,h2RendererB21)+h2RendererC21;
    h2MenuTemp9.xy=h2RendererM21.xy;
    // renderer instruction 22
    vec4 h2RendererA22=vpConstant(15u).yyyy;
    vec4 h2RendererB22=vec4(0.0);
    vec4 h2RendererC22=vec4(0.0);
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    h2RendererM22=h2RendererA22;
    h2MenuTemp9.z=h2RendererM22.z;
    // renderer instruction 23
    vec4 h2RendererA23=vpConstant(7u).xyzz;
    vec4 h2RendererB23=vec4(0.0);
    vec4 h2RendererC23=-h2MenuTemp0.xyzz;
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=h2RendererA23+h2RendererC23;
    h2MenuTemp5.xyz=h2RendererM23.xyz;
    // renderer instruction 24
    vec4 h2RendererA24=h2MenuTemp11.xyzz;
    vec4 h2RendererB24=h2MenuTemp11.xyzz;
    vec4 h2RendererC24=vec4(0.0);
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    vec4 h2RendererProduct24=h2MenuMultiply(h2RendererA24,h2RendererB24);
    precise float h2RendererDot24=(h2RendererProduct24.x+h2RendererProduct24.y)+h2RendererProduct24.z;
    h2RendererM24=vec4(h2RendererDot24);
    h2MenuTemp11.w=h2RendererM24.w;
    // renderer instruction 25
    vec4 h2RendererA25=h2MenuTemp8.xxxx;
    vec4 h2RendererB25=vpConstant(4u).wwww;
    vec4 h2RendererC25=vec4(0.0);
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererM25=min(h2RendererA25,h2RendererB25);
    h2MenuTemp8.x=h2RendererM25.x;
    // renderer instruction 26
    vec4 h2RendererA26=h2MenuTemp5.xyzz;
    vec4 h2RendererB26=h2MenuTemp5.xyzz;
    vec4 h2RendererC26=h2MenuTemp11.wwww;
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    vec4 h2RendererProduct26=h2MenuMultiply(h2RendererA26,h2RendererB26);
    precise float h2RendererDot26=(h2RendererProduct26.x+h2RendererProduct26.y)+h2RendererProduct26.z;
    h2RendererM26=vec4(h2RendererDot26);
    h2RendererL26=vec4(inversesqrt(abs(h2RendererC26.x)));
    h2MenuTemp5.w=h2RendererM26.w;
    h2MenuTemp1.w=h2RendererL26.w;
    // renderer instruction 27
    vec4 h2RendererA27=vpConstant(4u).wwww;
    vec4 h2RendererB27=vec4(0.0);
    vec4 h2RendererC27=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2RendererA27+h2RendererC27;
    h2MenuTemp8.xyz=h2RendererM27.xyz;
    // renderer instruction 28
    vec4 h2RendererA28=h2MenuTemp11.xyzz;
    vec4 h2RendererB28=h2MenuTemp1.wwww;
    vec4 h2RendererC28=h2MenuTemp5.wwww;
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2MenuMultiply(h2RendererA28,h2RendererB28);
    h2RendererL28=vec4(inversesqrt(abs(h2RendererC28.x)));
    h2MenuTemp11.xyz=h2RendererM28.xyz;
    h2MenuTemp1.x=h2RendererL28.x;
    // renderer instruction 29
    vec4 h2RendererA29=h2MenuTemp8.xyzz;
    vec4 h2RendererB29=h2MenuTemp8.xyzz;
    vec4 h2RendererC29=vec4(0.0);
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29);
    h2MenuTemp8.xyz=h2RendererM29.xyz;
    // renderer instruction 30
    vec4 h2RendererA30=h2MenuTemp5.xyzz;
    vec4 h2RendererB30=h2MenuTemp1.xxxx;
    vec4 h2RendererC30=vec4(0.0);
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=h2MenuMultiply(h2RendererA30,h2RendererB30);
    h2MenuTemp5.xyz=h2RendererM30.xyz;
    // renderer instruction 31
    vec4 h2RendererA31=h2MenuTemp8.yyyy;
    vec4 h2RendererB31=vec4(0.0);
    vec4 h2RendererC31=-h2MenuTemp8.xxxx;
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    h2RendererM31=h2RendererA31+h2RendererC31;
    h2MenuTemp8.y=h2RendererM31.y;
    // renderer instruction 32
    vec4 h2RendererA32=h2MenuTemp11.xyzz;
    vec4 h2RendererB32=h2MenuTemp5.xyzz;
    vec4 h2RendererC32=vec4(0.0);
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    vec4 h2RendererProduct32=h2MenuMultiply(h2RendererA32,h2RendererB32);
    precise float h2RendererDot32=(h2RendererProduct32.x+h2RendererProduct32.y)+h2RendererProduct32.z;
    h2RendererM32=vec4(h2RendererDot32);
    h2MenuTemp11.x=h2RendererM32.x;
    // renderer instruction 33
    vec4 h2RendererA33=vpConstant(15u).xxxx;
    vec4 h2RendererB33=h2MenuTemp8.yyyy;
    vec4 h2RendererC33=h2MenuTemp8.xxxx;
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33)+h2RendererC33;
    h2MenuTemp8.w=h2RendererM33.w;
    // renderer instruction 34
    vec4 h2RendererA34=h2MenuTemp11.xxxx;
    vec4 h2RendererB34=vpConstant(15u).yyyy;
    vec4 h2RendererC34=vec4(0.0);
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    h2RendererM34=max(h2RendererA34,h2RendererB34);
    h2MenuTemp11.x=h2RendererM34.x;
    // renderer instruction 35
    vec4 h2RendererA35=h2MenuTemp0.xyzw;
    vec4 h2RendererB35=vpConstant(0u).xyzw;
    vec4 h2RendererC35=vec4(0.0);
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    vec4 h2RendererProduct35=h2MenuMultiply(h2RendererA35,h2RendererB35);
    precise float h2RendererDot35=(h2RendererProduct35.x+h2RendererProduct35.y)+h2RendererProduct35.z;
    h2RendererDot35+=h2RendererB35.w;
    h2RendererM35=vec4(h2RendererDot35);
    vpOut[0].x=h2RendererM35.x;
    // renderer instruction 36
    vec4 h2RendererA36=h2MenuTemp0.xyzw;
    vec4 h2RendererB36=vpConstant(1u).xyzw;
    vec4 h2RendererC36=vec4(0.0);
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    vec4 h2RendererProduct36=h2MenuMultiply(h2RendererA36,h2RendererB36);
    precise float h2RendererDot36=(h2RendererProduct36.x+h2RendererProduct36.y)+h2RendererProduct36.z;
    h2RendererDot36+=h2RendererB36.w;
    h2RendererM36=vec4(h2RendererDot36);
    vpOut[0].y=h2RendererM36.y;
    // renderer instruction 37
    vec4 h2RendererA37=h2MenuTemp0.xyzw;
    vec4 h2RendererB37=vpConstant(2u).xyzw;
    vec4 h2RendererC37=vec4(0.0);
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    vec4 h2RendererProduct37=h2MenuMultiply(h2RendererA37,h2RendererB37);
    precise float h2RendererDot37=(h2RendererProduct37.x+h2RendererProduct37.y)+h2RendererProduct37.z;
    h2RendererDot37+=h2RendererB37.w;
    h2RendererM37=vec4(h2RendererDot37);
    vpOut[0].z=h2RendererM37.z;
    // renderer instruction 38
    vec4 h2RendererA38=h2MenuTemp0.xyzw;
    vec4 h2RendererB38=vpConstant(3u).xyzw;
    vec4 h2RendererC38=vec4(0.0);
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    vec4 h2RendererProduct38=h2MenuMultiply(h2RendererA38,h2RendererB38);
    precise float h2RendererDot38=(h2RendererProduct38.x+h2RendererProduct38.y)+h2RendererProduct38.z;
    h2RendererDot38+=h2RendererB38.w;
    h2RendererM38=vec4(h2RendererDot38);
    vpOut[0].w=h2RendererM38.w;
    // renderer instruction 39
    vec4 h2RendererA39=h2MenuTemp8.zzzw;
    vec4 h2RendererB39=vpConstant(15u).zzzw;
    vec4 h2RendererC39=vec4(0.0);
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    h2RendererM39=h2MenuMultiply(h2RendererA39,h2RendererB39);
    h2MenuTemp8.zw=h2RendererM39.zw;
    // renderer instruction 40
    vec4 h2RendererA40=-h2MenuTemp11.xxxx;
    vec4 h2RendererB40=vec4(0.0);
    vec4 h2RendererC40=vpConstant(4u).wwww;
    precise vec4 h2RendererM40=vec4(0.0),h2RendererL40=vec4(0.0);
    h2RendererM40=h2RendererA40+h2RendererC40;
    h2MenuTemp11.y=h2RendererM40.y;
    // renderer instruction 41
    vec4 h2RendererA41=-h2MenuTemp8.zzzw;
    vec4 h2RendererB41=vec4(0.0);
    vec4 h2RendererC41=vpConstant(4u).wwww;
    precise vec4 h2RendererM41=vec4(0.0),h2RendererL41=vec4(0.0);
    h2RendererM41=h2RendererA41+h2RendererC41;
    h2MenuTemp8.zw=h2RendererM41.zw;
    // renderer instruction 42
    vec4 h2RendererA42=vpInput[11].xyzw;
    vec4 h2RendererB42=h2MenuTemp11.yyyy;
    vec4 h2RendererC42=vpOut[0].wwww;
    precise vec4 h2RendererM42=vec4(0.0),h2RendererL42=vec4(0.0);
    h2RendererM42=h2MenuMultiply(h2RendererA42,h2RendererB42);
    float h2RendererReciprocal42=1.0/h2RendererC42.x;
    if(h2RendererReciprocal42>0.0 || floatBitsToUint(h2RendererReciprocal42)==0u)
        h2RendererReciprocal42=clamp(h2RendererReciprocal42,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal42)) h2RendererReciprocal42=clamp(h2RendererReciprocal42,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL42=vec4(h2RendererReciprocal42);
    h2MenuTemp10.xyzw=h2RendererM42.xyzw;
    h2MenuTemp1.w=h2RendererL42.w;
    // renderer instruction 43
    vec4 h2RendererA43=vpOut[0].xyzz;
    vec4 h2RendererB43=vpConstant(10u).xyzz;
    vec4 h2RendererC43=vec4(0.0);
    precise vec4 h2RendererM43=vec4(0.0),h2RendererL43=vec4(0.0);
    h2RendererM43=h2MenuMultiply(h2RendererA43,h2RendererB43);
    vpOut[0].xyz=h2RendererM43.xyz;
    // renderer instruction 44
    vec4 h2RendererA44=h2MenuTemp8.zzzz;
    vec4 h2RendererB44=h2MenuTemp8.wwww;
    vec4 h2RendererC44=vec4(0.0);
    precise vec4 h2RendererM44=vec4(0.0),h2RendererL44=vec4(0.0);
    h2RendererM44=h2MenuMultiply(h2RendererA44,h2RendererB44);
    vpOut[4].w=h2RendererM44.w;
    // renderer instruction 45
    vec4 h2RendererA45=h2MenuTemp9.xyzw;
    vec4 h2RendererB45=vpConstant(18u).xyzw;
    vec4 h2RendererC45=vec4(0.0);
    precise vec4 h2RendererM45=vec4(0.0),h2RendererL45=vec4(0.0);
    vec4 h2RendererProduct45=h2MenuMultiply(h2RendererA45,h2RendererB45);
    precise float h2RendererDot45=(h2RendererProduct45.x+h2RendererProduct45.y)+h2RendererProduct45.z;
    h2RendererDot45+=h2RendererB45.w;
    h2RendererM45=vec4(h2RendererDot45);
    vpOut[9].x=h2RendererM45.x;
    // renderer instruction 46
    vec4 h2RendererA46=h2MenuTemp9.xyzw;
    vec4 h2RendererB46=vpConstant(19u).xyzw;
    vec4 h2RendererC46=vec4(0.0);
    precise vec4 h2RendererM46=vec4(0.0),h2RendererL46=vec4(0.0);
    vec4 h2RendererProduct46=h2MenuMultiply(h2RendererA46,h2RendererB46);
    precise float h2RendererDot46=(h2RendererProduct46.x+h2RendererProduct46.y)+h2RendererProduct46.z;
    h2RendererDot46+=h2RendererB46.w;
    h2RendererM46=vec4(h2RendererDot46);
    vpOut[9].y=h2RendererM46.y;
    // renderer instruction 47
    vec4 h2RendererA47=h2MenuTemp9.xyzw;
    vec4 h2RendererB47=vpConstant(20u).xyzw;
    vec4 h2RendererC47=vec4(0.0);
    precise vec4 h2RendererM47=vec4(0.0),h2RendererL47=vec4(0.0);
    vec4 h2RendererProduct47=h2MenuMultiply(h2RendererA47,h2RendererB47);
    precise float h2RendererDot47=(h2RendererProduct47.x+h2RendererProduct47.y)+h2RendererProduct47.z;
    h2RendererDot47+=h2RendererB47.w;
    h2RendererM47=vec4(h2RendererDot47);
    vpOut[10].x=h2RendererM47.x;
    // renderer instruction 48
    vec4 h2RendererA48=h2MenuTemp9.xyzw;
    vec4 h2RendererB48=vpConstant(21u).xyzw;
    vec4 h2RendererC48=vec4(0.0);
    precise vec4 h2RendererM48=vec4(0.0),h2RendererL48=vec4(0.0);
    vec4 h2RendererProduct48=h2MenuMultiply(h2RendererA48,h2RendererB48);
    precise float h2RendererDot48=(h2RendererProduct48.x+h2RendererProduct48.y)+h2RendererProduct48.z;
    h2RendererDot48+=h2RendererB48.w;
    h2RendererM48=vec4(h2RendererDot48);
    vpOut[10].y=h2RendererM48.y;
    // renderer instruction 49
    vec4 h2RendererA49=h2MenuTemp9.xyzw;
    vec4 h2RendererB49=vpConstant(22u).xyzw;
    vec4 h2RendererC49=vec4(0.0);
    precise vec4 h2RendererM49=vec4(0.0),h2RendererL49=vec4(0.0);
    vec4 h2RendererProduct49=h2MenuMultiply(h2RendererA49,h2RendererB49);
    precise float h2RendererDot49=(h2RendererProduct49.x+h2RendererProduct49.y)+h2RendererProduct49.z;
    h2RendererDot49+=h2RendererB49.w;
    h2RendererM49=vec4(h2RendererDot49);
    vpOut[11].x=h2RendererM49.x;
    // renderer instruction 50
    vec4 h2RendererA50=h2MenuTemp9.xyzw;
    vec4 h2RendererB50=vpConstant(23u).xyzw;
    vec4 h2RendererC50=vec4(0.0);
    precise vec4 h2RendererM50=vec4(0.0),h2RendererL50=vec4(0.0);
    vec4 h2RendererProduct50=h2MenuMultiply(h2RendererA50,h2RendererB50);
    precise float h2RendererDot50=(h2RendererProduct50.x+h2RendererProduct50.y)+h2RendererProduct50.z;
    h2RendererDot50+=h2RendererB50.w;
    h2RendererM50=vec4(h2RendererDot50);
    vpOut[11].y=h2RendererM50.y;
    // renderer instruction 51
    vec4 h2RendererA51=h2MenuTemp9.xyzw;
    vec4 h2RendererB51=vpConstant(24u).xyzw;
    vec4 h2RendererC51=vec4(0.0);
    precise vec4 h2RendererM51=vec4(0.0),h2RendererL51=vec4(0.0);
    vec4 h2RendererProduct51=h2MenuMultiply(h2RendererA51,h2RendererB51);
    precise float h2RendererDot51=(h2RendererProduct51.x+h2RendererProduct51.y)+h2RendererProduct51.z;
    h2RendererDot51+=h2RendererB51.w;
    h2RendererM51=vec4(h2RendererDot51);
    vpOut[12].x=h2RendererM51.x;
    // renderer instruction 52
    vec4 h2RendererA52=h2MenuTemp9.xyzw;
    vec4 h2RendererB52=vpConstant(25u).xyzw;
    vec4 h2RendererC52=vec4(0.0);
    precise vec4 h2RendererM52=vec4(0.0),h2RendererL52=vec4(0.0);
    vec4 h2RendererProduct52=h2MenuMultiply(h2RendererA52,h2RendererB52);
    precise float h2RendererDot52=(h2RendererProduct52.x+h2RendererProduct52.y)+h2RendererProduct52.z;
    h2RendererDot52+=h2RendererB52.w;
    h2RendererM52=vec4(h2RendererDot52);
    vpOut[12].y=h2RendererM52.y;
    // renderer instruction 53
    vec4 h2RendererA53=vpInput[10].xyzw;
    vec4 h2RendererB53=h2MenuTemp11.xxxx;
    vec4 h2RendererC53=h2MenuTemp10.xyzw;
    precise vec4 h2RendererM53=vec4(0.0),h2RendererL53=vec4(0.0);
    h2RendererM53=h2MenuMultiply(h2RendererA53,h2RendererB53)+h2RendererC53;
    vpOut[3].xyzw=h2RendererM53.xyzw;
    // renderer instruction 54
    vec4 h2RendererA54=vpOut[0].xyzz;
    vec4 h2RendererB54=h2MenuTemp1.wwww;
    vec4 h2RendererC54=vpConstant(11u).xyzz;
    precise vec4 h2RendererM54=vec4(0.0),h2RendererL54=vec4(0.0);
    h2RendererM54=h2MenuMultiply(h2RendererA54,h2RendererB54)+h2RendererC54;
    vpOut[0].xyz=h2RendererM54.xyz;
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
