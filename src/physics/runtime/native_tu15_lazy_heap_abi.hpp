#pragma once
#include "halo2/native_tu15_lazy_heap.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"
#include "platform/native_tu15_millisecond_clock.hpp"

namespace halo2::native_lazy_heap_detail {
struct Activity {NativeTu15LazyHeapStats kernel;std::map<std::uint32_t,std::uint64_t> children,timestamps;std::uint64_t milliseconds{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_LAZY_HEAP");return p&&*p;}();return enabled;}
template<class State> bool read_milliseconds(State &s) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s)||!native_millisecond_clock_detail::admitted(s))return false;
    ++activity().milliseconds;native_millisecond_clock_detail::run_admitted(s);return true;
}
template<class State,class Call> bool run(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15LazyHeapPhase;using Slot=NativeTu15LazyHeapSlot;
    auto &g=s.gp;auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    struct Frame final:NativeTu15LazyHeapFrame {
        State &s;std::uint32_t base;
        explicit Frame(State &state):s(state),base(state.gp[4]-36u){}
        static unsigned offset(Slot slot){constexpr std::array<unsigned,5> offsets{0x14,0x18,0x1c,0x20,0x10};return offsets[unsigned(slot)];}
        std::uint32_t load(Slot slot)const override {
            const auto value=physics_step_detail::r(s.memory,base+offset(slot),4);
            s.gp[slot==Slot::start_high?1u:0u]=value;return value;
        }
        void store(Slot slot,std::uint32_t value,std::uint32_t pc)override {
            if(pc==0x14999fu){s.gp[2]=s.gp[0];s.gp[2]=s.flags.increment(s.gp[2],4,false);}
            physics_step_detail::w(s.memory,s.observe,pc,base+offset(slot),4,value);
            if(pc==0x14995eu)s.gp[3]=s.gp[0];
        }
    }frame(s);
    bool slow{},constructed{};unsigned clock_number{};
    NativeTu15LazyHeapPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case Phase::binding:g[0]=data.value;g[4]=s.flags.minus(g[4],20u,4);s.push(0x149938u,g[6]);g[6]=s.flags.logic(0,4);s.flags.minus(g[0],g[6],4);break;
        case Phase::slow_begin:slow=true;s.push(0x149943u,g[5]);s.push(0x149944u,g[7]);g[5]=0x800000u;s.push(0x14994au,g[3]);break;
        case Phase::attempt_clock:g[7]=s.flags.logic(0,4);s.flags.minus(g[5],g[6],4);break;
        case Phase::pressure_binding:g[0]=data.pointer;break;
        case Phase::pressure_count:s.flags.minus(data.count,g[6],4);break;
        case Phase::allocated:g[7]=g[0];s.flags.minus(g[7],g[6],4);break;
        case Phase::retry:clock_number=0;s.flags.minus(g[0],90u,4);break;
        case Phase::clock_deadline:s.flags.minus(g[0],data.previous,4);break;
        case Phase::notification_value:g[0]=s.flags.plus(g[0],200u,4);break;
        case Phase::elapsed_delta:
            g[0]=s.flags.minus(g[0],g[3],4);g[2]=s.flags.borrow(g[2],g[1],4);s.flags.minus(g[2],g[6],4);
            if(std::bit_cast<std::int32_t>(g[2])<0){g[0]=s.flags.logic(0,4);g[2]=s.flags.logic(0,4);}
            else if(!g[2])s.flags.minus(g[0],g[6],4);break;
        case Phase::yield_binding:g[2]=data.pointer;break;
        case Phase::yield_process:g[0]=data.pointer;break;
        case Phase::yield_cursor:case Phase::yield_fallback:g[1]=data.value;break;
        case Phase::yield_bound:s.flags.minus(g[1],data.previous,4);break;
        case Phase::backing:s.flags.minus(g[7],g[6],4);break;
        case Phase::reduce_request:g[5]=s.flags.right(g[5],1u,4,true);s.flags.minus(g[5],0x100000u,4);break;
        case Phase::constructor_binding:g[0]=data.value;break;
        case Phase::constructor_inputs:
            g[1]=g[7]+15u;g[1]=s.flags.logic(g[1]&0xfffffff0u,4);g[0]=s.flags.plus(g[0],0x105440u,4);
            g[1]=s.flags.minus(g[1],g[7],4);s.flags.minus(g[0],g[6],4);g[3]=s.pop();constructed=data.object!=0;break;
        case Phase::constructor_size:g[5]=s.flags.minus(g[5],g[1],4);g[5]=s.flags.logic(g[5]&0xfffffffcu,4);break;
        case Phase::publish_binding:s.push(constructed?0x149a9au:0x149ab2u,1u);break;
        case Phase::returning:g[0]=data.value;break;
        default:break;
        }
    };
    const auto child=[&](std::uint32_t pc,std::uint32_t next,std::uint32_t owner,auto &context){
        ++activity().children[owner];at(pc);s.push(pc,next);call(s,owner,pc,false);context=native_partition_temporary_detail::floating_point(s);
    };
    NativeTu15LazyHeapServices services;
    services.timestamp_counter=[&](std::uint32_t pc){
        at(pc);if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");
        ++activity().timestamps[pc];const auto ticks=s.services.timestamp_counter();g[0]=std::uint32_t(ticks);g[2]=std::uint32_t(ticks>>32u);return ticks;
    };
    services.allocate=[&](auto,auto &context){s.push(0x149980u,g[6]);s.push(0x149981u,g[6]);s.push(0x149982u,g[6]);s.push(0x149983u,g[5]);child(0x149984u,0x149989u,0x12fe00u,context);return g[0];};
    services.cleanup_first=[&](auto &context){child(0x1499a3u,0x1499a8u,0x12ef10u,context);};
    services.cleanup_second=[&](auto &context){child(0x1499a8u,0x1499adu,0x12ef60u,context);};
    services.cleanup_third=[&](auto &context){child(0x1499adu,0x1499b2u,0x12f0c0u,context);};
    services.milliseconds=[&](auto &context){const auto first=clock_number++==0u;child(first?0x1499b2u:0x1499bfu,first?0x1499b7u:0x1499c4u,0x33b590u,context);return g[0];};
    services.notify=[&](auto &context){child(0x1499ceu,0x1499d3u,0x12edf0u,context);};
    services.deadline_reached=[&](auto,auto &context){
        s.push(0x1499fbu,g[2]);s.push(0x1499fcu,g[0]);child(0x1499fdu,0x149a02u,0x22a5c0u,context);
        at(0x149a02u);s.fp.push(physics_step_detail::ext(physics_step_detail::f(physics_step_detail::r(s.memory,0x46a0d0u,4))));
        at(0x149a08u);s.compare_and_pop(s.fp.st(1));at(0x149a0au);s.fp.pop();context=native_partition_temporary_detail::floating_point(s);
        return s.flags.cf||s.flags.zf;
    };
    services.yield_context=[&](auto,auto &context){child(0x149a14u,0x149a19u,0x404d10u,context);return g[2];};
    services.yield=[&](auto,auto &context){child(0x149a33u,0x149a38u,0x2dbf74u,context);};
    services.trim=[&](auto,auto &context){child(constructed?0x149aa1u:0x149abau,constructed?0x149aa6u:0x149abfu,0x1cd130u,context);};
    at(0x149930u);native_tu15_lazy_default_heap(s.memory,frame,fp,services,phases,s.observe,&activity().kernel);
    if(slow){g[7]=s.pop();g[5]=s.pop();}g[6]=s.pop();g[4]=s.flags.plus(g[4],20u,4);s.pop();at(constructed?0x149ab1u:0x149acau);return true;
}
} // namespace halo2::native_lazy_heap_detail
