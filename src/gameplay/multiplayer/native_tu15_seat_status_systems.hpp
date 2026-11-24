#pragma once
// Native TU1.5 seat status, entered at 161610 (multiplayer engines unit):
// each local seat's player status, as the scoreboard shows it.
//
//   seat_status::update   161610()
//
// For each local seat with a player (the four handles at [4F54F0]+0xC,
// NONE for none), in order, 1618B0 answers the player's status code. A code
// other than the seat's record (12 bytes at 4F63D8 by seat: the code, a
// timer, the status's index) restarts the record: the code, the timer 0
// and the index NONE, then the index of the first status with that code
// among the multiplayer globals' statuses (the globals tag at [4ECC1C],
// its multiplayer tag +0x16C through the tag instances at 4F0414, that
// tag's runtime +0xC; its 0x1C-byte statuses +0x53C, the count +0x538, the
// int16 code at +4).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_status.hpp) maps the role to the original instruction
// and owns the ABI of the child (1618B0). The seat the original keeps in EBX
// across it is a Kept, refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_status {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t seats=0x4f54f0u;          // +0xC: each local seat's player
inline constexpr std::uint32_t records=0x4f63d8u;        // 12-byte status records, by seat
inline constexpr std::uint32_t game_globals=0x4ecc1cu;   // the globals tag's data
inline constexpr std::uint32_t tag_instances=0x4f0414u;  // 16-byte instances (+8 the tag's data)
}
inline constexpr std::uint32_t seat_count=4u,seat_players=0xcu,record_size=12u,tag_size=16u,tag_data=8u;
namespace record {inline constexpr std::uint32_t code=0u,timer=4u,index=8u;}
inline constexpr std::uint32_t multiplayer_tag=0x16cu,multiplayer_runtime=0xcu;
namespace runtime {inline constexpr std::uint32_t status_count=0x538u,statuses=0x53cu;}
inline constexpr std::uint32_t status_size=0x1cu,status_code=4u;

namespace seat_status {
enum class Access {seats,first_handle,seats_again,seat_player,previous_code,game_globals,multiplayer,instances,multiplayer_data,
    runtime_address,code_store,timer_store,index_store,count,statuses,status_value,matched_store,seats_next,next_handle};
enum class Outcome {no_player,visited};
// The original's EBX across 1618B0: the seat.
struct Kept {std::uint32_t seat;};
// Frame:
//   visiting()                  the registers saved before the first seat
//   status(handle, kept) -> code   1618B0(the handle pushed), refreshes kept
//   advancing(kept)             the next seat's start (0 after none, else the seat's next)
//   chosen(kept)                the next seat in its register
template<class Memory,class Frame> void restart(Memory &memory,std::uint32_t record_address,std::uint32_t code) {
    using enum Access;
    const auto tag=memory.load32(multiplayer,at(memory.load32(Access::game_globals,globals::game_globals),multiplayer_tag));
    const auto data=memory.load32(multiplayer_data,tag_size*(tag&0xffffu)+memory.load32(instances,globals::tag_instances)+tag_data);
    const auto runtime_value=memory.load32(runtime_address,at(data,multiplayer_runtime));
    memory.store32(code_store,at(record_address,record::code),code);
    memory.store32(timer_store,at(record_address,record::timer),0u);
    memory.store32(index_store,at(record_address,record::index),none);
    const auto count_value=memory.load32(count,at(runtime_value,runtime::status_count));
    if(signed_value(count_value)<=0)return;
    auto entry=memory.load32(statuses,at(runtime_value,runtime::statuses))+status_code;
    for(std::uint32_t index=0;;) {
        if(widen16(memory.load16(status_value,entry))==code){memory.store32(matched_store,at(record_address,record::index),index);return;}
        ++index;entry+=status_size;
        if(!(signed_value(index)<signed_value(count_value)))return;
    }
}
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{none};
    // The first local seat with a player.
    auto slot=memory.load32(seats,globals::seats)+seat_players;
    for(std::uint32_t k=0;signed_value(k)<signed_value(seat_count);++k,slot+=4u)
        if(memory.load32(first_handle,slot)!=none){kept.seat=k;break;}
    if(kept.seat==none)return Outcome::no_player;
    frame.visiting();
    for(;;) {
        if(kept.seat!=none) {
            const auto handle=memory.load32(seat_player,memory.load32(seats_again,globals::seats)+4u*kept.seat+seat_players);
            if(handle!=none) {
                const auto code=frame.status(handle,kept);
                const auto record_address=globals::records+record_size*kept.seat;
                if(code!=memory.load32(previous_code,at(record_address,record::code)))restart<Memory,Frame>(memory,record_address,code);
            }
        }
        // The next local seat with a player.
        frame.advancing(kept);
        auto next=none;
        auto k=kept.seat==none?0u:kept.seat+1u;
        if(signed_value(k)<signed_value(seat_count)) {
            auto next_slot=memory.load32(seats_next,globals::seats)+4u*k+seat_players;
            for(;;) {
                if(memory.load32(next_handle,next_slot)!=none){next=k;break;}
                ++k;next_slot+=4u;
                if(!(signed_value(k)<signed_value(seat_count)))break;
            }
        }
        kept.seat=next;
        frame.chosen(kept);
        if(kept.seat==none)return Outcome::visited;
    }
}
} // namespace seat_status
} // namespace halo2::native_seat_status
