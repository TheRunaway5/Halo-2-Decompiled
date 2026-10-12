#pragma once
#include "halo2/physics_motion_step.hpp"
#include "halo2/physics_contact_batches.hpp"
#include "halo2/physics_solver.hpp"

namespace halo2 {
struct SourcePhysicsConstraintStepInput {
    std::uint32_t step, solver_configuration, jacobian_context, gravity;
    std::uint32_t island, bodies, body_count, constraints, constraint_count;
};
struct SourcePhysicsConstraintStepServices {
    SourcePhysicsReciprocalSqrtEstimate reciprocal_sqrt;
    // Opt in to the complete compiled solver. Its schema callback and
    // initialization dependencies remain required real owners.
    std::optional<SourcePhysicsSolverServices> solver;
    // Verified31BBE0/31BD00/322380 virtual producers. Unknown vtable entries
    // remain required owners; no synthesized Jacobian/schema substitute.
    std::optional<SourcePhysicsSolverServices> jacobians;
    // Actual allocator, unknown virtual producer and alternate motion owners;
    // also the solver/producers when their compiled modules are not connected.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsContactBatchOwnerCall &,
        const SourcePhysicsContactRecordObserver &)> required_owner;
};
// Complete322690 orchestration, including arena selection, dynamic-motion
// preparation/readback, Jacobian traversal and scratch release. A connected
// solver executes all of322ED0. Connected verified producers execute their
// complete original bodies; unknown or disconnected entries remain required.
void source_step_physics_constraints_1_1(SourcePhysicsContactMemory &,
    const SourcePhysicsConstraintStepInput &, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsConstraintStepServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
