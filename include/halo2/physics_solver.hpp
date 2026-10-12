#pragma once
#include "halo2/physics_contact_batches.hpp"
#include "halo2/physics_motion_step.hpp"

namespace halo2 {
struct SourcePhysicsSolverServices {
    // Optional diagnostic instruction-site observer; no result or state input.
    std::function<void(std::uint32_t)> instruction;
    // Only actual schema callback, initialization clock and error owners.
    // Solver math and initialization/checksum execute in compiled C++.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsContactBatchOwnerCall &,
        const SourcePhysicsContactRecordObserver &)> required_owner;
    // Actual CPU estimate owner used by Jacobian production. Component
    // comparisons explicitly supply emulator observations; no host fallback.
    SourcePhysicsReciprocalSqrtEstimate reciprocal_sqrt;
};
struct SourcePhysicsSolverAbi {
    // Preserved guest values are only retained call-frame inputs. They never
    // supply a body, Jacobian, impulse, initialization flag or solve result.
    std::uint32_t ebx{}, ebp{}, esi{}, edi{};
};
std::uint32_t source_solve_physics_constraints(SourcePhysicsContactMemory &,
    std::uint32_t configuration, std::uint32_t schemas, std::uint32_t accumulators,
    std::uint32_t impulses, std::uint32_t entry_esp, SourcePhysicsContactImage,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsSolverServices & = {},
    const SourcePhysicsContactRecordObserver & = {}, SourcePhysicsSolverAbi = {});
} // namespace halo2
