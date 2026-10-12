#pragma once
#include "physics_vehicle_motion_internal.hpp"
namespace halo2::physics_vehicle_motion_math_detail {
using namespace physics_vehicle_motion_detail;
inline std::uint16_t control_word() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t result;asm volatile("fnstcw %0":"=m"(result));return result;
#else
    throw std::runtime_error("Required original contact control word backend unavailable");
#endif
}
inline void load_control_word(std::uint16_t value) {
    if((value&0x3fu)!=0x3fu)throw std::runtime_error("Required original contact unmasked exception owner unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0"::"m"(value):"memory");
#endif
}
inline long double read_extended(const M &m,std::uint32_t p) {
    if constexpr(sizeof(long double)<10)throw std::runtime_error("Required original contact extended precision backend unavailable");
    long double result{};m.read(p,std::span(reinterpret_cast<std::byte*>(&result),10));return result;
}
inline void write_extended(M &m,const O &o,std::uint32_t site,std::uint32_t p,long double value) {
    if constexpr(sizeof(long double)<10)throw std::runtime_error("Required original contact extended precision backend unavailable");
    std::array<std::byte,10> raw{};std::memcpy(raw.data(),&value,10);
    std::uint64_t lo{};std::uint16_t hi{};std::memcpy(&lo,raw.data(),8);std::memcpy(&hi,raw.data()+8,2);
    w(m,o,site,p,8,lo);w(m,o,site,p+8,2,hi);
}
struct MathState : MotionState {
    using MotionState::MotionState;
    void classify() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto input=fp.st(0);std::uint16_t status;
        asm volatile("fldt %1\n\tfxam\n\tfnstsw %0\n\tfstp %%st(0)":"=m"(status):"m"(input):"st");
        fp.status=std::uint16_t((fp.status&~0x4700u)|(status&0x4700u));
#else
        throw std::runtime_error("Required original contact FXAM backend unavailable");
#endif
    }
    void remainder() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto numerator=fp.st(0),denominator=fp.st(1);long double result;std::uint16_t status;
        asm volatile("fldt %3\n\tfldt %2\n\tfprem\n\tfnstsw %1\n\tfstpt %0\n\tfstp %%st(0)"
            :"=m"(result),"=m"(status):"m"(numerator),"m"(denominator):"st","st(1)");
        fp.st(0)=result;fp.status=std::uint16_t((fp.status&~0x4700u)|(status&0x4700u));
#else
        throw std::runtime_error("Required original contact FPREM backend unavailable");
#endif
    }
    void scale() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto value=fp.st(0),exponent=fp.st(1);long double result;
        asm volatile("fldt %2\n\tfldt %1\n\tfscale\n\tfstpt %0\n\tfstp %%st(0)"
            :"=m"(result):"m"(value),"m"(exponent):"st","st(1)");fp.st(0)=result;
#else
        throw std::runtime_error("Required original contact FSCALE backend unavailable");
#endif
    }
    std::uint32_t rotate(std::uint32_t value,unsigned count,unsigned n) {
        const auto width=n*8u;count=(count&31u)%width;value&=Flags::mask(n);if(!count)return value;
        const auto result=((value<<count)|(value>>(width-count)))&Flags::mask(n);
        flags.cf=(result&1u)!=0;if(count==1)flags.of=((result>>(width-1u))!=0)!=flags.cf;
        return result;
    }
};
} // namespace halo2::physics_vehicle_motion_math_detail
