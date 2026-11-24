#pragma once
// Native TU1.5 child unit search, entered at CB440 (physics live support
// unit): the first unit among an object's children with a given word.
//
//   child_unit::find   CB440(EAX = object handle; word) -> EAX, RET 4
//
// From the object's first child (+0x10) along the siblings (+0xC): the
// first biped or vehicle (1 << type +0xAA within the low two bits) whose
// word +0x1FC equals the argument's low word; NONE when there is none.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_child_unit.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_child_unit {
using native_object_update::ObjectData;
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,unit_word=0x1fcu;
inline constexpr std::uint32_t unit_types=(1u<<0)|(1u<<1);   // biped, vehicle

namespace child_unit {
enum class Access {headers,word,header_datums,object_address,first_child,child_address,type,child_word,sibling};
enum class Outcome {childless,found,none_matched};
struct Result {Outcome outcome;std::uint32_t handle;};
// Frame: word_slot() the argument's slot; walking() before the first child.
template<class Memory,class Frame> Result find(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto wanted=memory.load16(Access::word,frame.word_slot())&0xffffu;
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    auto child=memory.load32(first_child,at(object,offsetof(ObjectData,first_child)));
    if(child==none)return {Outcome::childless,child};
    frame.walking();
    for(;;) {
        const auto record=memory.load32(child_address,datums+header_size*(child&0xffffu)+header_address);
        const auto kind=memory.load8(type,at(record,offsetof(ObjectData,type)))&0xffu;
        if(((1u<<(kind&31u))&unit_types) && (memory.load16(child_word,at(record,unit_word))&0xffffu)==wanted)
            return {Outcome::found,child};
        child=memory.load32(sibling,at(record,offsetof(ObjectData,next_sibling)));
        if(child==none)return {Outcome::none_matched,child};
    }
}
} // namespace child_unit
} // namespace halo2::native_child_unit
