#pragma once
// Native TU1.5 matrix reset, entered at 144CB0 (sandbox special unit): a
// node matrix becomes a rotation at a position.
//
//   matrix_reset::reset   144CB0(EDX = matrix, ESI = position)
//
// The rotation (+4) is set by 144910; the matrix's position (+0x28) is
// cleared, its scale (+0) becomes one (46A0D0), and its position is then
// the given one. The matrix 144910 returns in EDX is the one completed.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_matrix_reset.hpp) maps the role to the original instruction
// and owns the ABI of the child 144910.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_matrix_reset {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}   // float
inline constexpr std::uint32_t rotation=4u,position=0x28u;

namespace matrix_reset {
enum class Access {position_clear_x,position_clear_y,position_clear_z,one,scale,source_x,target_x,source_y,target_y,
    source_z,target_z};
// The registers the original keeps.
struct Kept {std::uint32_t matrix;std::uint32_t source;};   // EDX (as 144910 returns it), ESI
// Frame:
//   rotate(kept)   144910(EAX the rotation); refreshes kept
template<class Memory,class Frame> void reset(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.rotate(kept);
    memory.store32(position_clear_x,at(kept.matrix,position),0u);
    memory.store32(position_clear_y,at(kept.matrix,position+4u),0u);
    memory.store32(position_clear_z,at(kept.matrix,position+8u),0u);
    memory.store32(scale,kept.matrix,memory.load32(one,globals::one));
    memory.store32(target_x,at(kept.matrix,position),memory.load32(source_x,kept.source));
    memory.store32(target_y,at(kept.matrix,position+4u),memory.load32(source_y,at(kept.source,4u)));
    memory.store32(target_z,at(kept.matrix,position+8u),memory.load32(source_z,at(kept.source,8u)));
}
} // namespace matrix_reset
} // namespace halo2::native_matrix_reset
