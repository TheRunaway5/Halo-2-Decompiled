#pragma once
#include "halo2/native_tu15_main_heap_allocate.hpp"
#include "physics/runtime/native_tu15_main_heap_abi.hpp"

namespace halo2::native_main_heap_allocation_detail {
struct Activity {std::uint64_t sized_wrappers{},allocations{},gap_probes{};};
inline Activity &activity(){thread_local Activity value;return value;}
using Phase=NativeTu15MainHeapAllocationPhase;
using Slot=NativeTu15MainHeapAllocationSlot;
template<class State> bool probe_gap(State &s) {
    if(native_main_heap_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().gap_probes;auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    struct Sink final:NativeTu15MainHeapCandidateSink {
        State &s;std::uint32_t address;
        explicit Sink(State &state):s(state),address(state.gp[6]){}
        void candidate(unsigned word,std::uint32_t value,std::uint32_t site)override {
            physics_step_detail::w(s.memory,s.observe,site,address+word*4u,4,value);
        }
    }sink(s);
    NativeTu15MainHeapAllocationPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case Phase::gap_begin:s.push(0x232d30u,g[3]);break;
        case Phase::gap_allocator:g[3]=data.allocator;break;
        case Phase::gap_capacity:g[0]=s.flags.logic(0,4);s.flags.minus(g[7],g[1],4);s.push(0x232d39u,g[5]);break;
        case Phase::gap_class:g[5]=data.memory_class;break;
        case Phase::gap_category:g[5]=data.category;break;
        case Phase::gap_aligned:g[1]=s.flags.plus(data.previous_offset,15u,4);g[1]=s.flags.logic(g[1]&0xfffffff0u,4);break;
        case Phase::gap_candidate:g[5]=data.end;s.flags.minus(g[7],g[5],4);break;
        case Phase::gap_base:g[0]=data.base;break;
        case Phase::gap_result:g[0]=s.flags.plus(g[0],g[1],4);break;
        default:break;
        }
    };
    const auto result=native_tu15_probe_main_heap_gap(s.memory,{g[1],g[2],g[7]},
        {[&]{at(0x232d31u);return physics_step_detail::r(s.memory,g[4]+8u,4);},
         [&]{at(0x232d3cu);return physics_step_detail::r(s.memory,g[4]+16u,4);}},sink,phases,s.observe);
    if(result.residual.required){at(result.residual.instruction);s.required(0x22e447u,result.residual.owner,true);return true;}
    g[5]=s.pop();g[3]=s.pop();s.pop();g[4]+=8u;at(0x232d61u);return true;
}

template<class State,class Call> bool allocate(State &s,Call call) {
    if(native_main_heap_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().allocations;auto &g=s.gp;const auto allocator=g[1];
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    struct Frame final:NativeTu15MainHeapAllocationFrame {
        State &s;std::uint32_t address;bool tail{};
        explicit Frame(State &state):s(state),address(state.gp[4]-0x24u){}
        static std::uint32_t offset(Slot slot){constexpr std::array<std::uint32_t,7> offsets{0x10u,0x14u,0x18u,0x1cu,0x20u,0x28u,0x2cu};return offsets[unsigned(slot)];}
        std::uint32_t load(Slot slot)const override {
            const auto value=physics_step_detail::r(s.memory,address+offset(slot),4);auto &g=s.gp;
            switch(slot) {
            case Slot::result:case Slot::index:g[0]=value;break;
            case Slot::row_cursor:g[1]=value;break;
            case Slot::candidate_offset:g[tail?5:2]=value;break;
            case Slot::candidate_bytes:case Slot::bytes:g[2]=value;break;
            case Slot::memory_class:g[tail?7:1]=value;break;
            }
            return value;
        }
        void store(Slot slot,std::uint32_t value,std::uint32_t site)override {
            if(site==0x232b1bu)s.flags.logic(s.gp[2],4);
            if(site==0x232c32u)s.flags.logic(s.gp[0],4);
            physics_step_detail::w(s.memory,s.observe,site,address+offset(slot),4,value);
        }
    }frame(s);
    bool tail_checked{};
    NativeTu15MainHeapAllocationPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case Phase::begin:
            g[4]=s.flags.minus(g[4],0x14u,4);s.push(0x232ad3u,g[3]);s.push(0x232ad4u,g[5]);s.push(0x232ad5u,g[6]);g[3]=g[1];break;
        case Phase::count:
            g[2]=data.count;g[6]=s.flags.logic(0,4);g[5]=s.flags.logic(0,4);s.flags.minus(g[2],g[6],4);s.push(0x232ae1u,g[7]);break;
        case Phase::tail_header:if(data.field==0u)g[1]=data.end;else g[6]=data.base;break;
        case Phase::tail_cursor:g[0]=data.count*8u;g[1]=s.flags.minus(g[1],g[0],4);break;
        case Phase::tail_record:
            if(data.field==0u){g[0]=data.offset;g[1]=s.flags.plus(g[1],g[6],4);}else{g[6]=data.length;g[6]=s.flags.plus(g[6],g[0],4);}break;
        case Phase::capacity_end:g[7]=data.end;break;
        case Phase::capacity:g[1]=data.count*8u+8u;g[0]=g[1];g[1]=s.flags.minus(g[7],g[0],4);s.flags.minus(g[6],g[1],4);break;
        case Phase::search_begin:g[1]=data.row;break;
        case Phase::gap_inputs:g[2]=data.bytes;g[7]=data.offset;g[0]=data.cursor+data.bytes;s.flags.minus(g[7],g[0],4);break;
        case Phase::gap_call:g[1]=data.memory_class;break;
        case Phase::gap_return:s.flags.logic(g[0],4);break;
        case Phase::search_advance:
            g[5]=data.length;g[0]=data.previous_index;g[6]=data.count;g[5]=s.flags.plus(g[5],data.offset,4);
            g[0]=s.flags.increment(g[0],4,false);g[1]=s.flags.minus(data.previous_row,8u,4);s.flags.minus(g[0],g[6],4);break;
        case Phase::insert_count:g[1]=data.previous_count;g[0]=data.index;g[1]=s.flags.increment(g[1],4,false);break;
        case Phase::insert_move_begin:g[1]=data.previous_count;g[2]=data.shift_offset;g[1]=s.flags.minus(g[1],g[0],4);g[6]=g[1];break;
        case Phase::insert_header:
            if(data.field==0u)g[1]=data.end;else{g[7]=data.base;g[1]=s.flags.minus(g[1],g[2],4);g[1]=s.flags.plus(g[1],g[7],4);}break;
        case Phase::insert_word:g[7]=data.value;break;
        case Phase::insert_advance:g[2]=s.flags.minus(g[2],8u,4);g[6]=s.flags.increment(g[6],4,true);break;
        case Phase::insert_publish_header:
            if(data.field==0u)g[1]=data.base;else{g[2]=data.index*8u+8u;g[0]=data.end;g[0]=s.flags.minus(g[0],g[2],4);}break;
        case Phase::insert_candidate:g[2]=data.value;break;
        case Phase::result_check:g[0]=data.result;s.flags.logic(g[0],4);break;
        case Phase::tail_inputs:
            frame.tail=true;if(data.field==0u){g[0]=data.count;tail_checked=false;}else g[6]=data.end;break;
        case Phase::tail_capacity:
            if(!tail_checked){g[0]=data.count*8u+8u;g[1]=s.flags.minus(g[6],g[0],4);g[0]=s.flags.logic(0,4);s.flags.minus(g[1],g[5],4);tail_checked=true;}
            else{g[7]=data.offset;s.flags.minus(g[1],g[7],4);}break;
        case Phase::memory_class:g[7]=data.memory_class;break;
        case Phase::category:g[7]=data.category;break;
        case Phase::aligned_cursor:g[5]=s.flags.plus(data.previous_offset,15u,4);g[5]=s.flags.logic(g[5]&0xfffffff0u,4);break;
        case Phase::tail_result_base:g[0]=data.base;break;
        case Phase::tail_result:g[0]=s.flags.plus(g[0],g[5],4);break;
        case Phase::tail_insert_inputs:if(data.field==0u)g[1]=data.previous_count;else g[0]=data.base;break;
        case Phase::tail_insert_count:g[1]=s.flags.increment(g[1],4,false);break;
        case Phase::tail_insert_word:
            if(data.field==0u){g[1]=s.flags.shift(g[1],3u,4);g[0]=s.flags.minus(g[0],g[1],4);}break;
        case Phase::returning:g[0]=data.result;break;
        default:break;
        }
    };
    NativeTu15MainHeapAllocationServices services;services.probe_gap=[&](const auto &,auto,auto memory_class,auto &){
        s.push(0x232b46u,memory_class);s.push(0x232b47u,g[3]);g[1]=g[5];g[6]=g[4]+0x24u;
        at(0x232b4eu);s.push(0x232b4eu,0x232b53u);call(s,0x232d30u,0x22e24eu);
        return NativeTu15MainHeapGapResult{g[0],g[1],g[2],g[7],0,0,{}};
    };
    const auto result=native_tu15_allocate_main_heap_with_phases(s.memory,allocator,frame,services,phases,s.observe);
    if(result.residual.required){at(result.residual.instruction);s.required(0x22e30du,result.residual.owner,true);return true;}
    g[7]=s.pop();g[6]=s.pop();g[5]=s.pop();g[3]=s.pop();g[4]=s.flags.plus(g[4],0x14u,4);s.pop();g[4]+=12u;at(0x232c59u);return true;
}

template<class State,class Call> bool allocate_sized(State &s,Call call) {
    if(native_main_heap_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().sized_wrappers;auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    at(0x232d00u);g[2]=physics_step_detail::r(s.memory,g[4]+8u,4);
    at(0x232d04u);g[0]=physics_step_detail::r(s.memory,g[1],4);
    s.push(0x232d06u,g[2]);at(0x232d07u);g[2]=physics_step_detail::r(s.memory,g[4]+8u,4);
    s.push(0x232d0bu,g[2]);s.push(0x232d0cu,16u);
    at(0x232d0eu);const auto owner=physics_step_detail::r(s.memory,g[0]+8u,4);
    s.push(0x232d0eu,0x232d11u);call(s,owner,0x22e40eu);
    s.pop();g[4]+=8u;at(0x232d11u);return true;
}
} // namespace halo2::native_main_heap_allocation_detail
