#version 450
// Halo2 raw class0..6 vertex catalog ABI2.
layout(location=0) in vec4 v0;
layout(location=1) in vec4 v1;
layout(location=2) in vec4 v2;
layout(set=1,binding=0,std140) uniform MenuWorldVertexUniforms {
    vec4 c[192];
    vec4 clipRange;
    vec2 fogParam;
    vec2 surfaceSize;
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
    // instruction 0
    h2MenuTemp0.xy=h2MenuMultiply(vpInput[1].xyzw,vpConstant(7u).wwww).xy;
    // instruction 1
    h2MenuTemp9.x=h2MenuAdd(-vpInput[2].yyyy,vpConstant(4u).wwww).x;
    // instruction 2
    h2MenuAddress=int(floor((h2MenuTemp0.yyyy).x));
    // instruction 3
    h2MenuTemp10.xyz=h2MenuMultiply(vpInput[0].xyzw,vpConstant(170u).xyzz).xyz;
    // instruction 4
    h2MenuTemp4.xyzw=h2MenuMultiply(vpInput[2].yyyy,vpConstant(uint(50+h2MenuAddress)).xyzw).xyzw;
    // instruction 5
    h2MenuTemp5.xyzw=h2MenuMultiply(vpInput[2].yyyy,vpConstant(uint(51+h2MenuAddress)).xyzw).xyzw;
    // instruction 6
    h2MenuTemp6.xyzw=h2MenuMultiply(vpInput[2].yyyy,vpConstant(uint(52+h2MenuAddress)).xyzw).xyzw;
    // instruction 7
    h2MenuAddress=int(floor((h2MenuTemp0.xxxx).x));
    // instruction 8
    h2MenuTemp10.xyz=h2MenuAdd(h2MenuTemp10.xyzw,vpConstant(171u).xyzz).xyz;
    // instruction 9
    h2MenuTemp4.xyzw=h2MenuMultiplyAdd(h2MenuTemp9.xxxx,vpConstant(uint(50+h2MenuAddress)).xyzw,h2MenuTemp4.xyzw).xyzw;
    // instruction 10
    h2MenuTemp5.xyzw=h2MenuMultiplyAdd(h2MenuTemp9.xxxx,vpConstant(uint(51+h2MenuAddress)).xyzw,h2MenuTemp5.xyzw).xyzw;
    // instruction 11
    h2MenuTemp6.xyzw=h2MenuMultiplyAdd(h2MenuTemp9.xxxx,vpConstant(uint(52+h2MenuAddress)).xyzw,h2MenuTemp6.xyzw).xyzw;
    // instruction 12
    h2MenuTemp0.x=h2MenuHomogeneousDot(h2MenuTemp10.xyzw,h2MenuTemp4.xyzw).x;
    // instruction 13
    h2MenuTemp0.y=h2MenuHomogeneousDot(h2MenuTemp10.xyzw,h2MenuTemp5.xyzw).y;
    // instruction 14
    h2MenuTemp0.z=h2MenuHomogeneousDot(h2MenuTemp10.xyzw,h2MenuTemp6.xyzw).z;
    // instruction 15
    vpOut[0].x=h2MenuHomogeneousDot(h2MenuTemp0.xyzw,vpConstant(0u).xyzw).x;
    // instruction 16
    vpOut[0].y=h2MenuHomogeneousDot(h2MenuTemp0.xyzw,vpConstant(1u).xyzw).y;
    // instruction 17
    vpOut[0].z=h2MenuHomogeneousDot(h2MenuTemp0.xyzw,vpConstant(2u).xyzw).z;
    // instruction 18
    vpOut[0].w=h2MenuHomogeneousDot(h2MenuTemp0.xyzw,vpConstant(3u).xyzw).w;
    // instruction 19
    h2MenuTemp5.xyz=h2MenuAdd(-h2MenuTemp0.xyzw,vpConstant(7u).xyzw).xyz;
    // instruction 20
    vpOut[0].xyz=h2MenuMultiply(vpOut[0].xyzw,vpConstant(10u).xyzw).xyz;
    h2MenuTemp1.w=h2MenuReciprocalClamp(vpOut[0].wwww).w;
    // instruction 21
    vpOut[5].xy=h2MenuHomogeneousDot(h2MenuTemp5.xyzw,vpConstant(26u).xyzw).xy;
    // instruction 22
    vpOut[0].xyz=h2MenuMultiplyAdd(vpOut[0].xyzw,h2MenuTemp1.wwww,vpConstant(11u).xyzz).xyz;
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
