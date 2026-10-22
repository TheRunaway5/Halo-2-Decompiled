#include "halo2/source_weather_plate_program.hpp"
#include <bit>
#include <cfloat>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif
#if defined(__FAST_MATH__)
#error "Original weather plate arithmetic requires strict floating-point compilation"
#endif
#if FLT_EVAL_METHOD != 0
#error "Original weather plate SSE arithmetic requires FLT_EVAL_METHOD==0"
#endif
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559 &&
    std::numeric_limits<float>::digits==24);
static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559 &&
    std::numeric_limits<double>::digits==53);

namespace halo2 {
namespace {
[[noreturn]] void unsupported(const char *message) {
    throw std::invalid_argument(message);
}
float stored(float v) {volatile float r=v;return r;}
float stored(double v) {volatile float r=static_cast<float>(v);return r;}
float add(float a,float b) {return stored(a+b);}
float sub(float a,float b) {return stored(a-b);}
float mul(float a,float b) {return stored(a*b);}
float div(float a,float b) {return stored(a/b);}
bool byte_flag(std::uint32_t v) {return (v&255u)!=0;}
void finite(float v) {
    if(!std::isfinite(v))unsupported("weather plate nonfinite source arithmetic is unproved");
}
std::int32_t floor32(double v) {
    const double q=std::floor(v);
    if(!std::isfinite(q) || q<double(std::numeric_limits<std::int32_t>::min()) ||
        q>double(std::numeric_limits<std::int32_t>::max()))
        unsupported("weather plate source integer overflow classification is unproved");
    return static_cast<std::int32_t>(q);
}
float quantized(float v) {
    // 3A67B..6CB: floor(x87 double(v)*65536), EAX/CVTSI2SS, MULSS2^-16.
    return mul(stored(static_cast<float>(floor32(double(v)*65536.))),1.f/65536.f);
}
std::uint32_t channel(float v) {
    finite(v);
    // 131ED0 FISTP at PC53/nearest-even, then &FF. Multiplication of a
    // binary32 value by the literal255 is exact within binary64 precision.
    const double q=std::nearbyint(double(v)*255.);
    if(q<double(std::numeric_limits<std::int32_t>::min()) ||
        q>double(std::numeric_limits<std::int32_t>::max()))
        unsupported("weather plate color FISTP overflow classification is unproved");
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(q))&255u;
}
std::uint32_t rgb(const SourceWeatherPlateCompositor &r,unsigned first) {
    return (channel(std::bit_cast<float>(r.words[first]))<<16)|
        (channel(std::bit_cast<float>(r.words[first+1]))<<8)|
        channel(std::bit_cast<float>(r.words[first+2]));
}
std::uint32_t slope_byte(float v) {
    const auto q=floor32(v);
    return q<0?0u:q>255?255u:static_cast<std::uint32_t>(q);
}
void arithmetic(const SourceWeatherPlateProgramInput &in) {
    if((in.source_fpcw&0x1f3fu)!=0x023fu ||
        (in.source_mxcsr&0xffffffc0u)!=0x1f80u || std::fegetround()!=FE_TONEAREST)
        unsupported("weather plate requires source nearest-even PC53 gradual arithmetic");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    if((_mm_getcsr()&0xffc0u)!=0x1f80u)
        unsupported("weather plate host SSE flush/rounding control differs from source");
#else
    unsupported("weather plate host gradual arithmetic control inspection is unproved");
#endif
}
}
SourceWeatherPlateProgramResult source_weather_plate_program(
    const SourceWeatherPlateProgramInput &in) {
    if(in.mode!=3 || !byte_flag(in.skip_state))
        unsupported("weather plate pure producer admits only mode3 with skipped raster state");
    arithmetic(in);
    finite(in.depth_start);finite(in.depth_end);finite(in.near_clip);finite(in.far_clip);
    if(!(in.far_clip>0.f))unsupported("weather plate positive source far clip is required");
    // Validate all four raw triples before constructing any result bank.
    const auto c0=rgb(in.compositor,1),c1=rgb(in.compositor,4),
        c2=rgb(in.compositor,7),tint=rgb(in.compositor,10);
    const float inv_far=div(1.f,in.far_clip);finite(inv_far);
    float start=mul(inv_far,in.depth_start),end=mul(inv_far,in.depth_end);
    finite(start);finite(end);start=quantized(start);end=quantized(end);
    if(!(end>start))return {};
    const float span=sub(end,start);finite(span);
    float slope=div(1.f,span);finite(slope);
    if(slope>128.f) {
        slope=128.f;
        const float minimum_span=div(1.f,slope);
        if(byte_flag(in.center))start=sub(mul(add(start,end),.5f),mul(minimum_span,.5f));
        else if(byte_flag(in.anchor_end))start=sub(end,minimum_span);
        else if(!byte_flag(in.anchor_start))return {};
    }
    finite(start);
    if(start<0.f) {
        if(!byte_flag(in.anchor_end) || !byte_flag(in.anchor_start))return {};
        start=0.f;
    }
    // The second packing floor must not replace the floating selected start:
    // centered half steps can produce a different optional depth output.
    const auto q=floor32(double(start)*65536.);
    auto low=static_cast<std::uint32_t>(q)&255u;
    auto high=static_cast<std::uint32_t>(q)>>8; // selected start is nonnegative
    if(high>255u)low=high=255u;
    unsigned power=0;
    while(stored(static_cast<float>(1u<<power))<slope)++power;
    if(power>7)unsupported("weather plate mode3 reciprocal class exceeds proven0..7");
    const auto a=slope_byte(stored((double(slope)*256.)/double(1u<<power)));
    const auto b=slope_byte(stored(double(slope)*.5));
    std::optional<float> depth;
    if(in.depth_requested) {
        float v=mul(in.far_clip,start);finite(v);
        if(!(in.near_clip<v))v=in.near_clip;
        depth=v;
    }
    PixelProgram pixel{};auto &w=pixel.words;
    w[1]=0x04209140;w[2]=0x18201140;w[3]=0xdd202df1;
    w[10]=0x00ff0000;w[18]=0x0000ff00;
    w[11]=low<<24;w[12]=high<<24;w[13]=0x01000000;
    w[27]=0x00000c00;w[28]=0x00000d00;w[29]=0x00000d00;
    w[34]=0x08010802;w[35]=0x04201140;w[36]=0x00002020;w[37]=0xcc202d20;
    w[45]=0x00003045;w[46]=0x00000c00;w[47]=0x00004d00;w[48]=0x00000c00;
    w[53]=0x00011108;w[54]=0x00008421;
    const bool alpha=byte_flag(in.compositor.words[0]);
    w[14]=c0;w[22]=c1;w[38]=alpha?0x19011a02:0x09010a02;w[49]=0x00003045;
    w[4]=0x1d201d00|(power>3?0x20u:0u);
    w[30]=0x00000d00|((power>2?0x20u:power>1?0x10u:0u)<<12);
    w[15]=c2;w[39]=alpha?0x24251b01:0x24250b01;w[50]=0x00001045;
    w[5]=0x1d201d00|(power>6?0x20u:0u);
    w[31]=0x00000d00|((power>5?0x20u:power>4?0x10u:0u)<<12);
    w[24]=(a<<24)|b;w[40]=0x04250000;w[51]=0x000000d0;
    w[6]=0x1d120c02;w[7]=0x1d2d0000;
    w[32]=0x00000d00|((power>0?0x10u:0u)<<12);w[33]=0x000000d0;
    w[43]=tint;w[8]=0x1d010000;w[9]=0x00001d00;
    return {pixel,depth};
}
}
