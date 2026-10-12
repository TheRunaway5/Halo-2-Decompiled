#pragma once
#include "halo2/object_physics_construction.hpp"
#include "halo2/raw_datum_pool.hpp"

namespace halo2 {
struct SourcePhysicsComponentConstructionState {
  std::uint32_t component_pool_slot,object_pool_slot,counter_slot,clock_slot;
};
struct PhysicsComponentConstructionConstants {std::uint32_t tick_scale_bits;};
struct PhysicsComponentConstructionFloatingPointContext {
  std::uint16_t x87_control_word;
  // Actual current SSE controls/status; native instructions update sticky
  // status here for subsequent source owners. Emulator status is not assigned.
  std::uint32_t mxcsr;
};
struct SourcePhysicsComponentConstructionServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
// Complete1D3340: actual signed16 clock input, source SSE multiply followed
// by x87 integer rounding, and complete component field initialization.
// GNU x86 backend needs the actual caller controls with exceptions masked;
// unsupported platforms stop. Source full x87 stack/sticky state is separate.
void source_physics_component_initialize(const SourcePhysicsComponentConstructionState &,
    std::uint32_t component,std::uint32_t object,const PhysicsComponentConstructionConstants &,
    PhysicsComponentConstructionFloatingPointContext &,
    const SourcePhysicsComponentConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
// Complete1D2AA0 composed with actual16D2C0 and1D3340, including dirty counter.
// Failed allocation retains the original address calculation and subsequent
// memory boundary; it does not silently skip initialization based on NONE.
std::uint32_t source_physics_component_allocate(const SourcePhysicsComponentConstructionState &,
    std::uint32_t object,const PhysicsComponentConstructionConstants &,
    PhysicsComponentConstructionFloatingPointContext &,
    const SourcePhysicsComponentConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
} // namespace halo2
