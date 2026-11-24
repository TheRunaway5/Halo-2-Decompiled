#pragma once
// Native TU1.5 seated blend, entered at 113B70 (live support unit): whether
// a seated unit's first animation blend is the full one.
//
//   seated_blend::full   113B70(EDI = unit) -> AL
//
// A unit (through the object headers at [4ECBD0]) without a parent (+0x14
// NONE) or a seat (word +0x1FC NONE) answers 0. Otherwise the blend is full
// unless C9550 (ECX the unit) says so or the action its animation state
// plays (+0x7C of the block at its int16 +0x12A) is one of the names below;
// then 113C70 (EAX the unit, 2 pushed) has the last word: when it answers 0
// that is the answer.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seated_blend.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seated_blend {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace unit {inline constexpr std::uint32_t parent=0x14u,animation=0x12au,seat=0x1fcu;}
inline constexpr std::uint32_t playing=0x7cu,last_word_mode=2u;
// The actions (string ids) that keep the blend from being full.
inline constexpr std::uint32_t partial_actions[]{0x400004au,0x400076cu,0x400076du,0x50000c3u,0x5000768u,0x5000769u,0x500076au,
    0x500076bu,0x600008cu,0x700076eu,0x80000c4u,0x800076fu,0xa000767u,0xc000043u,0xd00002bu,0xd000042u,0xe00002au,0xe00003bu,
    0xe0000c3u,0xf00003au};
inline bool partial(std::uint32_t action){for(const auto a:partial_actions)if(a==action)return true;return false;}

namespace seated_blend {
enum class Access {headers,datums,object_address,parent,seat,animation,action};
enum class Outcome {unseated,partial,full,overruled};
// The original's BL (the answer), ESI (the unit's object) and EDI (the unit) across the children.
struct Kept {std::uint32_t answer,object,unit;};
// Frame:
//   parent_tested(parent), seat_tested(seat)      OR ECX,-1; XOR AL,AL; CMP EDX,ECX; CMP word [ESI+1FC],CX
//   restricted(kept) -> AL      PUSH EBX; MOV ECX,EDI; MOV BL,1; C9550, refreshes kept
//   action_compared(action)     the original's binary search over the names (its compares' flags)
//   cleared(kept)               XOR BL,BL
//   last_word(kept) -> AL       113C70(EAX = unit; 2), refreshes kept
//   answered(kept)              MOV AL,BL
template<class Memory,class Frame> Outcome full(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    kept.object=memory.load32(object_address,datums_address+header_size*(kept.unit&0xffffu)+header_address);
    const auto parent_value=memory.load32(Access::parent,at(kept.object,unit::parent));
    frame.parent_tested(parent_value);
    if(parent_value==none)return Outcome::unseated;
    const auto seat_value=memory.load16(Access::seat,at(kept.object,unit::seat));
    frame.seat_tested(seat_value);
    if(seat_value==0xffffu)return Outcome::unseated;
    kept.answer=1u;
    bool blocked=frame.restricted(kept)&0xffu;
    if(!blocked) {
        const auto block=widen16(memory.load16(animation,at(kept.object,unit::animation)));
        const auto playing_action=memory.load32(action,block+kept.object+playing);
        frame.action_compared(playing_action);
        blocked=partial(playing_action);
    }
    if(blocked){kept.answer=0u;frame.cleared(kept);}
    if(!(frame.last_word(kept)&0xffu))return Outcome::overruled;
    frame.answered(kept);
    return (kept.answer&0xffu)?Outcome::full:Outcome::partial;
}
} // namespace seated_blend
} // namespace halo2::native_seated_blend
