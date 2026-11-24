#pragma once
// Native TU1.5 weapon trade, entered at E79C0 (combat action unit): whether a
// player's unit and another unit may trade weapons.
//
//   weapon_trade::allowed   E79C0(EAX = player; other unit) -> AL
//
// The other unit must be busy with something (+0x12C not NONE). Each unit
// holds the weapon its signed index (byte +0x212, NONE for none) names among
// its weapons (+0x218); both need one. The offset between them (the other's
// position less the player's unit's, CD440) normalized (30BD0, through the
// dispatch) must be longer than [46A0E8] and shorter than [46A0D0]; the
// player's unit must face along it (CDCC0; the dot product, z then y then x,
// above [456910]) and the other unit face against it (11B560; below 0); and
// each must take the other's weapon in hand (CFC90, through the dispatch:
// the unit, the weapon and its hands record; the third byte set).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_weapon_trade.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them, and the offset it
// keeps in XMM5 to XMM7 across 11B560, are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_weapon_trade {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,players=0x4f54f4u,shortest=0x46a0e8u,longest=0x46a0d0u,
    facing=0x456910u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,handle_mask=0xffffu,none=0xffffffffu;
namespace player {inline constexpr std::uint32_t size=0x21cu,unit=0x2cu;}
namespace unit {inline constexpr std::uint32_t busy=0x12cu,weapon_index=0x212u,weapons=0x218u;}
namespace hands {inline constexpr std::uint32_t held=2u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame below the saved registers: the answer, the offset, the other's
// position, the player's unit's vector, and the other unit's argument (the
// hands record once passed).
namespace local {inline constexpr std::uint32_t answer=0x13u,offset=0x14u,other_position=0x20u,vector=0x2cu,other=0x3cu,size=0x28u;}

namespace weapon_trade {
enum class Access {other_argument,headers_load,datums_load,other_load,busy_load,answer_clear,players_load,player_datums,datums_again,
    other_again,other_index,other_weapon,unit_handle,datums_third,unit_load,unit_index,unit_weapon,unit_again,other_argument_again,
    other_x,unit_x,offset_x,other_y,unit_y,offset_y,other_z,unit_z,offset_z,shortest_load,longest_load,unit_facing,vector_z,offset_z_load,
    vector_y,offset_y_load,offset_x_load,vector_x,facing_load,other_facing,vector_z_again,vector_y_again,vector_x_again,other_hands,
    other_held,unit_hands,unit_held,answer_set,answer_load};
// The original's EBX (the other's weapon), EBP (the unit's weapon), EDI (the player) and XMM5 to XMM7 (the offset's z, y and x).
struct Kept {std::uint32_t other_weapon,unit_weapon,player_address,dz,dy,dx;};
// Frame: see native_tu15_weapon_trade.hpp.
template<class Memory,class Frame> bool allowed(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The other unit, busy.
    frame.framing();
    const auto other_handle=memory.load32(other_argument,frame.local(local::other));
    const auto headers=memory.load32(headers_load,globals::object_headers);
    const auto other_masked=frame.masked(other_handle);
    frame.saving(0u);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    const auto other_offset=frame.offset_of(other_masked);
    const auto other_address=memory.load32(other_load,other_offset+datums+header_address);
    frame.saving(1u);
    const auto busy=memory.load32(busy_load,at(other_address,unit::busy));
    frame.compared(busy,none);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    const auto answered=[&](bool saved_weapons) {
        frame.restoring(saved_weapons);
        const auto answer=memory.load8(answer_load,frame.local(local::answer));
        frame.finished();
        return answer!=0u;
    };
    if(busy==none)return answered(false);
    // Each unit's weapon.
    const auto players=memory.load32(players_load,globals::players);
    kept.player_address=frame.player_address(memory.load32(player_datums,at(players,datum_data)));
    const auto datums_now=memory.load32(datums_again,at(headers,datum_data));
    const auto other_now=memory.load32(other_again,other_offset+datums_now+header_address);
    const auto other_slot=memory.load8(other_index,at(other_now,unit::weapon_index));
    kept.other_weapon=frame.saving_weapon(2u);
    if(frame.slot_compared(other_slot))kept.other_weapon=memory.load32(other_weapon,other_now+4u*frame.slot(other_slot)+unit::weapons);
    const auto unit_handle_value=memory.load32(unit_handle,at(kept.player_address,player::unit));
    const auto datums_last=memory.load32(datums_third,at(headers,datum_data));
    const auto unit_address=memory.load32(unit_load,datums_last+header_size*frame.indexed(unit_handle_value)+header_address);
    const auto unit_slot=memory.load8(unit_index,at(unit_address,unit::weapon_index));
    kept.unit_weapon=frame.saving_weapon(3u);
    if(frame.slot_compared(unit_slot))kept.unit_weapon=memory.load32(unit_weapon,unit_address+4u*frame.slot(unit_slot)+unit::weapons);
    const auto denied=[&]{return answered(true);};
    frame.compared(kept.other_weapon,none);
    if(kept.other_weapon==none)return denied();
    frame.compared(kept.unit_weapon,none);
    if(kept.unit_weapon==none)return denied();
    // The offset between them, normalized, within range.
    frame.positioned(memory.load32(unit_again,at(kept.player_address,player::unit)),local::vector,kept);
    frame.positioned(memory.load32(other_argument_again,frame.local(local::other)),local::other_position,kept);
    const auto component=[&](Access other_role,Access unit_role,Access store_role,std::uint32_t c) {
        const auto other_value=memory.load32(other_role,frame.local(local::other_position+c));
        const auto difference=sse::subtract(other_value,memory.load32(unit_role,frame.local(local::vector+c)));
        frame.differenced(difference,c);
        memory.store32(store_role,frame.local(local::offset+c),difference);
    };
    component(other_x,unit_x,offset_x,x);
    component(other_y,unit_y,offset_y,y);
    component(other_z,unit_z,offset_z,z);
    const auto length=frame.normalized(kept);
    if(sse::not_above(length,frame.shortest_compared(memory.load32(shortest_load,globals::shortest))))return denied();
    const auto longest=memory.load32(longest_load,globals::longest);
    frame.longest_compared(longest);
    if(sse::not_above(longest,length))return denied();
    // The player's unit faces along the offset.
    frame.faced(memory.load32(unit_facing,at(kept.player_address,player::unit)),kept);
    const auto az=memory.load32(vector_z,frame.local(local::vector+z));
    kept.dz=memory.load32(offset_z_load,frame.local(local::offset+z));
    const auto ay=memory.load32(vector_y,frame.local(local::vector+y));
    kept.dy=memory.load32(offset_y_load,frame.local(local::offset+y));
    kept.dx=memory.load32(offset_x_load,frame.local(local::offset+x));
    const auto y_term=sse::multiply(ay,kept.dy);
    const auto partial=sse::add(sse::multiply(az,kept.dz),y_term);
    const auto x_term=sse::multiply(kept.dx,memory.load32(vector_x,frame.local(local::vector+x)));
    const auto along=sse::add(partial,x_term);
    frame.dotted(along,x_term);
    if(sse::not_above(along,frame.facing_compared(memory.load32(facing_load,globals::facing))))return denied();
    // The other unit faces against it.
    frame.other_faced(memory.load32(other_facing,frame.local(local::other)),kept);
    const auto bz=memory.load32(vector_z_again,frame.local(local::vector+z));
    const auto by=memory.load32(vector_y_again,frame.local(local::vector+y));
    const auto other_partial=sse::add(sse::multiply(bz,kept.dz),sse::multiply(by,kept.dy));
    const auto bx=memory.load32(vector_x_again,frame.local(local::vector+x));
    const auto other_x_term=sse::multiply(bx,kept.dx);
    const auto against=sse::add(other_partial,other_x_term);
    frame.other_dotted(against,other_x_term);
    if(sse::not_above(0u,against))return denied();
    // Each takes the other's weapon in hand.
    if(!frame.held(memory.load32(other_hands,frame.local(local::other)),kept.unit_weapon,kept))return denied();
    if(!frame.byte_tested(memory.load8(other_held,frame.local(local::other+hands::held))))return denied();
    if(!frame.held(memory.load32(unit_hands,at(kept.player_address,player::unit)),kept.other_weapon,kept))return denied();
    if(!frame.byte_tested(memory.load8(unit_held,frame.local(local::other+hands::held))))return denied();
    memory.store8(answer_set,frame.local(local::answer),1u);
    return denied();
}
} // namespace weapon_trade
} // namespace halo2::native_weapon_trade
