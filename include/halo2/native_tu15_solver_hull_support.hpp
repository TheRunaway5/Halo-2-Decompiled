#pragma once
#include "halo2/native_tu15_contact_shape.hpp"
namespace halo2 {
// Source313290's three input Vec4 values are ordinary point-pair DATA.
// Context+48 owns the simplex {first[4],second[4],count}; context+50 owns
// the difference vertices. All addresses are typed DATA, never CPU state.
struct NativeTu15SolverHullSupportStats {
    std::uint64_t append_calls{},append_completed{},refusals{};
    std::uint64_t pair_calls{},pair_completed{},shape_support_calls{};
    std::uint64_t initialize_calls{},initialize_completed{};
};
struct NativeTu15SolverHullSupportServices {
    std::function<void(NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,
        std::uint32_t direction,std::uint32_t output,NativeTu15PhysicsFrameFp&)>support;
};
struct NativeTu15SolverHullInitializeScratch {
    std::uint32_t scalar{},direction{},point_pair{},transformed_direction{},squared_vector{};
};
// Actual313560; returns the same ordinary output-flag DATA identity. The flag
// is fully written on both branches; the plane is published only on success.
std::uint32_t native_tu15_initialize_solver_hull(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t output_flag,std::uint32_t output_plane,
    NativeTu15SolverHullInitializeScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15SolverHullSupportServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullSupportStats* = nullptr);
// Actual313390. The sixteen-byte transformed direction is caller-owned DATA
// with a lifetime through the second support callback; it is not a CPU frame.
void native_tu15_solver_hull_support_pair(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t direction,std::uint32_t point_pair,
    std::uint32_t transformed_direction,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15SolverHullSupportServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullSupportStats* = nullptr);
void native_tu15_append_solver_hull_vertex(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t point_pair,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SolverHullSupportStats* = nullptr);
} // namespace halo2
