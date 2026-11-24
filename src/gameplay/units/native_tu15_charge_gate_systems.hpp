#pragma once
// Native TU1.5 charge gate, entered at 105FD0 (physics live support unit):
// whether an object's charge and its first entry let it act.
//
//   charge_gate::open   105FD0(EAX = object handle) -> AL
//
// An object whose charge (the float +0x184) is at least 1 (COMISS, ordered)
// is closed. Otherwise it is open unless its definition (its tag, +0,
// through the tag instances) lists entries (+0x2C0 the count, positive;
// +0x2C4 the 0x5C-byte entries) whose first has a positive word +0xA while
// the object's words +0x22C and +0x22A are both zero.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_charge_gate.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_charge_gate {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t one=0x46a0d0u;              // float 1.0
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
inline constexpr std::uint32_t charge=0x184u,count=0x2c0u,entries=0x2c4u,first_entry_word=0xau,second_word=0x22cu,first_word=0x22au;

namespace charge_gate {
enum class Access {headers,header_datums,object_address,tag,tags,charge,one,definition,count,entries,entry_word,second,first};
enum class Outcome {charged,no_entries,empty_entry,second_set,first_set,closed};
struct Result {Outcome outcome;bool open;};
template<class Memory> Result open(Memory &memory,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(tag,object);
    const auto instances=memory.load32(tags,globals::tag_instances);
    const auto value=memory.load32(Access::charge,at(object,native_charge_gate::charge));
    const bool below=sse::below(value,memory.load32(one,globals::one));
    const auto owner=memory.load32(definition,16u*(tag_handle&0xffffu)+instances+8u);
    if(!below)return {Outcome::charged,false};
    if(signed_value(memory.load32(Access::count,at(owner,native_charge_gate::count)))<=0)return {Outcome::no_entries,true};
    const auto list=memory.load32(Access::entries,at(owner,native_charge_gate::entries));
    if(std::int16_t(memory.load16(Access::entry_word,at(list,first_entry_word)))<=0)return {Outcome::empty_entry,true};
    if(memory.load16(second,at(object,second_word))&0xffffu)return {Outcome::second_set,true};
    if(memory.load16(first,at(object,first_word))&0xffffu)return {Outcome::first_set,true};
    return {Outcome::closed,false};
}
} // namespace charge_gate
} // namespace halo2::native_charge_gate
