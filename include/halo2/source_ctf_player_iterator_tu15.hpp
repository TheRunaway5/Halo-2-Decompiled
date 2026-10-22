#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original TU1.5 player iterator1A8F10. The actual pool/row/unit
// state and the required native16EA60 callee remain caller-owned inputs.
bool source_ctf_player_iterator_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_ctf_player_iterator_tu15(
    SourcePhysicsContactMemory &memory, std::uint32_t actual_entry,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint floating_point,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observer = {});
} // namespace halo2
