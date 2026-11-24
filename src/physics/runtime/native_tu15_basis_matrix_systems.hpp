#pragma once
// Native TU1.5 basis matrix, entered at 144BF0 (AI frame and scene device
// constructor units): a node matrix from a forward axis, an up axis and a
// position.
//
//   basis_matrix::compose   144BF0(ECX = forward, EAX = up, ESI = position,
//                                  EDX = matrix)
//
// The matrix (52 bytes) gets the scale 1.0 (the global at 46A0D0), the
// forward axis, the left axis up x forward (each component with the
// original's operand order: x = fz*uy - uz*fy, y = uz*fx - fz*ux,
// z = fy*ux - fx*uy), the up axis, and the position, which is cleared to
// zero first and then copied.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_basis_matrix.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_basis_matrix {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;

namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}   // float 1.0
// The node matrix layout: scale, forward, left, up, position.
namespace matrix {inline constexpr std::uint32_t scale=0u,forward=4u,left=0x10u,up=0x1cu,position=0x28u;}

namespace basis_matrix {
enum class Access {
    one,scale_store,forward_x,forward_x_store,forward_y,forward_y_store,forward_z,forward_z_store,
    fy,fx,uy,ux,fz,ux_again,uz,fy_again,uz_again,fx_again,fz_again,uy_again,left_z_store,left_y_store,left_x_store,
    up_x,up_x_store,up_y,up_y_store,up_z,up_z_store,position_clear_x,position_clear_y,position_clear_z,
    position_x,position_x_store,position_y,position_y_store,position_z,position_z_store
};
// The cross product's intermediate values (the original's XMM1-XMM3).
struct Left {std::uint32_t x,y,z,uz_fy;};
// Frame: cleared() XORPS XMM0; crossed(left) the cross product's registers.
template<class Memory,class Frame> void compose(Memory &memory,Frame &frame,std::uint32_t forward,std::uint32_t up,
                                                std::uint32_t position,std::uint32_t target) {
    using enum Access;
    memory.store32(scale_store,at(target,matrix::scale),memory.load32(one,globals::one));
    memory.store32(forward_x_store,at(target,matrix::forward),memory.load32(forward_x,forward));
    memory.store32(forward_y_store,at(target,matrix::forward+4u),memory.load32(forward_y,forward+4u));
    memory.store32(forward_z_store,at(target,matrix::forward+8u),memory.load32(forward_z,forward+8u));
    // left = up x forward, loads and arithmetic in the original's order.
    const auto f_y=memory.load32(fy,forward+4u);
    const auto f_x=memory.load32(fx,forward);
    const auto fx_uy=sse::multiply(f_x,memory.load32(uy,up+4u));
    const auto fy_ux=sse::multiply(f_y,memory.load32(ux,up));
    const auto f_z=memory.load32(fz,forward+8u);
    const auto fz_ux=sse::multiply(f_z,memory.load32(ux_again,up));
    const auto u_z=memory.load32(uz,up+8u);
    const auto uz_fy=sse::multiply(u_z,memory.load32(fy_again,forward+4u));
    Left left{};
    left.z=sse::subtract(fy_ux,fx_uy);
    const auto u_z_again=memory.load32(uz_again,up+8u);
    left.y=sse::subtract(sse::multiply(u_z_again,memory.load32(fx_again,forward)),fz_ux);
    const auto f_z_again=memory.load32(fz_again,forward+8u);
    const auto fz_uy=sse::multiply(f_z_again,memory.load32(uy_again,up+4u));
    memory.store32(left_z_store,at(target,matrix::left+8u),left.z);
    frame.cleared();
    memory.store32(left_y_store,at(target,matrix::left+4u),left.y);
    left.x=sse::subtract(fz_uy,uz_fy);
    memory.store32(left_x_store,at(target,matrix::left),left.x);
    left.uz_fy=uz_fy;
    frame.crossed(left);
    memory.store32(up_x_store,at(target,matrix::up),memory.load32(up_x,up));
    memory.store32(up_y_store,at(target,matrix::up+4u),memory.load32(up_y,up+4u));
    memory.store32(up_z_store,at(target,matrix::up+8u),memory.load32(up_z,up+8u));
    memory.store32(position_clear_x,at(target,matrix::position),0);
    memory.store32(position_clear_y,at(target,matrix::position+4u),0);
    memory.store32(position_clear_z,at(target,matrix::position+8u),0);
    memory.store32(position_x_store,at(target,matrix::position),memory.load32(position_x,position));
    memory.store32(position_y_store,at(target,matrix::position+4u),memory.load32(position_y,position+4u));
    memory.store32(position_z_store,at(target,matrix::position+8u),memory.load32(position_z,position+8u));
}
} // namespace basis_matrix
} // namespace halo2::native_basis_matrix
