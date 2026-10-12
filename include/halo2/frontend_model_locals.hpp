#pragma once
#include "halo2/model.hpp"
#include "halo2/frontend_model_primary.hpp"
#include "halo2/content.hpp"

namespace halo2 {
// Original112070's final single-held grip: source type2, one frame, codec3.
// Store the actual quantized input and animated mask, not a normalized
// replacement for the primary channel or a list of guessed finger names.
struct FrontendModelGrip {
  std::uint32_t source_graph=null_datum,source_index=null_datum;
  std::vector<std::uint32_t> nodes;
  std::vector<std::optional<std::array<std::int16_t,4>>> rotations;
};
// Retained primary frame sampled into model order. The optional literal
// source codec inputs preserve packed rotation arithmetic. Menu channel
// interpolation defaults disabled; sampling truncates without changing the
// retained clock. Generic raw keys remain available for explicit diagnostics.
std::vector<BonePose> frontend_model_locals(const RenderModel&,
    const AnimationClip&,float retained_frame,std::span<const std::uint32_t> destination_mask={},
    const FrontendModelPrimary*source=nullptr,bool interpolate=false);
// Original28C960 multiplies integer quaternion lanes by RSQRTPS(norm),
// with norm=(x*x+z*z)+(y*y+w*w). Binary32 inverse sqrt is the explicit
// software seed convention, not a physical Xbox lookup assertion.
std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4>);
std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4>,float inverse_sqrt_seed);
// Model-order rotation replacements only; translations/scales and nodes
// outside the source animation mask are retained byte-for-byte.
void apply_frontend_model_grip(const RenderModel&,std::span<BonePose>,
    const FrontendModelGrip&,std::span<const std::uint32_t> destination_mask={});
}
