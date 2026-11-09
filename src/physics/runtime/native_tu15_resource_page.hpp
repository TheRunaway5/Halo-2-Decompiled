#pragma once
#include "halo2/native_tu15_resource_page_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <cstdlib>

namespace halo2::native_resource_page_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RESOURCE_PAGE");return p&&*p;}();return enabled;}
template<class State,class Call>bool bridge(State&s,Call call){
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x126060u&&site<=0x1260b5u)return false;
    }
    using P=NativeTu15ResourcePagePhase;
    const auto position=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto progress=[&](P phase,const NativeTu15ResourcePage&d){
        auto&g=s.gp;
        switch(phase){
        case P::bytes:position(0x126060u);break;
        case P::cache:g[1]=d.bytes;position(0x126063u);break;
        case P::policy:
            g[2]=d.cache;position(0x126069u);s.push(0x126069u,g[3]);position(0x12606au);s.push(0x12606au,g[6]);
            g[0]=d.mode*3u;position(0x12606eu);break;
        case P::page_result:g[6]=d.page_handle;s.flags.minus(d.page_handle,0xffffffffu,4);break;
        case P::inactive_pool:position(0x126086u);break;
        case P::specific:g[2]=d.inactive_pool;break;
        case P::rows:position(0x126091u);break;
        case P::pending:
            g[1]=d.rows;g[0]=d.record;s.flags.plus((d.specific_datum&0xffffu)*16u,d.rows,4);position(0x12609eu);break;
        case P::job:position(0x1260a1u);break;
        case P::resource:position(0x1260a8u);break;
        case P::page_handle:position(0x1260abu);break;
        case P::finished:
            position(d.allocated?0x1260aeu:0x1260b1u);g[6]=s.pop();
            if(!d.allocated)g[0]=g[3];position(d.allocated?0x1260afu:0x1260b4u);g[3]=s.pop();
            position(d.allocated?0x1260b0u:0x1260b5u);s.pop();break;
        }
    };
    struct Owners {
        State&s;Call&call;decltype(position)&at;
        void invoke(std::uint32_t target,std::uint32_t site,std::uint32_t continuation){at(site);s.push(site,continuation);call(s,target,site,false);}
        NativeTu15ResourcePageResume page(const NativeTu15ResourcePage&d){
            s.gp[0]=d.policy;at(0x126075u);s.push(0x126075u,s.gp[0]);at(0x126076u);s.push(0x126076u,s.gp[1]);at(0x126077u);s.push(0x126077u,s.gp[2]);
            s.gp[3]=s.flags.logic(0,4);invoke(0x13fe50u,0x12607au,0x12607fu);
            return{s.gp[0],s.gp[2],s.gp[7],s.gp[6],s.gp[3]};
        }
        NativeTu15ResourcePageResume specific(const NativeTu15ResourcePage&){
            invoke(0x16e770u,0x12608cu,0x126091u);return{s.gp[0],s.gp[2],s.gp[7],s.gp[6],s.gp[3]};
        }
        void store(std::uint32_t address,std::uint32_t value,unsigned width,std::uint32_t pc){physics_step_detail::w(s.memory,s.observe,pc,address,width,value);}
    }owners{s,call,position};
    native_tu15_allocate_resource_page(s.memory,s.gp[7],s.gp[0],owners,progress);return true;
}
} // namespace halo2::native_resource_page_detail
