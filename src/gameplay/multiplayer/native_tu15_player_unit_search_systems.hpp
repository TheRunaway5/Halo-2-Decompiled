#pragma once
// Native TU1.5 player unit search, entered at 153360 (multiplayer engines
// unit): the units near a player's unit, each handed to 1538B0.
//
//   player_unit_search::run   153360(player) -> RET 4
//
// A player with a unit (+0x2C of the 0x21C-byte players at [4F54F4]) that
// rides in nothing (its parent +0x14 NONE) searches the sphere around it
// (BD080: none, the types 0x89C, the unit's +0x28 and +0x30, its radius +0x3C
// plus 456918, at most 16 objects into the frame). Each object found, in
// order: a biped or vehicle (its type, int8 +0xAA, of 2..3) goes to 1538B0
// with the player; an object of type 11 or 7 appends its children (+0x10,
// then each one's sibling +0xC) while the list has room.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the found list in the original's frame
// and the argument it rereads; the boundary adapter
// (native_tu15_player_unit_search.hpp) maps the role to the original
// instruction and owns the ABI of the children (BD080, 1538B0). The values
// the original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_unit_search {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::widen8;
namespace sse=native_aim_clusters::sse;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u,object_headers=0x4ecbd0u;
inline constexpr std::uint32_t margin=0x456918u;   // float, added to the unit's radius
}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,unit_offset=0x2cu,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t next_sibling=0xcu,first_child=0x10u,parent=0x14u,origin=0x28u,center=0x30u,
    radius=0x3cu,type=0xaau;}
inline constexpr std::uint32_t search_types=0x89cu,capacity=16u,first_unit_type=2u,last_unit_type=3u,parent_type=0xbu,
    other_parent_type=7u;

namespace player_unit_search {
enum class Access {player_argument,players,player_data,player_unit,headers,datums,unit_object,unit_parent,unit_radius,margin,
    radius_store,found_handle,found_datums,found_object,found_type,player_again,headers_again,type_again,first_child,
    child_datums,appended,child_object,child_sibling};
enum class Outcome {no_unit,riding,searched};
// The original's EBX (the object headers), EBP (the found object's index),
// ESI (the found count) and EDI (the found object) across the children.
struct Kept {std::uint32_t headers,index,count,object;};
// Frame:
//   argument(), found(k)                the player's slot, the found list's entry k, now
//   framed()                            SUB ESP,40 (after the player's reads)
//   querying(radius)                    the query's count, list and radius slot pushed (the radius stored there)
//   radius_slot()                       the pushed slot the radius is stored into
//   query(origin, center, kept) -> AX   BD080(none, the types, origin, center; then the pushed), refreshes kept
//   saving_edi()                        EDI saved before the list's walk
//   offer(player, handle, kept)         1538B0(the player, the handle), refreshes kept
//   next(kept)                          the next object's index in its register
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto player=memory.load32(player_argument,frame.argument());
    const auto data=memory.load32(player_data,at(memory.load32(players,globals::players),datum_data));
    const auto unit=memory.load32(Access::player_unit,data+player_size*(player&0xffffu)+unit_offset);
    frame.framed();
    if(unit==none)return Outcome::no_unit;
    Kept kept{};
    kept.headers=memory.load32(headers,globals::object_headers);
    const auto unit_address=memory.load32(unit_object,memory.load32(datums,at(kept.headers,datum_data))+header_size*(unit&0xffffu)+header_address);
    if(memory.load32(unit_parent,at(unit_address,object::parent))!=none)return Outcome::riding;
    const auto radius_value=memory.load32(unit_radius,at(unit_address,object::radius));
    const auto radius=sse::add(radius_value,memory.load32(Access::margin,globals::margin));
    frame.querying(radius);
    memory.store32(radius_store,frame.radius_slot(),radius);
    kept.count=widen16(frame.query(at(unit_address,object::origin),at(unit_address,object::center),kept));
    kept.index=0u;
    if(signed_value(kept.count)<=0)return Outcome::searched;
    frame.saving_edi();
    do {
        const auto handle=memory.load32(found_handle,frame.found(kept.index));
        kept.object=memory.load32(found_object,memory.load32(found_datums,at(kept.headers,datum_data))+header_size*(handle&0xffffu)+header_address);
        const auto type=signed_value(widen8(memory.load8(found_type,at(kept.object,object::type))));
        if(type>=std::int32_t(first_unit_type) && type<=std::int32_t(last_unit_type)) {
            frame.offer(memory.load32(player_again,frame.argument()),handle,kept);
            kept.headers=memory.load32(headers_again,globals::object_headers);
        }
        const auto kind=memory.load8(type_again,at(kept.object,object::type))&0xffu;
        if(kind==parent_type || kind==other_parent_type) {
            for(auto child=memory.load32(first_child,at(kept.object,object::first_child));child!=none && kept.count<capacity;++kept.count) {
                const auto child_headers=memory.load32(child_datums,at(kept.headers,datum_data));
                memory.store32(appended,frame.found(kept.count),child);
                child=memory.load32(child_sibling,at(memory.load32(child_object,child_headers+header_size*(child&0xffffu)+header_address),object::next_sibling));
            }
        }
        ++kept.index;
        frame.next(kept);
    } while(signed_value(kept.index)<signed_value(kept.count));
    return Outcome::searched;
}
} // namespace player_unit_search
} // namespace halo2::native_player_unit_search
