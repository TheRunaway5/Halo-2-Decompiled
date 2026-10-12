#version 450
// The authority is the retained rendered GS bank, not instrumented VSOut.
layout(set=1,binding=0,std430) readonly buffer Records { uint data[]; } records;
layout(location=0) out vec4 vtxD0;
layout(location=1) out vec4 vtxD1;
layout(location=2) out vec4 vtxB0;
layout(location=3) out vec4 vtxB1;
layout(location=4) out float vtxFog;
layout(location=5) out vec4 vtxT0;
layout(location=6) out vec4 vtxT1;
layout(location=7) out vec4 vtxT2;
layout(location=8) out vec4 vtxT3;
layout(location=9) flat out vec4 vtxPos0;
layout(location=10) flat out vec4 vtxPos1;
layout(location=11) flat out vec4 vtxPos2;
layout(location=12) flat out float triMZ;
vec4 load4(uint first) {
    return uintBitsToFloat(uvec4(records.data[first],records.data[first+1u],
        records.data[first+2u],records.data[first+3u]));
}
void main() {
    uint first=uint(gl_VertexIndex)*51u;
    gl_Position=load4(first);
    vtxPos0=load4(first+38u);
    vtxPos1=load4(first+42u);
    vtxPos2=load4(first+46u);
    triMZ=uintBitsToFloat(records.data[first+50u]);
    // Bounded source modules have no color/texture-dependent rejection or
    // depth stores. Valid finite constants avoid undefined dummy cube lookups.
    vtxD0=vtxD1=vtxB0=vtxB1=vec4(1);
    vtxFog=1.0;
    vtxT0=vtxT1=vtxT2=vtxT3=vec4(1);
}
