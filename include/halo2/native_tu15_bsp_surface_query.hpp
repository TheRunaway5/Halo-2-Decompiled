#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
using NativeTu15BspProjectedPoint=std::array<std::uint32_t,2>;
struct NativeTu15BspSurfaceQuery {
    std::uint32_t descriptor{},candidate_leaf{},origin{},displacement{};
    std::uint16_t bitmap_limit{};
    std::uint32_t bitmap{},plane_index{},fraction_bits{};
    bool verify_polygon{};
};
// Actual2A0230. Node and child encodings use the source low16 sign/leaf bits.
std::uint32_t native_tu15_find_bsp_surface_2d(SourcePhysicsContactMemory&,
    std::uint32_t tree,std::uint32_t node,std::uint32_t point,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
std::uint32_t native_tu15_find_bsp_surface_2d_value(SourcePhysicsContactMemory&,
    std::uint32_t tree,std::uint32_t node,const NativeTu15BspProjectedPoint&,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
// Actual1E6E80. Numeric public2Dpoint preserves lazy source reads; the native
// parent uses its initialized two-word arithmetic DATA without a guest frame.
bool native_tu15_point_in_bsp_surface(SourcePhysicsContactMemory&,
    std::uint32_t descriptor,std::uint32_t bitmap,std::uint16_t bitmap_limit,
    std::uint32_t surface,std::uint16_t axis,bool winding,std::uint32_t point,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
bool native_tu15_point_in_bsp_surface_value(SourcePhysicsContactMemory&,
    std::uint32_t descriptor,std::uint32_t bitmap,std::uint16_t bitmap_limit,
    std::uint32_t surface,std::uint16_t axis,bool winding,const NativeTu15BspProjectedPoint&,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
// Actual1E6CB0, including sole native2A0230/1E6E80 children. Source's dead
// argument-slot reuse is ordinary independent arithmetic DATA here.
std::uint32_t native_tu15_query_bsp_leaf_surface(SourcePhysicsContactMemory&,
    const NativeTu15BspSurfaceQuery&,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
} // namespace halo2
