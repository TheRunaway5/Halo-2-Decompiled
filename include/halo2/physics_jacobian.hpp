#pragma once
#include "halo2/physics_solver.hpp"

namespace halo2 {
// Canonical source entry identity selects a fully translated producer/helper.
// Mutable body, contact and schema data come from its actual retained banks.
// Unknown entries and unowned CPU estimates cannot fabricate a result.
// EAX is defined only where a verified caller consumes it (326820 payload
// pointer and1D2D70 property lookup). Other entries are called for their bank
// outputs; unused EAX can contain platform x87 exception-status bits.
std::uint32_t source_build_physics_jacobians(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, std::uint32_t receiver,
    std::span<const std::uint32_t> arguments, std::uint32_t entry_esp,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsSolverServices & = {},
    const SourcePhysicsContactRecordObserver & = {}, SourcePhysicsSolverAbi = {});
} // namespace halo2
