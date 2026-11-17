#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "runtime/native_tu15_diagnostic_logger.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_diagnostic_logger_owner_detail {
inline constexpr std::uint32_t entry=0x2e16d0u;
struct Activity {std::uint64_t native_entries{},reference_entries{},retained_statements{};};
inline Activity&activity(){thread_local Activity value;return value;}
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_DIAGNOSTIC_LOGGER");return p&&*p;}();return enabled;
}
inline bool protocol(const SourcePhysicsWorldRuntimeServices&services){
    const auto&c=services.instruction_cursor;
    if(!services.instruction)return !c.position&&!c.literal_service_sites;
    return c.position&&c.callback_type&&services.instruction.target_type()==*c.callback_type&&
        c.executor==std::this_thread::get_id()&&c.literal_service_sites&&c.service_sites.size()==3&&
        c.service_sites[0]==0x38846eu&&c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace);
}
inline bool input_supported(const SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&services){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return protocol(services)&&abi.floating_point&&!abi.active_mmx&&abi.x87_depth<=8&&
        !(abi.x87_status&0x8080u)&&(abi.floating_point->x87_control_word&0x3fu)==0x3fu&&
        (abi.floating_point->mxcsr&0x1f80u)==0x1f80u&&!(abi.floating_point->mxcsr&0xffff0000u)&&
        native_raw_x87_detail::controls_supported(native_raw_x87_detail::capture());
#else
    (void)abi;(void)services;return false;
#endif
}
// Actual fixed-TU1.5 childless owner. No required-owner callback is invoked,
// and the store observer is unused. Unknown input contracts remain unhandled.
// World required-owner routing is authenticated separately from this adapter.
inline bool run(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRuntimeCall&call,
    SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(call.target!=entry||!input_supported(abi,services))return false;
    using State=physics_world_runtime_detail::WorldState;
    State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    auto publish=[&](auto&current,auto){abi=current.save();};
    try{
        if(reference()){
            ++activity().reference_entries;
            // Independently retained two-instruction reference from the exact
            // complete original bytes8D4108 C3; never delegate to the DATA core.
            native_raw_x87_detail::Scope scope{state};
            ++activity().retained_statements;abi=state.save();
            if(services.instruction)scope.io([&]{services.instruction(0x2e16d0u);});
            state.gp[0]=state.gp[1]+8u;
            ++activity().retained_statements;abi=state.save();
            if(services.instruction)scope.io([&]{services.instruction(0x2e16d3u);});
            scope.io([&]{state.pop();});
        }else{
            ++activity().native_entries;
            native_diagnostic_logger_detail::run_private(state,publish);
        }
    }catch(...){abi=state.save();throw;}
    abi=state.save();return true;
#else
    (void)memory;(void)call;(void)abi;(void)services;(void)observe;return false;
#endif
}
}
