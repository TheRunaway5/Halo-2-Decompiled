#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/native_tu15_record_lookup_kernel.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_record_lookup_detail {
inline std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t at) {
    std::array<std::byte,4> bytes{};memory.read(at,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
using Exit=NativeTu15RecordLookupExit;
using Lookup=NativeTu15RecordLookup;
// Descriptor slot records name an object through the low half of a datum.
// The source permits signed indices before the slot base: preserve its wrapping
// 32-bit address arithmetic rather than adding a new negative-index guard.
inline Lookup lookup(const SourcePhysicsContactMemory&memory,std::uint32_t descriptor,std::uint32_t index) {
    return native_tu15_record_lookup(memory,descriptor,index,[](auto,const auto&){});
}
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RECORD_LOOKUP");return p&&*p;}();return enabled;}
inline std::uint32_t found_flags(std::uint32_t datum) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags{},overflow{};
    // AF after SHL is architecturally undefined. Preserve its actual native
    // CPU behavior with the original CMP/AND/SHL producers in one block;
    // unrelated C++ memory/address operations must not seed host flags.
    asm volatile("cmpl $-1,%%ecx; andl $65535,%%ecx; shll $4,%%ecx; seto %%dl; lahf; movzbl %%ah,%%eax; movzbl %%dl,%%edx"
        :"+c"(datum),"=a"(flags),"=d"(overflow)::"cc");
    return flags|((overflow&1u)<<11u);
#else
    (void)datum;return 0;
#endif
}
template<class State>bool bridge(State&s) {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x1e3270u&&site<=0x1e329du)return false;
    }
    const auto descriptor=s.gp[2],index=s.gp[1];s.push(0x1e3270u,s.gp[6]);
    const auto result=lookup(s.memory,descriptor,index);s.gp[0]=result.object;s.gp[1]=result.escaped_index;
    if(result.exit==Exit::bounds)s.flags.minus(index,result.count,4);
    else if(result.exit==Exit::absent)s.flags.logic(0,4);
    else s.flags.load(found_flags(result.datum));
    s.gp[6]=s.pop();
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x1e329du;
    s.pop();return true;
#endif
}
} // namespace halo2::native_record_lookup_detail
