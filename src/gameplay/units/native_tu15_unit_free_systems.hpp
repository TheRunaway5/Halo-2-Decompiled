#pragma once
// Native TU1.5 unit free test, entered at CAAA0 (physics live support
// unit): whether a unit is free to act.
//
//   unit_free::free   CAAA0(EAX = unit handle) -> AL
//
// The result byte, kept in the slot the entry PUSH ECX makes, starts at 1.
// The unit is not free when it has a parent (+0x14), when bit 26 or 27 of
// its flag block (the signed offset +0x346, +4) is set, or when any of its
// flag bits 0x16, 8, 0x12, 0, 1, 0xA and 0xB is set (E9530, in that order).
// Nor is it free while either of its two slots (signed byte indices at
// +0x212, handles at +0x218) holds an object in an active state (105950,
// with EBX NONE: any entry). Otherwise the result is the byte.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_free.hpp) maps the role to the original instruction and
// owns the ABI of the children; the handle, the header table and the slot
// walk the original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_free {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace unit {inline constexpr std::uint32_t parent=0x14u,flag_block=0x346u,slot_indices=0x212u,slots=0x218u;}
inline constexpr std::uint32_t flag_block_word=4u,blocking_bits[]{26u,27u};
inline constexpr std::uint32_t tested_flags[]{0x16u,8u,0x12u,0u,1u,0xau,0xbu};
inline constexpr std::uint32_t slot_count=2u;

namespace unit_free {
enum class Access {headers,header_datums,unit_address,parent,result_init,block_offset,block_word,datums_again,
    unit_again,slot_index,slot,result_read};
enum class Outcome {parented,blocked,flagged,slot_active,free};
struct Kept {
    std::uint32_t handle;    // EDI
    std::uint32_t headers;   // EBX: the header table
    std::uint32_t header;    // ESI: the unit's header offset, then the unit
    std::uint32_t slot;      // EDI in the slot walk: the slot index's address
    std::uint32_t index;     // EBP in the slot walk
};
// Frame:
//   result_slot()            the result byte's address
//   flag(kept, bit)          E9530(EAX the handle, ECX the bit); refreshes kept; returns AL
//   slot_walk(kept)          PUSH EBP; XOR EBP,EBP; LEA EDI,[ESI+212]
//   active(kept, handle)     105950(EAX the handle, EBX NONE); refreshes kept; returns AL
//   stepped(kept)            INC EBP; INC EDI; CMP EBP,2
template<class Memory,class Frame> Outcome free(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    Kept kept{handle,memory.load32(headers,globals::object_headers),header_size*(handle&0xffffu),0u,0u};
    const auto datums=memory.load32(header_datums,at(kept.headers,datum_data));
    const auto address=memory.load32(unit_address,datums+kept.header+header_address);
    const auto parent_handle=memory.load32(parent,at(address,unit::parent));
    memory.store8(result_init,frame.result_slot(),1u);
    if(parent_handle!=none)return Outcome::parented;
    for(const auto bit:blocking_bits) {
        const auto block=widen16(memory.load16(block_offset,at(address,unit::flag_block)));
        if((memory.load32(block_word,block+address+flag_block_word)>>bit)&1u)return Outcome::blocked;
    }
    for(const auto bit:tested_flags)if(frame.flag(kept,bit))return Outcome::flagged;
    const auto datums_now=memory.load32(datums_again,at(kept.headers,datum_data));
    kept.header=memory.load32(unit_again,datums_now+kept.header+header_address);
    frame.slot_walk(kept);
    kept.index=0;kept.slot=kept.header+unit::slot_indices;
    do {
        const auto slot_value=std::uint32_t(std::int32_t(std::int8_t(memory.load8(slot_index,kept.slot))))&0xffffu;
        if(slot_value!=0xffffu) {
            const auto slot_handle=memory.load32(slot,kept.header+unit::slots+4u*widen16(slot_value));
            if(slot_handle!=none && frame.active(kept,slot_handle))return Outcome::slot_active;
        }
        ++kept.index;++kept.slot;
        frame.stepped(kept);
    } while(std::int32_t(kept.index)<std::int32_t(slot_count));
    (void)memory.load8(result_read,frame.result_slot());
    return Outcome::free;
}
} // namespace unit_free
} // namespace halo2::native_unit_free
