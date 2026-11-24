#pragma once
// Native TU1.5 orientation frames (physics live support unit): a 0x34-byte
// frame built from a forward and an up vector, and a vector projected onto
// a frame's axes.
//
//   orientation_frame::build   144210(ECX = frame*, EDX = forward*, EAX = up*)
//   frame_projection::project  1452F0(ECX = frame*, EAX = vector*, EDX = out*) -> EAX out
//
// The frame is a scale (+0, 1.0), the forward vector (+4), the left vector
// (+0x10, up x forward), the up vector (+0x1C) and a zero position (+0x28).
// The projection is the vector's dot products with the forward, left and up
// axes, the vector first divided by the frame's scale unless the scale
// equals the constant 1 (ordered).
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order; each input is read where the original reads it, between the
// stores, so an output over its inputs reads what was stored.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_orientation_frame.hpp, native_tu15_frame_projection.hpp)
// maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_orientation_frame {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}   // float 1.0
namespace layout {inline constexpr std::uint32_t scale=0u,forward=4u,left=0x10u,up=0x1cu,position=0x28u;}

namespace orientation_frame {
enum class Access {
    one,scale_store,forward,forward_store,
    // The cross product's inputs, in the original's order.
    forward_y_z,up_x_z,forward_x_z,up_y_z,forward_z_y,up_x_y,up_z_x,forward_y_x,up_z_y,forward_x_y,forward_z_x,up_y_x,
    left_z_store,left_y_store,left_x_store,up,up_store,position_store
};
// The XMM registers the original leaves.
struct Registers {std::uint32_t xmm1,xmm2,xmm3;};
// Frame: saved() the pushes; component(k) before each copied component;
// built(registers) at the end.
template<class Memory,class Frame> void build(Memory &memory,Frame &frame,std::uint32_t out,std::uint32_t ahead,
                                              std::uint32_t up_vector) {
    using enum Access;
    memory.store32(scale_store,at(out,layout::scale),memory.load32(Access::one,globals::one));
    frame.saved();
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(forward_store,at(out,layout::forward+4u*k),memory.load32(Access::forward,at(ahead,4u*k)));
    }
    const auto fy=memory.load32(forward_y_z,at(ahead,4u));
    const auto fy_ux=sse::multiply(fy,memory.load32(up_x_z,up_vector));
    const auto fx=memory.load32(forward_x_z,ahead);
    const auto fx_uy=sse::multiply(fx,memory.load32(up_y_z,at(up_vector,4u)));
    const auto fz=memory.load32(forward_z_y,at(ahead,8u));
    const auto fz_ux=sse::multiply(fz,memory.load32(up_x_y,up_vector));
    const auto uz=memory.load32(up_z_x,at(up_vector,8u));
    const auto uz_fy=sse::multiply(uz,memory.load32(forward_y_x,at(ahead,4u)));
    const auto left_z=sse::subtract(fy_ux,fx_uy);
    const auto uz_again=memory.load32(up_z_y,at(up_vector,8u));
    const auto uz_fx=sse::multiply(uz_again,memory.load32(forward_x_y,ahead));
    const auto left_y=sse::subtract(uz_fx,fz_ux);
    const auto fz_again=memory.load32(forward_z_x,at(ahead,8u));
    const auto fz_uy=sse::multiply(fz_again,memory.load32(up_y_x,at(up_vector,4u)));
    memory.store32(left_z_store,at(out,layout::left+8u),left_z);
    memory.store32(left_y_store,at(out,layout::left+4u),left_y);
    const auto left_x=sse::subtract(fz_uy,uz_fy);
    memory.store32(left_x_store,at(out,layout::left),left_x);
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        const auto value=memory.load32(Access::up,at(up_vector,4u*k));
        memory.store32(up_store,at(out,layout::up+4u*k),value);
    }
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(position_store,at(out,layout::position+4u*k),0u);
    }
    frame.built(Registers{left_y,left_x,uz_fy});
}
} // namespace orientation_frame

namespace frame_projection {
enum class Access {
    scale,one,x,y,z,
    // The axes' components, in the original's order (z, y, x of each axis).
    forward_z,forward_y,forward_x,forward_store,left_z,left_y,left_x,left_store,up_z,up_y,up_x,up_store
};
// The XMM registers the original leaves.
struct Registers {std::uint32_t xmm0,xmm1,xmm2,xmm3,xmm4;};
// Frame: compared(scale, one) the scale tested; built(registers) at the end.
template<class Memory,class Frame> void project(Memory &memory,Frame &frame,std::uint32_t axes,std::uint32_t vector,
                                               std::uint32_t out) {
    using enum Access;
    const auto scale_value=memory.load32(Access::scale,at(axes,layout::scale));
    const auto one_value=memory.load32(Access::one,globals::one);
    auto vx=memory.load32(Access::x,vector);
    auto vy=memory.load32(Access::y,at(vector,4u));
    auto vz=memory.load32(Access::z,at(vector,8u));
    frame.compared(scale_value,one_value);
    if(native_perpendicular::unequal(scale_value,one_value)) {
        const auto inverse=sse::divide(one_value,scale_value);
        vx=sse::multiply(inverse,vx);vy=sse::multiply(inverse,vy);vz=sse::multiply(inverse,vz);
    }
    // (axis.z * z + axis.y * y) + axis.x * x, each axis read z first.
    struct Dot {std::uint32_t sum,y_product,x_product;};
    const auto dot=[&](std::uint32_t axis,Access z_role,Access y_role,Access x_role) {
        const auto z_component=memory.load32(z_role,at(axes,axis+8u));
        const auto z_product=sse::multiply(z_component,vz);
        const auto y_component=memory.load32(y_role,at(axes,axis+4u));
        const auto y_product=sse::multiply(y_component,vy);
        const auto partial=sse::add(z_product,y_product);
        const auto x_component=memory.load32(x_role,at(axes,axis));
        const auto x_product=sse::multiply(x_component,vx);
        return Dot{sse::add(partial,x_product),y_product,x_product};
    };
    const auto forward=dot(layout::forward,forward_z,forward_y,forward_x);
    memory.store32(forward_store,out,forward.sum);
    const auto left=dot(layout::left,left_z,left_y,left_x);
    memory.store32(left_store,at(out,4u),left.sum);
    const auto up=dot(layout::up,up_z,up_y,up_x);
    memory.store32(up_store,at(out,8u),up.sum);
    frame.built(Registers{up.sum,vx,up.x_product,up.y_product,left.x_product});
}
} // namespace frame_projection
} // namespace halo2::native_orientation_frame
