#pragma once
#include "halo2/native_tu15_solver_hull_single_vertex.hpp"
namespace halo2 {
struct NativeTu15SolverHullQueryInput {
    NativeTu15ContactComponent first,second;
    std::uint32_t relative_transform{},simplex{},differences{},output_plane{};
};
struct NativeTu15SolverHullQueryScratch {
    // Aligned84B arithmetic context: four basis vectors, two shapes, simplex,
    // initialized field4C and differences pointer. No CPU/control slots.
    std::uint32_t context{};
    NativeTu15SolverHullSingleVertexScratch single_vertex;
};
struct NativeTu15SolverHullQueryStats {
    std::uint64_t attempts{},completed{},refusals{},profile_events{};
    NativeTu15SolverHullSingleVertexStats hull;
};
// Actual314710 wrapper; current partial hull child supports its qualified
// one-vertex source domain only. Wider children remain explicit refusals.
std::uint32_t native_tu15_query_solver_hull(SourcePhysicsContactMemory&,
    NativeTu15SolverHullQueryInput,NativeTu15SolverHullQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15SolverHullSupportServices&,
    const std::function<std::uint64_t()>& timestamp_counter={},
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullQueryStats* = nullptr);
} // namespace halo2
