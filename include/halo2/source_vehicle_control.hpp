#pragma once
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// Complete Map Pack 1.1 208D80 / TU1.5 20D680 scalar vehicle rate control.
// Inputs are raw float words; rates address the original four authored values:
// positive maximum, reverse maximum, acceleration and deceleration.
// Preserves every value store, including repeated/clamping stores and aliasing
// with rates. Caller scratch/register/x87-stack ownership is outside this API.
void source_vehicle_control_accelerate(
    SourcePhysicsContactMemory &, std::uint32_t value, std::uint32_t rates,
    std::uint32_t direction_word, std::uint32_t dt_word,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete 208E70 / 20D770 target approach, including its real rate callee.
// Equality/unordered comparisons retain the original branch semantics.
// These kernels do not allocate a vehicle or implement the driving controller.
bool source_vehicle_control_approach(
    SourcePhysicsContactMemory &, std::uint32_t value, std::uint32_t rates,
    std::uint32_t direction_word, std::uint32_t dt_word,
    std::uint32_t target_word, SourcePhysicsContactImage,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
