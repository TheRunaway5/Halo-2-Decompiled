#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
// Synchronous canonical-PC adapter used by the original TU1.5 composition.
// Its table and source services outlive the composed entry. Arbitrary callback
// wrappers retain full instruction delivery and cannot use this contract.
struct PhysicsInstructionRemapTu15 {
    struct Binding { std::uint32_t key,value; };
    const SourcePhysicsContactRuntimeServices *source{};
    std::span<const Binding> bindings;

    const Binding *find(std::uint32_t pc)const {
        const auto at=std::lower_bound(bindings.begin(),bindings.end(),pc,
            [](const auto &row,auto value){return row.key<value;});
        return at!=bindings.end() && at->key==pc?&*at:nullptr;
    }
    void operator()(std::uint32_t pc)const {
        if(!source->instruction)return;
        const auto *row=find(pc);
        if(!row)throw std::runtime_error("Unbound TU1.5 phantom/world source observation");
        source->instruction(row->value);
    }
    const SourcePhysicsContactRuntimeServices::InstructionCursor *live_cursor()const {
        if(!source || !source->instruction)return nullptr;
        const auto &cursor=source->instruction_cursor;
        if(!cursor.position || !cursor.callback_type ||
           source->instruction.target_type()!=*cursor.callback_type ||
           cursor.executor!=std::this_thread::get_id() || !cursor.literal_service_sites ||
           cursor.service_sites.size()!=3 || cursor.service_sites[0]!=0x38846eu ||
           cursor.service_sites[1]!=0x21f590u || cursor.service_sites[2]!=0x33b595u)
            return nullptr;
        return &cursor;
    }
    bool covers(std::span<const std::uint32_t> sites,std::uint32_t delta)const {
        for(const auto pc:sites) {
            const auto *row=find(pc);
            if(!row || row->value!=pc+delta)return false;
        }
        return true;
    }
};
} // namespace halo2
