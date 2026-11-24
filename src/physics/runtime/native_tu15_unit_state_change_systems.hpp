#pragma once
// Native TU1.5 unit state change, entered at 1115D0 (physics live support
// unit): the state a unit's current one (the string id +0x14C) leads to,
// and the change when another is chosen.
//
//   unit_state_change::change   1115D0(EAX unit handle, DL flag; held byte) -> EAX state; RET 4
//
// A unit ([4ECBD0], 12-byte entries, the address at +8) in the direct state
// answers it at once. Five states lead to themselves; two lead to
// themselves, or with the flag to the flagged state; any other answers
// none. Unless the held byte (the argument's low byte) is set, 111D10 (EAX
// the unit, EDX a local, ESI the argument's slot) may choose a state into
// the argument's slot: a chosen state other than the one led to is the
// answer, and the unit's counter (byte +3 of the block at the unit's signed
// +0x33E) counts down; past zero it restarts at 4, otherwise 1D6430 (EDI
// the unit's animation, its signed +0x12A; ECX and EDX none; the state led
// to, 1 and 0 pushed) changes the animation.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_unit_state_change.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue for each exit. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_state_change {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,restart=4u;
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace unit {inline constexpr std::uint32_t animation=0x12au,state=0x14cu,block=0x33eu;}
namespace block {inline constexpr std::uint32_t counter=3u;}
// The string ids of the states.
namespace state {
inline constexpr std::uint32_t none=0x7000101u,direct=0x50000cbu;
inline constexpr std::uint32_t first=0x4000089u,second=0x6000084u,third=0x700002cu,fourth=0x7000039u,fifth=0x70000c9u;
inline constexpr std::uint32_t low=0x6000085u,high=0x6000086u,flagged=0x6000087u;
}
// What a state leads to: nothing, an answer at once, or a state to change to.
enum class Lead {none,direct,change};
struct Next {Lead lead;std::uint32_t state;};
inline constexpr Next next_state(std::uint32_t current,bool flag) {
    switch(current) {
    case state::direct:return {Lead::direct,current};
    case state::first:case state::second:case state::third:case state::fourth:case state::fifth:return {Lead::change,current};
    case state::low:case state::high:return {Lead::change,flag?state::flagged:current};
    default:return {Lead::none,state::none};
    }
}
// The frame (offsets from the ESP after ECX, EBX, EBP, ESI and EDI are
// pushed): 111D10's local in ECX's slot and the argument.
namespace local {inline constexpr std::uint32_t chosen_local=0x10u,argument=0x18u;}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace unit_state_change {
enum class Access {objects_load,objects_data,unit_load,block_offset,state_load,held_load,chosen_load,counter_load,counter_store,
    counter_reset,animation_offset};
// The original's exits, each with its own epilogue: the direct state, the
// counter restarted, and every other.
enum class Exit {direct,restarted,common};
// The original's EBX (the unit), EBP (the block), ESI (the chosen state's
// slot, then the chosen state) and EDI (the state led to).
struct Kept {std::uint32_t unit_address,block_address,chosen,next;};

// Frame: see native_tu15_unit_state_change.hpp.
template<class Memory,class Frame> Exit change(Memory &memory,Frame &frame,Kept kept,std::uint32_t handle,bool flag) {
    using enum Access;
    // The unit and its state.
    frame.reserving();
    frame.saving(1u);
    frame.saving(2u);
    frame.saving(3u);
    const auto objects_header=memory.load32(objects_load,globals::objects);
    const auto objects=memory.load32(objects_data,at(objects_header,datum_data));
    kept.unit_address=memory.load32(unit_load,objects+entry::size*frame.unit_index(handle)+entry::address);
    const auto offset=signed_word(memory.load16(block_offset,at(kept.unit_address,unit::block)));
    const auto current=memory.load32(state_load,at(kept.unit_address,unit::state));
    kept.block_address=frame.block_address(offset,kept.unit_address);
    const auto next=next_state(current,flag);
    kept.next=frame.searched(current,next);
    if(next.lead==Lead::none)return Exit::common;
    if(next.lead==Lead::direct)return Exit::direct;

    // Another state chosen.
    if(frame.set(memory.load8(held_load,frame.local(local::argument))))return Exit::common;
    if(!frame.chosen(kept))return Exit::common;
    kept.chosen=memory.load32(chosen_load,frame.local(local::argument));
    if(!frame.different(kept.chosen,kept.next))return Exit::common;
    const auto counter=frame.counted_down(memory.load8(counter_load,at(kept.block_address,block::counter)));
    memory.store8(counter_store,at(kept.block_address,block::counter),counter);
    if(frame.negative(counter)) {
        frame.leaving_restarted();
        memory.store8(counter_reset,at(kept.block_address,block::counter),restart);
        return Exit::restarted;
    }
    frame.change_pushed(kept.next);
    const auto animation=signed_word(memory.load16(animation_offset,at(kept.unit_address,unit::animation)));
    frame.changed(kept,animation);
    return Exit::common;
}
} // namespace unit_state_change
} // namespace halo2::native_unit_state_change
