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
    vec4 h2RendererA0=vpInput[0].xyzz;
    vec4 h2RendererB0=vpConstant(170u).xyzz;
    vec4 h2RendererC0=vec4(0.0);
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    h2RendererM0=h2MenuMultiply(h2RendererA0,h2RendererB0);
    h2MenuTemp10.xyz=h2RendererM0.xyz;
    // renderer instruction 1
    vec4 h2RendererA1=vpInput[2].xyzz;
    vec4 h2RendererB1=vpConstant(50u).xyzz;
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    vec4 h2RendererProduct1=h2MenuMultiply(h2RendererA1,h2RendererB1);
    precise float h2RendererDot1=(h2RendererProduct1.x+h2RendererProduct1.y)+h2RendererProduct1.z;
    h2RendererM1=vec4(h2RendererDot1);
    h2MenuTemp11.x=h2RendererM1.x;
    // renderer instruction 2
    vec4 h2RendererA2=h2MenuTemp10.xyzz;
    vec4 h2RendererB2=vec4(0.0);
    vec4 h2RendererC2=vpConstant(171u).xyzz;
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    h2RendererM2=h2RendererA2+h2RendererC2;
    h2MenuTemp10.xyz=h2RendererM2.xyz;
    // renderer instruction 3
    vec4 h2RendererA3=vpInput[2].xyzz;
    vec4 h2RendererB3=vpConstant(51u).xyzz;
    vec4 h2RendererC3=vec4(0.0);
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    vec4 h2RendererProduct3=h2MenuMultiply(h2RendererA3,h2RendererB3);
    precise float h2RendererDot3=(h2RendererProduct3.x+h2RendererProduct3.y)+h2RendererProduct3.z;
    h2RendererM3=vec4(h2RendererDot3);
    h2MenuTemp11.y=h2RendererM3.y;
    // renderer instruction 4
    vec4 h2RendererA4=h2MenuTemp10.xyzw;
    vec4 h2RendererB4=vpConstant(50u).xyzw;
    vec4 h2RendererC4=vec4(0.0);
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    vec4 h2RendererProduct4=h2MenuMultiply(h2RendererA4,h2RendererB4);
    precise float h2RendererDot4=(h2RendererProduct4.x+h2RendererProduct4.y)+h2RendererProduct4.z;
    h2RendererDot4+=h2RendererB4.w;
    h2RendererM4=vec4(h2RendererDot4);
    h2MenuTemp0.x=h2RendererM4.x;
    // renderer instruction 5
    vec4 h2RendererA5=h2MenuTemp10.xyzw;
    vec4 h2RendererB5=vpConstant(51u).xyzw;
    vec4 h2RendererC5=vec4(0.0);
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    vec4 h2RendererProduct5=h2MenuMultiply(h2RendererA5,h2RendererB5);
    precise float h2RendererDot5=(h2RendererProduct5.x+h2RendererProduct5.y)+h2RendererProduct5.z;
    h2RendererDot5+=h2RendererB5.w;
    h2RendererM5=vec4(h2RendererDot5);
    h2MenuTemp0.y=h2RendererM5.y;
    // renderer instruction 6
    vec4 h2RendererA6=h2MenuTemp10.xyzw;
    vec4 h2RendererB6=vpConstant(52u).xyzw;
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    vec4 h2RendererProduct6=h2MenuMultiply(h2RendererA6,h2RendererB6);
    precise float h2RendererDot6=(h2RendererProduct6.x+h2RendererProduct6.y)+h2RendererProduct6.z;
    h2RendererDot6+=h2RendererB6.w;
    h2RendererM6=vec4(h2RendererDot6);
    h2MenuTemp0.z=h2RendererM6.z;
    // renderer instruction 7
    vec4 h2RendererA7=vpInput[2].xyzz;
    vec4 h2RendererB7=vpConstant(52u).xyzz;
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    vec4 h2RendererProduct7=h2MenuMultiply(h2RendererA7,h2RendererB7);
    precise float h2RendererDot7=(h2RendererProduct7.x+h2RendererProduct7.y)+h2RendererProduct7.z;
    h2RendererM7=vec4(h2RendererDot7);
    h2MenuTemp11.z=h2RendererM7.z;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp0.xyzw;
    vec4 h2RendererB8=vpConstant(13u).xyzw;
    vec4 h2RendererC8=vec4(0.0);
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    vec4 h2RendererProduct8=h2MenuMultiply(h2RendererA8,h2RendererB8);
    precise float h2RendererDot8=(h2RendererProduct8.x+h2RendererProduct8.y)+h2RendererProduct8.z;
    h2RendererDot8+=h2RendererB8.w;
    h2RendererM8=vec4(h2RendererDot8);
    h2MenuTemp8.x=h2RendererM8.x;
    // renderer instruction 9
    vec4 h2RendererA9=h2MenuTemp0.xyzw;
    vec4 h2RendererB9=vpConstant(14u).xyzw;
    vec4 h2RendererC9=vec4(0.0);
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    vec4 h2RendererProduct9=h2MenuMultiply(h2RendererA9,h2RendererB9);
    precise float h2RendererDot9=(h2RendererProduct9.x+h2RendererProduct9.y)+h2RendererProduct9.z;
    h2RendererDot9+=h2RendererB9.w;
    h2RendererM9=vec4(h2RendererDot9);
    h2MenuTemp8.y=h2RendererM9.y;
    // renderer instruction 10
    vec4 h2RendererA10=h2MenuTemp0.xyzw;
    vec4 h2RendererB10=vpConstant(12u).xyzw;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererDot10+=h2RendererB10.w;
    h2RendererM10=vec4(h2RendererDot10);
    h2MenuTemp8.z=h2RendererM10.z;
    // renderer instruction 11
    vec4 h2RendererA11=vpConstant(4u).wwww;
    vec4 h2RendererB11=vec4(0.0);
    vec4 h2RendererC11=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    h2RendererM11=h2RendererA11+h2RendererC11;
    h2MenuTemp8.xyz=h2RendererM11.xyz;
    // renderer instruction 12
    vec4 h2RendererA12=vpConstant(6u).xyzz;
    vec4 h2RendererB12=h2MenuTemp11.xyzz;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererM12=vec4(h2RendererDot12);
    h2MenuTemp5.z=h2RendererM12.z;
    vpOut[12].z=h2RendererM12.z;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp8.xyzz;
    vec4 h2RendererB13=vpConstant(15u).yyyy;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    h2RendererM13=max(h2RendererA13,h2RendererB13);
    h2MenuTemp8.xyz=h2RendererM13.xyz;
    // renderer instruction 14
    vec4 h2RendererA14=h2MenuTemp8.xyzz;
    vec4 h2RendererB14=h2MenuTemp8.xyzz;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    h2RendererM14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    h2MenuTemp8.xyz=h2RendererM14.xyz;
    // renderer instruction 15
    vec4 h2RendererA15=h2MenuTemp8.xyzz;
    vec4 h2RendererB15=vpConstant(4u).wwww;
    vec4 h2RendererC15=vec4(0.0);
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    h2RendererM15=min(h2RendererA15,h2RendererB15);
    h2MenuTemp8.xyz=h2RendererM15.xyz;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp5.zzzz;
    vec4 h2RendererB16=vpConstant(15u).yyyy;
    vec4 h2RendererC16=vec4(0.0);
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=max(h2RendererA16,h2RendererB16);
    h2MenuTemp5.z=h2RendererM16.z;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp8.xxxx;
    vec4 h2RendererB17=vec4(0.0);
    vec4 h2RendererC17=h2MenuTemp8.yyyy;
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    h2RendererM17=h2RendererA17+h2RendererC17;
    h2MenuTemp8.x=h2RendererM17.x;
    // renderer instruction 18
    vec4 h2RendererA18=vpConstant(4u).xyzz;
    vec4 h2RendererB18=h2MenuTemp11.xyzz;
    vec4 h2RendererC18=h2MenuTemp5.zzzz;
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    vec4 h2RendererProduct18=h2MenuMultiply(h2RendererA18,h2RendererB18);
    precise float h2RendererDot18=(h2RendererProduct18.x+h2RendererProduct18.y)+h2RendererProduct18.z;
    h2RendererM18=vec4(h2RendererDot18);
    float h2RendererLog18=log2(abs(h2RendererC18.x));
    h2RendererL18=vec4(floor(h2RendererLog18),abs(h2RendererC18.x)/exp2(floor(h2RendererLog18)),h2RendererLog18,1.0);
    h2MenuTemp5.x=h2RendererM18.x;
    h2MenuTemp1.x=h2RendererL18.x;
    // renderer instruction 19
    vec4 h2RendererA19=h2MenuTemp8.xxxx;
    vec4 h2RendererB19=vpConstant(4u).wwww;
    vec4 h2RendererC19=vec4(0.0);
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=min(h2RendererA19,h2RendererB19);
    h2MenuTemp8.x=h2RendererM19.x;
    // renderer instruction 20
    vec4 h2RendererA20=h2MenuTemp1.xxxx;
    vec4 h2RendererB20=vpConstant(27u).xyyy;
    vec4 h2RendererC20=vec4(0.0);
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    h2RendererM20=h2MenuMultiply(h2RendererA20,h2RendererB20);
    h2MenuTemp4.xy=h2RendererM20.xy;
    // renderer instruction 21
    vec4 h2RendererA21=vpConstant(4u).wwww;
    vec4 h2RendererB21=vec4(0.0);
    vec4 h2RendererC21=-h2MenuTemp8.xyzz;
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    h2RendererM21=h2RendererA21+h2RendererC21;
    h2MenuTemp8.xyz=h2RendererM21.xyz;
    // renderer instruction 22
    vec4 h2RendererA22=vpConstant(5u).xyzz;
    vec4 h2RendererB22=h2MenuTemp11.xyzz;
    vec4 h2RendererC22=h2MenuTemp4.yyyy;
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    vec4 h2RendererProduct22=h2MenuMultiply(h2RendererA22,h2RendererB22);
    precise float h2RendererDot22=(h2RendererProduct22.x+h2RendererProduct22.y)+h2RendererProduct22.z;
    h2RendererM22=vec4(h2RendererDot22);
    h2RendererL22=vec4(exp2(floor(h2RendererC22.x)),fract(h2RendererC22.x),exp2(h2RendererC22.x),1.0);
    h2MenuTemp5.y=h2RendererM22.y;
    h2MenuTemp1.x=h2RendererL22.x;
    // renderer instruction 23
    vec4 h2RendererA23=h2MenuTemp8.xyzz;
    vec4 h2RendererB23=h2MenuTemp8.xyzz;
    vec4 h2RendererC23=vec4(0.0);
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=h2MenuMultiply(h2RendererA23,h2RendererB23);
    h2MenuTemp8.xyz=h2RendererM23.xyz;
    // renderer instruction 24
    vec4 h2RendererA24=h2MenuTemp8.yyyy;
    vec4 h2RendererB24=vec4(0.0);
    vec4 h2RendererC24=-h2MenuTemp8.xxxx;
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    h2RendererM24=h2RendererA24+h2RendererC24;
    h2MenuTemp8.y=h2RendererM24.y;
    // renderer instruction 25
    vec4 h2RendererA25=vec4(0.0);
    vec4 h2RendererB25=vec4(0.0);
    vec4 h2RendererC25=h2MenuTemp4.xxxx;
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererL25=vec4(exp2(floor(h2RendererC25.x)),fract(h2RendererC25.x),exp2(h2RendererC25.x),1.0);
    h2MenuTemp4.x=h2RendererL25.x;
    // renderer instruction 26
    vec4 h2RendererA26=vpConstant(15u).xxxx;
    vec4 h2RendererB26=h2MenuTemp8.yyyy;
    vec4 h2RendererC26=h2MenuTemp8.xxxx;
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    h2RendererM26=h2MenuMultiply(h2RendererA26,h2RendererB26)+h2RendererC26;
    h2MenuTemp8.w=h2RendererM26.w;
    // renderer instruction 27
    vec4 h2RendererA27=h2MenuTemp5.xyyy;
    vec4 h2RendererB27=h2MenuTemp1.xxxx;
    vec4 h2RendererC27=h2MenuTemp5.xyzz;
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2MenuMultiply(h2RendererA27,h2RendererB27);
    h2RendererL27=h2RendererC27;
    h2MenuTemp5.xy=h2RendererM27.xy;
    vpOut[12].xy=h2RendererL27.xy;
    // renderer instruction 28
    vec4 h2RendererA28=h2MenuTemp8.zzzw;
    vec4 h2RendererB28=vpConstant(15u).zzzw;
    vec4 h2RendererC28=vec4(0.0);
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2MenuMultiply(h2RendererA28,h2RendererB28);
    h2MenuTemp8.zw=h2RendererM28.zw;
    // renderer instruction 29
    vec4 h2RendererA29=h2MenuTemp4.xxxx;
    vec4 h2RendererB29=vpConstant(26u).xxxx;
    vec4 h2RendererC29=vec4(0.0);
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29);
    h2MenuTemp5.z=h2RendererM29.z;
    // renderer instruction 30
    vec4 h2RendererA30=h2MenuTemp5.xyyy;
    vec4 h2RendererB30=vpConstant(26u).yyyy;
    vec4 h2RendererC30=vpConstant(26u).zwww;
    precise vec4 h2RendererM30=vec4(0.0),h2RendererL30=vec4(0.0);
    h2RendererM30=h2MenuMultiply(h2RendererA30,h2RendererB30)+h2RendererC30;
    h2MenuTemp5.xy=h2RendererM30.xy;
    // renderer instruction 31
    vec4 h2RendererA31=-h2MenuTemp8.zzzw;
    vec4 h2RendererB31=vec4(0.0);
    vec4 h2RendererC31=vpConstant(4u).wwww;
    precise vec4 h2RendererM31=vec4(0.0),h2RendererL31=vec4(0.0);
    h2RendererM31=h2RendererA31+h2RendererC31;
    h2MenuTemp8.zw=h2RendererM31.zw;
    // renderer instruction 32
    vec4 h2RendererA32=h2MenuTemp5.xyzz;
    vec4 h2RendererB32=vpConstant(27u).zzzz;
    vec4 h2RendererC32=vec4(0.0);
    precise vec4 h2RendererM32=vec4(0.0),h2RendererL32=vec4(0.0);
    h2RendererM32=h2MenuMultiply(h2RendererA32,h2RendererB32);
    h2MenuTemp5.xyz=h2RendererM32.xyz;
    // renderer instruction 33
    vec4 h2RendererA33=h2MenuTemp8.zzzz;
    vec4 h2RendererB33=h2MenuTemp8.wwww;
    vec4 h2RendererC33=vec4(0.0);
    precise vec4 h2RendererM33=vec4(0.0),h2RendererL33=vec4(0.0);
    h2RendererM33=h2MenuMultiply(h2RendererA33,h2RendererB33);
    h2MenuTemp10.w=h2RendererM33.w;
    vpOut[4].w=h2RendererM33.w;
    // renderer instruction 34
    vec4 h2RendererA34=h2MenuTemp5.xyzz;
    vec4 h2RendererB34=h2MenuTemp10.wwww;
    vec4 h2RendererC34=vec4(0.0);
    precise vec4 h2RendererM34=vec4(0.0),h2RendererL34=vec4(0.0);
    h2RendererM34=h2MenuMultiply(h2RendererA34,h2RendererB34);
    h2MenuTemp5.xyz=h2RendererM34.xyz;
    // renderer instruction 35
    vec4 h2RendererA35=h2MenuTemp0.xyzw;
    vec4 h2RendererB35=vpConstant(3u).xyzw;
    vec4 h2RendererC35=vec4(0.0);
    precise vec4 h2RendererM35=vec4(0.0),h2RendererL35=vec4(0.0);
    vec4 h2RendererProduct35=h2MenuMultiply(h2RendererA35,h2RendererB35);
    precise float h2RendererDot35=(h2RendererProduct35.x+h2RendererProduct35.y)+h2RendererProduct35.z;
    h2RendererDot35+=h2RendererB35.w;
    h2RendererM35=vec4(h2RendererDot35);
    vpOut[0].w=h2RendererM35.w;
    // renderer instruction 36
    vec4 h2RendererA36=h2MenuTemp0.xyzw;
    vec4 h2RendererB36=vpConstant(0u).xyzw;
    vec4 h2RendererC36=vec4(0.0);
    precise vec4 h2RendererM36=vec4(0.0),h2RendererL36=vec4(0.0);
    vec4 h2RendererProduct36=h2MenuMultiply(h2RendererA36,h2RendererB36);
    precise float h2RendererDot36=(h2RendererProduct36.x+h2RendererProduct36.y)+h2RendererProduct36.z;
    h2RendererDot36+=h2RendererB36.w;
    h2RendererM36=vec4(h2RendererDot36);
    vpOut[0].x=h2RendererM36.x;
    // renderer instruction 37
    vec4 h2RendererA37=h2MenuTemp0.xyzw;
    vec4 h2RendererB37=vpConstant(1u).xyzw;
    vec4 h2RendererC37=vpOut[0].wwww;
    precise vec4 h2RendererM37=vec4(0.0),h2RendererL37=vec4(0.0);
    vec4 h2RendererProduct37=h2MenuMultiply(h2RendererA37,h2RendererB37);
    precise float h2RendererDot37=(h2RendererProduct37.x+h2RendererProduct37.y)+h2RendererProduct37.z;
    h2RendererDot37+=h2RendererB37.w;
    h2RendererM37=vec4(h2RendererDot37);
    h2RendererL37=vec4(1.0/h2RendererC37.x);
    h2MenuTemp1.w=h2RendererL37.w;
    vpOut[0].y=h2RendererM37.y;
    // renderer instruction 38
    vec4 h2RendererA38=h2MenuTemp0.xyzw;
    vec4 h2RendererB38=vpConstant(2u).xyzw;
    vec4 h2RendererC38=vpOut[0].wwww;
    precise vec4 h2RendererM38=vec4(0.0),h2RendererL38=vec4(0.0);
    vec4 h2RendererProduct38=h2MenuMultiply(h2RendererA38,h2RendererB38);
    precise float h2RendererDot38=(h2RendererProduct38.x+h2RendererProduct38.y)+h2RendererProduct38.z;
    h2RendererDot38+=h2RendererB38.w;
    h2RendererM38=vec4(h2RendererDot38);
    float h2RendererReciprocal38=1.0/h2RendererC38.x;
    if(h2RendererReciprocal38>0.0 || floatBitsToUint(h2RendererReciprocal38)==0u)
        h2RendererReciprocal38=clamp(h2RendererReciprocal38,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal38)) h2RendererReciprocal38=clamp(h2RendererReciprocal38,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL38=vec4(h2RendererReciprocal38);
    h2MenuTemp1.x=h2RendererL38.x;
    vpOut[0].z=h2RendererM38.z;
    // renderer instruction 39
    vec4 h2RendererA39=vpInput[4].xyzz;
    vec4 h2RendererB39=vpConstant(50u).xyzz;
    vec4 h2RendererC39=vec4(0.0);
    precise vec4 h2RendererM39=vec4(0.0),h2RendererL39=vec4(0.0);
    vec4 h2RendererProduct39=h2MenuMultiply(h2RendererA39,h2RendererB39);
    precise float h2RendererDot39=(h2RendererProduct39.x+h2RendererProduct39.y)+h2RendererProduct39.z;
    h2RendererM39=vec4(h2RendererDot39);
    h2MenuTemp3.x=h2RendererM39.x;
    // renderer instruction 40
    vec4 h2RendererA40=vpInput[4].xyzz;
    vec4 h2RendererB40=vpConstant(51u).xyzz;
    vec4 h2RendererC40=vec4(0.0);
    precise vec4 h2RendererM40=vec4(0.0),h2RendererL40=vec4(0.0);
    vec4 h2RendererProduct40=h2MenuMultiply(h2RendererA40,h2RendererB40);
    precise float h2RendererDot40=(h2RendererProduct40.x+h2RendererProduct40.y)+h2RendererProduct40.z;
    h2RendererM40=vec4(h2RendererDot40);
    h2MenuTemp3.y=h2RendererM40.y;
    // renderer instruction 41
    vec4 h2RendererA41=vpInput[4].xyzz;
    vec4 h2RendererB41=vpConstant(52u).xyzz;
    vec4 h2RendererC41=vec4(0.0);
    precise vec4 h2RendererM41=vec4(0.0),h2RendererL41=vec4(0.0);
    vec4 h2RendererProduct41=h2MenuMultiply(h2RendererA41,h2RendererB41);
    precise float h2RendererDot41=(h2RendererProduct41.x+h2RendererProduct41.y)+h2RendererProduct41.z;
    h2RendererM41=vec4(h2RendererDot41);
    h2MenuTemp3.z=h2RendererM41.z;
    // renderer instruction 42
    vec4 h2RendererA42=vpInput[3].xyzz;
    vec4 h2RendererB42=vpConstant(50u).xyzz;
    vec4 h2RendererC42=vec4(0.0);
    precise vec4 h2RendererM42=vec4(0.0),h2RendererL42=vec4(0.0);
    vec4 h2RendererProduct42=h2MenuMultiply(h2RendererA42,h2RendererB42);
    precise float h2RendererDot42=(h2RendererProduct42.x+h2RendererProduct42.y)+h2RendererProduct42.z;
    h2RendererM42=vec4(h2RendererDot42);
    h2MenuTemp2.x=h2RendererM42.x;
    // renderer instruction 43
    vec4 h2RendererA43=vpInput[3].xyzz;
    vec4 h2RendererB43=vpConstant(51u).xyzz;
    vec4 h2RendererC43=vec4(0.0);
    precise vec4 h2RendererM43=vec4(0.0),h2RendererL43=vec4(0.0);
    vec4 h2RendererProduct43=h2MenuMultiply(h2RendererA43,h2RendererB43);
    precise float h2RendererDot43=(h2RendererProduct43.x+h2RendererProduct43.y)+h2RendererProduct43.z;
    h2RendererM43=vec4(h2RendererDot43);
    h2MenuTemp2.y=h2RendererM43.y;
    // renderer instruction 44
    vec4 h2RendererA44=vpInput[3].xyzz;
    vec4 h2RendererB44=vpConstant(52u).xyzz;
    vec4 h2RendererC44=vec4(0.0);
    precise vec4 h2RendererM44=vec4(0.0),h2RendererL44=vec4(0.0);
    vec4 h2RendererProduct44=h2MenuMultiply(h2RendererA44,h2RendererB44);
    precise float h2RendererDot44=(h2RendererProduct44.x+h2RendererProduct44.y)+h2RendererProduct44.z;
    h2RendererM44=vec4(h2RendererDot44);
    h2MenuTemp2.z=h2RendererM44.z;
    // renderer instruction 45
    vec4 h2RendererA45=vpInput[1].xyyy;
    vec4 h2RendererB45=vpConstant(172u).xyyy;
    vec4 h2RendererC45=vpConstant(172u).zwww;
    precise vec4 h2RendererM45=vec4(0.0),h2RendererL45=vec4(0.0);
    h2RendererM45=h2MenuMultiply(h2RendererA45,h2RendererB45)+h2RendererC45;
    h2MenuTemp9.xy=h2RendererM45.xy;
    // renderer instruction 46
    vec4 h2RendererA46=vpConstant(4u).xyzz;
    vec4 h2RendererB46=h2MenuTemp5.zzzz;
    vec4 h2RendererC46=vec4(0.0);
    precise vec4 h2RendererM46=vec4(0.0),h2RendererL46=vec4(0.0);
    h2RendererM46=h2MenuMultiply(h2RendererA46,h2RendererB46);
    h2MenuTemp10.xyz=h2RendererM46.xyz;
    // renderer instruction 47
    vec4 h2RendererA47=vpConstant(5u).yyzx;
    vec4 h2RendererB47=h2MenuTemp5.zzzz;
    vec4 h2RendererC47=vec4(0.0);
    precise vec4 h2RendererM47=vec4(0.0),h2RendererL47=vec4(0.0);
    h2RendererM47=h2MenuMultiply(h2RendererA47,h2RendererB47);
    h2MenuTemp8.yzw=h2RendererM47.yzw;
    // renderer instruction 48
    vec4 h2RendererA48=vpConstant(7u).yyzx;
    vec4 h2RendererB48=vec4(0.0);
    vec4 h2RendererC48=-h2MenuTemp0.yyzx;
    precise vec4 h2RendererM48=vec4(0.0),h2RendererL48=vec4(0.0);
    h2RendererM48=h2RendererA48+h2RendererC48;
    h2MenuTemp7.xzw=h2RendererM48.xzw;
    // renderer instruction 49
    vec4 h2RendererA49=vpConstant(6u).xyzz;
    vec4 h2RendererB49=h2MenuTemp3.xyzz;
    vec4 h2RendererC49=vec4(0.0);
    precise vec4 h2RendererM49=vec4(0.0),h2RendererL49=vec4(0.0);
    vec4 h2RendererProduct49=h2MenuMultiply(h2RendererA49,h2RendererB49);
    precise float h2RendererDot49=(h2RendererProduct49.x+h2RendererProduct49.y)+h2RendererProduct49.z;
    h2RendererM49=vec4(h2RendererDot49);
    h2MenuTemp6.w=h2RendererM49.w;
    // renderer instruction 50
    vec4 h2RendererA50=vpConstant(6u).xyzz;
    vec4 h2RendererB50=h2MenuTemp2.xyzz;
    vec4 h2RendererC50=vec4(0.0);
    precise vec4 h2RendererM50=vec4(0.0),h2RendererL50=vec4(0.0);
    vec4 h2RendererProduct50=h2MenuMultiply(h2RendererA50,h2RendererB50);
    precise float h2RendererDot50=(h2RendererProduct50.x+h2RendererProduct50.y)+h2RendererProduct50.z;
    h2RendererM50=vec4(h2RendererDot50);
    h2MenuTemp6.y=h2RendererM50.y;
    // renderer instruction 51
    vec4 h2RendererA51=vpConstant(6u).xyzz;
    vec4 h2RendererB51=h2MenuTemp11.xyzz;
    vec4 h2RendererC51=vec4(0.0);
    precise vec4 h2RendererM51=vec4(0.0),h2RendererL51=vec4(0.0);
    vec4 h2RendererProduct51=h2MenuMultiply(h2RendererA51,h2RendererB51);
    precise float h2RendererDot51=(h2RendererProduct51.x+h2RendererProduct51.y)+h2RendererProduct51.z;
    h2RendererM51=vec4(h2RendererDot51);
    h2MenuTemp6.z=h2RendererM51.z;
    // renderer instruction 52
    vec4 h2RendererA52=h2MenuTemp9.xyyy;
    vec4 h2RendererB52=vpConstant(18u).xyyy;
    vec4 h2RendererC52=vpConstant(18u).zwww;
    precise vec4 h2RendererM52=vec4(0.0),h2RendererL52=vec4(0.0);
    h2RendererM52=h2MenuMultiply(h2RendererA52,h2RendererB52)+h2RendererC52;
    h2MenuTemp9.xy=h2RendererM52.xy;
    // renderer instruction 53
    vec4 h2RendererA53=vpOut[0].xyzw;
    vec4 h2RendererB53=vpConstant(21u).xyzw;
    vec4 h2RendererC53=vec4(0.0);
    precise vec4 h2RendererM53=vec4(0.0),h2RendererL53=vec4(0.0);
    vec4 h2RendererProduct53=h2MenuMultiply(h2RendererA53,h2RendererB53);
    precise float h2RendererDot53=(h2RendererProduct53.x+h2RendererProduct53.y)+h2RendererProduct53.z;
    h2RendererDot53+=h2RendererProduct53.w;
    h2RendererM53=vec4(h2RendererDot53);
    h2MenuTemp10.w=h2RendererM53.w;
    // renderer instruction 54
    vec4 h2RendererA54=vpOut[0].xyzw;
    vec4 h2RendererB54=vpConstant(22u).xyzw;
    vec4 h2RendererC54=vec4(0.0);
    precise vec4 h2RendererM54=vec4(0.0),h2RendererL54=vec4(0.0);
    vec4 h2RendererProduct54=h2MenuMultiply(h2RendererA54,h2RendererB54);
    precise float h2RendererDot54=(h2RendererProduct54.x+h2RendererProduct54.y)+h2RendererProduct54.z;
    h2RendererDot54+=h2RendererProduct54.w;
    h2RendererM54=vec4(h2RendererDot54);
    h2MenuTemp11.w=h2RendererM54.w;
    // renderer instruction 55
    vec4 h2RendererA55=-h2MenuTemp7.wxzz;
    vec4 h2RendererB55=vpConstant(6u).xyzz;
    vec4 h2RendererC55=vec4(0.0);
    precise vec4 h2RendererM55=vec4(0.0),h2RendererL55=vec4(0.0);
    vec4 h2RendererProduct55=h2MenuMultiply(h2RendererA55,h2RendererB55);
    precise float h2RendererDot55=(h2RendererProduct55.x+h2RendererProduct55.y)+h2RendererProduct55.z;
    h2RendererM55=vec4(h2RendererDot55);
    h2MenuTemp5.w=h2RendererM55.w;
    // renderer instruction 56
    vec4 h2RendererA56=vpOut[0].xyzz;
    vec4 h2RendererB56=vpConstant(10u).xyzz;
    vec4 h2RendererC56=vec4(0.0);
    precise vec4 h2RendererM56=vec4(0.0),h2RendererL56=vec4(0.0);
    h2RendererM56=h2MenuMultiply(h2RendererA56,h2RendererB56);
    vpOut[0].xyz=h2RendererM56.xyz;
    // renderer instruction 57
    vec4 h2RendererA57=h2MenuTemp6.wyzz;
    vec4 h2RendererB57=vpConstant(5u).wwww;
    vec4 h2RendererC57=vpConstant(5u).wwww;
    precise vec4 h2RendererM57=vec4(0.0),h2RendererL57=vec4(0.0);
    h2RendererM57=h2MenuMultiply(h2RendererA57,h2RendererB57)+h2RendererC57;
    vpOut[3].xyz=h2RendererM57.xyz;
    // renderer instruction 58
    vec4 h2RendererA58=h2MenuTemp9.xyyy;
    vec4 h2RendererB58=vpConstant(20u).xyyy;
    vec4 h2RendererC58=vpConstant(20u).zwww;
    precise vec4 h2RendererM58=vec4(0.0),h2RendererL58=vec4(0.0);
    h2RendererM58=h2MenuMultiply(h2RendererA58,h2RendererB58)+h2RendererC58;
    vpOut[9].xy=h2RendererM58.xy;
    // renderer instruction 59
    vec4 h2RendererA59=h2MenuTemp10.xyzz;
    vec4 h2RendererB59=h2MenuTemp3.xyzz;
    vec4 h2RendererC59=vec4(0.0);
    precise vec4 h2RendererM59=vec4(0.0),h2RendererL59=vec4(0.0);
    vec4 h2RendererProduct59=h2MenuMultiply(h2RendererA59,h2RendererB59);
    precise float h2RendererDot59=(h2RendererProduct59.x+h2RendererProduct59.y)+h2RendererProduct59.z;
    h2RendererM59=vec4(h2RendererDot59);
    vpOut[10].x=h2RendererM59.x;
    // renderer instruction 60
    vec4 h2RendererA60=h2MenuTemp10.xyzz;
    vec4 h2RendererB60=h2MenuTemp2.xyzz;
    vec4 h2RendererC60=vec4(0.0);
    precise vec4 h2RendererM60=vec4(0.0),h2RendererL60=vec4(0.0);
    vec4 h2RendererProduct60=h2MenuMultiply(h2RendererA60,h2RendererB60);
    precise float h2RendererDot60=(h2RendererProduct60.x+h2RendererProduct60.y)+h2RendererProduct60.z;
    h2RendererM60=vec4(h2RendererDot60);
    vpOut[10].y=h2RendererM60.y;
    // renderer instruction 61
    vec4 h2RendererA61=h2MenuTemp10.wwww;
    vec4 h2RendererB61=h2MenuTemp1.wwww;
    vec4 h2RendererC61=h2MenuTemp5.xxxx;
    precise vec4 h2RendererM61=vec4(0.0),h2RendererL61=vec4(0.0);
    h2RendererM61=h2MenuMultiply(h2RendererA61,h2RendererB61)+h2RendererC61;
    vpOut[10].z=h2RendererM61.z;
    // renderer instruction 62
    vec4 h2RendererA62=h2MenuTemp8.wyzz;
    vec4 h2RendererB62=h2MenuTemp3.xyzz;
    vec4 h2RendererC62=vec4(0.0);
    precise vec4 h2RendererM62=vec4(0.0),h2RendererL62=vec4(0.0);
    vec4 h2RendererProduct62=h2MenuMultiply(h2RendererA62,h2RendererB62);
    precise float h2RendererDot62=(h2RendererProduct62.x+h2RendererProduct62.y)+h2RendererProduct62.z;
    h2RendererM62=vec4(h2RendererDot62);
    vpOut[11].x=h2RendererM62.x;
    // renderer instruction 63
    vec4 h2RendererA63=h2MenuTemp8.wyzz;
    vec4 h2RendererB63=h2MenuTemp2.xyzz;
    vec4 h2RendererC63=vec4(0.0);
    precise vec4 h2RendererM63=vec4(0.0),h2RendererL63=vec4(0.0);
    vec4 h2RendererProduct63=h2MenuMultiply(h2RendererA63,h2RendererB63);
    precise float h2RendererDot63=(h2RendererProduct63.x+h2RendererProduct63.y)+h2RendererProduct63.z;
    h2RendererM63=vec4(h2RendererDot63);
    vpOut[11].y=h2RendererM63.y;
    // renderer instruction 64
    vec4 h2RendererA64=h2MenuTemp11.wwww;
    vec4 h2RendererB64=h2MenuTemp1.wwww;
    vec4 h2RendererC64=h2MenuTemp5.yyyy;
    precise vec4 h2RendererM64=vec4(0.0),h2RendererL64=vec4(0.0);
    h2RendererM64=h2MenuMultiply(h2RendererA64,h2RendererB64)+h2RendererC64;
    vpOut[11].z=h2RendererM64.z;
    // renderer instruction 65
    vec4 h2RendererA65=h2MenuTemp5.wwww;
    vec4 h2RendererB65=vpConstant(23u).xxxx;
    vec4 h2RendererC65=vpConstant(23u).yyyy;
    precise vec4 h2RendererM65=vec4(0.0),h2RendererL65=vec4(0.0);
    h2RendererM65=h2MenuMultiply(h2RendererA65,h2RendererB65)+h2RendererC65;
    vpOut[5].xw=h2RendererM65.xw;
    // renderer instruction 66
    vec4 h2RendererA66=vpOut[0].xyzz;
    vec4 h2RendererB66=h2MenuTemp1.xxxx;
    vec4 h2RendererC66=vpConstant(11u).xyzz;
    precise vec4 h2RendererM66=vec4(0.0),h2RendererL66=vec4(0.0);
    h2RendererM66=h2MenuMultiply(h2RendererA66,h2RendererB66)+h2RendererC66;
    vpOut[0].xyz=h2RendererM66.xyz;
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
