#pragma once
#include "halo2/model.hpp"

namespace halo2 {
// Literal input to original type0 primary codecs. Packed rotation words are
// retained separately from the generic float-key animation representation.
struct FrontendModelPrimary {
  struct Track {
    unsigned codec{};
    std::vector<unsigned> times;
    std::vector<std::array<std::int16_t,4>> rotations;
    std::vector<std::array<float,4>> values;
  };
  std::uint32_t source_graph=0xffffffffu,source_index=0xffffffffu;
  unsigned frame_count{};
  std::vector<std::uint32_t> nodes;
  std::vector<unsigned> components;
  std::vector<std::array<Track,3>> channels;
};
// Source2798A0 truncates the retained frame when channel interpolation is
// disabled. Menu primary flags003F/rate1 at30Hz disable it; the clock remains
// fractional. Codec integer blends use grouped RSQRT norm; software seed is
// explicit, with physical Xbox approximation remaining a verification limit.
std::vector<BonePose> sample_frontend_model_primary(const RenderModel&,
    const FrontendModelPrimary&,float retained_frame,
    std::span<const std::uint32_t> destination_mask={},bool interpolate=false);
// BF0C0 supplies model-order locals to the same codecs. Graph/model StringIDs
// need not match; the primary graph must cover the actual destination bank.
std::vector<BonePose> sample_world_object_primary(const RenderModel&,
    const FrontendModelPrimary&,float retained_frame,
    std::span<const std::uint32_t> destination_mask={},bool interpolate=false);
}
