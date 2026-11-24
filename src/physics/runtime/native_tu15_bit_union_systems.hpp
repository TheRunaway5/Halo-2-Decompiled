#pragma once
// Native TU1.5 bit vector union, entered at 140EC0 (physics live support
// unit): two bit vectors ORed into a third.
//
//   bit_union::unite   140EC0(EAX = a*, EDX = b*, ECX = bits; out*) -> RET 4
//
// The vectors hold (bits + 31) >> 5 words (an arithmetic shift; none for a
// count that comes out zero or negative). From the last word down, each
// out word becomes b's word ORed with a's, read where the original reads
// them, so an out vector over its inputs sees what was stored.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_bit_union.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_bit_union {
namespace bit_union {
enum class Access {out,right,left,store};
constexpr std::int32_t words_for(std::uint32_t bits){return std::int32_t(bits+0x1fu)>>5;}
// Frame: walking() before the words; out_slot() the out pointer's slot;
// stepped() after each word.
template<class Memory,class Frame> std::int32_t unite(Memory &memory,Frame &frame,std::uint32_t a,std::uint32_t b,std::uint32_t bits) {
    using enum Access;
    const auto words=words_for(bits);
    if(words<=0)return words;
    frame.walking();
    const auto target=memory.load32(out,frame.out_slot());
    for(auto k=std::uint32_t(words-1);;--k) {
        const auto value=memory.load32(right,b+4u*k);
        memory.store32(store,target+4u*k,value|memory.load32(left,a+4u*k));
        frame.stepped();
        if(k==0)break;
    }
    return words;
}
} // namespace bit_union
} // namespace halo2::native_bit_union
