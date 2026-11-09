#pragma once
#include "halo2/native_tu15_resource_touch_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <cstdlib>

namespace halo2::native_resource_touch_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_RESOURCE_TOUCH");return p&&*p;}();return enabled;}
template<class State,class Call>bool bridge(State&s,Call call) {
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x126200u&&site<=0x1262adu)return false;
    }
    using P=NativeTu15ResourceTouchPhase;using E=NativeTu15ResourceTouchExit;
    const auto position=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto compare_missing=[&]{s.flags.minus(s.gp[0],0xffffffffu,4);};
    const auto progress=[&](P phase,const NativeTu15ResourceTouch&d) {
        switch(phase) {
        case P::enabled:
            position(0x126200u);s.push(0x126200u,s.gp[3]);s.gp[3]=d.resource;position(0x126203u);break;
        case P::save_mode:
            s.gp[0]=d.mode-1u;s.flags.logic(d.priority_byte,1);
            position(0x126217u);s.push(0x126217u,s.gp[6]);
            s.gp[0]=s.flags.increment(d.mode-1u,4,false);s.gp[6]=d.mode;break;
        case P::handle:position(0x12621bu);break;
        case P::search:s.gp[0]=d.handle;compare_missing();break;
        case P::allocation_pool:compare_missing();position(0x12622du);break;
        case P::allocate:s.gp[2]=d.pool;break;
        case P::allocated_records:compare_missing();position(0x12623du);break;
        case P::allocated_mode:
            s.gp[2]=d.records;s.gp[0]=d.record;s.gp[1]=(d.datum&0xffffu)*3u;
            s.flags.logic(d.datum&0xffffu,4);position(0x12624bu);break;
        case P::restore_mode:
            if(d.exit==E::maintenance)position(0x12624eu);
            else{s.gp[0]=s.flags.plus(d.clock,30000u,4);position(0x12626fu);}
            s.gp[6]=s.pop();break;
        case P::allocated_owner:position(0x12624fu);break;
        case P::maintenance:position(0x126252u);s.gp[3]=s.pop();break;
        case P::clock_first:compare_missing();break;
        case P::deadline:position(0x12625du);break;
        case P::clock_second:s.flags.minus(d.clock,d.deadline,4);break;
        case P::deadline_store:position(0x126270u);break;
        case P::threshold:
            s.gp[0]=d.handle&0xffffu;s.gp[1]=d.mode*3u;s.flags.logic(d.handle&0xffffu,4);
            position(0x12627fu);break;
        case P::queue_pool:
            s.gp[1]=d.threshold;s.gp[2]=(d.handle&0xffffu)*3u;position(0x126289u);break;
        case P::queue_descriptor:s.gp[0]=d.queue;position(0x12628eu);break;
        case P::queue_records:s.gp[6]=d.queue_descriptor;position(0x126291u);break;
        case P::queue_time:s.gp[6]=d.queue_records;position(0x126294u);break;
        case P::save_timestamp_cursor:
            s.gp[0]=d.current;s.gp[2]=d.timestamp;position(0x12629bu);s.push(0x12629bu,s.gp[7]);break;
        case P::timestamp:position(0x12629cu);break;
        case P::restore_timestamp_cursor:
            s.gp[7]=d.previous;s.gp[6]=d.elapsed;s.flags.minus(d.elapsed,d.threshold,4);
            position(0x1262a4u);s.gp[7]=s.pop();break;
        case P::timestamp_store:
            s.gp[0]=s.flags.minus(d.current,d.threshold,4);position(0x1262a9u);break;
        case P::finished:
            if(d.exit==E::disabled){s.gp[0]=(s.gp[0]&0xffffff00u)|d.enabled;s.flags.logic(d.enabled,1);}
            else if(d.exit==E::found)compare_missing();
            else if(d.exit==E::allocation_deadline)s.flags.minus(d.clock,d.deadline,4);
            if(d.exit!=E::disabled&&d.exit!=E::deadline_updated){position(0x1262abu);s.gp[6]=s.pop();}
            position(d.exit==E::deadline_updated?0x126275u:0x1262acu);s.gp[3]=s.pop();
            position(d.exit==E::deadline_updated?0x126276u:0x1262adu);s.pop();break;
        }
    };
    struct Owners {
        State&s;Call&call;decltype(position)&at;
        void invoke(std::uint32_t target,std::uint32_t site,std::uint32_t continuation) {
            at(site);s.push(site,continuation);call(s,target,site,false);
        }
        std::uint32_t search(NativeTu15ResourceTouch&) {
            invoke(0x126190u,0x126223u,0x126228u);return s.gp[0];
        }
        NativeTu15ResourceTouchChild allocate(NativeTu15ResourceTouch&) {
            invoke(0x16e6a0u,0x126233u,0x126238u);return {s.gp[0],s.gp[2],s.gp[3],s.gp[6]};
        }
        std::uint32_t clock(NativeTu15ResourceTouch&,bool second) {
            invoke(0x33b590u,second?0x126265u:0x126258u,second?0x12626au:0x12625du);return s.gp[0];
        }
        void store(std::uint32_t address,std::uint32_t value,std::uint32_t site) {
            physics_step_detail::w(s.memory,s.observe,site,address,4,value);
        }
        void maintenance(NativeTu15ResourceTouch&) {at(0x126253u);call(s,0x1264e0u,0x126253u,true);}
    }owners{s,call,position};
    native_tu15_touch_resource(s.memory,s.gp[0],s.gp[1]&255u,owners,progress);return true;
}
} // namespace halo2::native_resource_touch_detail
