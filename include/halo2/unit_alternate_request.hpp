#pragma once
#include "halo2/unit_animation_resolution.hpp"
#include <optional>
namespace halo2 {
// Complete1DF670. Uses weapon+0C action bank and its exact repeated resource
// sweeps. Optional resolved outputs are absent iff source pointers areNULL.
void lookup_source_alternate_animation_request(
    std::uint32_t graph_address, SourceAnimationIdentifiers,
    std::span<std::byte> packed,
    std::array<std::span<std::byte>, 3> optional_resolved,
    const SourceAnimationGraphMemory &, const SourceAnimationResourceServices &,
    const AnimationResolutionObserver & = {});
struct SourceAlternateAvailabilityCall {
  std::uint32_t instruction, graph_address;
  SourceAnimationIdentifiers identifiers;
  std::optional<std::uint32_t> packed;
  friend bool operator==(const SourceAlternateAvailabilityCall &,
                         const SourceAlternateAvailabilityCall &) = default;
};
using SourceAlternateAvailabilityObserver =
    std::function<void(const SourceAlternateAvailabilityCall &)>;
// Complete1CFA90. Probes actual0E000044,0E000045,0B000046 requests in order;
// it checks availability, not root-transform sampling or resource IO success.
bool source_unit_alternate_animation_available(
    std::span<const std::byte> manager, const SourceAnimationGraphMemory &,
    const SourceAnimationResourceServices &,
    const SourceAlternateAvailabilityObserver & = {});
} // namespace halo2
