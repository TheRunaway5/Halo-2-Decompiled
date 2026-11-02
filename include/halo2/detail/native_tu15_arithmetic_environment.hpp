#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>

namespace halo2::detail {
// Retain the actual host x87 stack while a semantic scalar kernel owns a
// selected, initially empty arithmetic environment. No guest registers live here.
class NativeTu15ArithmeticEnvironment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned host_mxcsr_;
public:
    explicit NativeTu15ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native vector math requires masked original FP exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");
        _mm_setcsr(fp.mxcsr);
    }
    ~NativeTu15ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
}
#endif
