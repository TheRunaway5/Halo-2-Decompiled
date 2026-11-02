#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
namespace halo2 {
struct NativeTu15SolverHullEdgeScratch {
    std::uint32_t first_pair{},transformed_direction{},negative_direction{},
        first_projection{},second_projection{};
};
struct NativeTu15SolverHullEdgeStats {
    std::uint64_t attempts{},completed{},refusals{},selected_first{},selected_second{};
    NativeTu15SolverHullSupportStats support;
};
// Actual313470. Publishes a support-pair48B and its scalar projection at+30.
void native_tu15_select_solver_hull_edge(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t direction,std::uint32_t origin,
    std::uint32_t output_pair,NativeTu15SolverHullEdgeScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15SolverHullSupportServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullEdgeStats* = nullptr);
} // namespace halo2
