#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "physics_solver_internal.hpp"
#include <cstring>

namespace halo2::physics_contact_runtime_detail {
using namespace physics_step_detail;
using physics_solver_detail::divide;
using physics_solver_detail::root;
inline std::uint16_t source_x87_exception_flags() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t word;asm volatile("fnstsw %0":"=m"(word));return word&0x3fu;
#else
    throw std::runtime_error("Required original contact x87 environment backend unavailable");
#endif
}
inline void source_load_x87_exception_flags(std::uint16_t word) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Intel SDM Vol.1 Figure 8-9: the 28-byte environment stores SW at +4.
    // Preserve the host's live register tags, TOP, control word and pointers.
    std::array<std::uint32_t,7> environment;
    asm volatile("fnstenv %0":"=m"(environment)::"memory");
    environment[1]=(environment[1]&~0x3fu)|(word&0x3fu);
    asm volatile("fldenv %0"::"m"(environment):"memory");
#else
    throw std::runtime_error("Required original contact x87 environment backend unavailable");
#endif
}
struct ExtendedStack : physics_solver_detail::ExtendedStack {
    // Source TOP is independent of the number of live logical stack values.
    // A real caller can have an empty stack at any physical register index.
    unsigned top{};
    void push(long double x) {
        physics_solver_detail::ExtendedStack::push(x);top=(top+7u)&7u;
    }
    void pop() {
        physics_solver_detail::ExtendedStack::pop();top=(top+1u)&7u;
    }
    void comparison(long double b,unsigned pops) {
        status=physics_solver_detail::compare(st(0),b);
        for(unsigned i=0;i<pops;++i)pop();
    }
    std::uint16_t word()const { return (status&~0x383fu)|(top<<11u)|source_x87_exception_flags(); }
};
struct Flags : physics_solver_detail::Flags {
    bool af{};
    std::uint32_t logic(std::uint32_t x,unsigned n) { af=false;return physics_solver_detail::Flags::logic(x,n); }
    std::uint32_t plus(std::uint32_t a,std::uint32_t b,unsigned n) {
        const auto x=physics_solver_detail::Flags::plus(a,b,n);af=((a^b^x)&16u)!=0;return x;
    }
    std::uint32_t minus(std::uint32_t a,std::uint32_t b,unsigned n) {
        const auto x=physics_solver_detail::Flags::minus(a,b,n);af=((a^b^x)&16u)!=0;return x;
    }
    std::uint32_t borrow(std::uint32_t a,std::uint32_t b,unsigned n) {
        const auto carry=unsigned(cf),maskv=mask(n),sign=1u<<(n*8u-1u);a&=maskv;b&=maskv;
        const auto x=(a-b-carry)&maskv;cf=std::uint64_t(a)<std::uint64_t(b)+carry;
        of=(((a^b)&(a^x))&sign)!=0;af=((a^b^x)&16u)!=0;result(x,n);return x;
    }
    std::uint32_t increment(std::uint32_t a,unsigned n,bool down) {
        const auto carry=cf;const auto x=down?minus(a,1,n):plus(a,1,n);cf=carry;return x;
    }
    std::uint32_t shift(std::uint32_t a,unsigned count,unsigned n) {
        if(count&31u)af=false;return physics_solver_detail::Flags::shift(a,count,n);
    }
    std::uint32_t right(std::uint32_t a,unsigned count,unsigned n,bool arithmetic) {
        count&=31u;if(!count)return a;const auto width=n*8u;a&=mask(n);
        const bool sign=(a>>(width-1u))!=0;
        const auto extended=(arithmetic&&sign)?(a|~std::uint64_t(mask(n))):std::uint64_t(a);
        const auto x=std::uint32_t(std::bit_cast<std::int64_t>(extended)>>count)&mask(n);
        cf=count<=width?((a>>(count-1u))&1u)!=0:(arithmetic&&sign);
        if(count==1)of=arithmetic?false:sign;
        af=false;result(x,n);return x;
    }
    unsigned bits()const { return unsigned(cf)|(unsigned(pf)<<2u)|(unsigned(af)<<4u)|(unsigned(zf)<<6u)|(unsigned(sf)<<7u)|(unsigned(of)<<11u); }
    void load(unsigned x) { cf=x&1u;pf=x&4u;af=x&16u;zf=x&64u;sf=x&128u;of=x&2048u; }
    void scalar_compare(__m128 a,__m128 b,bool unordered) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        unsigned char carry,zero,parity;
        if(unordered)asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(a),"x"(b):"cc");
        else asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(a),"x"(b):"cc");
        cf=carry;zf=zero;pf=parity;af=sf=of=false;
#else
        throw std::runtime_error("Original contact scalar comparison backend unavailable");
#endif
    }
};
inline std::uint32_t integer_spill(long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t result;asm volatile("fldt %1\n\tfistpl %0":"=m"(result):"m"(x):"st");return result;
#else
    throw std::runtime_error("Original contact integer spill backend unavailable");
#endif
}
struct State {
    M &memory;const O &observe;const SourcePhysicsContactRuntimeServices &services;SourcePhysicsContactImage image;
    std::array<std::uint32_t,8> gp;__m128 xmm[8];Flags flags;ExtendedStack fp;
    std::uint32_t fs_base{},other_flags{};
    std::uint32_t reg(unsigned i,unsigned n=4,unsigned shift=0)const {return (gp[i]>>shift)&Flags::mask(n);}
    void set(unsigned i,std::uint32_t x,unsigned n=4,unsigned shift=0) {
        const auto mask=Flags::mask(n)<<shift;gp[i]=(gp[i]&~mask)|((x<<shift)&mask);
    }
    void push(std::uint32_t site,std::uint32_t x) {gp[4]-=4;w(memory,observe,site,gp[4],4,x);}
    std::uint32_t pop() {const auto x=r(memory,gp[4]);gp[4]+=4;return x;}
    void load(const SourcePhysicsContactRuntimeAbi &a) {
        if(a.active_mmx)throw std::runtime_error("Required original active MMX register-file owner unavailable");
        gp=a.gp;fs_base=a.fs_base;other_flags=a.eflags&~0x8d5u;flags.load(a.eflags);
        if(a.x87_depth>8)throw std::runtime_error("Invalid original contact x87 depth");
        fp.lanes=a.x87;fp.count=a.x87_depth;fp.status=a.x87_status;fp.top=(a.x87_status>>11u)&7u;
        if(a.floating_point) {
            const auto p=*a.floating_point;
            if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
                throw std::runtime_error("Required original contact unmasked exception owner unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            asm volatile("fldcw %0"::"m"(p.x87_control_word):"memory");
            _mm_setcsr(p.mxcsr);
#else
            throw std::runtime_error("Required original contact control word backend unavailable");
#endif
        }
        source_load_x87_exception_flags(a.x87_status);
        for(unsigned n=0;n<8;++n)xmm[n]=_mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(a.xmm[n].data())));
    }
    SourcePhysicsContactRuntimeAbi save()const {
        SourcePhysicsContactRuntimeAbi a{};a.gp=gp;a.fs_base=fs_base;a.eflags=other_flags|flags.bits();a.x87=fp.lanes;a.x87_depth=fp.count;a.x87_status=fp.word();
        std::uint16_t cw;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fnstcw %0":"=m"(cw));
#else
        throw std::runtime_error("Required original contact control word backend unavailable");
#endif
        a.floating_point=FP{cw,_mm_getcsr()};
        for(unsigned n=0;n<8;++n)_mm_storeu_si128(reinterpret_cast<__m128i*>(a.xmm[n].data()),_mm_castps_si128(xmm[n]));
        return a;
    }
    void required(std::uint32_t site,std::uint32_t target,bool tail) {
        if(!services.required_owner)throw std::runtime_error("Required original contact runtime callee unavailable");
        auto a=save();
        try {services.required_owner(memory,{site,target,tail},a,observe);}
        catch(...) {load(a);throw;}
        load(a);
    }
};
} // namespace halo2::physics_contact_runtime_detail
