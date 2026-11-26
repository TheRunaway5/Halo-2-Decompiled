#pragma once
// Native TU1.5 transform product, entered at 145560 (scene device constructor,
// physics live support, AI frame, and effect particle frame units): compose
// two scaled 3x4 rigid transforms (scale + forward/left/up basis + position).
//
//   transform_product::compose   145560(ECX = left, EDX = right, [ESP+4] = out); RET 4
//
// Each 0x34-byte transform stores its uniform scale at +0x00, three 3-vector
// basis columns (forward +0x04, left +0x10, up +0x1C), and translation
// position at +0x28. The output stores:
//   out.scale = right.scale * left.scale
//   out.pos   = ((right.pos.x*left.fwd + right.pos.y*left.left) + right.pos.z*left.up) * left.scale + left.pos
//   out.col   = right.col.x*left.fwd + (right.col.y*left.left + right.col.z*left.up)
// evaluated in the original's exact packed SSE instruction order, loading each
// 3-vector as (x, 0, y, z) via MOVSS + MOVHPS and storing each output 3-vector
// in reverse column order (pos, up, left, forward, scale) with the next
// column's z-load interleaved before the current column's stores so aliasing
// (out == left or out == right) matches byte for byte.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_transform_product.hpp) maps the role to the original
// instruction and maintains XMM0..XMM7 on live MXCSR.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <immintrin.h>

namespace halo2::native_transform_product {
using native_object_update::at;
namespace field {
inline constexpr std::uint32_t scale=0x00u,
    fwd_x=0x04u,fwd_y=0x08u,fwd_z=0x0cu,
    left_x=0x10u,left_y=0x14u,left_z=0x18u,
    up_x=0x1cu,up_y=0x20u,up_z=0x24u,
    pos_x=0x28u,pos_y=0x2cu,pos_z=0x30u;
}
inline constexpr std::uint32_t out_argument_offset=4u;

namespace transform_product {
enum class Access {
    out_argument,left_scale,right_scale,right_pos_x,right_pos_y,right_pos_z,
    left_fwd_x,left_fwd_yz,left_left_x,left_left_yz,left_up_x,left_up_yz,
    left_pos_x,left_pos_yz,
    right_up_z,out_pos_x,out_pos_yz,right_up_y,right_up_x,
    right_left_z,out_up_x,out_up_yz,right_left_y,right_left_x,
    right_fwd_z,out_left_x,out_left_yz,right_fwd_y,right_fwd_x,
    out_fwd_x,out_fwd_yz,out_scale};

// Frame: see native_tu15_transform_product.hpp.
template<class Memory,class Frame> void compose(Memory &memory,Frame &frame,std::uint32_t left,std::uint32_t right) {
    using enum Access;
    const auto out=memory.load32(out_argument,frame.argument_slot());
    const auto left_s=frame.splat(7u,memory.load32(left_scale,at(left,field::scale)));
    const auto right_s=frame.scalar(3u,memory.load32(right_scale,at(right,field::scale)));
    const auto rpx=frame.scalar(0u,memory.load32(right_pos_x,at(right,field::pos_x)));
    const auto rpy=frame.scalar(1u,memory.load32(right_pos_y,at(right,field::pos_y)));
    const auto rpz=frame.scalar(2u,memory.load32(right_pos_z,at(right,field::pos_z)));
    const auto px=frame.broadcast(0u,rpx);
    const auto py=frame.broadcast(1u,rpy);
    const auto pz=frame.broadcast(2u,rpz);
    const auto composed_scale=frame.mul_scalar(3u,right_s,left_s);
    const auto lfx=frame.scalar(4u,memory.load32(left_fwd_x,at(left,field::fwd_x)));
    const auto lf=memory.load_half(left_fwd_yz,at(left,field::fwd_y),lfx,8u);
    const auto llx=frame.scalar(5u,memory.load32(left_left_x,at(left,field::left_x)));
    const auto ll=memory.load_half(left_left_yz,at(left,field::left_y),llx,8u);
    const auto lux=frame.scalar(6u,memory.load32(left_up_x,at(left,field::up_x)));
    const auto lu=memory.load_half(left_up_yz,at(left,field::up_y),lux,8u);
    const auto t0=frame.mul_packed(0u,px,lf);
    const auto t1=frame.mul_packed(1u,py,ll);
    const auto t2=frame.mul_packed(2u,pz,lu);
    const auto pos_scaled=frame.mul_packed(0u,frame.add_packed(0u,frame.add_packed(0u,t0,t1),t2),left_s);
    const auto lpx=frame.scalar(1u,memory.load32(left_pos_x,at(left,field::pos_x)));
    const auto lp=memory.load_half(left_pos_yz,at(left,field::pos_y),lpx,8u);
    const auto out_pos=frame.add_packed(0u,pos_scaled,lp);

    auto next_z=frame.scalar(2u,memory.load32(right_up_z,at(right,field::up_z)));
    memory.store32(out_pos_x,at(out,field::pos_x),frame.lane0(out_pos));
    memory.store_half(out_pos_yz,at(out,field::pos_y),out_pos,8u);

    auto transform_column=[&](Access y_role,std::uint32_t y_off,Access x_role,std::uint32_t x_off) {
        const auto ry=frame.scalar(1u,memory.load32(y_role,at(right,y_off)));
        const auto rx=frame.scalar(0u,memory.load32(x_role,at(right,x_off)));
        const auto bz=frame.broadcast(2u,next_z);
        const auto by=frame.broadcast(1u,ry);
        const auto bx=frame.broadcast(0u,rx);
        const auto cz=frame.mul_packed(2u,bz,lu);
        const auto cy=frame.mul_packed(1u,by,ll);
        const auto cx=frame.mul_packed(0u,bx,lf);
        const auto cyz=frame.add_packed(1u,cy,cz);
        return frame.add_packed(0u,cx,cyz);
    };

    const auto out_up=transform_column(right_up_y,field::up_y,right_up_x,field::up_x);
    next_z=frame.scalar(2u,memory.load32(right_left_z,at(right,field::left_z)));
    memory.store32(out_up_x,at(out,field::up_x),frame.lane0(out_up));
    memory.store_half(out_up_yz,at(out,field::up_y),out_up,8u);

    const auto out_left=transform_column(right_left_y,field::left_y,right_left_x,field::left_x);
    next_z=frame.scalar(2u,memory.load32(right_fwd_z,at(right,field::fwd_z)));
    memory.store32(out_left_x,at(out,field::left_x),frame.lane0(out_left));
    memory.store_half(out_left_yz,at(out,field::left_y),out_left,8u);

    const auto out_fwd=transform_column(right_fwd_y,field::fwd_y,right_fwd_x,field::fwd_x);
    memory.store32(out_fwd_x,at(out,field::fwd_x),frame.lane0(out_fwd));
    memory.store_half(out_fwd_yz,at(out,field::fwd_y),out_fwd,8u);
    memory.store32(out_scale,at(out,field::scale),frame.lane0(composed_scale));
}
} // namespace transform_product
} // namespace halo2::native_transform_product
