#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"

namespace halo2 {
struct NativeTu15SolverHullPolytopeStateStats {
    std::uint64_t clear_calls{},clear_completed{},facets_cleared{},edges_cleared{},
        clip_calls{},clip_completed{},refusals{};
};
// Actual314870. Both array pointers and counts are current DATA, reread after
// each publication. A source store may therefore change later loop admission.
void native_tu15_clear_solver_hull_polytope_flags(SourcePhysicsContactMemory&,
    std::uint32_t polytope,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullPolytopeStateStats* = nullptr);
// Actual3147D0. The four-lane point and facet vectors are ordinary initialized
// DATA; lower/upper identities retain Source alias and current-read semantics.
std::uint32_t native_tu15_clip_solver_hull_facet(SourcePhysicsContactMemory&,
    std::uint32_t facet,std::uint32_t point,std::uint32_t lower,std::uint32_t upper,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullPolytopeStateStats* = nullptr);
} // namespace halo2
