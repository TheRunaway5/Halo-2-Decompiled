#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
struct NativeTu15ConstraintAccumulator {std::uint32_t identity{};};
struct NativeTu15Constraint {std::uint32_t identity{};};
struct NativeTu15ConstraintStackAllocator {std::uint32_t identity{};};
// Actual four-word builder packet, passed by mutable semantic reference.
// Residual bindings must publish retained storage for genuine remaining
// builders and recover their actual changes before the next constraint.
struct NativeTu15ConstraintBuildContext {
    std::uint32_t jacobians{},schemas{},jacobian_limit{},secondary_jacobians{};
};
struct NativeTu15ConstraintStepServices {
    std::function<std::uint32_t(NativeTu15ConstraintStackAllocator,NativeTu15PhysicsVirtualOwner,std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> allocate_stack_block;
    std::function<void(NativeTu15ConstraintStackAllocator,NativeTu15PhysicsVirtualOwner,std::uint32_t block,std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> release_stack_block;
    std::function<NativeTu15ConstraintAccumulator(NativeTu15PhysicsMotion,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsStepPacket,NativeTu15PhysicsGravity,NativeTu15ConstraintAccumulator,NativeTu15PhysicsFrameFp&)> setup_motion;
    std::function<void(NativeTu15Constraint,NativeTu15PhysicsVirtualOwner,std::uint32_t jacobian_input,NativeTu15ConstraintBuildContext&,NativeTu15PhysicsFrameFp&)> build_constraint;
    std::function<std::uint32_t(std::uint32_t solver_configuration,std::uint32_t schemas,NativeTu15ConstraintAccumulator,std::uint32_t result_slots,NativeTu15PhysicsFrameFp&)> solve_rows;
    std::function<NativeTu15ConstraintAccumulator(NativeTu15PhysicsMotion,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsStepPacket,std::uint32_t solver_commit_parameter,NativeTu15ConstraintAccumulator,NativeTu15PhysicsFrameFp&)> commit_motion;
};
struct NativeTu15ConstraintStepStats {
    std::uint64_t islands{},native_accumulator_setups{},other_accumulator_setups{},constraint_builds{},solver_calls{},native_fixed_commits{},other_motion_commits{},allocated_blocks{},released_blocks{};
};
// Retained semantic state across the actual solver dependency. This state is
// produced by the native prefix; component qualification may also obtain it
// from authenticated original private locals at the genuine service return.
struct NativeTu15ConstraintStepContinuation {
    NativeTu15PhysicsConstraintInput input;
    NativeTu15ConstraintAccumulator first_accumulator;
    std::uint32_t body_end{},allocated_block{},arena_end{};
};
void native_tu15_finish_island_constraints(SourcePhysicsContactMemory&,
    const NativeTu15ConstraintStepContinuation&,std::uint32_t actual_solver_result,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15ConstraintStepServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintStepStats* = nullptr);
// Complete actual326200 workspace partition/growth, accumulator/constraint
// traversal, solver dispatch, profile/commit and lifetime algorithm. Actual
// motion setup323560/3248D0 and fixed commit324800 are closed native leaves.
// Other accumulator/commit kinds, builders, allocator virtual operations and
// core solver326A40 remain explicit named actual dependencies. No result is
// supplied by a reference fixture, and no CPU register/flags file is used.
void native_tu15_step_island_constraints(SourcePhysicsContactMemory&,
    const NativeTu15PhysicsConstraintInput&,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ConstraintStepServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintStepStats* = nullptr);
} // namespace halo2
