#include "halo2/native_tu15_support_carrier_handoff.hpp"
#include <stdexcept>
namespace halo2 {
bool native_tu15_handoff_support_carrier(SourcePhysicsContactMemory&m,
    const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&abi,
    NativeTu15SupportCarrierHandoffPolicy policy,const SourcePhysicsContactRecordObserver&o,
    NativeTu15SupportCarrierHandoffActivity*activity){
    if(call.target!=0x184ec0u)return false;
    NativeTu15SupportCarrierHandoffActivity local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
        const auto read=[&](std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t n{};
            for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;};
        const auto sp=abi.gp[4];
        if(call.instruction!=0x1fa18du||call.tail||sp<policy.stack_first||
           std::uint64_t(sp)+28u>policy.stack_end||read(sp)!=0x1fa192u)
            throw std::runtime_error("Native support carrier requires its actual qualified caller");
        if(!policy.observers_read_only||(policy.first_trace_active&&*policy.first_trace_active)||
           (policy.second_trace_active&&*policy.second_trace_active))
            throw std::runtime_error("Native support carrier requires its retained-DATA observer domain");
        if(abi.x87_depth>7u||abi.active_mmx||!abi.floating_point||(abi.eflags&0x400u)||
           (abi.floating_point->x87_control_word&0x3fu)!=0x3fu||(abi.x87_status&0x8080u)||
           (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u||(abi.floating_point->mxcsr&0xffff0000u))
            throw std::runtime_error("Native support carrier requires its qualified preserved x87/SIMD/DF input");
        if((policy.data_first&15u)||policy.data_first<policy.stack_first||policy.data_bytes<0x100u||
           std::uint64_t(policy.data_first)+policy.data_bytes>sp)
            throw std::runtime_error("Native support carrier requires separate caller-owned DATA");
        const NativeTu15SupportCarrierRequest request{abi.gp[1],abi.gp[3],read(sp+4u),read(sp+8u),
            read(sp+12u),read(sp+16u),read(sp+20u),read(sp+24u)};
        for(const auto range:{std::pair{request.previous_velocity,12u},std::pair{request.point,12u},
            std::pair{request.previous_frame,52u},std::pair{request.current_frame,52u},
            std::pair{request.velocity,12u},std::pair{request.delta,12u},std::pair{request.basis,36u}})
            if(std::uint64_t(range.first)+range.second>0x100000000ull||
               (std::uint64_t(range.first)<std::uint64_t(policy.data_first)+0x100u&&
                std::uint64_t(policy.data_first)<std::uint64_t(range.first)+range.second))
                throw std::runtime_error("Native support carrier public DATA aliases its temporary values");
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        bool result{};
        try{
            if(policy.instruction)policy.instruction(call.target);
            const auto p=policy.data_first;
            result=native_tu15_update_support_carrier(m,request,{p,p+0x40u,p+0x80u,p+0xc0u,p+0xd0u},fp,o,&stats.carriers);
        }catch(...){publish();throw;}
        publish();abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(result);abi.gp[4]=sp+28u;
        ++stats.completed;return true;
    }catch(...){++stats.refusals;throw;}
}
}
