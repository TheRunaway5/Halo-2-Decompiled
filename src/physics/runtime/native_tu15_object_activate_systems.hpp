#pragma once
// Native TU1.5 object activation, entered at B92C0 (physics live support and
// AI frame units): an inactive object marked active.
//
//   object_activate::activate   B92C0(EDI = object handle)
//
// An object whose header is already active (flags +2 bit 0), whose header
// word +4 is NONE, or which has a parent (+0x14), is left alone. Otherwise
// the header is marked active; an object of type 0, 1, 7, 11 or 12 (the
// header's type +3) is handed to 1CC3F0 (ECX the handle); and an object
// with bit 14 of its attachment flags (+4) counts one more in the word +4 of
// the record [4EABC4].
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_activate.hpp) maps the role to the original
// instruction and owns the ABI of 1CC3F0; the object the original keeps in
// ESI across it is a Kept, refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_activate {
using native_object_update::ObjectData;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t counters=0x4eabc4u;         // the record whose word +4 counts the bit-14 objects
}
inline constexpr std::uint32_t datum_data=0x44u,header_word=4u,header_type=3u,counter=4u;
inline constexpr std::uint32_t active=1u<<0,notified_types=0x1883u,counted_bit=14u;

namespace object_activate {
enum class Access {headers,header_datums,header_flags,word,object_address,parent,header_store,type,attachment,
    counter_record,count,count_store};
enum class Outcome {already_active,no_word,parented,activated,notified,counted};
struct Kept {std::uint32_t object;};   // ESI
struct Result {Outcome outcome;bool notified;};
// Frame: notify(kept, handle) 1CC3F0; refreshes kept.
template<class Memory,class Frame> Result activate(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto header=memory.load32(header_datums,at(table,datum_data))+std::uint32_t(sizeof(ObjectHeader))*(handle&0xffffu);
    const auto flags=memory.load8(header_flags,at(header,offsetof(ObjectHeader,flags)))&0xffu;
    if(flags&active)return {Outcome::already_active,false};
    if((memory.load16(word,at(header,header_word))&0xffffu)==0xffffu)return {Outcome::no_word,false};
    Kept kept{memory.load32(object_address,at(header,offsetof(ObjectHeader,object)))};
    if(memory.load32(parent,at(kept.object,offsetof(ObjectData,parent)))!=none)return {Outcome::parented,false};
    memory.store8(header_store,at(header,offsetof(ObjectHeader,flags)),flags|active);
    const auto kind=memory.load8(type,at(header,header_type))&0xffu;
    const bool notify=((1u<<(kind&31u))&notified_types)!=0;
    if(notify)frame.notify(kept,handle);
    if(!((memory.load32(attachment,at(kept.object,offsetof(ObjectData,attachment_flags)))>>counted_bit)&1u))
        return {notify?Outcome::notified:Outcome::activated,notify};
    const auto record=memory.load32(counter_record,globals::counters);
    memory.store16(count_store,at(record,counter),memory.load16(count,at(record,counter))+1u);
    return {Outcome::counted,notify};
}
} // namespace object_activate
} // namespace halo2::native_object_activate
