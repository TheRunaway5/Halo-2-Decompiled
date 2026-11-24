#pragma once
// Native TU1.5 unit aim enabled, entered at 1139D0 (physics live support
// unit): whether a unit's aim follows its controls.
//
//   unit_aim_enabled::enabled   1139D0(EDI unit handle) -> AL
//
// The unit ([4ECBD0], 12-byte entries, the address at +8) keeps its aim
// only in the channel states (+0xA4 of the block at the unit's signed
// +0x33E) listed below, NONE among them. When C9550 (ECX the unit)
// answers, the aim is dropped; otherwise the unit's animation state (+0x7C
// of the block at its signed +0x12A) drops it when listed. 113C70 (EAX the
// unit, 3 pushed) must answer too, or its answer is the routine's.
//
// The original searches each list as a compiled binary search, part of the
// first through a jump table in the code ([113B60] index bytes, [113B58]
// targets); they are sets here, and the adapter keeps the search's compare
// chain (its flags, EAX as the SUBs leave it, EDX the table's byte).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_unit_aim_enabled.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue for each exit. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_aim_enabled {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,ready_mode=3u;
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace unit {inline constexpr std::uint32_t animation=0x12au,channels=0x33eu;}
namespace channels {inline constexpr std::uint32_t state=0xa4u;}
namespace animation {inline constexpr std::uint32_t state=0x7cu;}
// The jump table of the first search, in the code: its first state and its
// index bytes and targets.
namespace jump_table {inline constexpr std::uint32_t first=0x9000004u,last=0x9000009u,indices=0x113b60u,targets=0x113b58u;}
// The channel states that keep the aim.
inline constexpr std::uint32_t aiming_states[]{0xffffffffu,0x500000au,0x6000006u,0x6000007u,0x9000004u,0x9000005u,0x9000008u,
    0x9000009u,0xa000040u,0xa0005b9u,0xa0005bbu,0xb0005b2u,0xc000073u,0xc000075u,0xc000077u,0xc0006b3u,0xc0006cdu,0xe000038u,
    0xe000607u,0xe000608u,0xe000609u,0xe00060au,0xe00067du,0x11000074u,0x11000076u,0x130005bcu,0x140005b3u};
// The animation states that drop it.
inline constexpr std::uint32_t blocking_animations[]{0x400004au,0x600008cu,0xa00003eu,0xb000059u,0xb00005au,0xc000043u,0xc000058u,
    0xc00005bu,0xd00003fu,0xd000042u,0xe00003bu,0xe0000c2u,0xf00003au,0x1000003cu};
template<std::size_t N> constexpr bool among(const std::uint32_t (&names)[N],std::uint32_t name) {
    for(const auto n:names)if(n==name)return true;
    return false;
}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace unit_aim_enabled {
enum class Access {objects_load,objects_data,unit_load,channels_offset,animation_offset,state_load,jump_index,jump_target,
    animation_state};
// The original's exits: 113C70's answer, or the aim's.
enum class Exit {unready,answered};
// The original's EBX (the answer in BL), ESI (the animation block) and EDI
// (the unit's handle).
struct Kept {std::uint32_t answer,animation_address,handle;};

// Frame: see native_tu15_unit_aim_enabled.hpp.
template<class Memory,class Frame> Exit enabled(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The unit and its channel state.
    const auto objects_header=memory.load32(objects_load,globals::objects);
    const auto objects=memory.load32(objects_data,at(objects_header,datum_data));
    const auto unit_address=memory.load32(unit_load,objects+entry::size*frame.unit_index(kept.handle)+entry::address);
    const auto channels_block=signed_word(memory.load16(channels_offset,at(unit_address,unit::channels)));
    frame.saving(1u);
    frame.saving(2u);
    const auto animation_block=signed_word(memory.load16(animation_offset,at(unit_address,unit::animation)));
    kept.animation_address=frame.animation_address(animation_block,unit_address);
    const auto state=memory.load32(state_load,channels_block+unit_address+channels::state);
    if(frame.state_searched(state)) {
        const auto index=memory.load8(jump_index,jump_table::indices+(state-jump_table::first));
        memory.load32(jump_target,jump_table::targets+4u*index);
    }
    kept.answer=frame.state_answered(among(aiming_states,state));

    // Dropped by C9550, or by the animation.
    if(frame.dropped_by_unit(kept))kept.answer=frame.dropped(false);
    else {
        const auto animation_value=memory.load32(animation_state,at(kept.animation_address,animation::state));
        frame.animation_searched(animation_value);
        if(among(blocking_animations,animation_value))kept.answer=frame.dropped(true);
    }
    return frame.ready(kept)?Exit::answered:Exit::unready;
}
} // namespace unit_aim_enabled
} // namespace halo2::native_unit_aim_enabled
