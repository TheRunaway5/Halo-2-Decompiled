#pragma once
// Native TU1.5 pool reset, entered at 140CF0 (renderer transparent storage,
// scene device constructor, particle draw and startup storage units): a
// pool's storage cleared and its elements linked free.
//
//   pool_reset::reset   140CF0(EDX = pool)
//
// The pool's storage (+0x3C) holds its header dwords (+0x20 of them) and
// its elements (+0x24 of them, each its size +0x28 plus 12 bytes). It is
// cleared as REP STOSD then REP STOSB clear it, in the direction the string
// direction flag gives; then the free list (+0x38) starts empty and every
// element, in order, is pushed onto it (its +8 the previous head).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter (native_tu15_pool_reset.hpp)
// maps the role to the original instruction and owns the string
// instructions' registers.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_pool_reset {
using native_object_update::at;
using native_object_update::signed_value;
namespace pool {inline constexpr std::uint32_t header_words=0x20u,count=0x24u,size=0x28u,free_list=0x38u,storage=0x3cu;}
inline constexpr std::uint32_t element_extra=12u,next_link=8u;

namespace pool_reset {
enum class Access {size,count,header,word_clear,byte_clear,size_again,count_again,free_clear,head,element_next,head_store};
enum class Outcome {empty,linked};
// Frame: step(width) the direction flag's step; words_started(bytes) and
// bytes_started(bytes) the string instructions' registers.
template<class Memory,class Frame> Outcome reset(Memory &memory,Frame &frame,std::uint32_t pool_address) {
    using enum Access;
    const auto element=memory.load32(Access::size,at(pool_address,pool::size))+element_extra;
    const auto elements=element*memory.load32(count,at(pool_address,pool::count));
    const auto header_bytes=memory.load32(header,at(pool_address,pool::header_words))<<2;
    const auto bytes=elements+header_bytes;
    frame.words_started(bytes);
    auto to=at(pool_address,pool::storage);
    const auto word_step=frame.step(4u);
    for(std::uint32_t k=0;k<bytes>>2;++k,to+=word_step)memory.store32(word_clear,to,0u);
    frame.bytes_started(bytes);
    const auto byte_step=frame.step(1u);
    for(std::uint32_t k=0;k<(bytes&3u);++k,to+=byte_step)memory.store8(byte_clear,to,0u);
    // The free list.
    const auto stride=memory.load32(size_again,at(pool_address,pool::size))+element_extra;
    auto next=header_bytes+pool_address+pool::storage;
    auto remaining=memory.load32(count_again,at(pool_address,pool::count));
    memory.store32(free_clear,at(pool_address,pool::free_list),0u);
    if(signed_value(remaining)<=0)return Outcome::empty;
    do {
        memory.store32(Access::element_next,at(next,next_link),memory.load32(head,at(pool_address,pool::free_list)));
        memory.store32(head_store,at(pool_address,pool::free_list),next);
        next+=stride;
    } while(--remaining);
    return Outcome::linked;
}
} // namespace pool_reset
} // namespace halo2::native_pool_reset
