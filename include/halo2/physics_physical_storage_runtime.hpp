#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
namespace halo2 {
// Complete physical stage/partition and allocation/protection wrappers.
void source_run_physics_physical_storage_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourcePhysicsPhysicalResourceEvent {
  bool allocation;
  std::array<std::uint32_t,5> request;
  std::uint32_t result;
};
using SourcePhysicsPhysicalResourceObserver=std::function<void(const SourcePhysicsPhysicalResourceEvent &)>;
// TU1.5 actual import frames over the existing native physical-page owner.
// Explicit native stdcall preserves GP/FP/flags except EAX/ESP on allocation;
// protection consumes stack only. This does not claim Xbox kernel clobbers.
void source_run_physics_physical_storage_with_native_pages(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    SourceRuntimeHostPhysicalMemory &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsPhysicalResourceObserver & = {});
} // namespace halo2
