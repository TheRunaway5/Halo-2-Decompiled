#pragma once
// Native TU1.5 object team, entered at C8E70 (physics live support unit): the
// team an object counts for (its word +0x138).
//
//   object_team::update   C8E70(EAX = object)
//
// An object (through the object headers at [4ECBD0]) held back (bit 2 of its
// byte +0x10A) is left as it is. One with a player (+0x13C) takes the
// player's team (int8 +0xC0 of its 0x21C-byte record among the players at
// [4F54F4]); one with an actor (+0x12C) the actor's team (word +0x24 of its
// 0x888-byte record among the actors at [5020C8]). A vehicle (type byte +0xAA
// 1) takes its riders' team: its carrier (D2B60, EBX an iterator on the
// stack) and each rider after it (D2BD0, EDX the iterator: the vehicle +0xC,
// the rider +0x10, the seat word +0x14) give, from the riders in seats, the
// team of a rider whose seat (0xB0 bytes at the vehicle's definition +0x1CC,
// through the tag instances at 4F0414) carries flag bit 11 as the second team
// and any other's as the first, until both are known. The vehicle's team is
// the first when the two agree or no second is known, and NONE otherwise
// (and NONE with no first). Any other object is left as it is.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the second team,
// the iterator); the boundary adapter (native_tu15_object_team.hpp) maps the
// role to the original instruction and owns the ABI of the children. The
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_team {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,players=0x4f54f4u,actors=0x5020c8u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,player_size=0x21cu,actor_size=0x888u,
    tag_size=16u,tag_data=8u,seat_size=0xb0u,no_team=0xffffu;
namespace object {inline constexpr std::uint32_t type=0xaau,flags_byte=0x10au,actor=0x12cu,team=0x138u,player=0x13cu;
    inline constexpr std::uint32_t held=4u,vehicle=1u;}
namespace player {inline constexpr std::uint32_t team=0xc0u;}
namespace actor {inline constexpr std::uint32_t team=0x24u;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu;}
inline constexpr std::uint32_t second_team_seat=11u;
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t second=-0x1c,iterator=-0x18,vehicle=-0xc,rider=-8,seat=-4;}

namespace object_team {
enum class Access {headers,datums,object_address,flags_byte,player,players_array,players_data,player_team,player_team_store,actor,
    actors_array,actors_data,actor_team,actor_team_store,object_type,second_clear,instances,second_load,rider_load,seat_load,
    rider_object,vehicle_load,vehicle_object,vehicle_tag,definition_address,seats_load,seat_flags,second_team,second_store,
    first_team,second_final,team_none,team_store};
enum class Outcome {held,player_team,actor_team,untouched,no_team,riders_team};
// The original's EBX (the iterator, then the tag instances), EBP (the first team), ESI (the object datums) and EDI (the
// object) across the children.
struct Kept {std::uint32_t held,first,datums,object;};
// Frame:
//   local(offset), framing(), indexing(object)          SUB ESP,1C; PUSH ESI; ...; PUSH EDI
//   held_tested(byte), saving_ebp(player), leaving_ebp()   SHR CL,2; TEST CL,1; PUSH EBP; OR EBP,-1; CMP; POP EBP
//   carrier(object, kept), next(kept) -> AL              D2B60(EAX = object, EBX = the iterator) and D2BD0(EDX = the iterator)
//   first_tested(first), second_tested(second), seat_tested(seat), vehicle_tested(vehicle), seat_flags_tested(flags)
//   decided(none_team)                                   the walk's end: EBX and EBP popped before a NONE store
//   actor_tested(actor), type_tested(type), saving_ebx()  CMP ECX,EBP; CMP byte [EDI+AA],1; PUSH EBX; LEA EBX
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t object_handle,Kept kept) {
    using enum Access;
    const auto headers_address=memory.load32(headers,globals::object_headers);
    frame.framing();
    kept.datums=memory.load32(datums,at(headers_address,datum_data));
    frame.indexing(object_handle);
    kept.object=memory.load32(object_address,kept.datums+header_size*(object_handle&0xffffu)+header_address);
    const auto byte=memory.load8(flags_byte,at(kept.object,object::flags_byte));
    frame.held_tested(byte);
    if(byte&object::held)return Outcome::held;
    const auto player_handle=memory.load32(Access::player,at(kept.object,object::player));
    kept.first=none;
    frame.saving_ebp(player_handle,kept);
    if(player_handle!=none) {
        const auto data=memory.load32(players_data,at(memory.load32(players_array,globals::players),datum_data));
        const auto team=widen8(memory.load8(player_team,player_size*(player_handle&0xffffu)+data+player::team))&0xffffu;
        frame.leaving_ebp(0u,team);
        memory.store16(player_team_store,at(kept.object,object::team),team);
        return Outcome::player_team;
    }
    const auto actor_handle=memory.load32(Access::actor,at(kept.object,object::actor));
    frame.actor_tested(actor_handle);
    if(actor_handle!=none) {
        const auto data=memory.load32(actors_data,at(memory.load32(actors_array,globals::actors),datum_data));
        const auto team=memory.load16(actor_team,actor_size*(actor_handle&0xffffu)+data+actor::team);
        frame.leaving_ebp(1u,team);
        memory.store16(actor_team_store,at(kept.object,object::team),team);
        return Outcome::actor_team;
    }
    const auto type=memory.load8(object_type,at(kept.object,object::type));
    frame.type_tested(type);
    if(type!=object::vehicle)return Outcome::untouched;
    // The riders' teams.
    frame.saving_ebx();
    kept.held=frame.local(local::iterator);
    memory.store32(second_clear,frame.local(local::second),kept.first);
    frame.carrier(object_handle,kept);
    std::uint32_t second=0u;
    bool both=false;
    if(frame.next(kept)&0xffu) {
        kept.held=memory.load32(instances,globals::tag_instances);
        for(;;) {
            frame.first_tested(kept.first);
            if((kept.first&0xffffu)!=no_team) {
                second=memory.load32(second_load,frame.local(local::second));
                frame.second_tested(second);
                if((second&0xffffu)!=no_team){both=true;break;}
            }
            const auto rider=memory.load32(rider_load,frame.local(local::rider));
            const auto seat=memory.load32(seat_load,frame.local(local::seat));
            frame.seat_tested(rider,seat);
            const auto rider_address=memory.load32(rider_object,kept.datums+header_size*(rider&0xffffu)+header_address);
            if((seat&0xffffu)!=0xffffu) {
                const auto vehicle_handle=memory.load32(vehicle_load,frame.local(local::vehicle));
                frame.vehicle_tested(vehicle_handle);
                if(vehicle_handle!=none) {
                    const auto vehicle_address=memory.load32(vehicle_object,kept.datums+header_size*(vehicle_handle&0xffffu)+header_address);
                    const auto tag=memory.load32(vehicle_tag,vehicle_address);
                    const auto seat_offset=seat_size*widen16(seat&0xffffu);
                    const auto definition_value=memory.load32(definition_address,tag_size*(tag&0xffffu)+kept.held+tag_data);
                    const auto seats=memory.load32(seats_load,at(definition_value,definition::seats));
                    const auto flags=memory.load32(seat_flags,seats+seat_offset);
                    frame.seat_flags_tested(flags,seat_offset);
                    if((flags>>second_team_seat)&1u) {
                        const auto team=memory.load16(second_team,at(rider_address,object::team));
                        memory.store16(second_store,frame.local(local::second),team);
                    } else {
                        kept.first=(kept.first&0xffff0000u)|memory.load16(first_team,at(rider_address,object::team));
                    }
                }
            }
            if(!(frame.next(kept)&0xffu)) {
                second=memory.load32(second_final,frame.local(local::second));
                break;
            }
        }
        (void)both;
    } else {
        frame.decided(true);
        memory.store16(team_none,at(kept.object,object::team),no_team);
        return Outcome::no_team;
    }
    // The first team when the second agrees or is unknown; NONE otherwise, and with no first.
    const auto first_team_value=kept.first&0xffffu;
    const bool none_team=first_team_value==no_team || (first_team_value!=(second&0xffffu) && (second&0xffffu)!=no_team);
    frame.decided(none_team);
    if(none_team) {
        memory.store16(team_none,at(kept.object,object::team),no_team);
        return Outcome::no_team;
    }
    memory.store16(team_store,at(kept.object,object::team),first_team_value);
    return Outcome::riders_team;
}
} // namespace object_team
} // namespace halo2::native_object_team
