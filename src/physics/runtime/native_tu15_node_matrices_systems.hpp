#pragma once
// Native TU1.5 node matrices, entered at BAC30 (sandbox special and physics
// live support units): where an object's node matrices are and how many.
//
//   node_matrices::of   BAC30(EAX = object handle; count*) -> EAX
//
// The object's node matrices start at its int16 offset +0x116; their size
// in bytes is the int16 +0x114, and the count (written through the
// argument) is that size over the 0x34-byte matrix, as the original's
// unsigned multiply by 4EC4EC4F and shift by 4 computes it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_node_matrices.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_node_matrices {
using native_object_update::at;
using native_object_update::object_of;
using native_object_update::widen16;
inline constexpr std::uint32_t matrices_size=0x114u,matrices_offset=0x116u;
inline constexpr std::uint32_t reciprocal=0x4ec4ec4fu,reciprocal_shift=4u;   // / 0x34, with MUL's high half
constexpr std::uint32_t matrix_count(std::uint32_t bytes) {
    return std::uint32_t((std::uint64_t(bytes)*reciprocal)>>32)>>reciprocal_shift;
}

namespace node_matrices {
enum class Local {count};
enum class Access {object_table,object_datums,object_address,size,count_argument,count_store,offset};
template<class Memory,class Frame> std::uint32_t of(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto bytes=widen16(memory.load16(size,at(object,matrices_size)));
    const auto count=memory.load32(count_argument,frame.local(Local::count));
    memory.store32(count_store,count,matrix_count(bytes));
    return object+widen16(memory.load16(offset,at(object,matrices_offset)));
}
} // namespace node_matrices
} // namespace halo2::native_node_matrices
