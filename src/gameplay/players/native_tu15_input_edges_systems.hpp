#pragma once
// Native TU1.5 input edges, entered at 1521B0 (physics live support unit): a
// player's new input reduced to what changed since the last.
//
//   input_edges::update   1521B0(EAX = player handle, ECX = input*)
//
// The input (the caller's copy of the 0x5C-byte input record) keeps only
// the buttons (+0) the player (0x21C bytes of the players array at 4F54F4)
// did not already hold (+0x38); the player then holds the input's buttons
// of the held mask. The input's two triggers (byte +0x1C: bits 0 and 4
// held) gain their pressed (bits 2 and 6) and released (bits 3 and 7)
// against the player's last (byte +0x3C, bits 0 and 4), which then become
// the input's. A latched trigger (the player's byte +2, bits 4 and 5)
// clears the trigger's four bits from the input, and stays latched while
// the trigger is held.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_input_edges.hpp) maps the role to the original instruction;
// the original keeps the trigger bits in stack locals between them, which
// the adapter owns.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_input_edges {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u;}     // DataArray* of 0x21C-byte players
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu;
namespace player {inline constexpr std::uint32_t latches=2u,held_buttons=0x38u,held_triggers=0x3cu;}
namespace input {inline constexpr std::uint32_t buttons=0u,triggers=0x1cu;}
inline constexpr std::uint32_t held_mask=0x16100074u;      // the buttons a player holds across inputs
struct Trigger {std::uint32_t held,pressed,released,bits,latch;};
inline constexpr Trigger first{0x01u,0x04u,0x08u,0x0fu,0x10u},second{0x10u,0x40u,0x80u,0xf0u,0x20u};

namespace input_edges {
enum class Access {players,player_data,buttons,held,buttons_store,last_triggers,held_store,triggers,trigger_word,last_again,
    pressed_store,bits,bits_store,latches,latches_again};
// Each bit the update sets or clears, in the original's order.
enum class Change {first_pressed,first_released,first_held,second_pressed,second_released,second_held,
    first_cleared,first_latch,second_cleared,second_latch};
// Frame: changing(change, on) before each change.
template<class Memory,class Frame> void assign(Memory &memory,Frame &frame,Change change,std::uint32_t address,
                                              std::uint32_t bits,bool on,bool through_word=false) {
    frame.changing(change,on);
    const auto value=through_word?memory.load16(Access::bits,address):memory.load8(Access::bits,address);
    const auto next=on?value|bits:value&~bits;
    if(through_word)memory.store16(Access::bits_store,address,next);else memory.store8(Access::bits_store,address,next);
}
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t input_record) {
    using enum Access;
    const auto players_array=memory.load32(players,globals::players);
    const auto player_address=memory.load32(player_data,at(players_array,datum_data))+player_size*(handle&0xffffu);
    const auto buttons_value=memory.load32(buttons,at(input_record,input::buttons));
    const auto held_value=memory.load32(held,at(player_address,player::held_buttons));
    memory.store32(buttons_store,at(input_record,input::buttons),buttons_value&~held_value);
    const auto last_value=memory.load8(last_triggers,at(player_address,player::held_triggers));
    memory.store32(held_store,at(player_address,player::held_buttons),buttons_value&held_mask);
    const auto triggers_address=at(input_record,input::triggers);
    const bool first_held=memory.load8(triggers,triggers_address)&first.held;
    const auto word=memory.load16(trigger_word,triggers_address);
    const bool first_was_held=last_value&first.held;
    const bool second_was_held=memory.load8(last_again,at(player_address,player::held_triggers))&second.held;
    const bool second_held=word&second.held;
    // The first trigger's pressed goes in with the word the input was read as.
    frame.changing(Change::first_pressed,first_held && !first_was_held);
    memory.store16(pressed_store,triggers_address,first_held && !first_was_held?word|first.pressed:word&~first.pressed);
    assign(memory,frame,Change::first_released,triggers_address,first.released,!first_held && first_was_held);
    assign(memory,frame,Change::first_held,at(player_address,player::held_triggers),first.held,first_held);
    assign(memory,frame,Change::second_pressed,triggers_address,second.pressed,second_held && !second_was_held);
    assign(memory,frame,Change::second_released,triggers_address,second.released,!second_held && second_was_held);
    assign(memory,frame,Change::second_held,at(player_address,player::held_triggers),second.held,second_held,!second_held);
    const auto latches_address=at(player_address,player::latches);
    if(memory.load8(latches,latches_address)&first.latch) {
        assign(memory,frame,Change::first_cleared,triggers_address,first.bits,false);
        assign(memory,frame,Change::first_latch,latches_address,first.latch,first_held,!first_held);
    }
    if(memory.load8(latches_again,latches_address)&second.latch) {
        assign(memory,frame,Change::second_cleared,triggers_address,second.bits,false);
        assign(memory,frame,Change::second_latch,latches_address,second.latch,second_held,second_held);
    }
}
} // namespace input_edges
} // namespace halo2::native_input_edges
