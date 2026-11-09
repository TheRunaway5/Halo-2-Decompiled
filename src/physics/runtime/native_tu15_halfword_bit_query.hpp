#pragma once
#include "halo2/native_tu15_halfword_bit_query_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_halfword_bit_query_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_HALFWORD_BIT_QUERY");return p&&*p;}();return enabled;}
struct Activity {std::uint64_t calls{};};
inline Activity&activity(){thread_local Activity value;return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#endif
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        for(const auto site:c->service_sites)if(site>=0xfa5d0u&&site<=0xfa5e3u)return false;
    }
    return true;
}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State>std::uint32_t shifted_mask(State&s,std::uint32_t value,unsigned count){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);auto shift=count;
    // Preserve the incoming arithmetic context for undefined AF/OF. This
    // native primitive also keeps the explicit retained reference accurate.
    // SAHF/LAHF avoid writes into a compiler-owned red zone.
    asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((shift&1u)<<11u));return value;
#else
    return s.flags.shift(value,count,4);
#endif
}
template<class State,class Before=Noop>struct Access {
    State&s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    void publish_mask(const NativeTu15HalfwordBitQuery&data){
        shifted_mask(s,1u,data.index);s.gp[0]=data.mask;
    }
    std::uint32_t read(NativeTu15HalfwordBitQueryPhase phase,const NativeTu15HalfwordBitQuery&data,std::uint32_t address,unsigned width){
        if(phase==NativeTu15HalfwordBitQueryPhase::argument){publish_mask(data);at(0xfa5d7u);}
        else{s.gp[1]=data.pointer;at(0xfa5dbu);}
        std::array<std::byte,4>bytes{};s.memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void complete(const NativeTu15HalfwordBitQuery&data){
        s.gp[2]=data.word;s.flags.logic(data.tested,4);s.gp[0]=(data.mask&0xffffff00u)|std::uint32_t(data.present);
        at(0xfa5e3u);s.pop();s.gp[4]+=4u;
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    ++activity().calls;Access<State,Before> access{s,before};
    native_tu15_halfword_contains_bit(access,s.gp[1],s.gp[4]+4u);return true;
}
} // namespace halo2::native_halfword_bit_query_detail
