#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/transform.hpp"
#include <array>
#include <vector>

namespace halo2 {
// Actual TU1.5 183C10 output: one 64-byte page contains four XYZ/CMP vertices.
// The signed 11/11/10 components are projection UV and baked brightness.
// This is a read-only geometry export, not the original decal draw scheduler.
struct SourceDecalCacheVertex {
    Vec3 position;
    std::array<float,3> projection;
    std::uint32_t packed_projection;
};
struct SourceDecalCacheGeometry {
    std::uint32_t datum,definition,cache_datum,tick,tint;
    std::int16_t cluster;
    std::uint8_t layer,flags;
    float lifetime,decay;
    std::vector<std::array<SourceDecalCacheVertex,4>> quads;
    std::uint32_t primary_datum{},next_cluster_datum{};
    std::int8_t bitmap_index{};
};
// Source owns all pools, salts, cache pages and clipping. Requires a quiescent
// completed source update. No records or cache allocations are synthesized.
std::vector<SourceDecalCacheGeometry>
source_decal_vertex_cache_tu15(const SourceRuntimeMemory &);
} // namespace halo2
