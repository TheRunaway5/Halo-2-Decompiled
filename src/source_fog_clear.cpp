#include "halo2/source_fog_clear.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif
#if defined(__FAST_MATH__)
#error "Original fog clear selection requires strict binary32 arithmetic"
#endif

namespace halo2 {
namespace {
float stored(float value) {volatile float result=value;return result;}
float mul(float a,float b) {return stored(a*b);}
float sub(float a,float b) {return stored(a-b);}
float sum(float a,float b) {return stored(a+b);}
float clamp(float value) {return value<0.f?0.f:value>1.f?1.f:value;}
void finite(float value) {
    if(!std::isfinite(value))throw FormatError("Nonfinite source fog clear input is unproved");
}
void finite(Vec3 value) {finite(value.x);finite(value.y);finite(value.z);}
std::uint64_t rounded(std::uint64_t value,unsigned shift,bool negative,unsigned mode) {
    if(!shift)return value;
    const auto integer=shift>=64?0:value>>shift;
    const auto remainder=shift>=64?value:value&((std::uint64_t{1}<<shift)-1);
    if(!remainder)return integer;
    bool increment=false;
    if(mode==0 && shift<64) {
        const auto half=std::uint64_t{1}<<(shift-1);
        increment=remainder>half || (remainder==half && (integer&1));
    } else if(mode==1)increment=negative;
    else if(mode==2)increment=!negative;
    return integer+increment;
}
std::uint32_t channel(float value,unsigned precision,unsigned rounding) {
    const auto word=std::bit_cast<std::uint32_t>(value);
    const bool negative=word>>31;
    const auto exponent=(word>>23)&255u;
    if(exponent==255)return 0; // Masked FISTP indefinite, then&FF.
    std::uint64_t significand=(word&0x7fffffu)|(exponent?0x800000u:0);
    significand*=255u; // Exact binary32 times255 fits32 significant bits.
    int power=exponent?int(exponent)-150:-149;
    if(precision==0 && significand) {
        const auto bits=std::bit_width(significand);
        if(bits>24) {
            const unsigned shift=bits-24;
            significand=rounded(significand,shift,negative,rounding);
            power+=int(shift);
        }
    }
    const std::uint64_t limit=negative?0x80000000ull:0x7fffffffull;
    std::uint64_t integer;
    if(power>=0) {
        if(power>=32 || significand>(limit>>power))return 0;
        integer=significand<<power;
    } else integer=rounded(significand,unsigned(-power),negative,rounding);
    if(integer>limit)return 0;
    const auto signed_word=negative?0u-std::uint32_t(integer):std::uint32_t(integer);
    return signed_word&255u;
}
}
Vec3 source_fog_clear_color(const SourceFogClearInput &in) {
    if(std::fegetround()!=FE_TONEAREST)throw FormatError("Source fog clear requires nearest-even arithmetic");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    if((_mm_getcsr()&0xffc0u)!=0x1f80u)throw FormatError("Source fog clear requires gradual binary32 arithmetic");
#endif
    finite(in.retained_color);finite(in.sky_color_premultiplied);finite(in.sky_opacity);
    finite(in.primary.color);finite(in.secondary.color);
    finite(in.primary.maximum_density);finite(in.secondary.maximum_density);
    finite(in.planar_density);finite(in.interpolation);
    if(in.sky_model_control)return in.sky_opacity>=1.f?in.sky_color_premultiplied:in.retained_color;
    if(in.planar_density!=0.f)return in.retained_color;
    const auto p=in.primary.maximum_density,s=in.secondary.maximum_density,t=in.interpolation;
    if(p==1.f && (s==0.f || t==1.f))return in.primary.color;
    if(s==1.f && (p==0.f || t==0.f))return in.secondary.color;
    if(p!=1.f && s!=1.f)return in.retained_color;
    // Literal12E7CF..12E88B SSE order; no FMA or planar-transition substitute.
    const auto primary=mul(sub(1.f,mul(sub(1.f,t),s)),p);
    const auto secondary=mul(sub(1.f,mul(t,p)),s);
    return {clamp(sum(mul(in.secondary.color.x,secondary),mul(primary,in.primary.color.x))),
        clamp(sum(mul(in.secondary.color.y,secondary),mul(in.primary.color.y,primary))),
        clamp(sum(mul(in.secondary.color.z,secondary),mul(in.primary.color.z,primary)))};
}
std::uint32_t source_clear_color_rgb8(Vec3 color,std::uint16_t control) {
    const unsigned precision=(control>>8)&3u,rounding=(control>>10)&3u;
    // The actual original caller uses023F; the retained helper tests also
    // use027F. Reserved bit6 does not select precision or rounding.
    if((control&0xf0bfu)!=0x003fu || precision==1)
        throw FormatError("Unsupported source clear-color x87 control word");
    return (channel(color.x,precision,rounding)<<16)|
        (channel(color.y,precision,rounding)<<8)|channel(color.z,precision,rounding);
}
}
