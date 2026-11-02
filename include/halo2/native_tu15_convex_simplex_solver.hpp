#pragma once
#include "halo2/native_tu15_convex_simplex_single.hpp"
namespace halo2 {
struct NativeTu15ConvexSimplexSolverInput {
    std::uint32_t work{};
    NativeTu15ContactComponent first,second;
    std::uint32_t relative_transform{},support_cache{},plane{};
};
struct NativeTu15ConvexSimplexSolverScratch {
    // constant owns16bytes: normalization literals and rounded projection
    // threshold/trial scalars. The complete caller-owned arena stays live.
    std::uint32_t epsilon{},squared_length{},projection{},normalization{},constant{};
    NativeTu15ConvexSimplexSingleScratch single;
};
struct NativeTu15ConvexSimplexSolverServices {
    std::function<void(SourcePhysicsContactMemory&,NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,std::uint32_t feature_ids,
        std::uint32_t count,std::uint32_t output,NativeTu15PhysicsFrameFp&)> features;
    NativeTu15ConvexSimplexSingleServices single;
};
struct NativeTu15ConvexSimplexSolverStats {
    std::uint64_t attempts{},completed{},refusals{},feature_queries{},
        initial_count_refusals{},projection_refusals{},alternate_solver_refusals{};
    NativeTu15ConvexSimplexSingleStats single;
};
// Actual30E930 selected vertex/vertex and first-edge/vertex paths, including
// first-shape projection/support iteration with a fixed second vertex.
// On reduction, flag0 uses30DE60/count1; flag1
// keeps the current simplex and returns3 at30F5CF. The caller's work+0C
// flag is initialized DATA. Every other used work byte is defined by this
// kernel or its same-view native providers. Wider simplex/projection branches
// refuse explicitly. No CPU executor or raw private observation capability.
std::uint32_t native_tu15_solve_convex_contact_simplex(
    NativeTu15ConvexSimplexMemory&,NativeTu15ConvexSimplexSolverInput,
    NativeTu15ConvexSimplexSolverScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15ConvexSimplexSolverServices&,
    const std::function<std::uint64_t()>& timestamp_counter={},
    NativeTu15ConvexSimplexSolverStats* = nullptr);
} // namespace halo2
