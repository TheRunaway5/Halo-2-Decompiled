#pragma once
#include "halo2/unit_alternate_request.hpp"
#include "halo2/unit_channel_request.hpp"
#include "halo2/unit_constructors.hpp"
#include <optional>
namespace halo2 {
struct SourceUnitPoseRequestServices {
  SourceUnitChannelRequestServices channels;
};
struct SourceUnitPoseCall {
  std::uint32_t instruction, entry;
  std::array<std::uint32_t, 4> arguments;
  bool returning;
  std::optional<std::uint32_t> result;
  friend bool operator==(const SourceUnitPoseCall &,
                         const SourceUnitPoseCall &) = default;
};
using SourceUnitPoseCallObserver = std::function<void(const SourceUnitPoseCall &)>;
// Complete source scratch-channel initializer inside1CF3C0. It writes all32
// bytes; only literal imported one_bits is an input, never a selected clip.
void construct_source_unit_pose_channel(std::span<std::byte>,
                                         std::uint32_t one_bits);
// Complete1CEF10. Actual retained manager/channel and variant/resource/clamp
// owners are required. GraphNONE or failed lookup returns actual false.
bool install_source_unit_alternate_pose_channel(
    std::span<std::byte> manager, std::span<std::byte> channel,
    std::uint32_t weapon_class, std::uint32_t weapon, std::uint32_t activity,
    std::uint32_t flags, const SourceUnitPoseRequestServices &,
    const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &channel_stores = {});
// Complete1CF3C0. The temporary32-byte channel is entirely initialized by
// original instructions before use; no Unit/body or selected clip is supplied.
void lookup_source_unit_alternate_pose(
    std::span<std::byte> manager, std::uint32_t weapon_class,
    std::uint32_t weapon, std::uint32_t activity, std::span<std::byte> packed,
    const SourceUnitPoseRequestServices &, const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &packed_stores = {});
// Complete1CF360: existing complete1CFDE0 with actual mode and flags0.
void lookup_source_unit_resolved_pose(
    std::span<std::byte> manager, std::uint32_t weapon_class,
    std::uint32_t weapon, std::uint32_t activity, std::span<std::byte> packed,
    const SourceUnitPoseRequestServices &, const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &packed_stores = {});
// Complete1CF4C0: current-sentinel normalization, alternate then actual
// resolution fallback. Returns through caller-owned packed output.
void lookup_source_unit_pose_request(
    std::span<std::byte> manager, std::uint32_t weapon_class,
    std::uint32_t weapon, std::uint32_t activity, std::span<std::byte> packed,
    const SourceUnitPoseRequestServices &, const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &packed_stores = {});
// Complete1CFB90, including its graph lookup before index admission.
void lookup_source_unit_pose_index(
    std::span<const std::byte> manager, std::uint32_t index,
    std::span<std::byte> packed, const SourceUnitPoseRequestServices &,
    const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &packed_stores = {});
// Complete115AF0 and114BD0 over actual source-owned header/body/offsets.
// They allocate no body storage and supply no pose, ready or resource state.
void update_source_unit_posture_channel(
    std::uint32_t unit, const UnitConstructorObjectPool &,
    const SourceUnitPoseRequestServices &, const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &body_stores = {});
void update_source_unit_pose_requests(
    std::uint32_t unit, const UnitConstructorObjectPool &,
    const SourceUnitPoseRequestServices &, const SourceUnitPoseCallObserver & = {},
    const UnitAnimationSelectionObserver &body_stores = {});
} // namespace halo2
