#pragma once
#include "halo2/unit_animation_resolution.hpp"

namespace halo2 {
struct SourceUnitChannelRequestServices {
  SourceAnimationGraphMemory graphs;
  SourceAnimationResourceServices resources;
  std::uint32_t one_bits;
  // Complete1DF670 for source channel type1. A missing owner throws.
  std::function<std::uint32_t(std::uint32_t graph_address,
                              SourceAnimationIdentifiers)>
      alternate_lookup;
  // Complete1CEDA0 with caller-owned actual RNG and resource state.
  std::function<std::uint32_t(std::span<std::byte> manager,
                              std::uint32_t packed)>
      variant;
  // Complete1C9F50 with actual definitions, dt and frame arithmetic.
  std::function<void(std::span<std::byte> channel, std::uint32_t frame_bits)>
      clamp_frame;
};
// General complete1CEE00: channel can be inside manager or actual extension
// storage. Required lower owners run against the same retained spans.
bool install_source_unit_requested_channel(
    std::span<std::byte> manager, std::span<std::byte> channel,
    std::uint32_t packed, std::uint32_t activity, std::uint32_t flags,
    std::uint32_t first_byte, std::uint32_t second_byte,
    std::uint32_t signed_byte, const SourceUnitChannelRequestServices &,
    const UnitAnimationSelectionObserver &channel_observer = {});
// Complete1CEC20. Outputs/stores refer to channel32; manager90 is read by
// this body, though actual required resource/variant owners may mutate it.
// No channel/manager/body allocation, readiness or fallback callback occurs.
bool update_source_unit_channel_request(
    std::span<std::byte> manager, std::span<std::byte> channel,
    std::uint32_t weapon_class, std::uint32_t weapon,
    const SourceUnitChannelRequestServices &,
    const UnitAnimationSelectionObserver &channel_observer = {});
} // namespace halo2
