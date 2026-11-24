#pragma once
// Native TU1.5 polygon inside, entered at 243510 (light portal and physics
// live support units): whether a point lies inside a convex polygon.
//
//   polygon_inside::contains   243510(EBX = point, EDX = vertices; word count, tolerance) -> AL, RET 8
//
// The polygon's vertices are (x, y) float pairs. The point is inside when,
// for each edge (each vertex to the next, the last back to the first), the
// cross product of the point's offset from the vertex with the edge,
// (p.y - v.y) * (n.x - v.x) - (p.x - v.x) * (n.y - v.y), is not below minus
// the tolerance. A polygon of no vertices holds every point.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), and the boundary
// adapter (native_tu15_polygon_inside.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_polygon_inside {
using native_object_update::at;
using native_object_update::widen16;
using native_object_update::signed_value;
namespace sse=native_aim_clusters::sse;
inline constexpr std::uint32_t vertex_size=8u,zero_bits=0u;

namespace polygon_inside {
enum class Access {count,tolerance,count_again,point_x,point_y,vertex_y,vertex_x,next_x,next_y,edge_x,edge_y,count_check};
enum class Outcome {empty,inside,outside};
struct Cross {std::uint32_t dy,dx,ex,ey,product,cross;};
// Frame: argument(k) the k-th argument's slot; floored(floor) the floor's register;
// crossed(i, cross) each edge's registers.
template<class Memory,class Frame> Outcome contains(Memory &memory,Frame &frame,std::uint32_t point,std::uint32_t vertices) {
    using enum Access;
    if(std::int16_t(memory.load16(count,frame.argument(1u)))<=0)return Outcome::empty;
    const auto floor=sse::subtract(zero_bits,memory.load32(tolerance,frame.argument(2u)));
    frame.floored(floor);
    for(std::uint32_t i=0;;) {
        const auto vertex_count=widen16(memory.load16(count_again,frame.argument(1u)));
        const auto x=memory.load32(point_x,at(point,0u));
        const auto y=memory.load32(point_y,at(point,4u));
        const auto vertex=vertices+vertex_size*widen16(i&0xffffu);
        Cross c{};
        c.dy=sse::subtract(y,memory.load32(vertex_y,at(vertex,4u)));
        c.dx=sse::subtract(x,memory.load32(vertex_x,at(vertex,0u)));
        const auto following=widen16(i&0xffffu)+1u;
        const auto next=vertices+vertex_size*(signed_value(following)>=signed_value(vertex_count)?0u:following);
        const auto next_x=memory.load32(Access::next_x,at(next,0u));
        const auto next_y=memory.load32(Access::next_y,at(next,4u));
        c.ex=sse::subtract(next_x,memory.load32(edge_x,at(vertex,0u)));
        c.ey=sse::subtract(next_y,memory.load32(edge_y,at(vertex,4u)));
        c.dy=sse::multiply(c.dy,c.ex);
        c.product=sse::multiply(c.dx,c.ey);
        c.cross=sse::subtract(c.dy,c.product);
        frame.crossed(i,c);
        if(!sse::not_above(floor,c.cross))return Outcome::outside;
        ++i;
        if(std::int16_t(i&0xffffu)>=std::int16_t(memory.load16(count_check,frame.argument(1u))))return Outcome::inside;
    }
}
} // namespace polygon_inside
} // namespace halo2::native_polygon_inside
