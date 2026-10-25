#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_damage_record_native_tu15.hpp"
#include <atomic>
#include <bit>
#include <optional>

namespace halo2::physics_world_runtime_detail {
inline std::atomic<std::uint64_t> damage_record_native_calls{},damage_record_native_admitted{},damage_record_native_rejected{};
struct IndexedDamageRecordTu15 {
    std::uint32_t record,flag;
};
// Original 105460 compares the descriptor count and selector as signed words.
// Preserve its literal layout and wraparound, without inferring a field meaning
// or introducing a new handle/index admission rule.
inline std::optional<IndexedDamageRecordTu15> indexed_damage_record_native_tu15(
    std::uint32_t body,std::uint32_t count,std::uint32_t selector) {
    if(std::bit_cast<std::int32_t>(count)<=std::bit_cast<std::int32_t>(selector))return {};
    const auto offset=selector*0x34u;
    return IndexedDamageRecordTu15{body+offset+0x1a4u,body+offset+0x1a8u};
}
// Whole structured native owner. The adapter retains source ABI, stack stores,
// integer flags and fault order because the enclosing original caller observes
// them. The typed selection above supplies the branch and record addresses.
template<class State>
void damage_record_native_tu15(State &s) {
    damage_record_native_calls.fetch_add(1,std::memory_order_relaxed);
    const auto site=[&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);};
    site(0x105460u);s.gp[2]=r(s.memory,0x4ecbd0u,4);
    site(0x105466u);s.gp[2]=r(s.memory,s.gp[2]+0x44u,4);
    site(0x105469u);s.gp[0]=s.flags.logic(s.gp[0]&0xffffu,4);
    site(0x10546eu);s.push(0x10546eu,s.gp[6]);
    site(0x10546fu);s.gp[0]=s.gp[0]+s.gp[0]*2u;
    site(0x105472u);s.gp[2]=r(s.memory,s.gp[2]+s.gp[0]*4u+8u,4);
    site(0x105476u);s.gp[6]=r(s.memory,s.gp[2],4);
    site(0x105478u);s.gp[6]=s.flags.logic(s.gp[6]&0xffffu,4);
    site(0x10547eu);s.push(0x10547eu,s.gp[7]);
    site(0x10547fu);s.gp[7]=r(s.memory,0x4f0414u,4);
    site(0x105485u);s.gp[6]=s.flags.shift(s.gp[6],4u,4);
    site(0x105488u);s.gp[6]=r(s.memory,s.gp[6]+s.gp[7]+8u,4);
    site(0x10548cu);s.gp[7]=r(s.memory,s.gp[6]+0x2d0u,4);
    site(0x105492u);s.set(0,s.flags.logic(0u,1),1,0);
    site(0x105494u);s.flags.minus(s.gp[7],s.gp[1],4);
    const auto selected=indexed_damage_record_native_tu15(s.gp[2],s.gp[7],s.gp[1]);
    site(0x105496u);s.gp[7]=s.pop();
    site(0x105497u);s.gp[6]=s.pop();
    site(0x105498u);
    if(selected) {
        damage_record_native_admitted.fetch_add(1,std::memory_order_relaxed);
        site(0x10549au);s.gp[1]=s.flags.product(s.gp[1],0x34u);
        site(0x10549du);w(s.memory,s.observe,0x10549du,selected->flag,1,
            s.flags.logic(r(s.memory,selected->flag,1)|4u,1));
        site(0x1054a5u);s.gp[0]=selected->record;
        site(0x1054acu);s.set(0,1u,1,0);
    } else damage_record_native_rejected.fetch_add(1,std::memory_order_relaxed);
    site(0x1054aeu);s.pop();
}
}
