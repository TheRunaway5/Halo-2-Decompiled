#include "halo2/native_tu15_overlap_query_handoff.hpp"
#include <stdexcept>

namespace halo2 {
bool native_tu15_handoff_overlap_query(SourcePhysicsContactMemory&memory,
    const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&abi,
    NativeTu15OverlapQueryHandoffPolicy policy,
    const std::function<std::uint64_t()>&clock,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15OverlapQueryHandoffActivity*activity){
    if(call.target!=0x2f73c0u&&call.target!=0x2f9ca0u&&call.target!=0x2fab70u)return false;
    NativeTu15OverlapQueryHandoffActivity local;auto&stats=activity?*activity:local;
    ++stats.attempts;
    try{
        const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);
            std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;};
        const auto sp=abi.gp[4];
        const bool cdecl=call.target==0x2fab70u;
        const auto site=cdecl?0x2f1806u:call.target==0x2f9ca0u?0x2f6fabu:0x2f4d3cu;
        const auto continuation=site+(cdecl?7u:3u);
        if(call.instruction!=site||call.tail||sp<policy.stack_first||
            std::uint64_t(sp)+20u>policy.stack_end||word(sp)!=continuation)
            throw std::runtime_error("Native overlap query requires its actual qualified caller");
        if(!policy.observers_read_only||
            (policy.first_trace_active&&*policy.first_trace_active)||
            (policy.second_trace_active&&*policy.second_trace_active))
            throw std::runtime_error("Native overlap query requires its retained-DATA observer domain");
        if(abi.x87_depth||abi.active_mmx||!abi.floating_point)
            throw std::runtime_error("Native overlap query requires its qualified empty x87/FP input");
        if((policy.data_first&15u)||policy.data_first<policy.stack_first||
            policy.data_bytes<0x2000u||
            std::uint64_t(policy.data_first)+policy.data_bytes>sp)
            throw std::runtime_error("Native overlap query requires disjoint caller-owned DATA");
        const auto first=word(sp+4u),second=word(sp+8u),configuration=word(sp+12u),collector=word(sp+16u);
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,
            abi.x87_status,abi.floating_point->mxcsr};
        const auto publish_fp=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        NativeTu15ConvexSimplexMemory guarded(memory);
        const auto p=policy.data_first+0x1000u;
        const NativeTu15ContactShapeScratch child{p+0x900u,p+0x910u,nullptr};
        const NativeTu15ConvexSimplexBindingScratch solver{
            {p+0xe00u,p+0xe04u,p+0xe08u,p+0xe10u,p+0xe20u,
                {p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,
                    {p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}}},
            {p+0x800u,p+0x810u,&child}};
        NativeTu15ConvexOverlapQueryServices services;
        services.geometry=native_tu15_make_bound_convex_contact_services(guarded,solver,0,clock,&stats.simplex);
        SourcePhysicsContactRecordObserver retained;
        if(observe)retained=[&](const auto&event){
            if(policy.instruction)policy.instruction(event.instruction);
            observe(event);
        };
        NativeTu15OverlapQueryBinding query(guarded,{policy.data_first,0x1000u},
            std::move(services),retained,&stats.queries);
        try{
            if(policy.instruction)policy.instruction(call.target);
            query.query({abi.gp[1]},{call.target},{first},{second},{configuration},{collector},fp);
        }catch(...){publish_fp();throw;}
        // Actual2FAB70 RET removes only its return address; the real2F1790
        // caller owns the four argument words and removes them at2F1818.
        publish_fp();abi.gp[4]=sp+(cdecl?4u:20u);++stats.completed;return true;
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
