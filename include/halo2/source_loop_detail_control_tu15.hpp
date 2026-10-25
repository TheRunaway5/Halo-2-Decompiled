#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <span>
namespace halo2 {
struct SourceLoopDetailBoundsTu15 {
    std::array<float,2> yaw,pitch,distance;
};
// Complete223870 offset and the distinct221980/22216D deadline arithmetic.
// These consume the caller's actual secondary word, never an admission or
// listener result. The selected source profile uses x87CW027F/MXCSR1F80.
Vec3 source_loop_detail_offset_tu15(const SourceLoopDetailBoundsTu15&,std::uint32_t& seed);
struct SourceLoopDetailSpatialTu15 {Vec3 offset,position;float control{},gain_db{};};
SourceLoopDetailSpatialTu15 source_loop_detail_spatial_tu15(
    std::span<const std::byte,68> parent,const SourceLoopDetailBoundsTu15&,std::uint32_t& seed);
std::int32_t source_loop_detail_deadline_tu15(std::array<float,2> period,
    std::uint32_t flags,std::int32_t clock,std::uint32_t& seed,bool initial);
}
