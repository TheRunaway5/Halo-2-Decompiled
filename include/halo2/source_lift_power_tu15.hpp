#pragma once
#include "halo2/source_lift_scalar.hpp"
#include "halo2/source_math_error_tu15.hpp"
#include <cstdint>
#include <stdexcept>

namespace halo2 {
// Closed numerical portion of the TU1.5 1DA530 positive-exponent dependency:
// original float loads, 37CDE8 argument conversion, 37CE0C finite/zero math,
// restored-control acceleration multiply and final binary32 store.
// NaN/domain and range records use the native CRT policy and a typed native
// thread errno cell; a missing required cell fails at its actual source owner.
// Same-host original comparisons do not prove Pentium III transcendental
// approximation identity; that hardware qualification remains separate.
struct SourceLiftPowerOutcome {
  float acceleration{};
  std::uint32_t mxcsr{};
  std::uint16_t x87_status{};
};
SourceLiftPowerOutcome source_lift_power_tu15(float absolute_target,float exponent,
    float acceleration,SourceLiftScalarEnvironment,const SourceMathErrorServicesTu15& = {});
// Callback seam for source_lift_scalar_tu15, which already owns its numeric
// environment. Preserves the control word and leaves actual accrued exception
// flags for the scalar's remaining arithmetic. Requires masked exceptions.
float source_lift_power_tu15_current(float absolute_target,float exponent,float acceleration,
    const SourceMathErrorServicesTu15& = {});
float source_lift_power_tu15_current_lazy(float absolute_target,float exponent,
    const std::function<float()>& acceleration,const SourceMathErrorServicesTu15& = {});
}
