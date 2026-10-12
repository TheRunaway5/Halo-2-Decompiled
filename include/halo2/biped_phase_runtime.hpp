#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
// Native action/input/recovery/root-motion, falling producers and BC910 marking adapters.
// Each consumes only the source-defined incoming ABI. Existing authenticated
// animation and physical-object modules execute against the same owned bank;
// every reached lower owner retains its actual ABI and remains mandatory.
std::uint32_t
source_call_biped_runtime_1_1(SourcePhysicsContactMemory &,
                              const SourceRuntimeCall &,
                              SourcePhysicsFactoryFloatingPoint,
                              const SourceUnitAnimationRuntimeServices &,
                              const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
