#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Test-only compiled copies of existing complete native owners. No callee result
// is substituted; the ordinary world retains its existing production providers.
bool source_ctf_activation_test_support_owns_entry(std::uint32_t);
void source_run_ctf_activation_test_support(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
