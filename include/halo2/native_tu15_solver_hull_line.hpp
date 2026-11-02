#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
namespace halo2 {
struct NativeTu15SolverHullLineScratch {
    std::uint32_t first_pair{},second_pair{},transformed_direction{},
        negative_direction{},first_distance{},second_distance{};
};
struct NativeTu15SolverHullLineStats {
    std::uint64_t attempts{},completed{},refusals{},selected_first{},selected_second{};
    NativeTu15SolverHullSupportStats support;
};
// Actual3136A0: choose and append one of two real native support pairs.
// Scratch contains ordinary fully defined Vec4/scalar DATA, no CPU frame.
void native_tu15_extend_solver_hull_line(SourcePhysicsContactMemory&,
    std::uint32_t context,NativeTu15SolverHullLineScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15SolverHullSupportServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullLineStats* = nullptr);
} // namespace halo2
