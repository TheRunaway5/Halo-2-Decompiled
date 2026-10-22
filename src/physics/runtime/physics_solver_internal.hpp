#pragma once
#include "halo2/physics_solver.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2::physics_solver_detail {
using namespace physics_step_detail;
// Utilities for statically translated solver control flow. There is no
// instruction fetch, opcode decoder, generic interpreter or executable image
// execution in production. Generated paths compile directly to native code.
struct Flags {
    bool cf{}, zf{}, sf{}, of{}, pf{};
    static std::uint32_t mask(unsigned n) { return n == 4 ? 0xffffffffu : (1u << (n * 8u)) - 1u; }
    void result(std::uint32_t x, unsigned n) {
        x &= mask(n); zf = x == 0; sf = (x >> (n * 8u - 1u)) != 0;
        pf = (std::popcount(x & 255u) & 1u) == 0;
    }
    std::uint32_t logic(std::uint32_t x, unsigned n) { cf = of = false; result(x,n); return x & mask(n); }
    std::uint32_t plus(std::uint32_t a, std::uint32_t b, unsigned n) {
        const auto maskv=mask(n), sign=1u<<(n*8u-1u);a&=maskv;b&=maskv;
        const auto full=std::uint64_t(a)+b;const auto x=std::uint32_t(full)&maskv;
        cf=full>maskv;of=((~(a^b)&(a^x))&sign)!=0;result(x,n);return x;
    }
    std::uint32_t minus(std::uint32_t a, std::uint32_t b, unsigned n) {
        const auto maskv=mask(n), sign=1u<<(n*8u-1u);a&=maskv;b&=maskv;
        const auto x=(a-b)&maskv;cf=a<b;of=(((a^b)&(a^x))&sign)!=0;result(x,n);return x;
    }
    std::uint32_t increment(std::uint32_t a, unsigned n, bool down) {
        const bool carry=cf;const auto x=down?minus(a,1,n):plus(a,1,n);cf=carry;return x;
    }
    std::uint32_t shift(std::uint32_t a, unsigned count, unsigned n) {
        count&=31u;if(!count)return a;
        const auto x=std::uint32_t(std::uint64_t(a&mask(n))<<count)&mask(n);
        if(count<=n*8u)cf=((a>>(n*8u-count))&1u)!=0;
        if(count==1)of=((x>>(n*8u-1u))!=0)!=cf;
        result(x,n);return x;
    }
    std::uint32_t product(std::uint32_t a,std::uint32_t b) {
        const auto full=std::int64_t(si(a))*si(b);const auto x=std::uint32_t(full);
        cf=of=full!=std::int64_t(si(x));return x;
    }
};
inline long double divide(long double a,long double b) { volatile long double x=a/b;return x; }
inline long double root(long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double out;asm volatile("fldt %1\n\tfsqrt\n\tfstpt %0":"=m"(out):"m"(x):"st");return out;
#else
    throw std::runtime_error("Original solver x87 square-root backend unavailable");
#endif
}
inline std::uint16_t compare(long double a,long double b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t status;asm volatile("fldt %2\n\tfldt %1\n\tfcompp\n\tfnstsw %0":"=m"(status):"m"(a),"m"(b):"st");return status;
#else
    throw std::runtime_error("Original solver x87 comparison backend unavailable");
#endif
}
struct ExtendedStack {
    std::array<long double,8> lanes{};unsigned count{};std::uint16_t status{};
    long double &st(unsigned i) {
        if(i>=count)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
        return lanes[i];
    }
    void push(long double x) {
        if(count==8)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
        for(unsigned i=count;i;i--)lanes[i]=lanes[i-1];
        lanes[0]=x;++count;
    }
    void pop() { st(0);for(unsigned i=1;i<count;++i)lanes[i-1]=lanes[i];--count; }
    void comparison(long double b,unsigned pops) {
        status=compare(st(0),b);for(unsigned i=0;i<pops;++i)pop();
    }
    std::uint16_t word()const { return (status&~0x3800u)|(((8u-count)&7u)<<11u); }
};
struct State {
    M &memory;const O &observe;const SourcePhysicsSolverServices &services;SourcePhysicsContactImage image;
    std::array<std::uint32_t,8> gp{};__m128 xmm[8]{};Flags flags;ExtendedStack fp;
    // EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI; all widths are explicit in generated code.
    std::uint32_t reg(unsigned i,unsigned n=4,unsigned shift=0)const { return (gp[i]>>shift)&Flags::mask(n); }
    void set(unsigned i,std::uint32_t x,unsigned n=4,unsigned shift=0) {
        const auto mask=Flags::mask(n)<<shift;gp[i]=(gp[i]&~mask)|((x<<shift)&mask);
    }
    void push(std::uint32_t site,std::uint32_t x) { gp[4]-=4;w(memory,observe,site,gp[4],4,x); }
    std::uint32_t pop() {const auto x=r(memory,gp[4]);gp[4]+=4;return x;}
    void quotient(std::uint32_t denominator,bool signed_division) {
        const auto numerator=(std::uint64_t(gp[2])<<32u)|gp[0];
        if(!denominator)throw std::runtime_error("Required original solver integer division-fault owner unavailable");
        if(signed_division) {
            const auto a=std::bit_cast<std::int64_t>(numerator),b=std::int64_t(si(denominator));
            if(a==std::numeric_limits<std::int64_t>::min()&&b==-1)
                throw std::runtime_error("Required original solver integer division-fault owner unavailable");
            const auto q=a/b;if(q<std::numeric_limits<std::int32_t>::min()||q>std::numeric_limits<std::int32_t>::max())
                throw std::runtime_error("Required original solver integer division-fault owner unavailable");
            gp[0]=std::uint32_t(q);gp[2]=std::uint32_t(a%b);
        } else {
            const auto q=numerator/denominator;if(q>0xffffffffu)
                throw std::runtime_error("Required original solver integer division-fault owner unavailable");
            gp[0]=std::uint32_t(q);gp[2]=std::uint32_t(numerator%denominator);
        }
    }
    void required(std::uint32_t site,std::uint32_t target,std::uint32_t next,
        std::optional<std::uint32_t> receiver,unsigned count) {
        push(site,next);
        if(!services.required_owner)throw std::runtime_error("Required original solver callback/initialization owner unavailable");
        std::vector<std::uint32_t> args;for(unsigned i=0;i<count;++i)args.push_back(r(memory,gp[4]+4u+i*4u));
        gp[0]=services.required_owner(memory,{site,target,gp[4],receiver,std::move(args)},observe);
        gp[4]+=4; // These original lower calls are cdecl; caller removes arguments.
    }
};
} // namespace halo2::physics_solver_detail
