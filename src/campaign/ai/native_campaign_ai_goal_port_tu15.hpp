#pragma once
// Private primitives shared by the native 26A380 goal evaluation and its
// scoring callees: retained memory in original order and width, one SSE
// scalar instruction per call under the caller's MXCSR, and the original x87
// sequences under the caller's control word. Not a public interface.
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <stdexcept>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <xmmintrin.h>
#define HALO2_NATIVE_GOAL_EVALUATION_X86 1
#endif

namespace halo2::goal_tu15 {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using i16=std::int16_t;using i32=std::int32_t;
inline i32 sx16(u32 value){return i32(i16(u16(value)));}

// Retained memory: reads and stores in original order and width; every
// store carries its original PC for the observer.
class Port {
public:
    SourcePhysicsContactMemory &memory;const SourcePhysicsContactRecordObserver &observe;
    u32 r(u32 address,unsigned width)const {
        std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));u32 value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
        return value;
    }
    u32 r8(u32 address)const{return r(address,1);}
    u32 r16(u32 address)const{return r(address,2);}
    u32 r32(u32 address)const{return r(address,4);}
    // One 8-byte operand (FLD QWORD).
    std::array<std::byte,8> r64(u32 address)const{std::array<std::byte,8> raw{};memory.read(address,raw);return raw;}
    void w(u32 site,u32 address,u32 value,unsigned width)const {
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observe){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observe)observe(event);
    }
    void w8(u32 site,u32 address,u32 value)const{w(site,address,value,1);}
    void w16(u32 site,u32 address,u32 value)const{w(site,address,value,2);}
    void w32(u32 site,u32 address,u32 value)const{w(site,address,value,4);}
};

inline void require_masked(const NativeTu15PhysicsFrameFp &fp,const char *owner) {
    if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u) || (fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u))
        throw std::invalid_argument(std::string(owner)+" requires masked original FP exceptions");
}

#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
// One SSE scalar instruction per call under the caller's MXCSR (sticky
// flags, DAZ/FTZ and rounding are the original's). Operand order follows
// the original: op(destination, source).
class Simd {
    NativeTu15PhysicsFrameFp &fp_;
    static __m128 scalar(u32 value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(value)));}
    static u32 bits(__m128 value){return std::bit_cast<u32>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
public:
    explicit Simd(NativeTu15PhysicsFrameFp &fp):fp_(fp){require_masked(fp,"Native 26A380 goal evaluation");}
    NativeTu15PhysicsFrameFp &fp()const{return fp_;}
#define HALO2_GOAL_BINARY(name,instruction) \
    u32 name(u32 a,u32 b)const { \
        __m128 x=scalar(a);const __m128 y=scalar(b);u32 host{},csr=fp_.mxcsr; \
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\t" instruction " %[y],%[x]\n\tstmxcsr %[c]\n\tldmxcsr %[h]" \
            :[x]"+x"(x),[h]"+m"(host),[c]"+m"(csr):[y]"x"(y):"memory"); \
        fp_.mxcsr=csr;return bits(x); \
    }
    HALO2_GOAL_BINARY(add,"addss")
    HALO2_GOAL_BINARY(sub,"subss")
    HALO2_GOAL_BINARY(mul,"mulss")
    HALO2_GOAL_BINARY(div,"divss")
#undef HALO2_GOAL_BINARY
    // COMISS a,b: {JA, JAE}. Unordered sets ZF/PF/CF: neither above nor
    // above-or-equal; JBE is taken.
    std::array<bool,2> order(u32 a,u32 b)const {
        const __m128 x=scalar(a),y=scalar(b);u32 host{},csr=fp_.mxcsr;unsigned char above{},above_equal{};
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tcomiss %[y],%[x]\n\tseta %[a]\n\tsetae %[e]\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
            :[a]"=&q"(above),[e]"=&q"(above_equal),[h]"+m"(host),[c]"+m"(csr):[x]"x"(x),[y]"x"(y):"cc","memory");
        fp_.mxcsr=csr;return {above!=0,above_equal!=0};
    }
    bool above(u32 a,u32 b)const{return order(a,b)[0];}
    bool above_equal(u32 a,u32 b)const{return order(a,b)[1];}
};

// x87 sequences of the original, from an empty stack under the caller's
// control word; status (C1, TOP, sticky flags) is retained.
struct X87 {
    // FLD a; FSQRT; FSTP m32.
    static u32 root(NativeTu15PhysicsFrameFp &fp,u32 squared) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
        asm volatile("flds %1\n\tfsqrt\n\tfstps %0":"=m"(out):"m"(squared):"memory","st");
        return out;
    }
    // FLD v; FABS; FLD m32 eps; FCOMIP ST(1); FSTP ST(0): JA (eps > |v|).
    static bool eps_above_abs(NativeTu15PhysicsFrameFp &fp,u32 value,u32 eps) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char above{};
        asm volatile("flds %1\n\tfabs\n\tflds %2\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %0"
            :"=q"(above):"m"(value),"m"(eps):"cc","memory","st","st(1)");
        return above!=0;
    }
    // 26AAA4: FLD v; FSQRT; FST m32 root; FABS; FLD eps; FCOMIP; FSTP ST0.
    static std::pair<u32,bool> root_eps_above_abs(NativeTu15PhysicsFrameFp &fp,u32 squared,u32 eps) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char above{};u32 root{};
        asm volatile("flds %2\n\tfsqrt\n\tfsts %1\n\tfabs\n\tflds %3\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %0"
            :"=q"(above),"=m"(root):"m"(squared),"m"(eps):"cc","memory","st","st(1)");
        return {root,above!=0};
    }
};
#endif
}
