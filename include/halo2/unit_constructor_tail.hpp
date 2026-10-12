#pragma once
#include "halo2/unit_animation_selection.hpp"
#include "halo2/unit_constructors.hpp"
#include <array>
#include <functional>
#include <span>

namespace halo2 {
// Literal selected-image operands. No executable tuning values are defaults.
struct UnitConstructorTailConstants {
  std::uint32_t one_bits, zero_bits, normal_blend_bits, special_blend_bits;
  std::array<std::array<std::uint32_t, 2>, 4> activity_pairs;
};
struct UnitConstructorTailMetadata {
  std::function<std::uint32_t(std::uint32_t datum)> address;
  // Actual forwarding pointers may address a different explicitly owned tag.
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
};
enum class UnitConstructorTailBuffer { Unit, Packed };
struct UnitConstructorTailStore {
  std::uint32_t instruction;
  UnitConstructorTailBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const UnitConstructorTailStore &,
                         const UnitConstructorTailStore &) = default;
};
using UnitConstructorTailObserver =
    std::function<void(const UnitConstructorTailStore &)>;
// Exact defined owning operands, not guest scratch registers. Manager spans
// are supplied separately (1D0A60 uses ECX;1CFA90 uses ESI).
struct UnitConstructorTailCall {
  std::uint32_t entry, unit, eax;
  bool eax_input;
  std::array<std::uint32_t, 5> arguments{};
  unsigned count{};
  friend bool operator==(const UnitConstructorTailCall &,
                         const UnitConstructorTailCall &) = default;
};
struct UnitConstructorTailServices {
  UnitConstructorObjectPool objects;
  UnitConstructorTailMetadata tags;
  SourceAnimationGraphMemory graphs;
  UnitConstructorTailConstants constants;
  // Complete1E13C0 owner; invoked only when flat lookup actually finds a row.
  std::function<void(std::uint32_t graph_address, std::uint32_t packed)>
      resource;
  // Actual runtime517FE8 owner; read only for a unit with player13C !=NONE.
  std::function<std::uint8_t()> player_transition_gate;
  // Required1D0A60,114F00,1CFA90,BB240,114BD0. Only the first and third
  // consume defined returned AL. Missing owners throw; no success defaults.
  std::function<std::uint8_t(std::span<std::byte> manager,
                             const UnitConstructorTailCall &)>
      invoke;
};
struct UnitActivityRequest {
  std::uint32_t flags;
  std::uint32_t mode, weapon_class, weapon, activity;
  std::uint32_t minimum_blend_bits;
  std::uint8_t force;
};

// Complete original bodies. Existing source-created signed component offsets
// and full body storage are prerequisites; these functions allocate no bytes.
void reset_source_unit_subchannels(std::uint32_t unit,
                                   const UnitConstructorTailServices &,
                                   const UnitConstructorTailObserver & = {});
void reset_source_unit_named_animation(
    std::uint32_t unit, const UnitConstructorTailServices &,
    const UnitConstructorTailObserver & = {});
void reset_source_unit_aim_animation(std::uint32_t unit,
                                     const UnitConstructorTailServices &,
                                     const UnitConstructorTailObserver & = {});
void reset_source_unit_action_cache(std::uint32_t unit,
                                    const UnitConstructorTailServices &,
                                    const UnitConstructorTailObserver & = {});
void initialize_source_unit_extension(std::uint32_t unit,
                                      const UnitConstructorTailServices &,
                                      const UnitConstructorTailObserver & = {});
// 116680 owns UDLG reference/name metadata, not collision data.
void initialize_source_unit_variant_metadata(
    std::uint32_t unit, std::uint32_t variant,
    const UnitConstructorTailServices &,
    const UnitConstructorTailObserver & = {});
std::uint8_t
bind_source_unit_activity(std::uint32_t unit, const UnitActivityRequest &,
                          const UnitConstructorTailServices &,
                          const UnitConstructorTailObserver & = {});
std::uint32_t find_source_model_node(std::uint32_t model, std::uint32_t name,
                                     const UnitConstructorTailMetadata &);
void find_source_named_animation(std::uint32_t graph_address,
                                 std::uint32_t name,
                                 std::span<std::byte> packed_output,
                                 const UnitConstructorTailServices &,
                                 const UnitConstructorTailObserver & = {});
std::uint32_t source_unit_transition_duration(
    std::uint32_t next, std::uint32_t previous, std::uint8_t mode_changed,
    std::uint8_t force_normal, std::uint32_t minimum_bits,
    const UnitConstructorTailConstants &);
} // namespace halo2
