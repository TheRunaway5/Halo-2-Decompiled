#pragma once
// Native TU1.5 player relation, entered at 1902E0 (physics live support and
// map world support units): whether an object belongs with one of the four
// local players' units.
//
//   player_relation::find   1902E0(object, flag*, player_index*)
//
// The index output (when given) starts as NONE. Unless the flag byte is 1
// nothing is searched. For each local player slot 0..3 (the handles at
// [4F54F0] + 0xC), a present player whose type (158290) is 0 or 1 has its
// unit (+0x2C of its 0x21C-byte record in the players array 4F54F4); when
// the object is within the unit's hierarchy or the unit within the
// object's (BCFE0 both ways), the index output takes the slot and the flag
// byte is cleared.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_relation.hpp) maps the role to the original
// instruction and owns the ABI of the children (158290, BCFE0); the values
// the original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_relation {
using native_object_update::at;
using native_object_update::none;

namespace globals {
inline constexpr std::uint32_t local_players=0x4f54f0u;   // +0xC: four player handles
inline constexpr std::uint32_t players=0x4f54f4u;         // DataArray* of player records
}
inline constexpr std::uint32_t local_player_handles=0xcu,local_player_count=4u;
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,player_unit_field=0x2cu;
inline constexpr std::uint32_t related_types=0x3u;        // player types 0 and 1

namespace player_relation {
enum class Local {object,flag,index_out};
enum class Access {
    index_out_argument,index_clear,flag_argument,flag_value,local_players,player_handle,players_table,players_data,
    player_unit,object_argument,index_out_reload,index_store,flag_reload,flag_clear
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t slot;      // EDI: the local player slot
    std::uint32_t handles;   // EBP: the slot's handle address
    std::uint32_t present;   // EBX: BL whether the slot has a player (the object, later)
    std::uint32_t player;    // ESI: the player handle (the unit, later)
};
enum class Exit {unflagged,unrelated,related};
// Frame:
//   local(Local)                   an argument's address now
//   player_type(kept)              158290 for the slot; returns EAX
//   within(kept, object, unit)     BCFE0 (EAX the object, ESI the unit);
//                                  returns AL
//   contains(kept, object, unit)   BCFE0 (EAX the unit, ESI the object);
//                                  returns AL
//   present(kept, player)          the slot's player was read (SETNZ BL)
//   slot_check(kept)               CMP EDI,-1 at the loop's top
//   advance(kept)                  the next slot and handle address
template<class Memory,class Frame> Exit find(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    if(const auto out=memory.load32(index_out_argument,local(Local::index_out)))memory.store32(index_clear,out,none);
    if(memory.load8(flag_value,memory.load32(flag_argument,local(Local::flag)))!=1u)return Exit::unflagged;
    Kept kept{};
    kept.handles=memory.load32(local_players,globals::local_players)+local_player_handles;
    for(kept.slot=0;;) {
        frame.slot_check(kept);
        if(kept.slot!=none) {
            kept.player=memory.load32(player_handle,kept.handles);
            frame.present(kept,kept.player);
            if(kept.present&0xffu) {
                const auto type=frame.player_type(kept);
                if(((1u<<(type&31u))&related_types) && (kept.present&0xffu)) {
                    const auto data=memory.load32(players_data,at(memory.load32(players_table,globals::players),datum_data));
                    const auto unit=memory.load32(player_unit,(kept.player&0xffffu)*player_size+data+player_unit_field);
                    kept.player=unit;
                    if(unit!=none) {
                        const auto object=memory.load32(object_argument,local(Local::object));
                        if((frame.within(kept,object,unit)&0xffu) || (frame.contains(kept)&0xffu)) {
                            if(const auto out=memory.load32(index_out_reload,local(Local::index_out)))
                                memory.store32(index_store,out,kept.slot);
                            memory.store8(flag_clear,memory.load32(flag_reload,local(Local::flag)),0);
                            return Exit::related;
                        }
                    }
                }
            }
        }
        frame.advance(kept);
        if(!(std::int32_t(kept.slot)<std::int32_t(local_player_count)))return Exit::unrelated;
    }
}
} // namespace player_relation
} // namespace halo2::native_player_relation
