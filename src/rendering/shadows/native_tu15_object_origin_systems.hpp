#pragma once
// Native TU1.5 object origin, entered at 30C40 (shadow frame unit): an
// object's origin and its +0x3C word, copied out.
//
//   object_origin::copy   30C40(ECX = object handle, EAX = point*, [ESP+4] = word*)
//
// The object's position (+0x30, three floats) is copied to the point and
// its word at +0x3C to the second output.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_origin.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_origin {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t position=0x30u,word=0x3cu;}

namespace object_origin {
enum class Access {headers,header_datums,object_address,position_read,position_store,word_read,word_target,word_store};
// Frame: argument() (the word pointer's slot), component(k) before each copy.
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t point) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(position_store,at(point,4u*k),memory.load32(position_read,at(address,object::position+4u*k)));
    }
    const auto word=memory.load32(word_read,at(address,object::word));
    memory.store32(word_store,memory.load32(word_target,frame.argument()),word);
}
} // namespace object_origin
} // namespace halo2::native_object_origin
