#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15FunctionCurveStats {
  std::uint64_t transition_calls{},periodic_calls{},table_samples{},native_remainders{},
    unsupported_types{},unsupported_inputs{},unsupported_fp_profiles{};
};
// Native17F8A0 /17F790 authored lookup/interpolation. Type0 follows its real
// bypass. Nonzero types admit authenticated transition1..7 /periodic1..11
// finite-domain math; unsupported math never routes an original CPU owner.
std::uint32_t native_tu15_evaluate_transition_curve(SourcePhysicsContactMemory&,
  std::uint16_t type,std::uint32_t parameter_bits,NativeTu15PhysicsFrameFp&,
  unsigned actual_preserved_x87_depth,NativeTu15FunctionCurveStats* = nullptr);
std::uint32_t native_tu15_evaluate_periodic_curve(SourcePhysicsContactMemory&,
  std::uint16_t type,std::uint32_t parameter_bits,NativeTu15PhysicsFrameFp&,
  unsigned actual_preserved_x87_depth,NativeTu15FunctionCurveStats* = nullptr);
} // namespace halo2
