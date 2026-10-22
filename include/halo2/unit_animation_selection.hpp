#pragma once
#include "halo2/unit_animation.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
using SourceAnimationIdentifiers = std::array<std::uint32_t, 4>;
struct SourceAnimationGraphMemory {
  // Source virtual addresses are opaque data tokens, never host pointers.
  // Importer must retain actual original count/order/stride/pointer words and
  // resolve complete read-only backing. Missing/malformed backing must fail.
  std::function<std::uint32_t(std::uint32_t graph_datum)> graph_address;
  std::function<std::span<const std::byte>(std::uint32_t address,
                                           std::size_t bytes)>
      read;
  // Namespace of the compiled dual-class exception in1CFCE0. Original
  // fixtures retain this default; MCC owners verify their named cache ID.
  std::uint32_t dual_weapon_class=0x0400054bu;
};
// Complete pure1e0f50,1deb10,1cfce0,29c6a0 owners. Bank headers are
// signed count32/address32. Tree strides14/14/34, action/map stride8.
std::uint32_t find_source_animation_record(std::uint32_t bank,
                                           std::uint32_t key,
                                           std::uint32_t stride,
                                           const SourceAnimationGraphMemory &);
std::uint32_t find_source_animation_weapon(std::uint32_t graph_address,
                                           std::uint32_t mode,
                                           std::uint32_t weapon_class,
                                           std::uint32_t weapon,
                                           const SourceAnimationGraphMemory &);
// Pure weapon-to-class map portion of1CFCE0, after sentinel normalization.
// Dual requests retain their class even when the graph maps the weapon.
std::uint32_t source_animation_weapon_class(
    std::uint32_t graph_address, std::uint32_t weapon_class,
    std::uint32_t weapon, const SourceAnimationGraphMemory &);
SourceAnimationIdentifiers
normalize_source_animation_request(std::span<const std::byte> manager,
                                   SourceAnimationIdentifiers,
                                   const SourceAnimationGraphMemory &);
// Original iterator state is28 bytes: graph, inputmode/class/weapon, counter,
// current mode/class record tokens. Outputs retain their incoming values when
// original branches do not write them. No inherited/default clip is invented.
std::uint32_t
next_source_animation_weapon(std::span<std::byte> state,
                             std::array<std::uint32_t, 3> &resolved,
                             const SourceAnimationGraphMemory &);

struct UnitAnimationSelectionStore {
  std::uint32_t instruction;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const UnitAnimationSelectionStore &,
                         const UnitAnimationSelectionStore &) = default;
};
using UnitAnimationSelectionObserver =
    std::function<void(const UnitAnimationSelectionStore &)>;
struct UnitAnimationSelectionRequest {
  SourceAnimationIdentifiers identifiers;
  std::uint32_t selection_flags, installation_flags;
};
struct UnitAnimationResolvedRequest {
  // Complete1cfde0 writes both packets, even if its defined AL is false.
  SourceAnimationIdentifiers requested, resolved;
  std::uint32_t packed;
  bool admitted;
};
struct UnitAnimationSecondarySelection {
  std::uint32_t packed;
  std::uint8_t eligible;
};
struct UnitAnimationThirdSelection {
  std::uint32_t packed, activity;
};
struct UnitAnimationSelectionServices {
  // Exact executable465030,465048,46504c bits. No tuning payload defaults.
  std::uint32_t one_bits, zero_bits, frame_subtractor_bits;
  // Complete source definition resolver including signed packed inheritance.
  std::function<const UnitAnimationDefinition &(std::uint32_t graph,
                                                std::uint32_t packed)>
      definition;
  // Required full1cfde0, including1deb60 and resource1e13c0/124e40 owners.
  std::function<UnitAnimationResolvedRequest(std::span<std::byte> manager,
                                             SourceAnimationIdentifiers,
                                             std::uint32_t selection_flags)>
      resolve_request;
  // Required full1d0790/1d06c0 owners. Shared globals/timing/layer graph and
  // resource producers remain explicit; NONE results are never defaults here.
  std::function<UnitAnimationSecondarySelection(
      std::span<std::byte> manager, std::uint32_t mode, std::uint32_t activity)>
      secondary;
  std::function<UnitAnimationThirdSelection(std::span<std::byte> manager,
                                            SourceAnimationIdentifiers)>
      third;
  // Required full1ceda0: actual graph variants and RNG stream, called in order.
  std::function<std::uint32_t(std::span<std::byte> manager,
                              std::size_t channel_offset,
                              std::uint32_t old_packed)>
      variant;
  // Existing complete1c9f50 owner may satisfy this callback; caller must retain
  // its actual definition resolver/arithmetic. Mutations remain observable.
  std::function<void(std::span<std::byte> manager, std::size_t channel_offset,
                     std::uint32_t frame_bits)>
      clamp_frame;
  // Required1d1a00 transform/sampler and1d1b70→1e16f0 resource owners.
  std::function<void(std::span<std::byte> manager)> root_transform_delta;
  std::function<void(std::span<std::byte> manager,
                     std::array<std::uint32_t, 3> requested,
                     std::uint32_t first, std::uint32_t second)>
      resources;
};

// Complete1d0590 only resets the three channels, preserving manager60..8f.
void reset_source_unit_animation_channels(
    std::span<std::byte> manager, std::uint32_t one_bits,
    const UnitAnimationSelectionObserver & = {});
// Complete1c9e60 installation. Signed finalbyte is stored as signedword.
bool install_source_unit_animation_channel(
    std::span<std::byte> manager, std::size_t offset, std::uint32_t graph,
    std::uint32_t flags, std::uint32_t packed, std::uint32_t activity,
    std::uint32_t first_byte, std::uint32_t second_byte,
    std::uint32_t signed_byte, std::uint32_t one_bits,
    const UnitAnimationSelectionObserver & = {});
// Complete1d0a60 direct owner, plus complete1cee00 installation wrapper and
// pure validity/authored-flag gates. Required nested producers above must run
// when called; unavailable services throw at that boundary after prior stores.
// This is not a complete live action/default selection integration by itself.
bool select_source_unit_animation(std::span<std::byte> manager,
                                  const UnitAnimationSelectionRequest &,
                                  const UnitAnimationSelectionServices &,
                                  const UnitAnimationSelectionObserver & = {});
} // namespace halo2
