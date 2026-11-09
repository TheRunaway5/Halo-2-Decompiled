#pragma once
#include "halo2/native_tu15_raw_allocator.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_raw_allocator_detail {
struct Activity {std::uint64_t allocations{},releases{},tagged_allocations{},tagged_releases{},
    allocate_owners{},release_owners{},default_heaps{},fixed_tail_entries{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_RAW_ALLOCATOR");return p&&*p;}();return enabled;}
template<class State> class Storage final:public NativeTu15ArrayAllocatorStorage {
    State &s_;NativeTu15RawAllocatorMemory memory_;bool releasing_;
    mutable NativeTu15ArrayHeap kind_{};
    std::uint32_t read(std::uint32_t p)const{return physics_step_detail::r(s_.memory,p,4);}
public:
    bool default_path{};
    Storage(State &s,bool releasing):s_(s),memory_(s.memory),releasing_(releasing){}
    NativeTu15ArrayHeap kind()const{return kind_;}
    std::uint32_t mode()const override{const auto value=memory_.mode();s_.gp[0]=s_.flags.increment(value,4,true);return value;}
    std::uint32_t heap(NativeTu15ArrayHeap kind)const override {
        kind_=kind;const auto value=memory_.heap(kind);auto &g=s_.gp;g[1]=value;
        if(releasing_&&kind!=NativeTu15ArrayHeap::secondary) {
            g[0]=value+(kind==NativeTu15ArrayHeap::primary?0x4340u:0x30u);s_.flags.minus(g[0],g[6],4);
            if(std::bit_cast<std::int32_t>(g[0])<=std::bit_cast<std::int32_t>(g[6])){g[0]=value+(kind==NativeTu15ArrayHeap::primary?0xf0340u:0x15030u);s_.flags.minus(g[0],g[6],4);}
        }
        return value;
    }
    std::uint32_t owner(std::uint32_t receiver,bool releasing)const override {
        auto &g=s_.gp;
        if(releasing) {
            const auto slot=default_path||kind_==NativeTu15ArrayHeap::fixed?2u:0u;
            g[slot]=read(receiver);if(default_path)g[1]=receiver;
            s_.push(default_path?0x149cb1u:kind_==NativeTu15ArrayHeap::fixed?0x149c8au:0x149c67u,g[6]);
            return read(g[slot]+4u);
        }
        if(kind_==NativeTu15ArrayHeap::fixed&&!default_path){g[0]=read(receiver);return read(g[0]);}
        g[2]=read(receiver);
        s_.push(default_path?0x149c29u:0x149c1au,g[6]);s_.push(default_path?0x149c2au:0x149c1bu,g[7]);
        if(default_path)g[1]=receiver;
        return read(g[2]);
    }
    std::uint32_t owner_with_inputs(std::uint32_t receiver,bool,const NativeTu15ArrayOwnerInputs &inputs)const override {
        if(inputs.before_table)inputs.before_table();auto &g=s_.gp;g[0]=read(receiver);
        if(inputs.after_table)inputs.after_table();s_.push(0x149c0au,g[6]);s_.push(0x149c0bu,g[7]);return read(g[0]);
    }
    std::uint32_t secondary_start(std::uint32_t receiver)const override {
        const auto value=memory_.secondary_start(receiver);s_.gp[0]=value;s_.flags.minus(value,s_.gp[6],4);return value;
    }
    std::uint32_t secondary_size(std::uint32_t receiver)const override {
        const auto value=memory_.secondary_size(receiver);s_.gp[2]=value;s_.gp[0]=s_.flags.plus(s_.gp[0],value,4);s_.flags.minus(s_.gp[0],s_.gp[6],4);return value;
    }
};
template<class State,class Call> bool allocate(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().allocations;auto &g=s.gp;Storage<State> storage(s,false);auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    NativeTu15ArrayAllocatorArguments arguments;
    arguments.memory_class=[&]{s.push(0x149c00u,g[6]);return g[6]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    arguments.bytes=[&]{s.push(0x149c05u,g[7]);return g[7]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    NativeTu15RawAllocatorServices services;unsigned number{};bool tail=false;
    services.allocate_owner=[&](const auto &request,auto &context){
        const auto which=number++;const auto pc=which==0u?0x149c0cu:which==1u?0x149c1cu:0x149c2du;
        const auto next=which==0u?0x149c0eu:which==1u?0x149c1eu:0x149c2fu;
        ++activity().allocate_owners;activity().last_owner=request.owner;at(pc);s.push(pc,next);call(s,request.owner,pc,false);
        context=native_partition_temporary_detail::floating_point(s);if(which<2u)s.flags.logic(g[0],4);return g[0];
    };
    services.allocate_tail_owner=[&](auto,auto owner,auto &context){
        tail=true;++activity().fixed_tail_entries;activity().last_owner=owner;at(0x149c3cu);call(s,owner,0x149c3cu,true);
        context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    services.default_heap=[&](auto &context){
        storage.default_path=true;++activity().default_heaps;activity().last_owner=0x149930u;at(0x149c22u);s.push(0x149c22u,0x149c27u);
        call(s,0x149930u,0x149c22u,false);context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    native_tu15_route_raw_allocation_delayed(storage,arguments,fp,services);
    if(!tail){g[7]=s.pop();g[6]=s.pop();s.pop();g[4]+=8u;at(0x149c31u);}return true;
}
template<class State,class Call> bool release(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().releases;auto &g=s.gp;Storage<State> storage(s,true);auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    s.push(0x149c40u,g[6]);g[6]=physics_step_detail::r(s.memory,g[4]+8u,4);s.flags.logic(g[6],4);
    std::uint32_t final=0x149cb6u;NativeTu15RawAllocatorServices services;
    services.release_owner=[&](const auto &request,auto &context){
        const auto fixed=storage.kind()==NativeTu15ArrayHeap::fixed;const auto pc=storage.default_path?0x149cb2u:fixed?0x149c8bu:0x149c68u;
        final=storage.default_path?0x149cb6u:fixed?0x149c8fu:0x149c6cu;
        ++activity().release_owners;activity().last_owner=request.owner;at(pc);s.push(pc,final-1u);call(s,request.owner,pc,false);
        context=native_partition_temporary_detail::floating_point(s);
    };
    services.default_heap=[&](auto &context){
        storage.default_path=true;++activity().default_heaps;activity().last_owner=0x149930u;at(0x149ca8u);s.push(0x149ca8u,0x149cadu);
        call(s,0x149930u,0x149ca8u,false);context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    native_tu15_route_raw_release(storage,g[6],fp,services);
    g[6]=s.pop();s.pop();g[4]+=4u;at(final);return true;
}
template<class State,class Call> bool allocate_tagged(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().tagged_allocations;auto &g=s.gp;const auto allocator=g[1];auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    bool nonnull{};NativeTu15RawTaggedPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case NativeTu15RawTaggedPhase::allocation_table:g[0]=data.table;break;
        case NativeTu15RawTaggedPhase::allocation_owner_inputs:s.push(0x23316cu,g[6]);g[2]=g[7]+16u;s.push(0x233170u,g[2]);break;
        case NativeTu15RawTaggedPhase::allocation_result:nonnull=data.block!=0u;s.flags.logic(g[0],4);break;
        case NativeTu15RawTaggedPhase::allocation_magic:g[7]=s.pop();break;
        default:break;
        }
    };
    NativeTu15ArrayAllocatorArguments arguments;
    arguments.memory_class=[&]{s.push(0x233162u,g[6]);return g[6]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    arguments.bytes=[&]{s.push(0x233167u,g[7]);return g[7]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    NativeTu15ArrayGrowthServices services;services.allocate=[&](const auto &request,auto &context){
        ++activity().allocate_owners;activity().last_owner=request.owner;at(0x233171u);s.push(0x233171u,0x233174u);call(s,request.owner,0x233171u,false);
        context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    native_tu15_allocate_tagged_raw_block_with_phases(s.memory,allocator,arguments,fp,services,phases,s.observe);
    if(nonnull){g[0]=s.flags.plus(g[0],16u,4);g[6]=s.pop();s.pop();g[4]+=8u;at(0x233189u);}
    else{g[7]=s.pop();g[0]=s.flags.logic(0,4);g[6]=s.pop();s.pop();g[4]+=8u;at(0x233190u);}return true;
}
template<class State,class Call> bool release_tagged(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().tagged_releases;auto &g=s.gp;const auto allocator=g[1];auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    NativeTu15RawTaggedPhases phases;phases.observe=[&](auto phase,const auto &data){
        switch(phase) {
        case NativeTu15RawTaggedPhase::release_class_inputs:s.push(0x2331a4u,g[6]);break;
        case NativeTu15RawTaggedPhase::release_class:g[6]=data.memory_class;break;
        case NativeTu15RawTaggedPhase::release_size_inputs:g[0]=s.flags.plus(g[0],0xfffffff0u,4);s.push(0x2331abu,g[6]);break;
        case NativeTu15RawTaggedPhase::release_bytes:g[6]=s.flags.plus(data.size_word,16u,4);s.push(0x2331b2u,g[6]);break;
        case NativeTu15RawTaggedPhase::release_table:g[2]=data.table;s.push(0x2331bbu,g[0]);break;
        default:break;
        }
    };
    NativeTu15ArrayGrowthServices services;services.release=[&](const auto &request,auto &context){
        ++activity().release_owners;activity().last_owner=request.owner;at(0x2331bcu);s.push(0x2331bcu,0x2331bfu);call(s,request.owner,0x2331bcu,false);
        context=native_partition_temporary_detail::floating_point(s);
    };
    native_tu15_release_tagged_raw_block_with_phases(s.memory,allocator,[&]{at(0x2331a0u);return g[0]=physics_step_detail::r(s.memory,g[4]+4u,4);},fp,services,phases,s.observe);
    g[6]=s.pop();s.pop();g[4]+=4u;at(0x2331c0u);return true;
}
} // namespace halo2::native_raw_allocator_detail
