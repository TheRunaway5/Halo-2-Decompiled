#pragma once
#include "halo2/native_tu15_physical_allocation.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_physical_allocation_detail {
struct Activity {
    NativeTu15PhysicalAllocationStats kernel;
    std::map<std::uint32_t,std::uint64_t> children;
};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_PHYSICAL_ALLOCATION");return p&&*p;}();return enabled;}
template<class State,class Call> bool run(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using Phase=NativeTu15PhysicalAllocationPhase;auto &g=s.gp;
    auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    struct Frame final:NativeTu15PhysicalAllocationFrame {
        State &s;std::uint32_t entry_sp;
        explicit Frame(State &state):s(state),entry_sp(state.gp[4]){}
        std::uint32_t requested_bytes()const override {
            const auto value=physics_step_detail::r(s.memory,entry_sp+4u,4);s.gp[5]=value;return value;
        }
        std::uint32_t attribute(unsigned index)const override {
            const auto value=physics_step_detail::r(s.memory,entry_sp+8u+index*4u,4);
            constexpr std::array<unsigned,3> registers{0,1,2};s.gp[registers.at(index)]=value;return value;
        }
        void record_slot(std::uint32_t value,std::uint32_t pc)override {
            // The source reuses its original requested-bytes argument, not
            // a new local. Header/caller aliases can affect the later load.
            physics_step_detail::w(s.memory,s.observe,pc,entry_sp+4u,4,value);
        }
        std::uint32_t record_slot()const override {
            const auto value=physics_step_detail::r(s.memory,entry_sp+4u,4);s.gp[0]=value;return value;
        }
    }frame(s);
    bool allocated{};
    NativeTu15PhysicalAllocationPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case Phase::partition_binding:g[1]=data.pool;s.push(0x12fe06u,g[3]);s.push(0x12fe07u,g[5]);break;
        case Phase::requested:
            s.push(0x12fe0cu,g[6]);s.push(0x12fe0du,1u);g[0]=g[5]+0x2023u;
            s.push(0x12fe15u,g[0]);s.push(0x12fe16u,g[1]);g[6]=s.flags.logic(0,4);break;
        case Phase::handle:g[3]=g[0];s.flags.minus(g[3],0xffffffffu,4);allocated=data.handle!=0xffffffffu;break;
        case Phase::partition:g[1]=data.pool;break;
        case Phase::page_table:g[2]=data.table;break;
        case Phase::page_records:g[2]=data.records;break;
        case Phase::page_shift:g[1]=data.shift;g[0]=s.flags.logic(g[0]&0xffffu,4)*3u;break;
        case Phase::page_offset:g[0]=data.offset;break;
        case Phase::descriptor_binding:g[2]=data.array;break;
        case Phase::shifted_offset:g[0]=s.flags.shift(g[0],g[1],4);break;
        case Phase::base:g[1]=data.base;break;
        case Phase::address:
            s.push(0x12fe52u,g[7]);g[7]=s.flags.logic(data.address,4);g[0]=g[3];g[6]=data.header;break;
        case Phase::record_array:g[1]=data.records;g[0]=s.flags.logic(g[0]&0xffffu,4)*5u;break;
        case Phase::record:g[2]=data.record;break;
        case Phase::encoded_address:g[0]=s.flags.logic(g[7]^0x02281972u,4);break;
        case Phase::previous:g[0]=data.previous;s.flags.logic(g[0],4);break;
        case Phase::link_inputs:if(data.field==0u)g[1]=data.protection_bytes;else g[2]=data.protection_address;break;
        case Phase::publish_list:s.push(0x12febfu,4u);s.push(0x12fec1u,g[1]);s.push(0x12fec2u,g[2]);break;
        case Phase::clear_record:g[1]=s.flags.logic(0,4);g[0]=s.flags.plus(g[0],2u,4);g[2]=g[0];break;
        case Phase::active_byte:s.flags.logic(data.value,1);break;
        case Phase::returning:g[0]=allocated?g[7]:g[6];break;
        default:break;
        }
    };
    const auto child=[&](std::uint32_t pc,std::uint32_t next,std::uint32_t owner,auto &context){
        ++activity().children[owner];at(pc);s.push(pc,next);call(s,owner,pc,false);
        context=native_partition_temporary_detail::floating_point(s);
    };
    NativeTu15PhysicalAllocationServices services;
    services.pages=[&](auto,auto,auto,auto &context){child(0x12fe19u,0x12fe1eu,0x13fe50u,context);return g[0];};
    services.allocate_specific=[&](auto,auto,auto &context){child(0x12fe65u,0x12fe6au,0x16e770u,context);return g[0];};
    services.protect=[&](auto,auto,auto,auto &context){child(0x12fec9u,0x12feceu,0x2db4cau,context);};
    at(0x12fe00u);native_tu15_allocate_physical_block(s.memory,frame,fp,services,phases,s.observe,&activity().kernel);
    if(allocated)g[7]=s.pop();g[6]=s.pop();g[5]=s.pop();g[3]=s.pop();s.pop();g[4]+=16u;
    at(allocated?0x12ff00u:0x12ff08u);return true;
}
} // namespace halo2::native_physical_allocation_detail
