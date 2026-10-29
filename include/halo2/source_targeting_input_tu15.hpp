#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
bool source_targeting_input_tu15_owns_entry(std::uint32_t);
// Complete1AD4F0 target producer and1AF490 position/velocity helper. The
// caller supplies the actual local user, preceding input/controller/observer
// and initialized output temporary.1AD4F0 intentionally leaves the output's
// vector/padding bytes untouched; its real1889D0 caller initializes them.
// Existing native owners execute every external call with the complete ABI.
void source_run_targeting_input_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
