#version 450
// Halo2 raw class0..6 vertex catalog ABI4.
layout(location=0) in vec4 v0;
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
    vec4 h2RendererA0=vpConstant(32u).xyzz;
    vec4 h2RendererB0=vec4(0.0);
    vec4 h2RendererC0=-vpInput[0].xyzz;
    precise vec4 h2RendererM0=vec4(0.0),h2RendererL0=vec4(0.0);
    h2RendererM0=h2RendererA0+h2RendererC0;
    h2MenuTemp10.xyz=h2RendererM0.xyz;
    // renderer instruction 1
    vec4 h2RendererA1=-vpInput[0].xyzz;
    vec4 h2RendererB1=vpConstant(31u).wwww;
    vec4 h2RendererC1=vpConstant(31u).xyzz;
    precise vec4 h2RendererM1=vec4(0.0),h2RendererL1=vec4(0.0);
    h2RendererM1=h2MenuMultiply(h2RendererA1,h2RendererB1)+h2RendererC1;
    h2MenuTemp4.xyz=h2RendererM1.xyz;
    // renderer instruction 2
    vec4 h2RendererA2=h2MenuTemp10.xyzz;
    vec4 h2RendererB2=vpConstant(35u).xyzz;
    vec4 h2RendererC2=vpInput[11].yyyy;
    precise vec4 h2RendererM2=vec4(0.0),h2RendererL2=vec4(0.0);
    vec4 h2RendererProduct2=h2MenuMultiply(h2RendererA2,h2RendererB2);
    precise float h2RendererDot2=(h2RendererProduct2.x+h2RendererProduct2.y)+h2RendererProduct2.z;
    h2RendererM2=vec4(h2RendererDot2);
    h2RendererL2=h2RendererC2;
    h2MenuTemp11.x=h2RendererM2.x;
    vpOut[3].w=h2RendererL2.w;
    // renderer instruction 3
    vec4 h2RendererA3=h2MenuTemp4.xyzz;
    vec4 h2RendererB3=h2MenuTemp4.xyzz;
    vec4 h2RendererC3=vpInput[11].zzzz;
    precise vec4 h2RendererM3=vec4(0.0),h2RendererL3=vec4(0.0);
    vec4 h2RendererProduct3=h2MenuMultiply(h2RendererA3,h2RendererB3);
    precise float h2RendererDot3=(h2RendererProduct3.x+h2RendererProduct3.y)+h2RendererProduct3.z;
    h2RendererM3=vec4(h2RendererDot3);
    h2RendererL3=h2RendererC3;
    h2MenuTemp4.w=h2RendererM3.w;
    vpOut[7].w=h2RendererL3.w;
    // renderer instruction 4
    vec4 h2RendererA4=h2MenuTemp11.xxxx;
    vec4 h2RendererB4=vec4(0.0);
    vec4 h2RendererC4=vpConstant(15u).yyyy;
    precise vec4 h2RendererM4=vec4(0.0),h2RendererL4=vec4(0.0);
    h2RendererM4=h2RendererA4+h2RendererC4;
    h2MenuTemp11.x=h2RendererM4.x;
    // renderer instruction 5
    vec4 h2RendererA5=vpConstant(36u).wwww;
    vec4 h2RendererB5=vpInput[11].xxxx;
    vec4 h2RendererC5=h2MenuTemp4.wwww;
    precise vec4 h2RendererM5=vec4(0.0),h2RendererL5=vec4(0.0);
    h2RendererM5=h2MenuMultiply(h2RendererA5,h2RendererB5);
    h2RendererL5=vec4(inversesqrt(abs(h2RendererC5.x)));
    h2MenuTemp4.w=h2RendererM5.w;
    h2MenuTemp1.w=h2RendererL5.w;
    // renderer instruction 6
    vec4 h2RendererA6=h2MenuTemp11.xxxx;
    vec4 h2RendererB6=vpConstant(15u).yyyy;
    vec4 h2RendererC6=vec4(0.0);
    precise vec4 h2RendererM6=vec4(0.0),h2RendererL6=vec4(0.0);
    h2RendererM6=vec4(greaterThanEqual(h2RendererA6,h2RendererB6));
    h2MenuTemp10.x=h2RendererM6.x;
    // renderer instruction 7
    vec4 h2RendererA7=h2MenuTemp4.xyzz;
    vec4 h2RendererB7=h2MenuTemp1.wwww;
    vec4 h2RendererC7=vec4(0.0);
    precise vec4 h2RendererM7=vec4(0.0),h2RendererL7=vec4(0.0);
    h2RendererM7=h2MenuMultiply(h2RendererA7,h2RendererB7);
    h2MenuTemp4.xyz=h2RendererM7.xyz;
    // renderer instruction 8
    vec4 h2RendererA8=h2MenuTemp4.wwww;
    vec4 h2RendererB8=h2MenuTemp10.xxxx;
    vec4 h2RendererC8=vec4(0.0);
    precise vec4 h2RendererM8=vec4(0.0),h2RendererL8=vec4(0.0);
    h2RendererM8=h2MenuMultiply(h2RendererA8,h2RendererB8);
    h2MenuTemp4.w=h2RendererM8.w;
    // renderer instruction 9
    vec4 h2RendererA9=-h2MenuTemp4.xyzz;
    vec4 h2RendererB9=h2MenuTemp4.wwww;
    vec4 h2RendererC9=vpInput[0].xyzz;
    precise vec4 h2RendererM9=vec4(0.0),h2RendererL9=vec4(0.0);
    h2RendererM9=h2MenuMultiply(h2RendererA9,h2RendererB9)+h2RendererC9;
    h2MenuTemp11.xyz=h2RendererM9.xyz;
    // renderer instruction 10
    vec4 h2RendererA10=h2MenuTemp11.xyzw;
    vec4 h2RendererB10=vpConstant(0u).xyzw;
    vec4 h2RendererC10=vec4(0.0);
    precise vec4 h2RendererM10=vec4(0.0),h2RendererL10=vec4(0.0);
    vec4 h2RendererProduct10=h2MenuMultiply(h2RendererA10,h2RendererB10);
    precise float h2RendererDot10=(h2RendererProduct10.x+h2RendererProduct10.y)+h2RendererProduct10.z;
    h2RendererDot10+=h2RendererB10.w;
    h2RendererM10=vec4(h2RendererDot10);
    vpOut[0].x=h2RendererM10.x;
    // renderer instruction 11
    vec4 h2RendererA11=h2MenuTemp11.xyzw;
    vec4 h2RendererB11=vpConstant(1u).xyzw;
    vec4 h2RendererC11=vec4(0.0);
    precise vec4 h2RendererM11=vec4(0.0),h2RendererL11=vec4(0.0);
    vec4 h2RendererProduct11=h2MenuMultiply(h2RendererA11,h2RendererB11);
    precise float h2RendererDot11=(h2RendererProduct11.x+h2RendererProduct11.y)+h2RendererProduct11.z;
    h2RendererDot11+=h2RendererB11.w;
    h2RendererM11=vec4(h2RendererDot11);
    vpOut[0].y=h2RendererM11.y;
    // renderer instruction 12
    vec4 h2RendererA12=h2MenuTemp11.xyzw;
    vec4 h2RendererB12=vpConstant(2u).xyzw;
    vec4 h2RendererC12=vec4(0.0);
    precise vec4 h2RendererM12=vec4(0.0),h2RendererL12=vec4(0.0);
    vec4 h2RendererProduct12=h2MenuMultiply(h2RendererA12,h2RendererB12);
    precise float h2RendererDot12=(h2RendererProduct12.x+h2RendererProduct12.y)+h2RendererProduct12.z;
    h2RendererDot12+=h2RendererB12.w;
    h2RendererM12=vec4(h2RendererDot12);
    vpOut[0].z=h2RendererM12.z;
    // renderer instruction 13
    vec4 h2RendererA13=h2MenuTemp11.xyzw;
    vec4 h2RendererB13=vpConstant(3u).xyzw;
    vec4 h2RendererC13=vec4(0.0);
    precise vec4 h2RendererM13=vec4(0.0),h2RendererL13=vec4(0.0);
    vec4 h2RendererProduct13=h2MenuMultiply(h2RendererA13,h2RendererB13);
    precise float h2RendererDot13=(h2RendererProduct13.x+h2RendererProduct13.y)+h2RendererProduct13.z;
    h2RendererDot13+=h2RendererB13.w;
    h2RendererM13=vec4(h2RendererDot13);
    vpOut[0].w=h2RendererM13.w;
    // renderer instruction 14
    vec4 h2RendererA14=vpOut[0].xyzz;
    vec4 h2RendererB14=vpConstant(10u).xyzz;
    vec4 h2RendererC14=vec4(0.0);
    precise vec4 h2RendererM14=vec4(0.0),h2RendererL14=vec4(0.0);
    h2RendererM14=h2MenuMultiply(h2RendererA14,h2RendererB14);
    vpOut[0].xyz=h2RendererM14.xyz;
    // renderer instruction 15
    vec4 h2RendererA15=vec4(0.0);
    vec4 h2RendererB15=vec4(0.0);
    vec4 h2RendererC15=vpOut[0].wwww;
    precise vec4 h2RendererM15=vec4(0.0),h2RendererL15=vec4(0.0);
    float h2RendererReciprocal15=1.0/h2RendererC15.x;
    if(h2RendererReciprocal15>0.0 || floatBitsToUint(h2RendererReciprocal15)==0u)
        h2RendererReciprocal15=clamp(h2RendererReciprocal15,5.421010862427522e-20,1.8446744073709552e19);
    else if(!isnan(h2RendererReciprocal15)) h2RendererReciprocal15=clamp(h2RendererReciprocal15,-1.8446744073709552e19,-5.421010862427522e-20);
    h2RendererL15=vec4(h2RendererReciprocal15);
    h2MenuTemp11.w=h2RendererL15.w;
    // renderer instruction 16
    vec4 h2RendererA16=vpOut[0].xyzz;
    vec4 h2RendererB16=h2MenuTemp11.wwww;
    vec4 h2RendererC16=vpConstant(11u).xyzz;
    precise vec4 h2RendererM16=vec4(0.0),h2RendererL16=vec4(0.0);
    h2RendererM16=h2MenuMultiply(h2RendererA16,h2RendererB16)+h2RendererC16;
    vpOut[0].xyz=h2RendererM16.xyz;
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
