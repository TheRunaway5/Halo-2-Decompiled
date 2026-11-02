#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
#include <map>
namespace halo2 {
struct NativeTu15SolverHullSingleVertexScratch {
    std::uint32_t scalar{},initialized_flag{};
    NativeTu15SolverHullInitializeScratch initialize;
};
struct NativeTu15SolverHullSingleVertexStats {
    std::uint64_t attempts{},completed{},refusals{},initial_count_refusals{},
        initializations{},returned_overlap{},returned_retry_limit{};
    NativeTu15SolverHullSupportStats support;
    std::map<std::uint32_t,std::uint64_t>refused_owners;
};
// Source313BB0's selected one-vertex reduction. Other initial hull sizes and
// Source3136A0 continuation are explicit refusals, never backend fallbacks.
// Compiler alloca/page-probing/SEH bytes are not physics DATA or native outputs.
std::uint32_t native_tu15_complete_single_vertex_solver_hull(
    SourcePhysicsContactMemory&,std::uint32_t context,std::uint32_t output_plane,
    NativeTu15SolverHullSingleVertexScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15SolverHullSupportServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullSingleVertexStats* = nullptr);
} // namespace halo2
