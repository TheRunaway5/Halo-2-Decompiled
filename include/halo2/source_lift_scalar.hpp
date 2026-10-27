#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
// Authenticated TU1.5 1DA530 operands. The caller retains
// their actual executable provenance; authored lift rules are separate inputs.
struct SourceLiftScalarConstants {
  float epsilon{}, one{}, half{}, reverse_minimum{}, reverse_maximum{};
};
struct SourceLiftScalarInput {
  bool allow_reverse{}, scale_by_mass{}, force_acceleration{};
  float mass{}, target{}, acceleration{}, velocity{}, velocity_limit{};
  float position{}, exponent{}, time_step{};
};
struct SourceLiftScalarResult { float velocity_delta{}, velocity_ratio{}; };
// The semantic kernel has no register machine or original stack. Its numeric
// controls are explicit because TU1.5 combines binary32 SSE with x87 arithmetic.
// Only masked-exception environments are currently admitted; synchronous host
// floating-point fault delivery remains a separate caller contract.
struct SourceLiftScalarEnvironment {
  std::uint16_t x87_control_word{0x027f};
  std::uint32_t mxcsr{0x1f80};
  std::uint8_t x87_exception_flags{};
  std::uint16_t x87_status{}; // Explicit condition bits and TOP for an empty source stack.
};
struct SourceLiftScalarOutcome {
  SourceLiftScalarResult result;
  std::uint32_t mxcsr{};
  std::uint16_t x87_status{}; // Actual native arithmetic status, not an emulated stack.
  bool used_x87{};
  std::uint16_t control_word{};
};
// Required owner of original pow, its x87 acceleration multiply and final
// FSTP32. A missing owner fails only when the positive-exponent branch runs.
using SourceLiftPower = std::function<float(float absolute_target,float exponent,float acceleration)>;
// Complete semantic TU1.5 1DA530 computation under the supplied controls.
// Returns its numerical side effects explicitly and restores the host caller's
// environment. A positive exponent still needs a separately proved native
// power/multiply/FSTP32 owner; there is no lifted or host-pow fallback.
SourceLiftScalarOutcome source_lift_scalar_tu15(const SourceLiftScalarInput&,
    const SourceLiftScalarConstants&,SourceLiftScalarEnvironment,
    const SourceLiftPower& = {});
// Scalar force consumer only. It neither admits phantom contacts nor obtains
// body mass/velocity, marker transforms, activation or a world-step callback.
// Compatibility entry for the previously qualified nearest-even/53-bit domain.
SourceLiftScalarResult source_lift_scalar(const SourceLiftScalarInput&,
    const SourceLiftScalarConstants&,const SourceLiftPower& = {});
}
