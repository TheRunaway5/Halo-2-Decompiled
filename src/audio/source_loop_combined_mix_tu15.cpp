#include "halo2/source_loop_combined_mix_tu15.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
    std::fenv_t environment;
    unsigned mxcsr;
    explicit FloatingPointScope(const SoundDriverFloatingPointContext& context):mxcsr(_mm_getcsr()) {
        if((context.x87_control_word&0x3fu)!=0x3fu ||
           (context.mxcsr&0x1f80u)!=0x1f80u || (context.mxcsr&0xffff0000u))
            throw std::runtime_error("Unmasked combined-loop floating-point exceptions unavailable");
        if(std::fegetenv(&environment)!=0)
            throw std::runtime_error("Combined-loop floating-point environment unavailable");
        _mm_setcsr(context.mxcsr);
        asm volatile("fldcw %0" :: "m"(context.x87_control_word) : "memory");
    }
    ~FloatingPointScope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
};
float real(std::uint32_t value){return std::bit_cast<float>(value);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}

void normalize(std::array<float,3>& point,float one,float epsilon) {
    float length;
    unsigned char degenerate;
    //30BD0 keeps the length on x87 through its epsilon comparison, and
    //separately stores a rounded float for the subsequent SSE reciprocal.
    asm volatile("flds %2\n\tfmul %%st(0),%%st\n\t"
        "flds %3\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "flds %4\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fsqrt\n\tfsts %0\n\tfabs\n\tflds %5\n\t"
        "fcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %1"
        : "=m"(length),"=qm"(degenerate)
        : "m"(point[0]),"m"(point[1]),"m"(point[2]),"m"(epsilon)
        : "cc","st","st(1)");
    if(degenerate)return;
    const auto reciprocal=div(one,length);
    for(auto& value:point)value=mul(value,reciprocal);
}
#endif
}

std::array<std::uint32_t,4> source_loop_combined_reset_tu15(
    const std::array<std::uint32_t,4>& accumulated,std::uint8_t previous,std::uint8_t frame) {
    return previous==frame?accumulated:std::array<std::uint32_t,4>{};
}

SourceLoopCombinedMixCalculationTu15 source_loop_combined_mix_tu15(
    const std::array<std::uint32_t,4>& accumulated,
    const std::array<std::uint32_t,3>& listener_local_point,
    std::uint32_t minimum,std::uint32_t maximum,std::uint32_t distance,
    const SourceLoopCombinedMixConstantsTu15& constants,
    const SoundDriverParameterConstants& parameters,const SoundDriverFloatingPointContext& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(context);
    SourceLoopCombinedMixCalculationTu15 result{};
    const auto floor=real(constants.minimum_distance),one=real(constants.one),half=real(constants.half);
    const auto bounded_minimum=real(minimum)>floor?real(minimum):floor;
    result.minimum=bits(bounded_minimum);
    result.gain=source_sound_distance_gain(result.minimum,maximum,distance,parameters,context);
    std::array<float,3> point;
    for(unsigned i=0;i<3;++i)point[i]=real(listener_local_point[i]);
    if(real(distance)>bounded_minimum)normalize(point,one,real(constants.normalization_epsilon));
    for(unsigned i=0;i<3;++i)result.normalized[i]=bits(point[i]);
    for(unsigned i=0;i<4;++i) {
        const auto x=mul(sub(point[0],real(constants.directions[i][0])),half);
        const auto y=mul(sub(point[1],real(constants.directions[i][1])),half);
        const auto z=mul(sub(point[2],real(constants.directions[i][2])),half);
        const auto squared=add(add(mul(z,z),mul(y,y)),mul(x,x));
        const auto bounded=0.f>squared?0.f:squared>one?one:squared;
        result.speakers[i]=bits(sub(one,bounded));
    }
    const auto gain=real(result.gain);
    float power=0.f;
    for(auto coefficient:result.speakers) {
        const auto weighted=mul(real(coefficient),gain);
        power=add(mul(weighted,weighted),power);
    }
    //The original uses reciprocal power, not reciprocal square-root power.
    const auto scale=power>one?div(one,power):one;
    for(unsigned i=0;i<4;++i)
        result.accumulated[i]=bits(add(mul(mul(real(result.speakers[i]),scale),gain),real(accumulated[i])));
    return result;
#else
    (void)accumulated;(void)listener_local_point;(void)minimum;(void)maximum;(void)distance;
    (void)constants;(void)parameters;(void)context;
    throw std::runtime_error("Original x87/SSE combined-loop backend unavailable");
#endif
}
}
