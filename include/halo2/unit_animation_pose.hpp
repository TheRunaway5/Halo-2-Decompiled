#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class UnitAnimationMaskOwner {
  None,
  Caller,
  Graph5C,
  Graph7C,
  Intersection
};
struct UnitAnimationLocalSample {
  std::uint32_t graph{}, packed{}, inheritance_mapping{};
  unsigned node_count{};
  float frame{}, weight{};
  bool interpolate{};
  UnitAnimationMaskOwner mask_owner{};
  std::span<const std::byte> node_mask;
};
// Remaining opaque source1d0f80 stack words 1,5,6. They are forwarded intact
// only to the actual override-root owner; no geometry is inferred from them.
struct UnitAnimationLocalContext {
  std::uint32_t word1{}, word5{}, word6{};
};
struct UnitAnimationBlendChannel {
  std::span<const std::byte> channel;
  float second_weight{};
};
struct UnitAnimationLocalPoseServices {
  std::int16_t tick_frequency{};
  bool layer_synchronization{}, third_channel_override{};
  std::function<std::span<const std::byte>(std::uint32_t graph,
                                           std::uint32_t packed)>
      definition6c;
  // Literal original root graph+5c through+9b: two 256-bit masks.
  std::function<std::span<const std::byte>(std::uint32_t graph)> graph_masks;
  std::function<std::uint32_t(std::uint32_t graph, std::uint32_t packed)>
      inheritance_mapping;
  std::function<void(std::uint32_t graph, std::uint32_t packed)>
      notify_delayed_resource;
  // Original280030 packed sampler/codecs. It owns actual replacement/overlay
  // arithmetic and resource resolution. Missing decoder never preserves an
  // old pose as a fabricated successful output.
  std::function<void(const UnitAnimationLocalSample &,
                     std::span<std::byte> actual_local_bank)>
      decode;
  // Original1d18f0 and1d1b00: required only on their admitted layer branches.
  std::function<UnitAnimationBlendChannel(std::span<const std::byte> manager)>
      blend_channel;
  std::function<void(std::span<const std::byte> manager,
                     UnitAnimationLocalContext, float weight,
                     unsigned node_count, std::span<const std::byte> node_mask,
                     std::span<std::byte> actual_local_bank)>
      override_root;
};
// 1ca950: source-owned sampling admission, original mask
// selection/intersection, fractional flag and exact packed/frame/count/weight
// delivery to280030.
void sample_source_unit_animation_locals(
    std::span<const std::byte> channel, float weight,
    std::span<const std::byte> node_mask, unsigned node_count,
    std::span<std::byte> local_bank, const UnitAnimationLocalPoseServices &);
// 1d0f80: selects existing manager channels by original flags/weights, never by
// velocity or guessed clip names. Post-sampling unit113830/IK/current matrices
// remain the calling producer's responsibility.
void build_source_unit_animation_locals(std::span<const std::byte> manager,
                                        UnitAnimationLocalContext, float weight,
                                        std::span<const std::byte> node_mask,
                                        unsigned node_count,
                                        std::span<std::byte> local_bank,
                                        const UnitAnimationLocalPoseServices &);
} // namespace halo2
