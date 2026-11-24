#pragma once
// Native TU1.5 object flag bit, entered at E9530 (physics live support
// unit): whether one bit of an object's flag bits is set.
//
//   object_flag_bit::test   E9530(EAX = object handle, ECX = bit) -> EAX
//
// The object's flag bits are 32-bit words that start 4 bytes into the bit
// block at the object's int16 offset +0x346; the bit's word is bit >> 5
// (arithmetic), its bit bit & 31. The result is 1 when it is set, else 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_flag_bit.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_flag_bit {
using native_object_update::at;
using native_object_update::object_of;
using native_object_update::widen16;
inline constexpr std::uint32_t flag_bits_offset=0x346u,first_word=4u;

namespace object_flag_bit {
enum class Access {object_table,object_datums,object_address,bits_offset,bit_word};
template<class Memory> std::uint32_t test(Memory &memory,std::uint32_t handle,std::uint32_t bit) {
    using enum Access;
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto bits=object+widen16(memory.load16(bits_offset,at(object,flag_bits_offset)));
    const auto word=std::uint32_t(std::int32_t(bit)>>5);
    return (memory.load32(bit_word,bits+first_word+4u*word)>>(bit&31u))&1u;
}
} // namespace object_flag_bit
} // namespace halo2::native_object_flag_bit
