#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <map>
#include <optional>
namespace halo2 {
bool source_object_functions_tu15_owns_entry(std::uint32_t);
void source_run_object_functions_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
void source_object_function_set_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,
    std::uint32_t name,float value,float frames);
void source_object_function_clear_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,std::uint32_t name);
std::optional<float> source_object_function_value_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t object,std::uint32_t name);
// Read-only samples of the original32 slots at the actual source game clock.
// Presentation borrows these values; it owns no replacement timer/eviction bank.
std::map<std::pair<std::uint32_t,std::uint32_t>,float>
    source_object_function_samples_tu15(PhysicsLiveWorldTu15 &);
}
