#pragma once
// Native TU1.5 animation record, entered at 1E5DF0 (physics live support and
// AI frame units): a 12-byte record of an animation reference filled in.
//
//   animation_record::fill   1E5DF0(EDI = record*; definition, reference) -> RET 8
//
// Unless the reference's high word is NONE, the entry comes from 1E3150 (EDX
// the definition, the reference pushed) and the value from 1E5A70 (ECX the
// definition, the reference pushed); the record takes the value (+0), the
// entry's +0x3C address (+4), its bytes +0x13 (+8) and +0x11 (+9) and its
// word +0x14 (+0xA).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_record.hpp) maps the role to the original
// instruction and owns the ABI of the children; the definition, the
// reference, the entry and the record the original keeps in EBP, EBX, ESI
// and EDI across them are a Kept, refreshed from the registers each
// returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_record {
using native_object_update::at;
using native_object_update::widen16;
using native_object_update::widen8;
namespace entry {inline constexpr std::uint32_t kind=0x11u,group=0x13u,word=0x14u,data=0x3cu;}
namespace record {inline constexpr std::uint32_t value=0u,data=4u,group=8u,kind=9u,word=0xau;}

namespace animation_record {
enum class Access {high,definition,reference,group,word,kind,data_store,group_store,value_store,kind_store,word_store};
struct Kept {std::uint32_t definition,reference,entry,record;};   // EBP, EBX, ESI, EDI
// Frame: definition_slot(), reference_slot(); entry(kept) 1E3150 and
// value(kept) 1E5A70, each refreshing kept and returning EAX.
template<class Memory,class Frame> bool fill(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum Access;
    const auto high_word=memory.load32(Access::high,frame.reference_slot())>>16;
    Kept kept{memory.load32(Access::definition,frame.definition_slot()),0u,0u,out};
    if((high_word&0xffffu)==0xffffu)return false;
    kept.reference=memory.load32(Access::reference,frame.reference_slot());
    kept.entry=frame.entry(kept);
    const auto value=frame.value(kept);
    const auto group_byte=memory.load8(Access::group,at(kept.entry,entry::group))&0xffu;
    const auto word_value=memory.load16(Access::word,at(kept.entry,entry::word))&0xffffu;
    const auto kind_byte=memory.load8(Access::kind,at(kept.entry,entry::kind))&0xffu;
    memory.store32(data_store,at(kept.record,record::data),kept.entry+entry::data);
    memory.store8(group_store,at(kept.record,record::group),group_byte);
    memory.store32(value_store,at(kept.record,record::value),value);
    memory.store8(kind_store,at(kept.record,record::kind),kind_byte);
    memory.store16(word_store,at(kept.record,record::word),word_value);
    return true;
}
} // namespace animation_record
} // namespace halo2::native_animation_record
