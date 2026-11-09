#pragma once
#include "halo2/native_tu15_array_allocator.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_array_allocator_detail {
struct Activity {std::uint64_t allocations{},releases{},allocate_owners{},release_owners{},default_heaps{},fixed_tail_entries{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_ARRAY_ALLOCATOR");return p&&*p;}();return enabled;}
template<class State> class Storage final:public NativeTu15ArrayAllocatorStorage {
    State &s_;NativeTu15ArrayAllocatorMemory memory_;bool releasing_;
    mutable unsigned allocation_owner_{};
    std::uint32_t read(std::uint32_t p)const{return physics_step_detail::r(s_.memory,p,4);}
public:
    bool default_path{};
    Storage(State &s,bool releasing):s_(s),memory_(s.memory),releasing_(releasing){}
    std::uint32_t mode()const override{const auto value=memory_.mode();s_.gp[0]=s_.flags.increment(value,4,true);return value;}
    std::uint32_t heap(NativeTu15ArrayHeap kind)const override {
        const auto value=memory_.heap(kind);auto &g=s_.gp;g[1]=value;
        if(releasing_&&kind!=NativeTu15ArrayHeap::secondary) {
            g[0]=value+(kind==NativeTu15ArrayHeap::primary?0x4340u:0x30u);s_.flags.minus(g[0],g[6],4);
            if(std::bit_cast<std::int32_t>(g[0])<=std::bit_cast<std::int32_t>(g[6])){g[0]=value+(kind==NativeTu15ArrayHeap::primary?0xf0340u:0x15030u);s_.flags.minus(g[0],g[6],4);}
        }
        return value;
    }
    std::uint32_t owner(std::uint32_t receiver,bool releasing)const override {
        s_.gp[0]=read(receiver);return read(s_.gp[0]+(releasing?20u:16u));
    }
    std::uint32_t owner_with_inputs(std::uint32_t receiver,bool releasing,const NativeTu15ArrayOwnerInputs &inputs)const override {
        auto &g=s_.gp;if(inputs.before_table)inputs.before_table();
        const auto which=releasing?(default_path?2u:0u):(allocation_owner_++?2u:0u);
        g[which]=read(receiver);if(inputs.after_table)inputs.after_table();
        if(!releasing) {
            const auto number=allocation_owner_-1u;
            s_.push(number==0u?0x149dbau:number==1u?0x149dcbu:0x149ddbu,g[6]);
            s_.push(number==0u?0x149dbbu:number==1u?0x149dccu:0x149ddcu,g[7]);
        }
        if(default_path)g[1]=receiver;
        return read(g[which]+(releasing?20u:16u));
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
    arguments.memory_class=[&]{s.push(0x149db0u,g[6]);return g[6]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    arguments.bytes=[&]{s.push(0x149db5u,g[7]);return g[7]=physics_step_detail::r(s.memory,g[4]+12u,4);};
    NativeTu15ArrayAllocatorServices services;unsigned owner_count{};bool tail=false;
    services.allocate_owner=[&](const auto &request,auto &context) {
        const auto number=owner_count++;const auto pc=number==0u?0x149dbcu:number==1u?0x149dcdu:0x149ddfu;
        const auto next=number==0u?0x149dbfu:number==1u?0x149dd0u:0x149de2u;
        const auto site=number==0u?0x148a2cu:number==1u?0x148a3du:0x148a4fu;
        ++activity().allocate_owners;activity().last_owner=request.owner;at(pc);s.push(pc,next);call(s,request.owner,site,false);
        context=native_partition_temporary_detail::floating_point(s);if(number<2u)s.flags.logic(g[0],4);return g[0];
    };
    services.allocate_tail_owner=[&](auto,auto owner,auto &context) {
        tail=true;++activity().fixed_tail_entries;activity().last_owner=owner;at(0x149defu);call(s,owner,0x148a5fu,true);
        context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    services.default_heap=[&](auto &context) {
        storage.default_path=true;++activity().default_heaps;activity().last_owner=0x149930u;
        at(0x149dd4u);s.push(0x149dd4u,0x149dd9u);call(s,0x149930u,0x148a44u,false);context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    native_tu15_route_array_allocation_delayed(storage,arguments,fp,services);
    if(!tail){g[7]=s.pop();g[6]=s.pop();s.pop();g[4]+=8u;at(0x149de4u);}return true;
}
template<class State,class Call> bool release(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().releases;auto &g=s.gp;Storage<State> storage(s,true);auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    s.push(0x149e00u,g[6]);g[6]=physics_step_detail::r(s.memory,g[4]+8u,4);s.flags.logic(g[6],4);
    NativeTu15ArrayAllocatorArguments arguments;
    arguments.memory_class=[&]{const auto value=physics_step_detail::r(s.memory,g[4]+16u,4);g[storage.default_path?1:2]=value;return value;};
    arguments.bytes=[&]{const auto slot=storage.default_path?1u:2u;s.push(storage.default_path?0x149e71u:0x149e2bu,g[slot]);
        const auto value=physics_step_detail::r(s.memory,g[4]+16u,4);g[slot]=value;s.push(storage.default_path?0x149e76u:0x149e30u,g[slot]);s.push(storage.default_path?0x149e77u:0x149e31u,g[6]);return value;};
    NativeTu15ArrayAllocatorServices services;bool called=false;
    services.release_owner=[&](const auto &request,auto &context) {
        called=true;++activity().release_owners;activity().last_owner=request.owner;const auto pc=storage.default_path?0x149e7au:0x149e32u;
        at(pc);s.push(pc,storage.default_path?0x149e7du:0x149e35u);call(s,request.owner,storage.default_path?0x148aeau:0x148aa2u,false);
        context=native_partition_temporary_detail::floating_point(s);
    };
    services.default_heap=[&](auto &context) {
        storage.default_path=true;++activity().default_heaps;activity().last_owner=0x149930u;at(0x149e66u);s.push(0x149e66u,0x149e6bu);
        call(s,0x149930u,0x148ad6u,false);context=native_partition_temporary_detail::floating_point(s);return g[0];
    };
    native_tu15_route_array_release_delayed(storage,g[6],arguments,fp,services);
    g[6]=s.pop();s.pop();g[4]+=12u;at(called&&!storage.default_path?0x149e36u:0x149e7eu);return true;
}
} // namespace halo2::native_array_allocator_detail
