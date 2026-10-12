#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
// Literal 32-byte channel and 0x90-byte manager storage. The unit's weapon
// overlay is extension+0x9c; it is not manager channel+0x20.
struct UnitAnimationTimingEvent {
  std::int16_t type, frame;
};
struct UnitAnimationSoundEvent {
  std::int16_t reference, frame;
  std::uint32_t marker;
};
struct UnitAnimationEffectEvent {
  std::int16_t reference, frame;
};
struct UnitAnimationReference {
  std::uint32_t tag;
  std::uint16_t flags;
};
struct UnitAnimationDefinition {
  std::int16_t frame_count{}, loop_frame{};
  std::uint8_t authored_flags{};
  std::vector<UnitAnimationTimingEvent> timing;
  std::vector<UnitAnimationSoundEvent> sounds;
  std::vector<UnitAnimationEffectEvent> effects;
  std::vector<UnitAnimationReference> sound_references, effect_references;
};
struct UnitAnimationCallback {
  std::uint32_t identity{}, context{};
};
struct UnitAnimationServices {
  // Source game-time+4, and global4770dc. These are explicit owner inputs.
  float tick_seconds{};
  bool layer_synchronization{};
  // Includes packed low-word inherited graph resolution; no guessed clip.
  std::function<const UnitAnimationDefinition &(std::uint32_t graph,
                                                std::uint32_t packed)>
      definition;
  // Original1ceda0 and its shared RNG/graph owners. Required only when called.
  std::function<std::uint32_t(std::span<std::byte> manager,
                              std::uint32_t old_packed)>
      select_variant;
  // Original event packet: type16, frame16, tag32, timing-id32, flags32,
  // marker32. Callback mutations are visible before the packed-ID recheck.
  std::function<void(UnitAnimationCallback, float frame,
                     const std::array<std::byte, 20> &event,
                     std::span<std::byte> manager,
                     std::span<std::byte> channel)>
      event;
  // Source1cefe0. The caller supplies the actual local banks and node count.
  // Missing producers throw; there is no successful default transition.
  std::function<void(std::span<std::byte> manager, float seconds,
                     unsigned node_count, std::span<std::byte> saved_locals,
                     std::span<const std::byte> current_locals)>
      save_local_transition;
};

// TU1.1 1ca2b0/1ca090/1c9f50/1ca3a0/1cae40, unchanged mapped TU1.5
// owners. These advance retained source channels; they never select locomotion
// from world velocity or invent action admission/physical state.
void advance_source_unit_animation_channel(std::span<std::byte> channel,
                                           std::span<std::byte> manager,
                                           UnitAnimationCallback,
                                           const UnitAnimationServices &);
void set_source_unit_animation_frame(std::span<std::byte> channel, float frame,
                                     std::span<std::byte> manager,
                                     UnitAnimationCallback,
                                     const UnitAnimationServices &);
void clamp_source_unit_animation_frame(std::span<std::byte> channel,
                                       float frame,
                                       const UnitAnimationServices &);

// 1cf100 owns three manager channels, completion clearing, 1cf070 layer event
// flags and normalized layer synchronization. It does not update extension+9c.
bool update_source_unit_animation_manager(
    std::span<std::byte> manager, unsigned node_count,
    std::span<std::byte> saved_locals,
    std::span<const std::byte> current_locals, UnitAnimationCallback,
    const UnitAnimationServices &);
// 1d0ed0/1dcd10: separate ordinary object phase, before unit/action update.
bool advance_source_unit_animation_transitions(std::span<std::byte> manager);
} // namespace halo2
