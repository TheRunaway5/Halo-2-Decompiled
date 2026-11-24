#pragma once
// Native TU1.5 object axes, entered at BBFF0 (physics live support and AI
// frame units): an object's forward (+0x70) and up (+0x7C) in the world.
//
//   object_axes::of   BBFF0(EAX = object handle, ESI = forward* or 0, EDI = up* or 0)
//
// An object without a parent (+0x14 NONE) holds its axes in world terms:
// they are copied. Otherwise they are turned by the parent's node matrix
// (the parent's node block, its signed offset +0x116, and the node the
// object's signed byte +0x18 names, 0x34 bytes apiece): each component of
// the result is the matrix's column (+4, +0x10, +0x1C for x; +8, +0x14,
// +0x20 for y; +0xC, +0x18, +0x24 for z) times the axis, summed z, y, x.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order (the matrix element the destination), on the live MXCSR. This is
// the game logic only. The game's memory is the retained TU1.5 bank, reached
// through a Memory object with typed loads and stores; every access names
// its role (Access), and the boundary adapter (native_tu15_object_axes.hpp)
// maps the role to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_object_axes {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t parent=0x14u,parent_node=0x18u,forward=0x70u,up=0x7cu,nodes=0x116u;}
inline constexpr std::uint32_t node_size=0x34u;
// The matrix elements each result component takes: its z, y and x factors.
namespace element {
inline constexpr std::uint32_t x_by_z=0x1cu,x_by_y=0x10u,x_by_x=4u,y_by_z=0x20u,y_by_y=0x14u,y_by_x=8u,
    z_by_z=0x24u,z_by_y=0x18u,z_by_x=0xcu;
}

namespace object_axes {
enum class Axis {forward,up};
enum class Access {
    headers,header_datums,object_address,parent,axis_copy,axis_store,parent_node,parent_address,nodes,
    axis_y,axis_z,x_by_z,x_by_y,axis_x,x_by_x,result_x,y_by_z,y_by_y,y_by_x,result_y,z_by_z,z_by_y,z_by_x,result_z
};
// The registers a turn leaves.
struct Turned {std::uint32_t xmm0,xmm1,xmm2,xmm3,xmm4;};
enum class Outcome {copied,turned};
// Frame:
//   tested(axis, output)   TEST ESI/EDI: whether the axis is wanted
//   copying(axis)          the copy of one axis (no parent)
//   turning(axis)          the turn of one axis
//   turned(Turned)         the registers after a turn
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t forward_out,
                                              std::uint32_t up_out) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto parent_handle=memory.load32(parent,at(address,object::parent));
    const Axis axes[]{Axis::forward,Axis::up};
    const std::uint32_t outputs[]{forward_out,up_out},sources[]{object::forward,object::up};
    if(parent_handle==none) {
        for(unsigned a=0;a<2u;++a) {
            if(!frame.tested(axes[a],outputs[a]))continue;
            frame.copying(axes[a]);
            for(std::uint32_t k=0;k<3u;++k)
                memory.store32(axis_store,at(outputs[a],4u*k),memory.load32(axis_copy,at(address,sources[a]+4u*k)));
        }
        return Outcome::copied;
    }
    const auto node_index=std::uint32_t(std::int32_t(std::int8_t(memory.load8(parent_node,at(address,object::parent_node)))));
    const auto parent_object=memory.load32(parent_address,datums+header_size*(parent_handle&0xffffu)+header_address);
    const auto matrix=widen16(memory.load16(nodes,at(parent_object,object::nodes)))+(node_size*node_index+parent_object);
    for(unsigned a=0;a<2u;++a) {
        if(!frame.tested(axes[a],outputs[a]))continue;
        frame.turning(axes[a]);
        const auto source=at(address,sources[a]),out=outputs[a];
        const auto y=memory.load32(axis_y,at(source,4u));
        const auto z=memory.load32(axis_z,at(source,8u));
        // x: the z and y terms, then the x term, in the original's order.
        const auto x_z_element=memory.load32(x_by_z,at(matrix,element::x_by_z));
        const auto x_y_element=memory.load32(x_by_y,at(matrix,element::x_by_y));
        const auto x=memory.load32(axis_x,source);
        const auto x_z=sse::multiply(x_z_element,z);
        const auto x_y=sse::multiply(x_y_element,y);
        auto sum=sse::add(x_z,x_y);
        sum=sse::add(sum,sse::multiply(memory.load32(x_by_x,at(matrix,element::x_by_x)),x));
        memory.store32(result_x,at(out,0u),sum);
        // y.
        const auto y_z_element=memory.load32(y_by_z,at(matrix,element::y_by_z));
        const auto y_y_element=memory.load32(y_by_y,at(matrix,element::y_by_y));
        const auto y_z=sse::multiply(y_z_element,z);
        const auto y_y=sse::multiply(y_y_element,y);
        sum=sse::add(y_z,y_y);
        const auto y_x=sse::multiply(memory.load32(y_by_x,at(matrix,element::y_by_x)),x);
        sum=sse::add(sum,y_x);
        memory.store32(result_y,at(out,4u),sum);
        // z.
        const auto z_z=sse::multiply(memory.load32(z_by_z,at(matrix,element::z_by_z)),z);
        const auto z_y=sse::multiply(memory.load32(z_by_y,at(matrix,element::z_by_y)),y);
        const auto z_x_element=memory.load32(z_by_x,at(matrix,element::z_by_x));
        sum=sse::add(z_z,z_y);
        const auto z_x=sse::multiply(z_x_element,x);
        sum=sse::add(sum,z_x);
        memory.store32(result_z,at(out,8u),sum);
        frame.turned({x,z_x,z_y,sum,y_x});
    }
    return Outcome::turned;
}
} // namespace object_axes
} // namespace halo2::native_object_axes
