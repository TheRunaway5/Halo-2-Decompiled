#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
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
    vec4 h2RendererA1=vpConstant(4u).wwww;
    vec4 h2RendererB1=vec4(0.0);
    vec4 h2RendererC1=vec4(0.0);
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    h2RendererM1=h2RendererA1;
    h2MenuTemp0.w=h2RendererM1.w;
    // renderer instruction 2
    vec4 h2RendererA2=h2MenuTemp10.xyzz;
    vec4 h2RendererB2=vec4(0.0);
    vec4 h2RendererC2=vpConstant(171u).xyzz;
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    h2RendererM2=h2RendererA2+h2RendererC2;
    h2MenuTemp10.xyz=h2RendererM2.xyz;
    // renderer instruction 3
    vec4 h2RendererA3=vpConstant(22u).wwww;
    vec4 h2RendererB3=vec4(0.0);
    vec4 h2RendererC3=vec4(0.0);
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    h2RendererM3=h2RendererA3;
    h2MenuTemp11.w=h2RendererM3.w;
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
    vec4 h2RendererA7=vpConstant(7u).xyzz;
    vec4 h2RendererB7=vec4(0.0);
    vec4 h2RendererC7=-h2MenuTemp0.xyzz;
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    h2RendererM7=h2RendererA7+h2RendererC7;
    h2MenuTemp10.xyz=h2RendererM7.xyz;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp0.xyzw;
    vec4 h2RendererB8=vpConstant(0u).xyzw;
    vec4 h2RendererC8=vec4(0.0);
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    vec4 h2RendererProduct8=h2MenuMultiply(h2RendererA8,h2RendererB8);
    precise float h2RendererDot8=(h2RendererProduct8.x+h2RendererProduct8.y)+h2RendererProduct8.z;
    h2RendererDot8+=h2RendererProduct8.w;
    h2RendererM8=vec4(h2RendererDot8);
    h2MenuTemp9.x=h2RendererM8.x;
    // renderer instruction 9
    vec4 h2RendererA9=h2MenuTemp10.xyzz;
    vec4 h2RendererB9=h2MenuTemp10.xyzz;
    vec4 h2RendererC9=vpConstant(23u).xyzw;
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    vec4 h2RendererProduct9=h2MenuMultiply(h2RendererA9,h2RendererB9);
    precise float h2RendererDot9=(h2RendererProduct9.x+h2RendererProduct9.y)+h2RendererProduct9.z;
    h2RendererM9=vec4(h2RendererDot9);
    h2RendererL9=h2RendererC9;
    h2MenuTemp11.y=h2RendererM9.y;
    vpOut[3].xyzw=h2RendererL9.xyzw;
    // renderer instruction 10
    vec4 h2RendererA10=h2MenuTemp0.xyzw;
    vec4 h2RendererB10=vpConstant(1u).xyzw;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererDot10+=h2RendererProduct10.w;
    h2RendererM10=vec4(h2RendererDot10);
    h2MenuTemp9.y=h2RendererM10.y;
    // renderer instruction 11
    vec4 h2RendererA11=h2MenuTemp0.xyzw;
    vec4 h2RendererB11=vpConstant(2u).xyzw;
    vec4 h2RendererC11=h2MenuTemp11.yyyy;
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererDot11+=h2RendererProduct11.w;
    h2RendererM11=vec4(h2RendererDot11);
    h2RendererL11=vec4(inversesqrt(abs(h2RendererC11.x)));
    h2MenuTemp9.z=h2RendererM11.z;
    h2MenuTemp1.w=h2RendererL11.w;
    // renderer instruction 12
    vec4 h2RendererA12=h2MenuTemp0.xyzw;
    vec4 h2RendererB12=vpConstant(3u).xyzw;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererProduct12.w;
    h2RendererM12=vec4(h2RendererDot12);
    h2MenuTemp9.w=h2RendererM12.w;
    vpOut[0].w=h2RendererM12.w;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp10.xyzz;
    vec4 h2RendererB13=h2MenuTemp1.wwww;
    vec4 h2RendererC13=h2MenuTemp9.xyzw;
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    h2RendererM13=h2MenuMultiply(h2RendererA13,h2RendererB13);
    h2RendererL13=h2RendererC13;
    h2MenuTemp10.xyz=h2RendererM13.xyz;
    vpOut[0].xyz=h2RendererL13.xyz;
    // renderer instruction 14
    vec4 h2RendererA14=h2MenuTemp9.wwww;
    vec4 h2RendererB14=vpConstant(21u).xzxx;
    vec4 h2RendererC14=h2MenuTemp9.xyzw;
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    h2RendererM14=h2MenuMultiply(h2RendererA14,h2RendererB14)+h2RendererC14;
    h2MenuTemp9.xyzw=h2RendererM14.xyzw;
    // renderer instruction 15
    vec4 h2RendererA15=h2MenuTemp10.yyzx;
    vec4 h2RendererB15=vec4(0.0);
    vec4 h2RendererC15=vpConstant(22u).yyzx;
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    h2RendererM15=h2RendererA15+h2RendererC15;
    h2MenuTemp8.yzw=h2RendererM15.yzw;
    // renderer instruction 16
    vec4 h2RendererA16=h2MenuTemp9.xyzw;
    vec4 h2RendererB16=vpConstant(21u).xyxx;
    vec4 h2RendererC16=vpOut[0].wwww;
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=h2MenuMultiply(h2RendererA16,h2RendererB16);
    float h2RendererReciprocal16=1.0/h2RendererC16.x;
    if(h2RendererReciprocal16>0.0 || floatBitsToUint(h2RendererReciprocal16)==0u)
        h2RendererReciprocal16=clamp(h2RendererReciprocal16,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal16)) h2RendererReciprocal16=clamp(h2RendererReciprocal16,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL16=vec4(h2RendererReciprocal16);
    h2MenuTemp9.xyzw=h2RendererM16.xyzw;
    h2MenuTemp1.w=h2RendererL16.w;
    // renderer instruction 17
    vec4 h2RendererA17=h2MenuTemp8.wyzz;
    vec4 h2RendererB17=h2MenuTemp8.wyzz;
    vec4 h2RendererC17=vec4(0.0);
    precise vec4 h2RendererM17=vec4(0.0),h2RendererL17=vec4(0.0);
    vec4 h2RendererProduct17=h2MenuMultiply(h2RendererA17,h2RendererB17);
    precise float h2RendererDot17=(h2RendererProduct17.x+h2RendererProduct17.y)+h2RendererProduct17.z;
    h2RendererM17=vec4(h2RendererDot17);
    h2MenuTemp10.w=h2RendererM17.w;
    // renderer instruction 18
    vec4 h2RendererA18=h2MenuTemp10.yyxx;
    vec4 h2RendererB18=vpConstant(5u).wwww;
    vec4 h2RendererC18=h2MenuTemp9.wwww;
    precise vec4 h2RendererM18=vec4(0.0),h2RendererL18=vec4(0.0);
    h2RendererM18=h2MenuMultiply(h2RendererA18,h2RendererB18);
    h2RendererL18=vec4(1.0/h2RendererC18.x);
    h2MenuTemp11.yz=h2RendererM18.yz;
    h2MenuTemp1.x=h2RendererL18.x;
    // renderer instruction 19
    vec4 h2RendererA19=vpOut[0].xyzz;
    vec4 h2RendererB19=vpConstant(10u).xyzz;
    vec4 h2RendererC19=h2MenuTemp10.wwww;
    precise vec4 h2RendererM19=vec4(0.0),h2RendererL19=vec4(0.0);
    h2RendererM19=h2MenuMultiply(h2RendererA19,h2RendererB19);
    h2RendererL19=vec4(inversesqrt(abs(h2RendererC19.x)));
    h2MenuTemp1.y=h2RendererL19.y;
    vpOut[0].xyz=h2RendererM19.xyz;
    // renderer instruction 20
    vec4 h2RendererA20=vpInput[1].yyyx;
    vec4 h2RendererB20=vpConstant(172u).yyyx;
    vec4 h2RendererC20=vpConstant(172u).wwwz;
    precise vec4 h2RendererM20=vec4(0.0),h2RendererL20=vec4(0.0);
    h2RendererM20=h2MenuMultiply(h2RendererA20,h2RendererB20)+h2RendererC20;
    h2MenuTemp10.yw=h2RendererM20.yw;
    // renderer instruction 21
    vec4 h2RendererA21=h2MenuTemp8.zzzz;
    vec4 h2RendererB21=h2MenuTemp1.yyyy;
    vec4 h2RendererC21=vec4(0.0);
    precise vec4 h2RendererM21=vec4(0.0),h2RendererL21=vec4(0.0);
    h2RendererM21=h2MenuMultiply(h2RendererA21,h2RendererB21);
    h2MenuTemp11.x=h2RendererM21.x;
    // renderer instruction 22
    vec4 h2RendererA22=vec4(0.0);
    vec4 h2RendererB22=vec4(0.0);
    vec4 h2RendererC22=h2MenuTemp11.wxxw;
    precise vec4 h2RendererM22=vec4(0.0),h2RendererL22=vec4(0.0);
    h2RendererL22=vec4(1.0,max(h2RendererC22.x,0.0),h2RendererC22.x>0.0?pow(max(h2RendererC22.y,0.0),clamp(h2RendererC22.w,-128.0,128.0)):0.0,1.0);
    h2MenuTemp10.z=h2RendererL22.z;
    // renderer instruction 23
    vec4 h2RendererA23=h2MenuTemp0.xyyy;
    vec4 h2RendererB23=vpConstant(20u).xyyy;
    vec4 h2RendererC23=vpConstant(20u).zwww;
    precise vec4 h2RendererM23=vec4(0.0),h2RendererL23=vec4(0.0);
    h2RendererM23=h2MenuMultiply(h2RendererA23,h2RendererB23)+h2RendererC23;
    vpOut[9].xy=h2RendererM23.xy;
    // renderer instruction 24
    vec4 h2RendererA24=h2MenuTemp9.xyzz;
    vec4 h2RendererB24=h2MenuTemp1.xxxx;
    vec4 h2RendererC24=vec4(0.0);
    precise vec4 h2RendererM24=vec4(0.0),h2RendererL24=vec4(0.0);
    h2RendererM24=h2MenuMultiply(h2RendererA24,h2RendererB24);
    vpOut[10].xyz=h2RendererM24.xyz;
    // renderer instruction 25
    vec4 h2RendererA25=h2MenuTemp1.xxxx;
    vec4 h2RendererB25=h2MenuTemp1.xxxx;
    vec4 h2RendererC25=vec4(0.0);
    precise vec4 h2RendererM25=vec4(0.0),h2RendererL25=vec4(0.0);
    h2RendererM25=vec4(greaterThanEqual(h2RendererA25,h2RendererB25));
    vpOut[10].w=h2RendererM25.w;
    // renderer instruction 26
    vec4 h2RendererA26=h2MenuTemp10.wyyy;
    vec4 h2RendererB26=vpConstant(18u).xyyy;
    vec4 h2RendererC26=vpConstant(18u).zwww;
    precise vec4 h2RendererM26=vec4(0.0),h2RendererL26=vec4(0.0);
    h2RendererM26=h2MenuMultiply(h2RendererA26,h2RendererB26)+h2RendererC26;
    vpOut[11].xy=h2RendererM26.xy;
    // renderer instruction 27
    vec4 h2RendererA27=h2MenuTemp11.zyyy;
    vec4 h2RendererB27=vec4(0.0);
    vec4 h2RendererC27=vpConstant(5u).wwww;
    precise vec4 h2RendererM27=vec4(0.0),h2RendererL27=vec4(0.0);
    h2RendererM27=h2RendererA27+h2RendererC27;
    vpOut[12].xy=h2RendererM27.xy;
    // renderer instruction 28
    vec4 h2RendererA28=h2MenuTemp10.zzzz;
    vec4 h2RendererB28=vpConstant(25u).xyzw;
    vec4 h2RendererC28=vec4(0.0);
    precise vec4 h2RendererM28=vec4(0.0),h2RendererL28=vec4(0.0);
    h2RendererM28=h2MenuMultiply(h2RendererA28,h2RendererB28);
    vpOut[4].xyzw=h2RendererM28.xyzw;
    // renderer instruction 29
    vec4 h2RendererA29=vpOut[0].xyzz;
    vec4 h2RendererB29=h2MenuTemp1.wwww;
    vec4 h2RendererC29=vpConstant(11u).xyzz;
    precise vec4 h2RendererM29=vec4(0.0),h2RendererL29=vec4(0.0);
    h2RendererM29=h2MenuMultiply(h2RendererA29,h2RendererB29)+h2RendererC29;
    vpOut[0].xyz=h2RendererM29.xyz;
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
