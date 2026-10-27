#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in vec4 v2;
layout(location=3) in vec4 v3;
layout(location=4) in int v4_cmp;
layout(location=5) in int v5_cmp;
layout(location=6) in int v6_cmp;
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
    vpInput[3]=v3;
    vpInput[4]=sourcePackedNormal(v4_cmp);
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
    vec4 h2RendererA0=-vpInput[2].yyyy;
    vec4 h2RendererB0=vec4(0.0);
    vec4 h2RendererC0=vpConstant(4u).wwww;
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    h2RendererM0=h2RendererA0+h2RendererC0;
    h2MenuTemp9.x=h2RendererM0.x;
    // renderer instruction 1
    vec4 h2RendererA1=vpInput[1].xyyy;
    vec4 h2RendererB1=vpConstant(7u).wwww;
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    h2RendererM1=h2MenuMultiply(h2RendererA1,h2RendererB1);
    h2MenuTemp0.xy=h2RendererM1.xy;
    // renderer instruction 2
    vec4 h2RendererA2=h2MenuTemp0.yyyy;
    vec4 h2RendererB2=vec4(0.0);
    vec4 h2RendererC2=vec4(0.0);
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    h2MenuAddress=int(floor(h2RendererA2.x));
    // renderer instruction 3
    vec4 h2RendererA3=vpInput[0].xyzz;
    vec4 h2RendererB3=vpConstant(170u).xyzz;
    vec4 h2RendererC3=vec4(0.0);
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    h2RendererM3=h2MenuMultiply(h2RendererA3,h2RendererB3);
    h2MenuTemp10.xyz=h2RendererM3.xyz;
    // renderer instruction 4
    vec4 h2RendererA4=vpInput[2].yyyy;
    vec4 h2RendererB4=vpConstant(uint(50+h2MenuAddress)).xyzw;
    vec4 h2RendererC4=vec4(0.0);
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    h2RendererM4=h2MenuMultiply(h2RendererA4,h2RendererB4);
    h2MenuTemp4.xyzw=h2RendererM4.xyzw;
    // renderer instruction 5
    vec4 h2RendererA5=vpInput[2].yyyy;
    vec4 h2RendererB5=vpConstant(uint(51+h2MenuAddress)).xyzw;
    vec4 h2RendererC5=vec4(0.0);
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    h2RendererM5=h2MenuMultiply(h2RendererA5,h2RendererB5);
    h2MenuTemp5.xyzw=h2RendererM5.xyzw;
    // renderer instruction 6
    vec4 h2RendererA6=vpInput[2].yyyy;
    vec4 h2RendererB6=vpConstant(uint(52+h2MenuAddress)).xyzw;
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    h2RendererM6=h2MenuMultiply(h2RendererA6,h2RendererB6);
    h2MenuTemp6.xyzw=h2RendererM6.xyzw;
    // renderer instruction 7
    vec4 h2RendererA7=h2MenuTemp0.xxxx;
    vec4 h2RendererB7=vec4(0.0);
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    h2MenuAddress=int(floor(h2RendererA7.x));
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp10.xyzz;
    vec4 h2RendererB8=vec4(0.0);
    vec4 h2RendererC8=vpConstant(171u).xyzz;
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    h2RendererM8=h2RendererA8+h2RendererC8;
    h2MenuTemp10.xyz=h2RendererM8.xyz;
    // renderer instruction 9
    vec4 h2RendererA9=h2MenuTemp9.xxxx;
    vec4 h2RendererB9=vpConstant(uint(50+h2MenuAddress)).xyzw;
    vec4 h2RendererC9=h2MenuTemp4.xyzw;
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    h2RendererM9=h2MenuMultiply(h2RendererA9,h2RendererB9)+h2RendererC9;
    h2MenuTemp4.xyzw=h2RendererM9.xyzw;
    // renderer instruction 10
    vec4 h2RendererA10=h2MenuTemp9.xxxx;
    vec4 h2RendererB10=vpConstant(uint(51+h2MenuAddress)).xyzw;
    vec4 h2RendererC10=h2MenuTemp5.xyzw;
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    h2RendererM10=h2MenuMultiply(h2RendererA10,h2RendererB10)+h2RendererC10;
    h2MenuTemp5.xyzw=h2RendererM10.xyzw;
    // renderer instruction 11
    vec4 h2RendererA11=h2MenuTemp9.xxxx;
    vec4 h2RendererB11=vpConstant(uint(52+h2MenuAddress)).xyzw;
    vec4 h2RendererC11=h2MenuTemp6.xyzw;
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    h2RendererM11=h2MenuMultiply(h2RendererA11,h2RendererB11)+h2RendererC11;
    h2MenuTemp6.xyzw=h2RendererM11.xyzw;
    // renderer instruction 12
    vec4 h2RendererA12=h2MenuTemp10.xyzw;
    vec4 h2RendererB12=h2MenuTemp4.xyzw;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererB12.w;
    h2RendererM12=vec4(h2RendererDot12);
    h2MenuTemp0.x=h2RendererM12.x;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp10.xyzw;
    vec4 h2RendererB13=h2MenuTemp5.xyzw;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    vec4 h2RendererProduct13=h2MenuMultiply(h2RendererA13,h2RendererB13);
    precise float h2RendererDot13=(h2RendererProduct13.x+h2RendererProduct13.y)+h2RendererProduct13.z;
    h2RendererDot13+=h2RendererB13.w;
    h2RendererM13=vec4(h2RendererDot13);
    h2MenuTemp0.y=h2RendererM13.y;
    // renderer instruction 14
    vec4 h2RendererA14=h2MenuTemp10.xyzw;
    vec4 h2RendererB14=h2MenuTemp6.xyzw;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    vec4 h2RendererProduct14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    precise float h2RendererDot14=(h2RendererProduct14.x+h2RendererProduct14.y)+h2RendererProduct14.z;
    h2RendererDot14+=h2RendererB14.w;
    h2RendererM14=vec4(h2RendererDot14);
    h2MenuTemp0.z=h2RendererM14.z;
    // renderer instruction 15
    vec4 h2RendererA15=vpInput[4].xyzz;
    vec4 h2RendererB15=h2MenuTemp4.xyzz;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    vec4 h2RendererProduct15=h2MenuMultiply(h2RendererA15,h2RendererB15);
    precise float h2RendererDot15=(h2RendererProduct15.x+h2RendererProduct15.y)+h2RendererProduct15.z;
    h2RendererM15=vec4(h2RendererDot15);
    h2MenuTemp11.x=h2RendererM15.x;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp0.xyzw;
    vec4 h2RendererB16=vpConstant(13u).xyzw;
    vec4 h2RendererC16=vec4(0.0);
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    vec4 h2RendererProduct16=h2MenuMultiply(h2RendererA16,h2RendererB16);
    precise float h2RendererDot16=(h2RendererProduct16.x+h2RendererProduct16.y)+h2RendererProduct16.z;
    h2RendererDot16+=h2RendererB16.w;
    h2RendererM16=vec4(h2RendererDot16);
    h2MenuTemp8.x=h2RendererM16.x;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp0.xyzw;
    vec4 h2RendererB17=vpConstant(14u).xyzw;
    vec4 h2RendererC17=vec4(0.0);
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    vec4 h2RendererProduct17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    precise float h2RendererDot17=(h2RendererProduct17.x+h2RendererProduct17.y)+h2RendererProduct17.z;
    h2RendererDot17+=h2RendererB17.w;
    h2RendererM17=vec4(h2RendererDot17);
    h2MenuTemp8.y=h2RendererM17.y;
    // renderer instruction 18
    vec4 h2RendererA18=h2MenuTemp0.xyzw;
    vec4 h2RendererB18=vpConstant(12u).xyzw;
    vec4 h2RendererC18=vec4(0.0);
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    vec4 h2RendererProduct18=h2MenuMultiply(h2RendererA18,h2RendererB18);
    precise float h2RendererDot18=(h2RendererProduct18.x+h2RendererProduct18.y)+h2RendererProduct18.z;
    h2RendererDot18+=h2RendererB18.w;
    h2RendererM18=vec4(h2RendererDot18);
    h2MenuTemp8.z=h2RendererM18.z;
    // renderer instruction 19
    vec4 h2RendererA19=vpConstant(4u).wwww;
    vec4 h2RendererB19=vec4(0.0);
    vec4 h2RendererC19=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2RendererA19+h2RendererC19;
    h2MenuTemp8.xyz=h2RendererM19.xyz;
    // renderer instruction 20
    vec4 h2RendererA20=vpInput[4].xyzz;
    vec4 h2RendererB20=h2MenuTemp5.xyzz;
    vec4 h2RendererC20=vec4(0.0);
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    vec4 h2RendererProduct20=h2MenuMultiply(h2RendererA20,h2RendererB20);
    precise float h2RendererDot20=(h2RendererProduct20.x+h2RendererProduct20.y)+h2RendererProduct20.z;
    h2RendererM20=vec4(h2RendererDot20);
    h2MenuTemp11.y=h2RendererM20.y;
    // renderer instruction 21
    vec4 h2RendererA21=h2MenuTemp8.xyzz;
    vec4 h2RendererB21=vpConstant(15u).yyyy;
    vec4 h2RendererC21=vec4(0.0);
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    h2RendererM21=max(h2RendererA21,h2RendererB21);
    h2MenuTemp8.xyz=h2RendererM21.xyz;
    // renderer instruction 22
    vec4 h2RendererA22=h2MenuTemp8.xyzz;
    vec4 h2RendererB22=h2MenuTemp8.xyzz;
    vec4 h2RendererC22=vec4(0.0);
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    h2RendererM22=h2MenuMultiply(h2RendererA22,h2RendererB22);
    h2MenuTemp8.xyz=h2RendererM22.xyz;
    // renderer instruction 23
    vec4 h2RendererA23=h2MenuTemp8.xyzz;
    vec4 h2RendererB23=vpConstant(4u).wwww;
    vec4 h2RendererC23=vec4(0.0);
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=min(h2RendererA23,h2RendererB23);
    h2MenuTemp8.xyz=h2RendererM23.xyz;
    // renderer instruction 24
    vec4 h2RendererA24=vpInput[4].xyzz;
    vec4 h2RendererB24=h2MenuTemp6.xyzz;
    vec4 h2RendererC24=vec4(0.0);
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    vec4 h2RendererProduct24=h2MenuMultiply(h2RendererA24,h2RendererB24);
    precise float h2RendererDot24=(h2RendererProduct24.x+h2RendererProduct24.y)+h2RendererProduct24.z;
    h2RendererM24=vec4(h2RendererDot24);
    h2MenuTemp11.z=h2RendererM24.z;
    // renderer instruction 25
    vec4 h2RendererA25=h2MenuTemp8.xxxx;
    vec4 h2RendererB25=vec4(0.0);
    vec4 h2RendererC25=h2MenuTemp8.yyyy;
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererM25=h2RendererA25+h2RendererC25;
    h2MenuTemp8.x=h2RendererM25.x;
    // renderer instruction 26
    vec4 h2RendererA26=vpInput[3].xyyy;
    vec4 h2RendererB26=vpConstant(172u).xyyy;
    vec4 h2RendererC26=vpConstant(172u).zwww;
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    h2RendererM26=h2MenuMultiply(h2RendererA26,h2RendererB26)+h2RendererC26;
    h2MenuTemp9.xy=h2RendererM26.xy;
    // renderer instruction 27
    vec4 h2RendererA27=vpConstant(15u).yyyy;
    vec4 h2RendererB27=vec4(0.0);
    vec4 h2RendererC27=vec4(0.0);
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2RendererA27;
    h2MenuTemp9.z=h2RendererM27.z;
    // renderer instruction 28
    vec4 h2RendererA28=vpConstant(7u).xyzz;
    vec4 h2RendererB28=vec4(0.0);
    vec4 h2RendererC28=-h2MenuTemp0.xyzz;
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2RendererA28+h2RendererC28;
    h2MenuTemp5.xyz=h2RendererM28.xyz;
    // renderer instruction 29
    vec4 h2RendererA29=h2MenuTemp11.xyzz;
    vec4 h2RendererB29=h2MenuTemp11.xyzz;
    vec4 h2RendererC29=vec4(0.0);
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    vec4 h2RendererProduct29=h2MenuMultiply(h2RendererA29,h2RendererB29);
    precise float h2RendererDot29=(h2RendererProduct29.x+h2RendererProduct29.y)+h2RendererProduct29.z;
    h2RendererM29=vec4(h2RendererDot29);
    h2MenuTemp11.w=h2RendererM29.w;
    // renderer instruction 30
    vec4 h2RendererA30=h2MenuTemp8.xxxx;
    vec4 h2RendererB30=vpConstant(4u).wwww;
    vec4 h2RendererC30=vec4(0.0);
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=min(h2RendererA30,h2RendererB30);
    h2MenuTemp8.x=h2RendererM30.x;
    // renderer instruction 31
    vec4 h2RendererA31=h2MenuTemp5.xyzz;
    vec4 h2RendererB31=h2MenuTemp5.xyzz;
    vec4 h2RendererC31=h2MenuTemp11.wwww;
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    vec4 h2RendererProduct31=h2MenuMultiply(h2RendererA31,h2RendererB31);
    precise float h2RendererDot31=(h2RendererProduct31.x+h2RendererProduct31.y)+h2RendererProduct31.z;
    h2RendererM31=vec4(h2RendererDot31);
    h2RendererL31=vec4(inversesqrt(abs(h2RendererC31.x)));
    h2MenuTemp5.w=h2RendererM31.w;
    h2MenuTemp1.w=h2RendererL31.w;
    // renderer instruction 32
    vec4 h2RendererA32=vpConstant(4u).wwww;
    vec4 h2RendererB32=vec4(0.0);
    vec4 h2RendererC32=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    h2RendererM32=h2RendererA32+h2RendererC32;
    h2MenuTemp8.xyz=h2RendererM32.xyz;
    // renderer instruction 33
    vec4 h2RendererA33=h2MenuTemp11.xyzz;
    vec4 h2RendererB33=h2MenuTemp1.wwww;
    vec4 h2RendererC33=h2MenuTemp5.wwww;
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33);
    h2RendererL33=vec4(inversesqrt(abs(h2RendererC33.x)));
    h2MenuTemp11.xyz=h2RendererM33.xyz;
    h2MenuTemp1.x=h2RendererL33.x;
    // renderer instruction 34
    vec4 h2RendererA34=h2MenuTemp8.xyzz;
    vec4 h2RendererB34=h2MenuTemp8.xyzz;
    vec4 h2RendererC34=vec4(0.0);
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    h2RendererM34=h2MenuMultiply(h2RendererA34,h2RendererB34);
    h2MenuTemp8.xyz=h2RendererM34.xyz;
    // renderer instruction 35
    vec4 h2RendererA35=h2MenuTemp5.xyzz;
    vec4 h2RendererB35=h2MenuTemp1.xxxx;
    vec4 h2RendererC35=vec4(0.0);
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    h2RendererM35=h2MenuMultiply(h2RendererA35,h2RendererB35);
    h2MenuTemp5.xyz=h2RendererM35.xyz;
    // renderer instruction 36
    vec4 h2RendererA36=h2MenuTemp8.yyyy;
    vec4 h2RendererB36=vec4(0.0);
    vec4 h2RendererC36=-h2MenuTemp8.xxxx;
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    h2RendererM36=h2RendererA36+h2RendererC36;
    h2MenuTemp8.y=h2RendererM36.y;
    // renderer instruction 37
    vec4 h2RendererA37=h2MenuTemp11.xyzz;
    vec4 h2RendererB37=h2MenuTemp5.xyzz;
    vec4 h2RendererC37=vec4(0.0);
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    vec4 h2RendererProduct37=h2MenuMultiply(h2RendererA37,h2RendererB37);
    precise float h2RendererDot37=(h2RendererProduct37.x+h2RendererProduct37.y)+h2RendererProduct37.z;
    h2RendererM37=vec4(h2RendererDot37);
    h2MenuTemp11.x=h2RendererM37.x;
    // renderer instruction 38
    vec4 h2RendererA38=vpConstant(15u).xxxx;
    vec4 h2RendererB38=h2MenuTemp8.yyyy;
    vec4 h2RendererC38=h2MenuTemp8.xxxx;
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    h2RendererM38=h2MenuMultiply(h2RendererA38,h2RendererB38)+h2RendererC38;
    h2MenuTemp8.w=h2RendererM38.w;
    // renderer instruction 39
    vec4 h2RendererA39=h2MenuTemp11.xxxx;
    vec4 h2RendererB39=vpConstant(15u).yyyy;
    vec4 h2RendererC39=vec4(0.0);
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    h2RendererM39=max(h2RendererA39,h2RendererB39);
    h2MenuTemp11.x=h2RendererM39.x;
    // renderer instruction 40
    vec4 h2RendererA40=h2MenuTemp0.xyzw;
    vec4 h2RendererB40=vpConstant(0u).xyzw;
    vec4 h2RendererC40=vec4(0.0);
    precise vec4 h2RendererM40=vec4(0.0),h2RendererL40=vec4(0.0);
    vec4 h2RendererProduct40=h2MenuMultiply(h2RendererA40,h2RendererB40);
    precise float h2RendererDot40=(h2RendererProduct40.x+h2RendererProduct40.y)+h2RendererProduct40.z;
    h2RendererDot40+=h2RendererB40.w;
    h2RendererM40=vec4(h2RendererDot40);
    vpOut[0].x=h2RendererM40.x;
    // renderer instruction 41
    vec4 h2RendererA41=h2MenuTemp0.xyzw;
    vec4 h2RendererB41=vpConstant(1u).xyzw;
    vec4 h2RendererC41=vec4(0.0);
    precise vec4 h2RendererM41=vec4(0.0),h2RendererL41=vec4(0.0);
    vec4 h2RendererProduct41=h2MenuMultiply(h2RendererA41,h2RendererB41);
    precise float h2RendererDot41=(h2RendererProduct41.x+h2RendererProduct41.y)+h2RendererProduct41.z;
    h2RendererDot41+=h2RendererB41.w;
    h2RendererM41=vec4(h2RendererDot41);
    vpOut[0].y=h2RendererM41.y;
    // renderer instruction 42
    vec4 h2RendererA42=h2MenuTemp0.xyzw;
    vec4 h2RendererB42=vpConstant(2u).xyzw;
    vec4 h2RendererC42=vec4(0.0);
    precise vec4 h2RendererM42=vec4(0.0),h2RendererL42=vec4(0.0);
    vec4 h2RendererProduct42=h2MenuMultiply(h2RendererA42,h2RendererB42);
    precise float h2RendererDot42=(h2RendererProduct42.x+h2RendererProduct42.y)+h2RendererProduct42.z;
    h2RendererDot42+=h2RendererB42.w;
    h2RendererM42=vec4(h2RendererDot42);
    vpOut[0].z=h2RendererM42.z;
    // renderer instruction 43
    vec4 h2RendererA43=h2MenuTemp0.xyzw;
    vec4 h2RendererB43=vpConstant(3u).xyzw;
    vec4 h2RendererC43=vec4(0.0);
    precise vec4 h2RendererM43=vec4(0.0),h2RendererL43=vec4(0.0);
    vec4 h2RendererProduct43=h2MenuMultiply(h2RendererA43,h2RendererB43);
    precise float h2RendererDot43=(h2RendererProduct43.x+h2RendererProduct43.y)+h2RendererProduct43.z;
    h2RendererDot43+=h2RendererB43.w;
    h2RendererM43=vec4(h2RendererDot43);
    vpOut[0].w=h2RendererM43.w;
    // renderer instruction 44
    vec4 h2RendererA44=h2MenuTemp8.zzzw;
    vec4 h2RendererB44=vpConstant(15u).zzzw;
    vec4 h2RendererC44=vec4(0.0);
    precise vec4 h2RendererM44=vec4(0.0),h2RendererL44=vec4(0.0);
    h2RendererM44=h2MenuMultiply(h2RendererA44,h2RendererB44);
    h2MenuTemp8.zw=h2RendererM44.zw;
    // renderer instruction 45
    vec4 h2RendererA45=-h2MenuTemp11.xxxx;
    vec4 h2RendererB45=vec4(0.0);
    vec4 h2RendererC45=vpConstant(4u).wwww;
    precise vec4 h2RendererM45=vec4(0.0),h2RendererL45=vec4(0.0);
    h2RendererM45=h2RendererA45+h2RendererC45;
    h2MenuTemp11.y=h2RendererM45.y;
    // renderer instruction 46
    vec4 h2RendererA46=-h2MenuTemp8.zzzw;
    vec4 h2RendererB46=vec4(0.0);
    vec4 h2RendererC46=vpConstant(4u).wwww;
    precise vec4 h2RendererM46=vec4(0.0),h2RendererL46=vec4(0.0);
    h2RendererM46=h2RendererA46+h2RendererC46;
    h2MenuTemp8.zw=h2RendererM46.zw;
    // renderer instruction 47
    vec4 h2RendererA47=vpInput[11].xyzw;
    vec4 h2RendererB47=h2MenuTemp11.yyyy;
    vec4 h2RendererC47=vpOut[0].wwww;
    precise vec4 h2RendererM47=vec4(0.0),h2RendererL47=vec4(0.0);
    h2RendererM47=h2MenuMultiply(h2RendererA47,h2RendererB47);
    float h2RendererReciprocal47=1.0/h2RendererC47.x;
    if(h2RendererReciprocal47>0.0 || floatBitsToUint(h2RendererReciprocal47)==0u)
        h2RendererReciprocal47=clamp(h2RendererReciprocal47,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal47)) h2RendererReciprocal47=clamp(h2RendererReciprocal47,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL47=vec4(h2RendererReciprocal47);
    h2MenuTemp10.xyzw=h2RendererM47.xyzw;
    h2MenuTemp1.w=h2RendererL47.w;
    // renderer instruction 48
    vec4 h2RendererA48=vpOut[0].xyzz;
    vec4 h2RendererB48=vpConstant(10u).xyzz;
    vec4 h2RendererC48=vec4(0.0);
    precise vec4 h2RendererM48=vec4(0.0),h2RendererL48=vec4(0.0);
    h2RendererM48=h2MenuMultiply(h2RendererA48,h2RendererB48);
    vpOut[0].xyz=h2RendererM48.xyz;
    // renderer instruction 49
    vec4 h2RendererA49=h2MenuTemp8.zzzz;
    vec4 h2RendererB49=h2MenuTemp8.wwww;
    vec4 h2RendererC49=vec4(0.0);
    precise vec4 h2RendererM49=vec4(0.0),h2RendererL49=vec4(0.0);
    h2RendererM49=h2MenuMultiply(h2RendererA49,h2RendererB49);
    vpOut[4].w=h2RendererM49.w;
    // renderer instruction 50
    vec4 h2RendererA50=h2MenuTemp9.xyzw;
    vec4 h2RendererB50=vpConstant(18u).xyzw;
    vec4 h2RendererC50=vec4(0.0);
    precise vec4 h2RendererM50=vec4(0.0),h2RendererL50=vec4(0.0);
    vec4 h2RendererProduct50=h2MenuMultiply(h2RendererA50,h2RendererB50);
    precise float h2RendererDot50=(h2RendererProduct50.x+h2RendererProduct50.y)+h2RendererProduct50.z;
    h2RendererDot50+=h2RendererB50.w;
    h2RendererM50=vec4(h2RendererDot50);
    vpOut[9].x=h2RendererM50.x;
    // renderer instruction 51
    vec4 h2RendererA51=h2MenuTemp9.xyzw;
    vec4 h2RendererB51=vpConstant(19u).xyzw;
    vec4 h2RendererC51=vec4(0.0);
    precise vec4 h2RendererM51=vec4(0.0),h2RendererL51=vec4(0.0);
    vec4 h2RendererProduct51=h2MenuMultiply(h2RendererA51,h2RendererB51);
    precise float h2RendererDot51=(h2RendererProduct51.x+h2RendererProduct51.y)+h2RendererProduct51.z;
    h2RendererDot51+=h2RendererB51.w;
    h2RendererM51=vec4(h2RendererDot51);
    vpOut[9].y=h2RendererM51.y;
    // renderer instruction 52
    vec4 h2RendererA52=h2MenuTemp9.xyzw;
    vec4 h2RendererB52=vpConstant(20u).xyzw;
    vec4 h2RendererC52=vec4(0.0);
    precise vec4 h2RendererM52=vec4(0.0),h2RendererL52=vec4(0.0);
    vec4 h2RendererProduct52=h2MenuMultiply(h2RendererA52,h2RendererB52);
    precise float h2RendererDot52=(h2RendererProduct52.x+h2RendererProduct52.y)+h2RendererProduct52.z;
    h2RendererDot52+=h2RendererB52.w;
    h2RendererM52=vec4(h2RendererDot52);
    vpOut[10].x=h2RendererM52.x;
    // renderer instruction 53
    vec4 h2RendererA53=h2MenuTemp9.xyzw;
    vec4 h2RendererB53=vpConstant(21u).xyzw;
    vec4 h2RendererC53=vec4(0.0);
    precise vec4 h2RendererM53=vec4(0.0),h2RendererL53=vec4(0.0);
    vec4 h2RendererProduct53=h2MenuMultiply(h2RendererA53,h2RendererB53);
    precise float h2RendererDot53=(h2RendererProduct53.x+h2RendererProduct53.y)+h2RendererProduct53.z;
    h2RendererDot53+=h2RendererB53.w;
    h2RendererM53=vec4(h2RendererDot53);
    vpOut[10].y=h2RendererM53.y;
    // renderer instruction 54
    vec4 h2RendererA54=h2MenuTemp9.xyzw;
    vec4 h2RendererB54=vpConstant(22u).xyzw;
    vec4 h2RendererC54=vec4(0.0);
    precise vec4 h2RendererM54=vec4(0.0),h2RendererL54=vec4(0.0);
    vec4 h2RendererProduct54=h2MenuMultiply(h2RendererA54,h2RendererB54);
    precise float h2RendererDot54=(h2RendererProduct54.x+h2RendererProduct54.y)+h2RendererProduct54.z;
    h2RendererDot54+=h2RendererB54.w;
    h2RendererM54=vec4(h2RendererDot54);
    vpOut[11].x=h2RendererM54.x;
    // renderer instruction 55
    vec4 h2RendererA55=h2MenuTemp9.xyzw;
    vec4 h2RendererB55=vpConstant(23u).xyzw;
    vec4 h2RendererC55=vec4(0.0);
    precise vec4 h2RendererM55=vec4(0.0),h2RendererL55=vec4(0.0);
    vec4 h2RendererProduct55=h2MenuMultiply(h2RendererA55,h2RendererB55);
    precise float h2RendererDot55=(h2RendererProduct55.x+h2RendererProduct55.y)+h2RendererProduct55.z;
    h2RendererDot55+=h2RendererB55.w;
    h2RendererM55=vec4(h2RendererDot55);
    vpOut[11].y=h2RendererM55.y;
    // renderer instruction 56
    vec4 h2RendererA56=h2MenuTemp9.xyzw;
    vec4 h2RendererB56=vpConstant(24u).xyzw;
    vec4 h2RendererC56=vec4(0.0);
    precise vec4 h2RendererM56=vec4(0.0),h2RendererL56=vec4(0.0);
    vec4 h2RendererProduct56=h2MenuMultiply(h2RendererA56,h2RendererB56);
    precise float h2RendererDot56=(h2RendererProduct56.x+h2RendererProduct56.y)+h2RendererProduct56.z;
    h2RendererDot56+=h2RendererB56.w;
    h2RendererM56=vec4(h2RendererDot56);
    vpOut[12].x=h2RendererM56.x;
    // renderer instruction 57
    vec4 h2RendererA57=h2MenuTemp9.xyzw;
    vec4 h2RendererB57=vpConstant(25u).xyzw;
    vec4 h2RendererC57=vec4(0.0);
    precise vec4 h2RendererM57=vec4(0.0),h2RendererL57=vec4(0.0);
    vec4 h2RendererProduct57=h2MenuMultiply(h2RendererA57,h2RendererB57);
    precise float h2RendererDot57=(h2RendererProduct57.x+h2RendererProduct57.y)+h2RendererProduct57.z;
    h2RendererDot57+=h2RendererB57.w;
    h2RendererM57=vec4(h2RendererDot57);
    vpOut[12].y=h2RendererM57.y;
    // renderer instruction 58
    vec4 h2RendererA58=vpInput[10].xyzw;
    vec4 h2RendererB58=h2MenuTemp11.xxxx;
    vec4 h2RendererC58=h2MenuTemp10.xyzw;
    precise vec4 h2RendererM58=vec4(0.0),h2RendererL58=vec4(0.0);
    h2RendererM58=h2MenuMultiply(h2RendererA58,h2RendererB58)+h2RendererC58;
    vpOut[3].xyzw=h2RendererM58.xyzw;
    // renderer instruction 59
    vec4 h2RendererA59=vpOut[0].xyzz;
    vec4 h2RendererB59=h2MenuTemp1.wwww;
    vec4 h2RendererC59=vpConstant(11u).xyzz;
    precise vec4 h2RendererM59=vec4(0.0),h2RendererL59=vec4(0.0);
    h2RendererM59=h2MenuMultiply(h2RendererA59,h2RendererB59)+h2RendererC59;
    vpOut[0].xyz=h2RendererM59.xyz;
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
