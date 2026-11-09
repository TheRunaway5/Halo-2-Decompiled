#pragma once
#include "halo2/native_tu15_physical_callbacks.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_physical_callbacks_detail {
struct Activity {NativeTu15PhysicalCallbackStats kernel;std::map<std::uint32_t,std::uint64_t> children;};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_PHYSICAL_CALLBACKS");return p&&*p;}();return enabled;}
template<class State,class Call> bool run(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15PhysicalCallbackPhase;auto &g=s.gp;
    auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    NativeTu15PhysicalCallbackPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case Phase::binding:g[1]=data.head;s.flags.logic(g[1],4);s.push(0x12ef18u,g[6]);g[6]=g[1];break;
        case Phase::callback_owner:g[0]=data.owner;s.flags.logic(g[0],4);break;
        case Phase::callback_context:g[1]=data.context;break;
        case Phase::callback_address:g[2]=data.address;break;
        case Phase::callback_call:s.push(0x12ef2du,g[1]);s.push(0x12ef2eu,g[2]);break;
        case Phase::reacquired_head:g[1]=data.head;break;
        case Phase::head_check:s.flags.logic(g[1],4);g[0]=g[1];break;
        case Phase::search_compare:s.flags.minus(g[0],g[6],4);break;
        case Phase::search_next:g[0]=data.scan;s.flags.logic(g[0],4);break;
        case Phase::next:g[6]=data.current;break;
        case Phase::restart:g[6]=g[1];break;
        case Phase::current_check:s.flags.logic(g[6],4);break;
        }
    };
    NativeTu15PhysicalCallbackServices services;services.invoke=[&](const auto &request,auto &context){
        ++activity().children[request.owner];at(0x12ef2fu);s.push(0x12ef2fu,0x12ef31u);
        call(s,request.owner,0x12ef2fu,false);context=native_partition_temporary_detail::floating_point(s);
        return NativeTu15PhysicalCallbackResult{g[6]};
    };
    at(0x12ef10u);native_tu15_notify_physical_allocations(s.memory,fp,services,phases,&activity().kernel);
    g[6]=s.pop();s.pop();at(0x12ef57u);return true;
}
} // namespace halo2::native_physical_callbacks_detail
