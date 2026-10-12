#pragma once
#include <functional>

namespace halo2 {
// Selected-image operands of TU1.1 1D5C70 / u5 1DA530. The caller retains
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
// Required owner of original pow, its x87 acceleration multiply and final
// FSTP32. A missing owner fails only when the positive-exponent branch runs.
using SourceLiftPower = std::function<float(float absolute_target,float exponent,float acceleration)>;
// Scalar force consumer only. It neither admits phantom contacts nor obtains
// body mass/velocity, marker transforms, activation or a world-step callback.
// Requires nearest-even SSE and x87's original 53-bit intermediate precision.
SourceLiftScalarResult source_lift_scalar(const SourceLiftScalarInput&,
    const SourceLiftScalarConstants&,const SourceLiftPower& = {});
}
