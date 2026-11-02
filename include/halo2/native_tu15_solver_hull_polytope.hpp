#pragma once
#include "halo2/native_tu15_solver_hull_support.hpp"
namespace halo2 {
struct NativeTu15SolverHullPolytopeStats {
    std::uint64_t minimum_calls{},minimum_completed{},facet_candidates{},
        sample_calls{},sample_completed{},refusals{};
};
// Actual3131B0; returns a current facet DATA identity, never a CPU register file.
std::uint32_t native_tu15_minimum_solver_hull_facet(SourcePhysicsContactMemory&,
    std::uint32_t polytope,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    NativeTu15SolverHullPolytopeStats* = nullptr);
// Actual313300: source-defined integer recurrence and three x87 samples.
// Source returns the third recurrence value; callers may discard it.
std::uint32_t native_tu15_sample_solver_hull_direction(SourcePhysicsContactMemory&,
    std::uint32_t seed,std::uint32_t lower_bits,std::uint32_t upper_bits,
    std::uint32_t output_xyz,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullPolytopeStats* = nullptr);
} // namespace halo2
