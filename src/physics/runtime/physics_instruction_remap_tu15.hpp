#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <algorithm>
#include <cstdlib>
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
        // Fall-through execution usually asks for the last row or the next
        // one. A hinted row is accepted only when it is exactly lower_bound's
        // result (equal key, previous key smaller), so the answer is the same.
        // HALO2_REFERENCE_PHANTOM_REMAP=1 always searches.
        static const bool reference=[]{const auto *p=std::getenv("HALO2_REFERENCE_PHANTOM_REMAP");return p && *p;}();
        thread_local const Binding *hint_table{};thread_local std::size_t hint{};
        if(!reference && hint_table==bindings.data())
            for(std::size_t candidate=hint;candidate<=hint+1 && candidate<bindings.size();++candidate)
                if(bindings[candidate].key==pc && (candidate==0 || bindings[candidate-1].key<pc)) {
                    hint=candidate;return &bindings[candidate];
                }
        const auto at=std::lower_bound(bindings.begin(),bindings.end(),pc,
            [](const auto &row,auto value){return row.key<value;});
        if(at==bindings.end() || at->key!=pc)return nullptr;
        if(!reference){hint_table=bindings.data();hint=std::size_t(at-bindings.begin());}
        return &*at;
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
