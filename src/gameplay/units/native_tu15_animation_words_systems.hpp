#pragma once
// Native TU1.5 animation words, entered at 111D10 (physics live support and
// AI frame units): two words of an object's animation block, when the block
// is in use.
//
//   animation_words::read   111D10(EAX = object handle, ESI = first*, EDX = second*) -> AL
//
// The block sits at the object's signed offset +0x12A. It is in use when its
// graph (+0x68), its handle (+0) and its animation index (the word +6) are
// all set (not NONE); then its +0x70 and +0x7C are written out and the
// result is 1. Otherwise both outputs become NONE and the result is 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_words.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_words {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,block_offset=0x12au;
namespace block {inline constexpr std::uint32_t handle=0u,index=6u,graph=0x68u,first=0x70u,second=0x7cu;}

namespace animation_words {
enum class Access {headers,header_datums,object_address,block,graph,handle,index,first,first_store,second,second_store,
    first_clear,second_clear};
template<class Memory> bool read(Memory &memory,std::uint32_t object_handle,std::uint32_t first_out,std::uint32_t second_out) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(object_handle&0xffffu)+header_address);
    const auto animation=widen16(memory.load16(Access::block,at(address,block_offset)))+address;
    if(memory.load32(graph,at(animation,block::graph))==none || memory.load32(handle,at(animation,block::handle))==none ||
       (memory.load16(index,at(animation,block::index))&0xffffu)==0xffffu) {
        memory.store32(first_clear,first_out,none);
        memory.store32(second_clear,second_out,none);
        return false;
    }
    memory.store32(first_store,first_out,memory.load32(first,at(animation,block::first)));
    memory.store32(second_store,second_out,memory.load32(second,at(animation,block::second)));
    return true;
}
} // namespace animation_words
} // namespace halo2::native_animation_words
