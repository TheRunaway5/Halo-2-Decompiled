#pragma once
// Native TU1.5 root notice, entered at 1FA110 (physics live support unit):
// a record whose object is a player root passed on to 184EC0.
//
//   root_notice::notify   1FA110(EAX = record*, ECX = other*; three arguments) -> AL, RET C
//
// When the record's object (+0x10, NONE for none) is one of the player
// roots ([51DAF0]), 184EC0 (through the unit's dispatch) gets the record's
// +0xE8 and +0x58, the other's +0x24 and the three arguments, ECX the
// object's cluster (+0xB4), and its answer in AL is the result; otherwise
// the result is 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_root_notice.hpp) maps the role to the original instruction
// and owns the ABI of 184EC0.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_root_notice {
using native_object_update::ObjectData;
using native_object_update::PlayerRoots;
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t player_roots=0x51daf0u;     // PlayerRoots*
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace record {inline constexpr std::uint32_t object=0x10u,first=0xe8u,second=0x58u;}
inline constexpr std::uint32_t other_part=0x24u;

namespace root_notice {
enum class Access {object,roots,count,root,headers,header_datums,object_address,third,second_argument,first_argument,cluster};
enum class Outcome {no_object,no_roots,not_root,notified};
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: argument_slot(k) the k-th argument's slot; walking() before the
// roots; stepped() after each; pushed(third) and passing(...) the
// arguments pushed; notice(first part, cluster) 184EC0, returns AL.
template<class Memory,class Frame> Result notify(Memory &memory,Frame &frame,std::uint32_t record_address,std::uint32_t other) {
    using enum Access;
    const auto handle=memory.load32(object,at(record_address,record::object));
    if(handle==none)return {Outcome::no_object,0u};
    const auto roots_address=memory.load32(roots,globals::player_roots);
    const auto count_value=memory.load32(count,at(roots_address,offsetof(PlayerRoots,count)));
    if(signed_value(count_value)<=0)return {Outcome::no_roots,0u};
    frame.walking();
    for(std::uint32_t k=0;;) {
        if(memory.load32(root,at(roots_address,offsetof(PlayerRoots,handles)+4u*k))==handle)break;
        ++k;
        frame.stepped();
        if(signed_value(k)>=signed_value(count_value))return {Outcome::not_root,0u};
    }
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object_record=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto third_value=memory.load32(third,frame.argument_slot(2));
    const auto second_value=memory.load32(second_argument,frame.argument_slot(1));
    frame.pushed(third_value);
    const auto first_value=memory.load32(first_argument,frame.argument_slot(0));
    frame.passing(second_value,first_value,other+other_part,record_address+record::second);
    const auto cluster_handle=memory.load32(cluster,at(object_record,offsetof(ObjectData,cluster)));
    return {Outcome::notified,frame.notice(record_address+record::first,cluster_handle)};
}
} // namespace root_notice
} // namespace halo2::native_root_notice
