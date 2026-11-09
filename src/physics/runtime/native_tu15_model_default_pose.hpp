#pragma once
#include "halo2/native_tu15_model_default_pose_kernel.hpp"
#include <cstdlib>
#include <emmintrin.h>

namespace halo2::native_model_default_pose_detail {
template<class State>void position(State& state,std::uint32_t pc) {
    if(state.instruction_cursor&&state.instruction_cursor->position)*state.instruction_cursor->position=pc;
}
inline bool reference_mode() {
    static const bool enabled=[]{const auto* p=std::getenv("HALO2_REFERENCE_MODEL_DEFAULT_POSE");return p&&*p;}();return enabled;
}
template<class State>bool bridge(State& state) {
    if(reference_mode()||state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{state.selected_observation_pcs;})if(!state.selected_observation_pcs)return false;
    if(state.services.instruction) {
        if(!state.literal_service_sites||!state.instruction_cursor||!state.instruction_cursor->position)return false;
        const auto* cursor=state.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(state.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x1707a0u&&site<=0x170813u)return false;
    }
    auto& gp=state.gp;const auto stack=gp[4];
    position(state,0x1707a0u);gp[0]=word(state.memory,stack+4u);const auto model=gp[0];
    position(state,0x1707a4u);gp[1]=word(state.memory,model+0x48u);const auto count=gp[1];
    position(state,0x1707a7u);state.push(0x1707a7u,gp[6]);gp[6]=0;state.flags.logic(count,4);
    if(std::bit_cast<std::int32_t>(count)>0) {
        position(state,0x1707aeu);gp[1]=word(state.memory,stack+8u);const auto destination=gp[1];
        position(state,0x1707b2u);const auto one=word(state.memory,0x46a0d0u);
        state.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(one)));
        position(state,0x1707bau);state.push(0x1707bau,gp[3]);
        position(state,0x1707bbu);state.push(0x1707bbu,gp[5]);gp[2]=0;gp[1]=state.flags.plus(gp[1],28u,4);
        position(state,0x1707c1u);state.push(0x1707c1u,gp[7]);
        const auto progress=[&](CopyPhase phase,const CopyProgress& data) {
            constexpr std::array loads{0x1707cau,0x1707d1u,0x1707d7u,0x1707ddu,0x1707e6u,0x1707edu,0x1707f3u};
            constexpr std::array stores{0x1707cfu,0x1707d4u,0x1707dau,0x1707e0u,0x1707ebu,0x1707f0u,0x1707f6u};
            switch(phase) {
            case CopyPhase::nodes:position(state,0x1707c2u);break;
            case CopyPhase::node:gp[0]=state.flags.plus(data.records,data.result.source_offset,4);gp[7]=data.source+24u;break;
            case CopyPhase::read_component:
                if(data.lane==4)gp[0]=state.flags.plus(gp[0],12u,4);
                position(state,loads[data.lane]);break;
            case CopyPhase::write_component:
                if(data.lane<3){gp[5]=data.value;if(!data.lane)gp[3]=data.result.destination;}
                else if(data.lane==3)gp[7]=data.value;
                else if(data.lane<6){gp[3]=data.value;if(data.lane==4)gp[7]=data.result.destination+16u;}
                else gp[0]=data.value;
                position(state,stores[data.lane]);break;
            case CopyPhase::reload_model:position(state,0x1707f9u);break;
            case CopyPhase::scale:gp[0]=data.result.model;position(state,0x1707fdu);break;
            case CopyPhase::count:position(state,0x170801u);break;
            case CopyPhase::advanced:
                gp[7]=data.result.count;gp[6]=data.result.copied;gp[2]=data.result.source_offset;
                gp[1]=data.result.destination+28u;state.flags.minus(gp[6],gp[7],4);break;
            }
        };
        copy(state.memory,model,destination,one,[&]{return word(state.memory,stack+4u);},state.observe,progress);
        position(state,0x17080fu);gp[7]=state.pop();position(state,0x170810u);gp[5]=state.pop();
        position(state,0x170811u);gp[3]=state.pop();
    }
    position(state,0x170812u);gp[6]=state.pop();position(state,0x170813u);
    state.pop();gp[4]+=8u;return true;
}
} // namespace halo2::native_model_default_pose_detail
