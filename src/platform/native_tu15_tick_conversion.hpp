#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_tick_conversion_detail {
inline bool reference() {
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_TICK_CONVERSION");return p&&*p;}();
    return enabled;
}
template<class State> bool admitted(State&s) {
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*p=s.instruction_cursor;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace)||(s.observe&&!p->observation_only_stores))return false;
    }
    return true;
}
// The two arithmetic helpers below use native wide integer operations. Only
// their authored stack/read/store and final ABI transport remains explicit.
// Arbitrary memory/division-fault partial ABI is separately unqualified.
template<class State> void signed_divide(State&s) {
    using namespace physics_step_detail;auto&g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p){at(pc);return r(s.memory,p,4);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){at(pc);w(s.memory,s.observe,pc,p,4,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){at(pc);s.push(pc,value);};
    push(0x32ae70u,g[7]);push(0x32ae71u,g[6]);push(0x32ae72u,g[3]);unsigned negatives{};
    const auto dividend_high=read(0x32ae75u,g[4]+0x14u);
    if(dividend_high&0x80000000u){
        ++negatives;const auto low=read(0x32ae7eu,g[4]+0x10u);
        store(0x32ae89u,g[4]+0x14u,0u-dividend_high-unsigned(low!=0));store(0x32ae8du,g[4]+0x10u,0u-low);
    }
    const auto divisor_high=read(0x32ae91u,g[4]+0x1cu);
    if(divisor_high&0x80000000u){
        ++negatives;const auto low=read(0x32ae9au,g[4]+0x18u);
        g[0]=0u-divisor_high-unsigned(low!=0);
        store(0x32aea5u,g[4]+0x1cu,g[0]);store(0x32aea9u,g[4]+0x18u,0u-low);
    }else g[0]=divisor_high;
    std::uint64_t quotient{};
    if(!g[0]){
        g[1]=read(0x32aeb1u,g[4]+0x18u);const auto high=read(0x32aeb5u,g[4]+0x14u);
        const auto low=read(0x32aebfu,g[4]+0x10u);
        if(!g[1])throw std::runtime_error("Native signed division requires nonzero divisor");
        quotient=((std::uint64_t(high)<<32u)|low)/g[1];
    }else{
        const auto high_divisor=g[0];const auto low_divisor=read(0x32aecbu,g[4]+0x18u);
        const auto high=read(0x32aecfu,g[4]+0x14u),low=read(0x32aed3u,g[4]+0x10u);
        auto divisor=(std::uint64_t(high_divisor)<<32u)|low_divisor;
        auto dividend=(std::uint64_t(high)<<32u)|low;
        while(divisor>>32u){divisor>>=1u;dividend>>=1u;}
        const auto estimate=std::uint32_t(dividend/divisor);
        const auto actual_high_divisor=read(0x32aee7u,g[4]+0x1cu);
        g[1]=std::uint32_t(std::uint64_t(estimate)*actual_high_divisor);
        const auto actual_low_divisor=read(0x32aeedu,g[4]+0x18u);
        const auto product=std::uint64_t(estimate)*actual_low_divisor;
        const auto product_high=(product>>32u)+g[1];bool overshoot=product_high>0xffffffffu;
        if(!overshoot){
            const auto numerator_high=read(0x32aef7u,g[4]+0x14u);
            overshoot=std::uint32_t(product_high)>numerator_high;
            if(std::uint32_t(product_high)==numerator_high)
                overshoot=std::uint32_t(product)>read(0x32aeffu,g[4]+0x10u);
        }
        quotient=std::uint32_t(estimate-unsigned(overshoot));
    }
    g[0]=std::uint32_t(quotient);g[2]=std::uint32_t(quotient>>32u);
    // Both source branches clear carry before the final sign-count DEC.
    s.flags.cf=false;s.flags.increment(negatives,4,true);
    if(negatives==1){g[2]=0u-g[2];g[0]=s.flags.minus(0,g[0],4);g[2]=s.flags.borrow(g[2],0,4);}
    at(0x32af14u);g[3]=s.pop();at(0x32af15u);g[6]=s.pop();at(0x32af16u);g[7]=s.pop();
    at(0x32af17u);s.pop();g[4]+=16u;
}

struct Product {std::uint32_t low,high,flags;};
inline Product narrow_product(std::uint32_t a,std::uint32_t b) {
    Product out{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags{},multiplier=b;
    // Capture the host's undefined MUL arithmetic flags too, without a hidden
    // PUSH into a compiler-owned red zone. The original narrow path's final
    // flag writer before MUL is OR of its two zero high words. Seed that exact
    // logic result: some CPUs retain undefined MUL flags from their inputs.
    asm volatile("movl $0,%%edx; orl %%edx,%%edx; mull %%ecx; movl %%eax,%0; seto %%cl; lahf; movzbl %%ah,%%eax"
        :"=&r"(out.low),"=d"(out.high),"=a"(flags),"+c"(multiplier):"2"(a):"cc");
    out.flags=flags|((multiplier&1u)<<11u);
#else
    throw std::runtime_error("Native wide multiplication requires its x86 arithmetic backend");
#endif
    return out;
}
template<class State>void multiply(State&s){
    using namespace physics_step_detail;auto&g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p){at(pc);return r(s.memory,p,4);};
    g[0]=read(0x32acd0u,g[4]+8u);g[1]=read(0x32acd4u,g[4]+16u);const bool wide=(g[0]|g[1])!=0;
    g[1]=read(0x32acdau,g[4]+12u);
    if(!wide){
        g[0]=read(0x32ace0u,g[4]+4u);const auto product=narrow_product(g[0],g[1]);
        g[0]=product.low;g[2]=product.high;s.flags.load(product.flags);at(0x32ace6u);s.pop();g[4]+=16u;return;
    }
    at(0x32ace9u);s.push(0x32ace9u,g[3]);const auto first=std::uint32_t(std::uint64_t(g[0])*g[1]);
    const auto low=read(0x32aceeu,g[4]+8u),high=read(0x32acf2u,g[4]+0x14u);
    const auto cross=first+std::uint32_t(std::uint64_t(low)*high);
    const auto final_low=read(0x32acf8u,g[4]+8u);const auto product=std::uint64_t(final_low)*g[1];
    g[0]=std::uint32_t(product);g[2]=s.flags.plus(std::uint32_t(product>>32u),cross,4);
    at(0x32ad00u);g[3]=s.pop();at(0x32ad01u);s.pop();g[4]+=16u;
}

struct Seconds {long double value{};std::uint16_t status{};};
inline Seconds integer_value(std::int64_t value){
    Seconds result;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fildq %2; fnstsw %1; fstpt %0":"=m"(result.value),"=m"(result.status):"m"(value):"st");
#else
    throw std::runtime_error("Native tick conversion requires its x87 arithmetic backend");
#endif
    return result;
}
inline Seconds ratio(long double numerator,long double divisor){
    Seconds result;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldt %2; fldt %3; fdivrp %%st,%%st(1); fnstsw %1; fstpt %0"
        :"=m"(result.value),"=m"(result.status):"m"(numerator),"m"(divisor):"st","st(1)");
#else
    throw std::runtime_error("Native tick conversion requires its x87 arithmetic backend");
#endif
    return result;
}
inline Seconds sum(long double fraction,long double quotient){
    Seconds result;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldt %2; fldt %3; faddp; fnstsw %1; fstpt %0"
        :"=m"(result.value),"=m"(result.status):"m"(fraction),"m"(quotient):"st","st(1)");
#else
    throw std::runtime_error("Native tick conversion requires its x87 arithmetic backend");
#endif
    return result;
}
template<class State>void retire_binary(State&s,const Seconds&result){
    s.fp.st(1)=result.value;s.fp.pop();s.fp.status=(s.fp.status&~0x200u)|(result.status&0x200u);
}
template<class State,class Before>bool convert(State&s,Before&&before){
    if(!admitted(s)||s.fp.count>6)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t control{};asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return false;
#else
    return false;
#endif
    using namespace physics_step_detail;auto&g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p){at(pc);return r(s.memory,p,4);};
    const auto read64=[&](std::uint32_t pc,std::uint32_t p){at(pc);std::array<std::byte,8> data{};s.memory.read(p,data);std::uint64_t result{};
        for(unsigned i=0;i<8;++i)result|=std::uint64_t(std::to_integer<unsigned>(data[i]))<<(i*8u);return std::bit_cast<std::int64_t>(result);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){at(pc);w(s.memory,s.observe,pc,p,4,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){at(pc);s.push(pc,value);};
    g[4]-=8u;push(0x22a5c3u,g[6]);g[6]=read(0x22a5c4u,g[4]+0x14u);
    push(0x22a5c8u,g[7]);g[7]=read(0x22a5c9u,g[4]+0x14u);
    push(0x22a5cdu,0);push(0x22a5cfu,0x2bb5c755u);push(0x22a5d4u,g[6]);push(0x22a5d5u,g[7]);
    // SUB ESP,8 is the last source flag writer before the first CRT entry.
    s.flags.minus(g[4]+32u,8,4);push(0x22a5d6u,0x22a5dbu);before(s,0x32ae70u);signed_divide(s);
    push(0x22a5dbu,0);push(0x22a5ddu,0x2bb5c755u);push(0x22a5e2u,g[2]);push(0x22a5e3u,g[0]);
    store(0x22a5e4u,g[4]+0x18u,g[0]);store(0x22a5e8u,g[4]+0x1cu,g[2]);
    push(0x22a5ecu,0x22a5f1u);before(s,0x32acd0u);multiply(s);
    const auto old_low=g[7];g[7]=old_low-g[0];s.flags.cf=old_low<g[0];g[6]=s.flags.borrow(g[6],g[2],4);
    store(0x22a5f5u,g[4]+0x14u,g[7]);store(0x22a5f9u,g[4]+0x18u,g[6]);
    const auto remainder=read64(0x22a5fdu,g[4]+0x14u);
    s.fp.push(integer_value(remainder).value);before(s,0x22a601u);
    store(0x22a601u,g[4]+0x14u,0x2bb5c755u);store(0x22a609u,g[4]+0x18u,0);
    const auto divisor=read64(0x22a611u,g[4]+0x14u);
    s.fp.push(integer_value(divisor).value);before(s,0x22a615u);
    at(0x22a615u);g[7]=s.pop();at(0x22a616u);g[6]=s.pop();
    retire_binary(s,ratio(s.fp.st(1),s.fp.st(0)));before(s,0x22a619u);
    const auto quotient=read64(0x22a619u,g[4]);s.fp.push(integer_value(quotient).value);
    retire_binary(s,sum(s.fp.st(1),s.fp.st(0)));before(s,0x22a61eu);
    at(0x22a61eu);g[4]=s.flags.plus(g[4],8,4);at(0x22a621u);s.pop();g[4]+=8u;return true;
}
template<class State>bool convert(State&s){return convert(s,[](auto&,auto){});}
template<class State>bool run(State&s,std::uint32_t entry){
    if(!admitted(s))return false;
    if(entry==0x22a5c0u)return convert(s);
    if(entry==0x32ae70u){signed_divide(s);return true;}
    if(entry==0x32acd0u){multiply(s);return true;}
    return false;
}
} // namespace halo2::native_tick_conversion_detail
