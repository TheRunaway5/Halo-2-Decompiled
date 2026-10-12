#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/source_runtime_call.hpp"

namespace halo2 {
// The same retained x86 call record is shared with the physics native ports;
// it carries addresses and arithmetic state, never instructions or gameplay
// outputs. This API uses only the authenticated TU1.1 image layout.
struct SourceUnitAnimationRuntimeAbi : SourcePhysicsContactRuntimeAbi {
  std::array<std::uint64_t, 8> mmx;
};
enum class SourceUnitAnimationEstimate { Reciprocal, ReciprocalSquareRoot };
using SourceUnitAnimationRuntimeCall = SourcePhysicsContactRuntimeCall;
struct SourceUnitAnimationRuntimeServices {
  std::function<void(std::uint32_t)> instruction;
  // Literal CPU estimate instruction boundary, required only when sampled.
  // There is no software inverse, captured table or host estimate default.
  std::function<std::uint32_t(SourceUnitAnimationEstimate, std::uint32_t)> estimate;
  // Optional real cache-byte delivery at124F30. Owns the actual entry ABI;
  // when absent the complete native raw cache path executes.
  std::function<void(SourcePhysicsContactMemory &,
      const SourceUnitAnimationRuntimeCall &, SourceUnitAnimationRuntimeAbi &,
      const SourcePhysicsContactRecordObserver &)> resource_delivery;
  // Reached cache scheduler and platform clock own their complete actual ABI.
  // No IO/residency result or current time is supplied by this module.
  std::function<void(SourcePhysicsContactMemory &,
      const SourceUnitAnimationRuntimeCall &, SourceUnitAnimationRuntimeAbi &,
      const SourcePhysicsContactRecordObserver &)> required_owner;
  // Original reached profiling RDTSC instructions, separate from game time.
  std::function<std::uint64_t()> timestamp_counter;
};
// Complete raw graph normalization/resolution, fallback/secondary lookup,
// resource demand/queued allocation, linked variant/RNG selection, transition
// capture, request admission, overlay installation, local pose composition
// and complete packed sampling for all original codec table entries1..8,
// physical hierarchy/quaternion delivery and current-node type callbacks. Bodies
// compile to static native C++ paths; no production instruction decoder runs.
void source_run_unit_animation_selection_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual local/physical/current-node and notification call adapters. All pose/metadata,
// scratch and source global banks remain owned by memory. CPU estimates and
// actual cache delivery are explicit services, shared with the caller.
std::uint32_t source_call_unit_animation_pose_1_1(
    SourcePhysicsContactMemory &, const SourceRuntimeCall &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Defined-input adapter used by the native unit phase and channel clocks.
// Preserved incidental register values are not logical dependencies.
std::uint32_t source_call_unit_animation_selection_1_1(
    SourcePhysicsContactMemory &, const SourceRuntimeCall &,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
