#pragma once
// Native TU1.5 animation mask query, entered at 113C70 (physics live support
// unit): whether neither of an object's two animation slots has a bit set.
//
//   animation_mask::clear   113C70(EAX = object handle; bit) -> AL, RET 4
//
// The slots are the object's block at its signed offset +0x12A (tag +0,
// argument +4, its high word +6) and the slot +0x9C of its block at the
// signed offset +0x33E (tag +0x9C, argument +0xA0, its high word +0xA2). A
// slot whose high word is NONE has no entry; otherwise 1E3150 (EDX the
// tag's definition, the argument pushed) finds the entry, whose mask word
// +0x18 is tested for the bit. The result is 0 when either mask has it,
// otherwise 1; the first slot's answer waits in the register the original
// keeps across the second call, so the BL it returns counts.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_mask.hpp) maps the role to the original
// instruction and owns the ABI of 1E3150; the bit, the second block and the
// answer the original keeps in EBP, ESI and BL across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_mask {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
inline constexpr std::uint32_t first_block=0x12au,second_block=0x33eu,second_slot=0x9cu,entry_mask=0x18u;
namespace slot {inline constexpr std::uint32_t tag=0u,argument=4u,high=6u;}

namespace animation_mask {
enum class Access {headers,header_datums,object_address,first_offset,bit,second_offset,first_high,first_tag,first_tags,
    first_definition,first_argument,first_mask,second_high,second_tag,second_tags,second_argument,second_definition,second_mask};
struct Kept {std::uint32_t bit,second,answer;};   // EBP, ESI, BL
enum class Outcome {first_set,second_set,neither};
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: bit_slot() the argument's slot; entry(kept, definition, argument,
// k) 1E3150, refreshes kept, returns EAX; marked(kept) the first answer
// cleared.
template<class Memory,class Frame> Result clear(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto first=widen16(memory.load16(first_offset,at(object,first_block)))+object;
    Kept kept{memory.load32(bit,frame.bit_slot()),0u,1u};
    kept.second=widen16(memory.load16(second_offset,at(object,second_block)))+object;
    // The tested bit of an entry, when there is one.
    const auto has_bit=[&](std::uint32_t entry,Access mask_role) {
        return entry && ((memory.load16(mask_role,at(entry,entry_mask))&0xffffu)&(1u<<(kept.bit&31u)));
    };
    if((memory.load16(first_high,at(first,slot::high))&0xffffu)!=0xffffu) {
        const auto tag_handle=memory.load32(first_tag,at(first,slot::tag));
        const auto instances=memory.load32(first_tags,globals::tag_instances);
        const auto definition=memory.load32(first_definition,16u*(tag_handle&0xffffu)+instances+8u);
        const auto entry=frame.entry(kept,definition,memory.load32(first_argument,at(first,slot::argument)),0u);
        if(has_bit(entry,first_mask)){kept.answer=0u;frame.marked(kept);}
    }
    const auto second_entry=at(kept.second,second_slot);
    if((memory.load16(second_high,at(second_entry,slot::high))&0xffffu)==0xffffu)
        return {kept.answer?Outcome::neither:Outcome::first_set,kept.answer};
    const auto tag_handle=memory.load32(second_tag,at(second_entry,slot::tag));
    const auto instances=memory.load32(second_tags,globals::tag_instances);
    const auto argument=memory.load32(second_argument,at(second_entry,slot::argument));
    const auto definition=memory.load32(second_definition,16u*(tag_handle&0xffffu)+instances+8u);
    const auto entry=frame.entry(kept,definition,argument,1u);
    if(has_bit(entry,second_mask))return {Outcome::second_set,0u};
    return {kept.answer?Outcome::neither:Outcome::first_set,kept.answer};
}
} // namespace animation_mask
} // namespace halo2::native_animation_mask
