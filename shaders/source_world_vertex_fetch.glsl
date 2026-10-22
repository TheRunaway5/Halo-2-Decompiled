#ifndef HALO2_SOURCE_WORLD_VERTEX_FETCH_GLSL
#define HALO2_SOURCE_WORLD_VERTEX_FETCH_GLSL
// Caller supplies sourceWorldVertexWord(uint), reading a bounded padded raw
// uint buffer. Addresses are byte offsets into unchanged little-endian source
// resources. This is a graphics fetch adapter, not the source CPU FP backend.
uint sourceWorldVertexBits(uint address,uint width) {
    uint shift=(address&3u)*8u;
    uint value=sourceWorldVertexWord(address>>2u)>>shift;
    if(shift+width>32u)value|=sourceWorldVertexWord((address>>2u)+1u)<<(32u-shift);
    return width==32u?value:value&((1u<<width)-1u);
}
// Correctly rounded binary32 n/d for the normalized positive domain. Split
// the scaled dividend to keep every integer intermediate within 32 bits.
// This avoids implementation-dependent reciprocal rounding in shader division.
uint sourceWorldVertexRatio(uint n,uint d) {
    if(n==0u)return 0u;
    uint shift=uint(findMSB(d)-findMSB(n));
    uint scaled=n<<shift;
    if(scaled<d){++shift;scaled<<=1u;}
    uint high=(scaled<<16u)/d;
    uint remainder=(scaled<<16u)%d;
    uint low=(remainder<<7u)/d;
    remainder=(remainder<<7u)%d;
    uint significand=(high<<7u)|low;
    if(remainder*2u>d)++significand;
    // The denominators 255 and 32767 are odd, so halfway cases cannot occur.
    return ((127u-shift)<<23u)+(significand-0x00800000u);
}
// Stable format ordinals are authenticated against MenuWorldSourceFormat by
// the host adapter. A float vector supplies absent XYZ=0 and W=1. Packed signed
// words supply integer defaults, matching the native R32_SINT declaration.
uvec4 sourceWorldVertexFetch(uint address,uint format) {
    if(format==4u)return uvec4(sourceWorldVertexBits(address,32u),0u,0u,1u);
    uvec4 value=uvec4(0u,0u,0u,0x3f800000u);
    if(format<2u) {
        uint count=format==0u?3u:2u;
        for(uint i=0u;i<count;++i)value[i]=sourceWorldVertexBits(address+i*4u,32u);
    } else if(format<4u) {
        uint count=format==2u?3u:2u;
        for(uint i=0u;i<count;++i) {
            int n=bitfieldExtract(int(sourceWorldVertexBits(address+i*2u,16u)),0,16);
            uint magnitude=uint(abs(n));
            value[i]=(n<0?0x80000000u:0u)|sourceWorldVertexRatio(min(magnitude,32767u),32767u);
        }
    } else {
        uint count=format-4u;
        for(uint i=0u;i<count;++i)value[i]=sourceWorldVertexRatio(sourceWorldVertexBits(address+i,8u),255u);
    }
    return value;
}
#endif
