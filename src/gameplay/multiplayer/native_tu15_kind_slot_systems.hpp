#pragma once
// Native TU1.5 kind slot, entered at 15CAB0 (physics live support unit): the
// first of a list of int16 kinds that a kind may take.
//
//   kind_slot::find   15CAB0(EDI = kinds, EDX = kind; count) -> EAX the index or NONE, RET 4
//
// With a current variant (the session's variant index, [4F63B8]+0xC14, into
// the variant records at 56DFA0, not NULL), a listed kind is taken when it is the kind itself; when both are
// 1 or 9; when it is 0xC; or when it is 0xD or 0xE and the kind is neither 1
// nor 9. Without one the first empty (zero) entry is taken.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_kind_slot.hpp) maps the role to the original instruction and
// keeps the compare tree's registers and flags.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_kind_slot {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t session=0x4f63b8u,variants=0x56dfa0u;}
inline constexpr std::uint32_t variant_index=0xc14u;
namespace kinds {inline constexpr std::uint32_t paired=1u,other_paired=9u,any=0xcu,open=0xdu,other_open=0xeu;}

namespace kind_slot {
enum class Access {session,index,count,variant,listed,empty};
enum class Outcome {none_listed,matched,none_empty,empty_found};
inline bool paired(std::uint32_t k){return k==kinds::paired || k==kinds::other_paired;}
// Whether the listed kind (a word) may be taken by the kind.
inline bool takes(std::uint32_t listed,std::uint32_t kind) {
    bool taken=widen16(listed)==kind;
    if(paired(kind) && paired(listed))taken=true;
    if(listed==kinds::any)return true;
    if(listed==kinds::open || listed==kinds::other_open)taken|=!paired(kind);
    return taken;
}
// Frame: argument() the count's slot now; considered(listed, kind, taken) the compare tree's registers and flags.
template<class Memory,class Frame> Outcome find(Memory &memory,Frame &frame,std::uint32_t list,std::uint32_t kind,std::uint32_t &answer) {
    using enum Access;
    const auto index_value=memory.load32(index,at(memory.load32(Access::session,globals::session),variant_index));
    const auto count_value=memory.load32(count,frame.argument());
    answer=none;
    if(memory.load32(variant,globals::variants+4u*index_value)) {
        for(std::uint32_t k=0;signed_value(k)<signed_value(count_value);++k) {
            const auto word=memory.load16(listed,list+2u*k)&0xffffu;
            const bool taken=takes(word,kind);
            frame.considered(word,kind,taken);
            if(taken){answer=k;return Outcome::matched;}
        }
        return Outcome::none_listed;
    }
    for(std::uint32_t k=0;signed_value(k)<signed_value(count_value);++k) {
        if(!(memory.load16(empty,list+2u*k)&0xffffu)){answer=k;return Outcome::empty_found;}
    }
    return Outcome::none_empty;
}
} // namespace kind_slot
} // namespace halo2::native_kind_slot
