#ifndef HALO2_SOURCE_DEPTH_STORAGE_GLSL
#define HALO2_SOURCE_DEPTH_STORAGE_GLSL
// Canonical integer-D24 carrier only. The physical float aspect stores N/2^24,
// not a normalized-depth approximation. Callers attest every source word is
// canonical; ordinary D32 depth writes/clears do not satisfy this contract.
uint sourceD24CarrierWord(uint n) {
    if(n>0x00ffffffu)return 0xffffffffu;
    if(n==0u)return 0u;
    uint top=uint(findMSB(n));
    return ((103u+top)<<23u)|((n<<(23u-top))&0x007fffffu);
}
bool sourceD24CarrierDecode(uint word,out uint n) {
    n=0u;
    if(word==0u)return true;
    uint exponent=word>>23u;
    if(exponent<103u || exponent>126u)return false;
    uint mantissa=(word&0x007fffffu)|0x00800000u;
    uint shift=126u-exponent;
    if((mantissa&((1u<<shift)-1u))!=0u)return false;
    n=mantissa>>shift;return true;
}
// Caller supplies a finite, source-clamped depth in [0, 0xFFFFFF]. Native
// D24 retains the original conversion adjustment. The D32 representation
// uses the integer encoder, including canonical positive zero.
float sourceD24AttachmentValue(float depth,uint encoding) {
    float n=floor(depth);
    return encoding==0u ? uintBitsToFloat(floatBitsToUint(n/16777216.0)+1u)
                        : uintBitsToFloat(sourceD24CarrierWord(uint(n)));
}
#endif
