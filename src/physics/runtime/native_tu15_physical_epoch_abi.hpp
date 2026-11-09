#pragma once
#include "halo2/native_tu15_physical_epoch.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_physical_epoch_detail {
struct Activity {NativeTu15PhysicalEpochStats kernel;std::map<std::uint32_t,std::uint64_t> children;};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_PHYSICAL_EPOCHS");return p&&*p;}();return enabled;}
template<class State>void position(State &s,std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;}
template<class State>bool reset_objects(State &s){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15PhysicalEpochPhase;auto &g=s.gp;
    NativeTu15PhysicalEpochPhases phases;phases.before_memory=[&](auto pc){position(s,pc);};phases.observe=[&](auto phase,const auto &data){
        switch(phase){
        case Phase::begin:position(s,0x12f040u);s.push(0x12f040u,g[3]);position(s,0x12f041u);s.push(0x12f041u,g[6]);break;
        case Phase::array:g[6]=data.array;break;
        case Phase::enabled:s.set(0,data.value,1,0);g[3]=s.flags.logic(0,4);s.flags.minus(s.reg(0,1,0),g[3]&255u,1);break;
        case Phase::iteration_begin:g[0]=0xffffffffu;position(s,0x12f056u);s.push(0x12f056u,g[7]);break;
        case Phase::index_advance:g[0]=s.flags.increment(g[0],4,false);s.flags.minus(g[0],data.scan?g[1]:g[3],4);break;
        case Phase::limit:if(data.scan)g[1]=data.limit;else s.flags.minus(g[0],data.limit,4);break;
        case Phase::bitmap:g[2]=data.bitmap;break;
        case Phase::bit_address:g[1]=s.flags.logic(g[0]&31u,4);g[7]=s.flags.shift(1u,g[1],4);g[1]=s.flags.right(g[0],5u,4,true);break;
        case Phase::bit_test:s.flags.logic(data.value&g[7],4);break;
        case Phase::occupied:s.flags.minus(g[0],0xffffffffu,4);break;
        case Phase::record_size:g[1]=data.stride;break;
        case Phase::record_base:g[2]=data.records;break;
        case Phase::record_address:g[1]=s.flags.product(g[1],g[0]);g[1]=s.flags.plus(g[1],g[2],4);break;
        case Phase::object:
            if(data.field<2u){g[2]=data.object;if(!data.field)s.flags.minus(g[2],g[3],4);}else g[1]=data.object;break;
        case Phase::finish:if(data.enabled){position(s,0x12f082u);g[7]=s.pop();}position(s,0x12f083u);g[6]=s.pop();break;
        default:break;
        }
    };
    position(s,0x12f040u);native_tu15_reset_physical_object_epochs(s.memory,phases,s.observe,&activity().kernel);
    position(s,0x12f08eu);g[3]=s.pop();position(s,0x12f08fu);s.pop();return true;
}
template<class State>bool rebase_pages(State &s){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15PhysicalEpochPhase;auto &g=s.gp;const auto pool=g[0];
    NativeTu15PhysicalEpochPhases phases;phases.before_memory=[&](auto pc){position(s,pc);};phases.observe=[&](auto phase,const auto &data){
        switch(phase){
        case Phase::begin:position(s,0x13fd10u);s.push(0x13fd10u,g[5]);position(s,0x13fd11u);s.push(0x13fd11u,g[6]);position(s,0x13fd12u);s.push(0x13fd12u,g[7]);break;
        case Phase::array:g[7]=data.array;break;
        case Phase::iteration_begin:g[0]=s.flags.logic(0xffffffffu,4);break;
        case Phase::index_advance:g[0]=s.flags.increment(g[0],4,false);if(data.scan)s.flags.minus(g[0],g[6],4);break;
        case Phase::limit:g[6]=data.limit;s.flags.minus(g[0],g[6],4);break;
        case Phase::bitmap:g[2]=data.bitmap;break;
        case Phase::bit_address:g[1]=s.flags.logic(g[0]&31u,4);g[5]=s.flags.shift(1u,g[1],4);g[1]=s.flags.right(g[0],5u,4,true);break;
        case Phase::bit_test:s.flags.logic(data.value&g[5],4);break;
        case Phase::occupied:s.flags.minus(g[0],0xffffffffu,4);break;
        case Phase::record_size:g[1]=data.stride;break;
        case Phase::record_base:g[2]=data.records;break;
        case Phase::record_address:g[1]=s.flags.product(g[1],g[0]);g[1]=s.flags.plus(g[1],g[2],4);break;
        case Phase::timestamp:g[5]=data.value;g[2]=s.flags.minus(0x7fffffffu,g[5],4);s.flags.minus(g[2],0x100u,4);break;
        case Phase::adjusted:g[6]=s.flags.minus(0x100u,g[2],4);break;
        default:break;
        }
    };
    position(s,0x13fd10u);native_tu15_rebase_physical_page_epochs(s.memory,pool,phases,s.observe,&activity().kernel);
    position(s,0x13fd4bu);g[7]=s.pop();position(s,0x13fd4cu);g[6]=s.pop();position(s,0x13fd4du);g[5]=s.pop();position(s,0x13fd4eu);s.pop();return true;
}
template<class State,class Call>bool tick(State &s,Call call){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15PhysicalEpochPhase;auto &g=s.gp;auto fp=native_partition_temporary_detail::floating_point(s);
    NativeTu15PhysicalEpochPhases phases;phases.before_memory=[&](auto pc){position(s,pc);};phases.observe=[&](auto phase,const auto &data){
        switch(phase){
        case Phase::counter:s.flags.increment(data.previous,4,false);break;
        case Phase::tick_begin:position(s,0x12f0cdu);s.push(0x12f0cdu,g[6]);break;
        case Phase::pool:g[6]=data.pool;break;
        case Phase::epoch:g[0]=data.value;position(s,0x12f0d7u);s.push(0x12f0d7u,g[7]);g[7]=0x7fffffffu;s.flags.minus(g[0],g[7],4);break;
        case Phase::epoch_advance:g[0]=s.flags.increment(g[0],4,false);break;
        case Phase::rebase_call:g[0]=g[6];break;
        default:break;
        }
    };
    NativeTu15PhysicalEpochServices services;
    services.reset_objects=[&](auto &context){
        ++activity().children[0x12f040u];position(s,0x12f0c8u);s.push(0x12f0c8u,0x12f0cdu);
        call(s,0x12f040u,0x12f0c8u,false);context=native_partition_temporary_detail::floating_point(s);
    };
    services.rebase_pages=[&](auto,auto &context){
        ++activity().children[0x13fd10u];position(s,0x12f0e3u);s.push(0x12f0e3u,0x12f0e8u);
        call(s,0x13fd10u,0x12f0e3u,false);context=native_partition_temporary_detail::floating_point(s);
        return NativeTu15PhysicalEpochResume{g[6],g[7]};
    };
    position(s,0x12f0c0u);native_tu15_advance_physical_epochs(s.memory,fp,services,phases,s.observe,&activity().kernel);
    position(s,0x12f106u);g[7]=s.pop();position(s,0x12f107u);g[6]=s.pop();position(s,0x12f108u);s.pop();return true;
}
template<class State,class Call>bool run(State &s,std::uint32_t entry,Call call){
    switch(entry){case 0x12f040u:return reset_objects(s);case 0x13fd10u:return rebase_pages(s);case 0x12f0c0u:return tick(s,call);default:return false;}
}
} // namespace halo2::native_physical_epoch_detail
