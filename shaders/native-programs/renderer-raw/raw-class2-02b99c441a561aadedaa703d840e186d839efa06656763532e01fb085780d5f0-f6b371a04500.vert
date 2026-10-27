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
    vpInput[11]=source.attributes[11];
    vpInput[12]=source.attributes[12];
    vpInput[13]=source.attributes[13];
    vpInput[14]=source.attributes[14];
    vpInput[15]=source.attributes[15];
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
    vec4 h2RendererA11=vpInput[3].xyzz;
    vec4 h2RendererB11=h2MenuTemp5.xyzz;
    vec4 h2RendererC11=vec4(0.0);
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererM11=vec4(h2RendererDot11);
    h2MenuTemp11.y=h2RendererM11.y;
    // renderer instruction 12
    vec4 h2RendererA12=vpInput[3].xyzz;
    vec4 h2RendererB12=h2MenuTemp6.xyzz;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererM12=vec4(h2RendererDot12);
    h2MenuTemp11.z=h2RendererM12.z;
    // renderer instruction 13
    vec4 h2RendererA13=vpConstant(7u).xyzz;
    vec4 h2RendererB13=vec4(0.0);
    vec4 h2RendererC13=-h2MenuTemp0.xyzz;
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    h2RendererM13=h2RendererA13+h2RendererC13;
    h2MenuTemp5.xyz=h2RendererM13.xyz;
    // renderer instruction 14
    vec4 h2RendererA14=-vpConstant(26u).xyzz;
    vec4 h2RendererB14=h2MenuTemp11.xyzz;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    vec4 h2RendererProduct14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    precise float h2RendererDot14=(h2RendererProduct14.x+h2RendererProduct14.y)+h2RendererProduct14.z;
    h2RendererM14=vec4(h2RendererDot14);
    h2MenuTemp11.w=h2RendererM14.w;
    // renderer instruction 15
    vec4 h2RendererA15=h2MenuTemp5.xyzz;
    vec4 h2RendererB15=h2MenuTemp5.xyzz;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    vec4 h2RendererProduct15=h2MenuMultiply(h2RendererA15,h2RendererB15);
    precise float h2RendererDot15=(h2RendererProduct15.x+h2RendererProduct15.y)+h2RendererProduct15.z;
    h2RendererM15=vec4(h2RendererDot15);
    h2MenuTemp5.w=h2RendererM15.w;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp0.xyzw;
    vec4 h2RendererB16=vpConstant(0u).xyzw;
    vec4 h2RendererC16=vec4(0.0);
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    vec4 h2RendererProduct16=h2MenuMultiply(h2RendererA16,h2RendererB16);
    precise float h2RendererDot16=(h2RendererProduct16.x+h2RendererProduct16.y)+h2RendererProduct16.z;
    h2RendererDot16+=h2RendererB16.w;
    h2RendererM16=vec4(h2RendererDot16);
    vpOut[0].x=h2RendererM16.x;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp0.xyzw;
    vec4 h2RendererB17=vpConstant(1u).xyzw;
    vec4 h2RendererC17=h2MenuTemp5.wwww;
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    vec4 h2RendererProduct17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    precise float h2RendererDot17=(h2RendererProduct17.x+h2RendererProduct17.y)+h2RendererProduct17.z;
    h2RendererDot17+=h2RendererB17.w;
    h2RendererM17=vec4(h2RendererDot17);
    h2RendererL17=vec4(inversesqrt(abs(h2RendererC17.x)));
    h2MenuTemp1.w=h2RendererL17.w;
    vpOut[0].y=h2RendererM17.y;
    // renderer instruction 18
    vec4 h2RendererA18=h2MenuTemp0.xyzw;
    vec4 h2RendererB18=vpConstant(2u).xyzw;
    vec4 h2RendererC18=vec4(0.0);
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    vec4 h2RendererProduct18=h2MenuMultiply(h2RendererA18,h2RendererB18);
    precise float h2RendererDot18=(h2RendererProduct18.x+h2RendererProduct18.y)+h2RendererProduct18.z;
    h2RendererDot18+=h2RendererB18.w;
    h2RendererM18=vec4(h2RendererDot18);
    vpOut[0].z=h2RendererM18.z;
    // renderer instruction 19
    vec4 h2RendererA19=h2MenuTemp5.xyzz;
    vec4 h2RendererB19=h2MenuTemp1.wwww;
    vec4 h2RendererC19=vec4(0.0);
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2MenuMultiply(h2RendererA19,h2RendererB19);
    h2MenuTemp5.xyz=h2RendererM19.xyz;
    // renderer instruction 20
    vec4 h2RendererA20=h2MenuTemp5.xyzz;
    vec4 h2RendererB20=h2MenuTemp11.xyzz;
    vec4 h2RendererC20=vec4(0.0);
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    vec4 h2RendererProduct20=h2MenuMultiply(h2RendererA20,h2RendererB20);
    precise float h2RendererDot20=(h2RendererProduct20.x+h2RendererProduct20.y)+h2RendererProduct20.z;
    h2RendererM20=vec4(h2RendererDot20);
    h2MenuTemp5.w=h2RendererM20.w;
    // renderer instruction 21
    vec4 h2RendererA21=h2MenuTemp0.xyzw;
    vec4 h2RendererB21=vpConstant(3u).xyzw;
    vec4 h2RendererC21=vec4(0.0);
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    vec4 h2RendererProduct21=h2MenuMultiply(h2RendererA21,h2RendererB21);
    precise float h2RendererDot21=(h2RendererProduct21.x+h2RendererProduct21.y)+h2RendererProduct21.z;
    h2RendererDot21+=h2RendererB21.w;
    h2RendererM21=vec4(h2RendererDot21);
    vpOut[0].w=h2RendererM21.w;
    // renderer instruction 22
    vec4 h2RendererA22=vpConstant(15u).yyyy;
    vec4 h2RendererB22=h2MenuTemp5.wwww;
    vec4 h2RendererC22=vec4(0.0);
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    h2RendererM22=max(h2RendererA22,h2RendererB22);
    h2MenuTemp10.w=h2RendererM22.w;
    // renderer instruction 23
    vec4 h2RendererA23=vpInput[2].xyyy;
    vec4 h2RendererB23=vpConstant(172u).xyyy;
    vec4 h2RendererC23=vpConstant(172u).zwww;
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=h2MenuMultiply(h2RendererA23,h2RendererB23)+h2RendererC23;
    h2MenuTemp9.xy=h2RendererM23.xy;
    // renderer instruction 24
    vec4 h2RendererA24=vpConstant(15u).yyyy;
    vec4 h2RendererB24=h2MenuTemp11.wwww;
    vec4 h2RendererC24=vpOut[0].wwww;
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    h2RendererM24=max(h2RendererA24,h2RendererB24);
    float h2RendererReciprocal24=1.0/h2RendererC24.x;
    if(h2RendererReciprocal24>0.0 || floatBitsToUint(h2RendererReciprocal24)==0u)
        h2RendererReciprocal24=clamp(h2RendererReciprocal24,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal24)) h2RendererReciprocal24=clamp(h2RendererReciprocal24,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL24=vec4(h2RendererReciprocal24);
    h2MenuTemp10.x=h2RendererM24.x;
    h2MenuTemp1.w=h2RendererL24.w;
    // renderer instruction 25
    vec4 h2RendererA25=-vpConstant(27u).xyzz;
    vec4 h2RendererB25=h2MenuTemp11.xyzz;
    vec4 h2RendererC25=vec4(0.0);
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    vec4 h2RendererProduct25=h2MenuMultiply(h2RendererA25,h2RendererB25);
    precise float h2RendererDot25=(h2RendererProduct25.x+h2RendererProduct25.y)+h2RendererProduct25.z;
    h2RendererM25=vec4(h2RendererDot25);
    h2MenuTemp10.y=h2RendererM25.y;
    // renderer instruction 26
    vec4 h2RendererA26=vpInput[13].xyzz;
    vec4 h2RendererB26=h2MenuTemp10.wwww;
    vec4 h2RendererC26=vec4(0.0);
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    h2RendererM26=h2MenuMultiply(h2RendererA26,h2RendererB26);
    h2MenuTemp11.xyz=h2RendererM26.xyz;
    // renderer instruction 27
    vec4 h2RendererA27=vpConstant(4u).wwww;
    vec4 h2RendererB27=vec4(0.0);
    vec4 h2RendererC27=-h2MenuTemp10.wwww;
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2RendererA27+h2RendererC27;
    h2MenuTemp10.w=h2RendererM27.w;
    // renderer instruction 28
    vec4 h2RendererA28=h2MenuTemp9.xyyy;
    vec4 h2RendererB28=vpConstant(18u).xyyy;
    vec4 h2RendererC28=vpConstant(18u).zwww;
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2MenuMultiply(h2RendererA28,h2RendererB28)+h2RendererC28;
    h2MenuTemp9.xy=h2RendererM28.xy;
    vpOut[9].xy=h2RendererM28.xy;
    // renderer instruction 29
    vec4 h2RendererA29=vpInput[11].yyzx;
    vec4 h2RendererB29=h2MenuTemp10.xxxx;
    vec4 h2RendererC29=vec4(0.0);
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29);
    h2MenuTemp8.yzw=h2RendererM29.yzw;
    // renderer instruction 30
    vec4 h2RendererA30=vpConstant(15u).yyyy;
    vec4 h2RendererB30=h2MenuTemp10.yyyy;
    vec4 h2RendererC30=h2MenuTemp9.xyyy;
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=max(h2RendererA30,h2RendererB30);
    h2RendererL30=h2RendererC30;
    h2MenuTemp10.x=h2RendererM30.x;
    vpOut[10].xy=h2RendererL30.xy;
    // renderer instruction 31
    vec4 h2RendererA31=vpInput[14].yyzx;
    vec4 h2RendererB31=h2MenuTemp10.wwww;
    vec4 h2RendererC31=h2MenuTemp11.yyzx;
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    h2RendererM31=h2MenuMultiply(h2RendererA31,h2RendererB31)+h2RendererC31;
    h2MenuTemp10.yzw=h2RendererM31.yzw;
    // renderer instruction 32
    vec4 h2RendererA32=-vpConstant(26u).xyzz;
    vec4 h2RendererB32=h2MenuTemp5.xyzz;
    vec4 h2RendererC32=vec4(0.0);
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    vec4 h2RendererProduct32=h2MenuMultiply(h2RendererA32,h2RendererB32);
    precise float h2RendererDot32=(h2RendererProduct32.x+h2RendererProduct32.y)+h2RendererProduct32.z;
    h2RendererM32=vec4(h2RendererDot32);
    h2MenuTemp11.x=h2RendererM32.x;
    // renderer instruction 33
    vec4 h2RendererA33=h2MenuTemp11.wwww;
    vec4 h2RendererB33=h2MenuTemp5.wwww;
    vec4 h2RendererC33=vec4(0.0);
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33);
    h2MenuTemp11.y=h2RendererM33.y;
    // renderer instruction 34
    vec4 h2RendererA34=vpOut[0].xyzz;
    vec4 h2RendererB34=vpConstant(10u).xyzz;
    vec4 h2RendererC34=vec4(0.0);
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    h2RendererM34=h2MenuMultiply(h2RendererA34,h2RendererB34);
    vpOut[0].xyz=h2RendererM34.xyz;
    // renderer instruction 35
    vec4 h2RendererA35=vpInput[12].xyzz;
    vec4 h2RendererB35=h2MenuTemp10.xxxx;
    vec4 h2RendererC35=h2MenuTemp8.wyzz;
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    h2RendererM35=h2MenuMultiply(h2RendererA35,h2RendererB35)+h2RendererC35;
    vpOut[3].xyz=h2RendererM35.xyz;
    // renderer instruction 36
    vec4 h2RendererA36=vpInput[15].xyzz;
    vec4 h2RendererB36=h2MenuTemp10.wyzz;
    vec4 h2RendererC36=vec4(0.0);
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    h2RendererM36=h2MenuMultiply(h2RendererA36,h2RendererB36);
    vpOut[4].xyz=h2RendererM36.xyz;
    // renderer instruction 37
    vec4 h2RendererA37=h2MenuTemp11.wwww;
    vec4 h2RendererB37=vpConstant(7u).wwww;
    vec4 h2RendererC37=vec4(0.0);
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    h2RendererM37=h2MenuMultiply(h2RendererA37,h2RendererB37);
    vpOut[3].w=h2RendererM37.w;
    // renderer instruction 38
    vec4 h2RendererA38=h2MenuTemp11.yyyy;
    vec4 h2RendererB38=vpConstant(6u).wwww;
    vec4 h2RendererC38=-h2MenuTemp11.xxxx;
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    h2RendererM38=h2MenuMultiply(h2RendererA38,h2RendererB38)+h2RendererC38;
    vpOut[4].w=h2RendererM38.w;
    // renderer instruction 39
    vec4 h2RendererA39=vpOut[0].xyzz;
    vec4 h2RendererB39=h2MenuTemp1.wwww;
    vec4 h2RendererC39=vpConstant(11u).xyzz;
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    h2RendererM39=h2MenuMultiply(h2RendererA39,h2RendererB39)+h2RendererC39;
    vpOut[0].xyz=h2RendererM39.xyz;
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
