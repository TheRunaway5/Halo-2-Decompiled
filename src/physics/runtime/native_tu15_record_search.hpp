#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_record_search_detail {
inline std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t at) {
    std::array<std::byte,4> bytes{};memory.read(at,bytes);std::uint32_t result{};
    for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return result;
}
inline void word(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&observe,
    std::uint32_t site,std::uint32_t at,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,at,{},{}};
    if(observe){event.before.resize(4);memory.read(at,event.before);}
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
    memory.write(at,bytes);if(observe){event.written.assign(bytes.begin(),bytes.end());observe(event);}
}
enum class Exit {bounds,endpoint,published};
struct Search {
    std::uint32_t result{},last_publication{},midpoint{},low{},high{};
    bool probed{};
    Exit exit{};
};
// Actual1E5810: signed-key binary search over an authored count/base header.
// Bounds/address arithmetic wraps as32bits; midpoint is an arithmetic half of
// that wrapping sum. Arguments and the result cell are real lazy memory views:
// duplicate choice, the second unequal-key read, and both result rereads matter.
inline Search search(SourcePhysicsContactMemory&memory,std::uint32_t table,std::uint32_t count,
    std::uint32_t caller_stack,const SourcePhysicsContactRecordObserver&observe) {
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    Search result;result.high=count-1u;
    word(memory,observe,0x1e581eu,caller_stack+4u,0);
    bool endpoint=false;
    while(!endpoint&&signed_word(result.low)<=signed_word(result.high)) {
        const auto records=word(memory,table+4u);
        endpoint=result.low==result.high;
        const auto sum=result.high+result.low;
        result.midpoint=(sum>>1u)|(sum&0x80000000u);result.probed=true;
        const auto stride=word(memory,caller_stack+12u);
        const auto candidate=records+result.midpoint*stride;
        const auto value=word(memory,candidate);
        const auto first_key=word(memory,caller_stack+8u);
        if(value==first_key)word(memory,observe,0x1e5848u,caller_stack+4u,candidate);
        else {
            const auto key=word(memory,caller_stack+8u);
            if(signed_word(value)>signed_word(key))result.high=result.midpoint-1u;
            else if(signed_word(value)<signed_word(key))result.low=result.midpoint+1u;
        }
        result.last_publication=word(memory,caller_stack+4u);
        if(result.last_publication){result.exit=Exit::published;break;}
    }
    if(!result.last_publication)result.exit=endpoint?Exit::endpoint:Exit::bounds;
    result.result=word(memory,caller_stack+4u);return result;
}
inline bool reference_mode() {
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RECORD_SEARCH");return p&&*p;}();return enabled;
}
template<class State>bool bridge(State&state) {
    if(reference_mode()||state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{state.selected_observation_pcs;})if(!state.selected_observation_pcs)return false;
    if(state.services.instruction) {
        if(!state.literal_service_sites||!state.instruction_cursor||!state.instruction_cursor->position)return false;
        const auto*cursor=state.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(state.observe&&!cursor->observation_only_stores)return false;
        // A caller-declared real instruction service inside this function must
        // retain its owner. The production service sites lie outside this span.
        for(const auto site:cursor->service_sites)if(site>=0x1e5810u&&site<=0x1e5870u)return false;
    }
    auto&gp=state.gp;const auto stack=gp[4];
    state.push(0x1e5810u,gp[3]);state.push(0x1e5811u,gp[5]);
    const auto table=word(state.memory,stack+4u);
    state.push(0x1e5816u,gp[6]);const auto count=word(state.memory,table);
    state.push(0x1e581du,gp[7]);
    const auto result=search(state.memory,table,count,stack,state.observe);
    gp[0]=result.result;
    if(result.probed){gp[1]=result.last_publication;gp[2]=result.midpoint;}
    if(result.exit==Exit::published)state.flags.logic(result.last_publication,4);
    else if(result.exit==Exit::endpoint)state.flags.logic(1,1);
    else state.flags.minus(result.low,result.high,4);
    gp[7]=state.pop();gp[6]=state.pop();gp[5]=state.pop();gp[3]=state.pop();
    if(state.instruction_cursor&&state.instruction_cursor->position)*state.instruction_cursor->position=0x1e5870u;
    state.pop();gp[4]+=12u;return true;
}
} // namespace halo2::native_record_search_detail
