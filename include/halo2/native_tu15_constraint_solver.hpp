#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
namespace halo2 {
struct NativeTu15ConstraintSolverInput {
    std::uint32_t configuration{},schemas{};
    NativeTu15ConstraintAccumulator accumulators;
    std::uint32_t result_slots{};
};
struct NativeTu15ConstraintSolverServices {
    // Actual3268D0 owns ready48D480 and all initialization side effects. The
    // solver rereads its ready byte afterward; no bool/success is supplied.
    std::function<void(NativeTu15PhysicsFrameFp&)> initialize;
    // Actual primary schema14 cdecl callback(row, schema+8). Its return value
    // is unused. The source rereads schema length after the genuine callback.
    std::function<void(NativeTu15PhysicsVirtualOwner,std::uint32_t jacobian,
                       std::uint32_t payload,NativeTu15PhysicsFrameFp&)> custom_schema;
};
struct NativeTu15ConstraintSolverStats {
    std::uint64_t calls{},initializations{},iterations{},primary_dispatches{},
        secondary_rows{},exported_rows{},custom_schemas{};
};
// Typed semantic solver for actual326A40. Initializer and customschema14 are
// explicit remaining real dependencies. No guest CPU file or instruction
// dispatcher is used. Full positive-iteration physical qualification is
// required before replacing the current production whole-solver residual.
std::uint32_t native_tu15_solve_constraint_rows(SourcePhysicsContactMemory&,
    const NativeTu15ConstraintSolverInput&,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ConstraintSolverServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintSolverStats* = nullptr);
} // namespace halo2
