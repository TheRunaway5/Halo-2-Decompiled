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
    vpInput[3]=sourcePackedNormal(v3_cmp);
    vpInput[4]=sourcePackedNormal(v4_cmp);
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
    vec4 h2RendererB0=vpConstant(0u).xyzw;
    vec4 h2RendererC0=vec4(0.0);
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    vec4 h2RendererProduct0=h2MenuMultiply(h2RendererA0,h2RendererB0);
    precise float h2RendererDot0=(h2RendererProduct0.x+h2RendererProduct0.y)+h2RendererProduct0.z;
    h2RendererDot0+=h2RendererB0.w;
    h2RendererM0=vec4(h2RendererDot0);
    vpOut[0].x=h2RendererM0.x;
    // renderer instruction 1
    vec4 h2RendererA1=vpInput[0].xyzw;
    vec4 h2RendererB1=vpConstant(1u).xyzw;
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    vec4 h2RendererProduct1=h2MenuMultiply(h2RendererA1,h2RendererB1);
    precise float h2RendererDot1=(h2RendererProduct1.x+h2RendererProduct1.y)+h2RendererProduct1.z;
    h2RendererDot1+=h2RendererB1.w;
    h2RendererM1=vec4(h2RendererDot1);
    vpOut[0].y=h2RendererM1.y;
    // renderer instruction 2
    vec4 h2RendererA2=vpInput[0].xyzw;
    vec4 h2RendererB2=vpConstant(2u).xyzw;
    vec4 h2RendererC2=vec4(0.0);
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    vec4 h2RendererProduct2=h2MenuMultiply(h2RendererA2,h2RendererB2);
    precise float h2RendererDot2=(h2RendererProduct2.x+h2RendererProduct2.y)+h2RendererProduct2.z;
    h2RendererDot2+=h2RendererB2.w;
    h2RendererM2=vec4(h2RendererDot2);
    vpOut[0].z=h2RendererM2.z;
    // renderer instruction 3
    vec4 h2RendererA3=vpInput[0].xyzw;
    vec4 h2RendererB3=vpConstant(3u).xyzw;
    vec4 h2RendererC3=vec4(0.0);
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    vec4 h2RendererProduct3=h2MenuMultiply(h2RendererA3,h2RendererB3);
    precise float h2RendererDot3=(h2RendererProduct3.x+h2RendererProduct3.y)+h2RendererProduct3.z;
    h2RendererDot3+=h2RendererB3.w;
    h2RendererM3=vec4(h2RendererDot3);
    vpOut[0].w=h2RendererM3.w;
    // renderer instruction 4
    vec4 h2RendererA4=vpConstant(6u).xyzw;
    vec4 h2RendererB4=vpInput[3].xyzw;
    vec4 h2RendererC4=vec4(0.0);
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    vec4 h2RendererProduct4=h2MenuMultiply(h2RendererA4,h2RendererB4);
    precise float h2RendererDot4=(h2RendererProduct4.x+h2RendererProduct4.y)+h2RendererProduct4.z;
    h2RendererM4=vec4(h2RendererDot4);
    h2MenuTemp2.y=h2RendererM4.y;
    // renderer instruction 5
    vec4 h2RendererA5=vpConstant(6u).xyzw;
    vec4 h2RendererB5=vpInput[4].xyzw;
    vec4 h2RendererC5=vpOut[0].wwww;
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    vec4 h2RendererProduct5=h2MenuMultiply(h2RendererA5,h2RendererB5);
    precise float h2RendererDot5=(h2RendererProduct5.x+h2RendererProduct5.y)+h2RendererProduct5.z;
    h2RendererM5=vec4(h2RendererDot5);
    h2RendererL5=vec4(1.0/h2RendererC5.x);
    h2MenuTemp2.x=h2RendererM5.x;
    h2MenuTemp1.w=h2RendererL5.w;
    // renderer instruction 6
    vec4 h2RendererA6=vpConstant(6u).xyzw;
    vec4 h2RendererB6=vpInput[2].xyzw;
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    vec4 h2RendererProduct6=h2MenuMultiply(h2RendererA6,h2RendererB6);
    precise float h2RendererDot6=(h2RendererProduct6.x+h2RendererProduct6.y)+h2RendererProduct6.z;
    h2RendererM6=vec4(h2RendererDot6);
    h2MenuTemp2.z=h2RendererM6.z;
    // renderer instruction 7
    vec4 h2RendererA7=vpConstant(4u).xyzw;
    vec4 h2RendererB7=vpInput[2].xyzw;
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    vec4 h2RendererProduct7=h2MenuMultiply(h2RendererA7,h2RendererB7);
    precise float h2RendererDot7=(h2RendererProduct7.x+h2RendererProduct7.y)+h2RendererProduct7.z;
    h2RendererM7=vec4(h2RendererDot7);
    h2MenuTemp3.x=h2RendererM7.x;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp2.xyzw;
    vec4 h2RendererB8=vpConstant(5u).wwww;
    vec4 h2RendererC8=vpConstant(5u).wwww;
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    h2RendererM8=h2MenuMultiply(h2RendererA8,h2RendererB8)+h2RendererC8;
    vpOut[3].xyz=h2RendererM8.xyz;
    // renderer instruction 9
    vec4 h2RendererA9=vpConstant(5u).xyzw;
    vec4 h2RendererB9=vpInput[2].xyzw;
    vec4 h2RendererC9=vec4(0.0);
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    vec4 h2RendererProduct9=h2MenuMultiply(h2RendererA9,h2RendererB9);
    precise float h2RendererDot9=(h2RendererProduct9.x+h2RendererProduct9.y)+h2RendererProduct9.z;
    h2RendererM9=vec4(h2RendererDot9);
    h2MenuTemp3.y=h2RendererM9.y;
    // renderer instruction 10
    vec4 h2RendererA10=vpConstant(6u).xyzw;
    vec4 h2RendererB10=vpInput[2].xyzw;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererM10=vec4(h2RendererDot10);
    h2MenuTemp3.z=h2RendererM10.z;
    // renderer instruction 11
    vec4 h2RendererA11=vpInput[0].xyzw;
    vec4 h2RendererB11=vpConstant(14u).xyzw;
    vec4 h2RendererC11=vec4(0.0);
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererDot11+=h2RendererB11.w;
    h2RendererM11=vec4(h2RendererDot11);
    h2MenuTemp9.y=h2RendererM11.y;
    // renderer instruction 12
    vec4 h2RendererA12=vpInput[0].xyzw;
    vec4 h2RendererB12=vpConstant(13u).xyzw;
    vec4 h2RendererC12=h2MenuTemp3.xyzw;
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererB12.w;
    h2RendererM12=vec4(h2RendererDot12);
    h2RendererL12=h2RendererC12;
    h2MenuTemp9.x=h2RendererM12.x;
    vpOut[12].xyz=h2RendererL12.xyz;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp3.zzzz;
    vec4 h2RendererB13=vpConstant(15u).yyyy;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    h2RendererM13=max(h2RendererA13,h2RendererB13);
    h2MenuTemp4.z=h2RendererM13.z;
    // renderer instruction 14
    vec4 h2RendererA14=vpInput[0].xyzw;
    vec4 h2RendererB14=vpConstant(12u).xyzw;
    vec4 h2RendererC14=h2MenuTemp4.zzzz;
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    vec4 h2RendererProduct14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    precise float h2RendererDot14=(h2RendererProduct14.x+h2RendererProduct14.y)+h2RendererProduct14.z;
    h2RendererDot14+=h2RendererB14.w;
    h2RendererM14=vec4(h2RendererDot14);
    float h2RendererLog14=log2(abs(h2RendererC14.x));
    h2RendererL14=vec4(floor(h2RendererLog14),abs(h2RendererC14.x)/exp2(floor(h2RendererLog14)),h2RendererLog14,1.0);
    h2MenuTemp9.z=h2RendererM14.z;
    h2MenuTemp1.x=h2RendererL14.x;
    // renderer instruction 15
    vec4 h2RendererA15=vpConstant(4u).wwww;
    vec4 h2RendererB15=vec4(0.0);
    vec4 h2RendererC15=-h2MenuTemp9.xyzw;
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    h2RendererM15=h2RendererA15+h2RendererC15;
    h2MenuTemp10.xyz=h2RendererM15.xyz;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp1.xxxx;
    vec4 h2RendererB16=vpConstant(27u).xyyy;
    vec4 h2RendererC16=vec4(0.0);
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=h2MenuMultiply(h2RendererA16,h2RendererB16);
    h2MenuTemp5.xy=h2RendererM16.xy;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp10.xyzw;
    vec4 h2RendererB17=vpConstant(15u).yyyy;
    vec4 h2RendererC17=h2MenuTemp5.xyzw;
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    h2RendererM17=max(h2RendererA17,h2RendererB17);
    h2RendererL17=vec4(exp2(floor(h2RendererC17.x)),fract(h2RendererC17.x),exp2(h2RendererC17.x),1.0);
    h2MenuTemp11.xyz=h2RendererM17.xyz;
    h2MenuTemp1.x=h2RendererL17.x;
    // renderer instruction 18
    vec4 h2RendererA18=vpOut[0].xyzw;
    vec4 h2RendererB18=vpConstant(21u).xyzw;
    vec4 h2RendererC18=vec4(0.0);
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    vec4 h2RendererProduct18=h2MenuMultiply(h2RendererA18,h2RendererB18);
    precise float h2RendererDot18=(h2RendererProduct18.x+h2RendererProduct18.y)+h2RendererProduct18.z;
    h2RendererDot18+=h2RendererProduct18.w;
    h2RendererM18=vec4(h2RendererDot18);
    h2MenuTemp2.w=h2RendererM18.w;
    // renderer instruction 19
    vec4 h2RendererA19=h2MenuTemp1.xxxx;
    vec4 h2RendererB19=vpConstant(26u).xxxx;
    vec4 h2RendererC19=vec4(0.0);
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2MenuMultiply(h2RendererA19,h2RendererB19);
    h2MenuTemp6.zw=h2RendererM19.zw;
    // renderer instruction 20
    vec4 h2RendererA20=h2MenuTemp11.xyzw;
    vec4 h2RendererB20=h2MenuTemp11.xyzw;
    vec4 h2RendererC20=h2MenuTemp5.yyyy;
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    h2RendererM20=h2MenuMultiply(h2RendererA20,h2RendererB20);
    h2RendererL20=vec4(exp2(floor(h2RendererC20.x)),fract(h2RendererC20.x),exp2(h2RendererC20.x),1.0);
    h2MenuTemp0.xyz=h2RendererM20.xyz;
    h2MenuTemp1.x=h2RendererL20.x;
    // renderer instruction 21
    vec4 h2RendererA21=vpOut[0].xyzw;
    vec4 h2RendererB21=vpConstant(22u).xyzw;
    vec4 h2RendererC21=vec4(0.0);
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    vec4 h2RendererProduct21=h2MenuMultiply(h2RendererA21,h2RendererB21);
    precise float h2RendererDot21=(h2RendererProduct21.x+h2RendererProduct21.y)+h2RendererProduct21.z;
    h2RendererDot21+=h2RendererProduct21.w;
    h2RendererM21=vec4(h2RendererDot21);
    h2MenuTemp3.w=h2RendererM21.w;
    // renderer instruction 22
    vec4 h2RendererA22=h2MenuTemp0.xyzw;
    vec4 h2RendererB22=vpConstant(4u).wwww;
    vec4 h2RendererC22=vec4(0.0);
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    h2RendererM22=min(h2RendererA22,h2RendererB22);
    h2MenuTemp2.xyz=h2RendererM22.xyz;
    // renderer instruction 23
    vec4 h2RendererA23=h2MenuTemp3.xyzw;
    vec4 h2RendererB23=h2MenuTemp1.xxxx;
    vec4 h2RendererC23=vec4(0.0);
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=h2MenuMultiply(h2RendererA23,h2RendererB23);
    h2MenuTemp7.xy=h2RendererM23.xy;
    // renderer instruction 24
    vec4 h2RendererA24=h2MenuTemp7.xyyy;
    vec4 h2RendererB24=vpConstant(26u).yyyy;
    vec4 h2RendererC24=vpConstant(26u).zwww;
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    h2RendererM24=h2MenuMultiply(h2RendererA24,h2RendererB24)+h2RendererC24;
    h2MenuTemp6.xy=h2RendererM24.xy;
    // renderer instruction 25
    vec4 h2RendererA25=h2MenuTemp2.xxxx;
    vec4 h2RendererB25=vec4(0.0);
    vec4 h2RendererC25=h2MenuTemp2.yyyy;
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererM25=h2RendererA25+h2RendererC25;
    h2MenuTemp3.x=h2RendererM25.x;
    // renderer instruction 26
    vec4 h2RendererA26=h2MenuTemp6.xyzw;
    vec4 h2RendererB26=vpConstant(27u).zzzz;
    vec4 h2RendererC26=vec4(0.0);
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    h2RendererM26=h2MenuMultiply(h2RendererA26,h2RendererB26);
    h2MenuTemp8.xyzw=h2RendererM26.xyzw;
    // renderer instruction 27
    vec4 h2RendererA27=h2MenuTemp3.xyzw;
    vec4 h2RendererB27=vpConstant(4u).wwww;
    vec4 h2RendererC27=vec4(0.0);
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=min(h2RendererA27,h2RendererB27);
    h2MenuTemp2.x=h2RendererM27.x;
    // renderer instruction 28
    vec4 h2RendererA28=vpConstant(4u).wwww;
    vec4 h2RendererB28=vec4(0.0);
    vec4 h2RendererC28=-h2MenuTemp2.xyzw;
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2RendererA28+h2RendererC28;
    h2MenuTemp4.xyz=h2RendererM28.xyz;
    // renderer instruction 29
    vec4 h2RendererA29=vpInput[1].xyzw;
    vec4 h2RendererB29=vpConstant(172u).xyyy;
    vec4 h2RendererC29=vpConstant(172u).zwww;
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29)+h2RendererC29;
    h2MenuTemp0.xy=h2RendererM29.xy;
    // renderer instruction 30
    vec4 h2RendererA30=h2MenuTemp4.xyzw;
    vec4 h2RendererB30=h2MenuTemp4.xyzw;
    vec4 h2RendererC30=vec4(0.0);
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=h2MenuMultiply(h2RendererA30,h2RendererB30);
    h2MenuTemp5.xyz=h2RendererM30.xyz;
    // renderer instruction 31
    vec4 h2RendererA31=h2MenuTemp5.yyyy;
    vec4 h2RendererB31=vec4(0.0);
    vec4 h2RendererC31=-h2MenuTemp5.xxxx;
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    h2RendererM31=h2RendererA31+h2RendererC31;
    h2MenuTemp6.y=h2RendererM31.y;
    // renderer instruction 32
    vec4 h2RendererA32=h2MenuTemp0.xyzw;
    vec4 h2RendererB32=vpConstant(18u).xyyy;
    vec4 h2RendererC32=vpConstant(18u).zwww;
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    h2RendererM32=h2MenuMultiply(h2RendererA32,h2RendererB32)+h2RendererC32;
    h2MenuTemp2.xy=h2RendererM32.xy;
    // renderer instruction 33
    vec4 h2RendererA33=vpConstant(15u).xxxx;
    vec4 h2RendererB33=h2MenuTemp6.yyyy;
    vec4 h2RendererC33=h2MenuTemp5.xxxx;
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33)+h2RendererC33;
    h2MenuTemp5.w=h2RendererM33.w;
    // renderer instruction 34
    vec4 h2RendererA34=h2MenuTemp2.xyzw;
    vec4 h2RendererB34=vpConstant(20u).xyyy;
    vec4 h2RendererC34=vpConstant(20u).zwww;
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    h2RendererM34=h2MenuMultiply(h2RendererA34,h2RendererB34)+h2RendererC34;
    vpOut[9].xy=h2RendererM34.xy;
    // renderer instruction 35
    vec4 h2RendererA35=h2MenuTemp5.xyzw;
    vec4 h2RendererB35=vpConstant(15u).zwzw;
    vec4 h2RendererC35=vec4(0.0);
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    h2RendererM35=h2MenuMultiply(h2RendererA35,h2RendererB35);
    h2MenuTemp7.zw=h2RendererM35.zw;
    // renderer instruction 36
    vec4 h2RendererA36=-h2MenuTemp7.xyzw;
    vec4 h2RendererB36=vec4(0.0);
    vec4 h2RendererC36=vpConstant(4u).wwww;
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    h2RendererM36=h2RendererA36+h2RendererC36;
    h2MenuTemp9.zw=h2RendererM36.zw;
    // renderer instruction 37
    vec4 h2RendererA37=vpConstant(7u).xyzw;
    vec4 h2RendererB37=vec4(0.0);
    vec4 h2RendererC37=-vpInput[0].xyzw;
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    h2RendererM37=h2RendererA37+h2RendererC37;
    h2MenuTemp6.xyz=h2RendererM37.xyz;
    // renderer instruction 38
    vec4 h2RendererA38=h2MenuTemp9.zzzz;
    vec4 h2RendererB38=h2MenuTemp9.wwww;
    vec4 h2RendererC38=vec4(0.0);
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    h2RendererM38=h2MenuMultiply(h2RendererA38,h2RendererB38);
    h2MenuTemp10.w=h2RendererM38.w;
    // renderer instruction 39
    vec4 h2RendererA39=h2MenuTemp8.xyzw;
    vec4 h2RendererB39=h2MenuTemp10.wwww;
    vec4 h2RendererC39=h2MenuTemp10.xyzw;
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    h2RendererM39=h2MenuMultiply(h2RendererA39,h2RendererB39);
    h2RendererL39=h2RendererC39;
    h2MenuTemp11.xyzw=h2RendererM39.xyzw;
    vpOut[4].w=h2RendererL39.w;
    // renderer instruction 40
    vec4 h2RendererA40=h2MenuTemp2.wwww;
    vec4 h2RendererB40=h2MenuTemp1.wwww;
    vec4 h2RendererC40=h2MenuTemp11.xxxx;
    precise vec4 h2RendererM40=vec4(0.0),h2RendererL40=vec4(0.0);
    h2RendererM40=h2MenuMultiply(h2RendererA40,h2RendererB40)+h2RendererC40;
    vpOut[10].z=h2RendererM40.z;
    // renderer instruction 41
    vec4 h2RendererA41=vpConstant(4u).xyzw;
    vec4 h2RendererB41=h2MenuTemp11.zzzz;
    vec4 h2RendererC41=vec4(0.0);
    precise vec4 h2RendererM41=vec4(0.0),h2RendererL41=vec4(0.0);
    h2RendererM41=h2MenuMultiply(h2RendererA41,h2RendererB41);
    h2MenuTemp4.xyz=h2RendererM41.xyz;
    // renderer instruction 42
    vec4 h2RendererA42=vpConstant(5u).xyzw;
    vec4 h2RendererB42=h2MenuTemp11.wwww;
    vec4 h2RendererC42=vec4(0.0);
    precise vec4 h2RendererM42=vec4(0.0),h2RendererL42=vec4(0.0);
    h2RendererM42=h2MenuMultiply(h2RendererA42,h2RendererB42);
    h2MenuTemp5.xyz=h2RendererM42.xyz;
    // renderer instruction 43
    vec4 h2RendererA43=h2MenuTemp4.xyzw;
    vec4 h2RendererB43=vpInput[4].xyzw;
    vec4 h2RendererC43=vec4(0.0);
    precise vec4 h2RendererM43=vec4(0.0),h2RendererL43=vec4(0.0);
    vec4 h2RendererProduct43=h2MenuMultiply(h2RendererA43,h2RendererB43);
    precise float h2RendererDot43=(h2RendererProduct43.x+h2RendererProduct43.y)+h2RendererProduct43.z;
    h2RendererM43=vec4(h2RendererDot43);
    vpOut[10].x=h2RendererM43.x;
    // renderer instruction 44
    vec4 h2RendererA44=h2MenuTemp4.xyzw;
    vec4 h2RendererB44=vpInput[3].xyzw;
    vec4 h2RendererC44=vec4(0.0);
    precise vec4 h2RendererM44=vec4(0.0),h2RendererL44=vec4(0.0);
    vec4 h2RendererProduct44=h2MenuMultiply(h2RendererA44,h2RendererB44);
    precise float h2RendererDot44=(h2RendererProduct44.x+h2RendererProduct44.y)+h2RendererProduct44.z;
    h2RendererM44=vec4(h2RendererDot44);
    vpOut[10].y=h2RendererM44.y;
    // renderer instruction 45
    vec4 h2RendererA45=h2MenuTemp5.xyzw;
    vec4 h2RendererB45=vpInput[4].xyzw;
    vec4 h2RendererC45=vec4(0.0);
    precise vec4 h2RendererM45=vec4(0.0),h2RendererL45=vec4(0.0);
    vec4 h2RendererProduct45=h2MenuMultiply(h2RendererA45,h2RendererB45);
    precise float h2RendererDot45=(h2RendererProduct45.x+h2RendererProduct45.y)+h2RendererProduct45.z;
    h2RendererM45=vec4(h2RendererDot45);
    vpOut[11].x=h2RendererM45.x;
    // renderer instruction 46
    vec4 h2RendererA46=h2MenuTemp5.xyzw;
    vec4 h2RendererB46=vpInput[3].xyzw;
    vec4 h2RendererC46=vec4(0.0);
    precise vec4 h2RendererM46=vec4(0.0),h2RendererL46=vec4(0.0);
    vec4 h2RendererProduct46=h2MenuMultiply(h2RendererA46,h2RendererB46);
    precise float h2RendererDot46=(h2RendererProduct46.x+h2RendererProduct46.y)+h2RendererProduct46.z;
    h2RendererM46=vec4(h2RendererDot46);
    vpOut[11].y=h2RendererM46.y;
    // renderer instruction 47
    vec4 h2RendererA47=h2MenuTemp3.wwww;
    vec4 h2RendererB47=h2MenuTemp1.wwww;
    vec4 h2RendererC47=h2MenuTemp11.yyyy;
    precise vec4 h2RendererM47=vec4(0.0),h2RendererL47=vec4(0.0);
    h2RendererM47=h2MenuMultiply(h2RendererA47,h2RendererB47)+h2RendererC47;
    vpOut[11].z=h2RendererM47.z;
    // renderer instruction 48
    vec4 h2RendererA48=-h2MenuTemp6.xyzw;
    vec4 h2RendererB48=vpConstant(6u).xyzw;
    vec4 h2RendererC48=vec4(0.0);
    precise vec4 h2RendererM48=vec4(0.0),h2RendererL48=vec4(0.0);
    vec4 h2RendererProduct48=h2MenuMultiply(h2RendererA48,h2RendererB48);
    precise float h2RendererDot48=(h2RendererProduct48.x+h2RendererProduct48.y)+h2RendererProduct48.z;
    h2RendererM48=vec4(h2RendererDot48);
    h2MenuTemp7.w=h2RendererM48.w;
    // renderer instruction 49
    vec4 h2RendererA49=vpOut[0].xyzw;
    vec4 h2RendererB49=vpConstant(10u).xyzw;
    vec4 h2RendererC49=vpOut[0].wwww;
    precise vec4 h2RendererM49=vec4(0.0),h2RendererL49=vec4(0.0);
    h2RendererM49=h2MenuMultiply(h2RendererA49,h2RendererB49);
    float h2RendererReciprocal49=1.0/h2RendererC49.x;
    if(h2RendererReciprocal49>0.0 || floatBitsToUint(h2RendererReciprocal49)==0u)
        h2RendererReciprocal49=clamp(h2RendererReciprocal49,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal49)) h2RendererReciprocal49=clamp(h2RendererReciprocal49,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL49=vec4(h2RendererReciprocal49);
    h2MenuTemp1.w=h2RendererL49.w;
    vpOut[0].xyz=h2RendererM49.xyz;
    // renderer instruction 50
    vec4 h2RendererA50=h2MenuTemp7.wwww;
    vec4 h2RendererB50=vpConstant(23u).xxxx;
    vec4 h2RendererC50=vpConstant(23u).yyyy;
    precise vec4 h2RendererM50=vec4(0.0),h2RendererL50=vec4(0.0);
    h2RendererM50=h2MenuMultiply(h2RendererA50,h2RendererB50)+h2RendererC50;
    vpOut[5].xw=h2RendererM50.xw;
    // renderer instruction 51
    vec4 h2RendererA51=vpOut[0].xyzw;
    vec4 h2RendererB51=h2MenuTemp1.wwww;
    vec4 h2RendererC51=vpConstant(11u).xyzz;
    precise vec4 h2RendererM51=vec4(0.0),h2RendererL51=vec4(0.0);
    h2RendererM51=h2MenuMultiply(h2RendererA51,h2RendererB51)+h2RendererC51;
    vpOut[0].xyz=h2RendererM51.xyz;
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
