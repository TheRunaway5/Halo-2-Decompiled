#pragma once
// Native TU1.5 animation fallbacks, entered at 1E5F20 (physics live support,
// AI frame and AI actor units): one lookup tried with ever wider keys.
//
//   animation_fallbacks::apply   1E5F20(AL = forced, ESI, EDI; a, b, c) -> RET C
//
// Unless neither AL nor c's low byte is set, 1E5E40 runs five times, each
// with ESI first and c last and between them the keys (EDI, a, b), then
// (EDI, a, any), (EDI, any, b), (EDI, any, any) and (any, any, any), "any"
// being 030000D9. The arguments are read from the stack for each call.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_fallbacks.hpp) maps the role to the original
// instruction and owns the ABI of 1E5E40; the second argument the original
// keeps in EBP across the calls is a Kept, refreshed from the register each
// returned (ESI and EDI are passed as the calls leave them).
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_fallbacks {
inline constexpr std::uint32_t any=0x030000d9u;
inline constexpr std::uint32_t lookups=5u;

namespace animation_fallbacks {
enum class Access {second,forced_byte,third,first};
struct Kept {std::uint32_t second;};   // EBP
enum class Key {first_argument,second_argument,any};
struct Lookup {bool own_edi;Key a,b;};
// The five lookups, in the original's order.
inline constexpr Lookup chain[lookups]{
    {true,Key::first_argument,Key::second_argument},{true,Key::first_argument,Key::any},
    {true,Key::any,Key::second_argument},{true,Key::any,Key::any},{false,Key::any,Key::any}};
// Frame: argument_slot(k); lookup(kept, k, keys...) 1E5E40, refreshes kept.
template<class Memory,class Frame> bool apply(Memory &memory,Frame &frame,std::uint32_t forced) {
    using enum Access;
    Kept kept{memory.load32(Access::second,frame.argument_slot(1))};
    if(!(forced&0xffu) && !(memory.load8(forced_byte,frame.argument_slot(2))&0xffu))return false;
    for(std::uint32_t k=0;k<lookups;++k) {
        const auto &step=chain[k];
        const auto c=memory.load32(third,frame.argument_slot(2));
        const auto a=step.a==Key::first_argument?memory.load32(first,frame.argument_slot(0)):any;
        const auto b=step.b==Key::second_argument?kept.second:any;
        frame.lookup(kept,k,step.own_edi,a,b,c);
    }
    return true;
}
} // namespace animation_fallbacks
} // namespace halo2::native_animation_fallbacks
