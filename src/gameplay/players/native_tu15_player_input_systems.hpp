#pragma once
// Native TU1.5 player input, entered at 1EEDB0 (physics live support unit):
// a player's new input becomes its local player's current input.
//
//   player_input::accept   1EEDB0(EAX = player handle; input*)
//
// A player (0x21C bytes of the players array at 4F54F4) with a local slot
// (int16 +0x28, NONE for none) owns a 0x1B0-byte local record (52BE64): its
// current input (+0x88, 23 words) becomes its previous input (+0xE4) and
// the new input becomes current (REP MOVSD both). When the player's unit
// (+0x2C, NONE for none) has no pending choice (int16 +0x1FC NONE) and the
// input's choice (byte +0x20, 0xFF for none) differs from the unit's
// (+0x216), the record counts the change (+0x197) and its choice timer
// (+0x196) becomes twice the low byte of the ticks per second ([51DF54]+2).
// 1F0F90 then takes the record's state (+0x140) and the input.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_input.hpp) maps the role to the original instruction
// and owns the ABI of the child 1F0F90.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_input {
using native_object_update::at;
using native_object_update::none;
using native_object_update::object_of;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;        // DataArray* of 0x21C-byte players
inline constexpr std::uint32_t locals=0x52be64u;         // 0x1B0-byte local player records
inline constexpr std::uint32_t game_time=0x51df54u;      // +2: ticks per second
}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,local_size=0x1b0u,input_words=0x17u;
namespace player {inline constexpr std::uint32_t local_slot=0x28u,unit=0x2cu;}
namespace record {inline constexpr std::uint32_t input=0x88u,previous=0xe4u,state=0x140u,timer=0x196u,changes=0x197u;}
namespace input {inline constexpr std::uint32_t choice=0x20u;}
namespace unit {inline constexpr std::uint32_t pending=0x1fcu,choice=0x216u;}
inline constexpr std::uint32_t no_choice=0xffu;

namespace player_input {
enum class Local {input};
enum class Access {
    players,input_argument,player_data,local_slot,locals,previous_read,previous_store,input_read,input_store,unit,
    object_table,object_datums,object_address,pending,choice,unit_choice,changes,game_time,changes_store,ticks,timer_store
};
// Frame:
//   local(Local)      the argument's address now
//   copying()         the first REP MOVSD starts (EBX, EDI, ECX, ESI)
//   copied()          the second starts
//   update()          1F0F90(EAX the record's state, ECX the input)
template<class Memory,class Frame> bool accept(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto players_array=memory.load32(players,globals::players);
    const auto input_address=memory.load32(input_argument,frame.local(Local::input));
    const auto player_address=(handle&0xffffu)*player_size+memory.load32(player_data,at(players_array,datum_data));
    const auto slot=widen16(memory.load16(local_slot,at(player_address,player::local_slot)));
    if(slot==none)return false;
    const auto local_record=memory.load32(locals,globals::locals)+slot*local_size;
    frame.copying();
    for(std::uint32_t word=0;word<input_words;++word)
        memory.store32(previous_store,at(local_record,record::previous+4u*word),
                       memory.load32(previous_read,at(local_record,record::input+4u*word)));
    frame.copied();
    for(std::uint32_t word=0;word<input_words;++word)
        memory.store32(input_store,at(local_record,record::input+4u*word),memory.load32(input_read,input_address+4u*word));
    const auto unit_handle=memory.load32(unit,at(player_address,player::unit));
    if(unit_handle!=none) {
        const auto unit_data=object_of(memory,unit_handle,object_table,object_datums,object_address);
        if(memory.load16(pending,at(unit_data,unit::pending))==0xffffu) {
            const auto wanted=memory.load8(choice,at(input_address,input::choice));
            if(wanted!=no_choice && wanted!=memory.load8(unit_choice,at(unit_data,unit::choice))) {
                const auto count=memory.load8(changes,at(local_record,record::changes));
                const auto time=memory.load32(game_time,globals::game_time);
                memory.store8(changes_store,at(local_record,record::changes),count+1u);
                memory.store8(timer_store,at(local_record,record::timer),memory.load8(ticks,at(time,2u))<<1);
            }
        }
    }
    frame.update();
    return true;
}
} // namespace player_input
} // namespace halo2::native_player_input
