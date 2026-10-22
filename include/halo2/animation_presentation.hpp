#pragma once
#include "halo2/fp_registration.hpp"

namespace halo2 {
struct FirstPersonRenderFrame {
  float frame{};
  bool interpolated{};
};
struct AnimationRenderObservation {
  std::uint32_t tick{}, unit=0xffffffffu, weapon=0xffffffffu,
      graph=0xffffffffu, animation=0xffffffffu;
  float fraction{}, frame{};
  bool first_person{}, interpolated{};
  // Borrowed actual draw output; valid only during the read-only callback.
  std::span<const Vertex> vertices;
  std::uint32_t ammunition_animation=0xffffffffu;
  float ammunition_frame{};
  std::span<const Transform> weapon_nodes; // Actual shared posed bank, read-only.
  std::span<const Transform> body_nodes; // Actual third-person skinning bank, read-only.
  float body_facing{};
  std::uint32_t body_activity=0xffffffffu;
  // Unmodified completed FP output before render interpolation. Diagnostics
  // can compare endpoints without re-entering original pose owners.
  std::span<const Transform> completed_weapon_nodes;
  // Actual retained native locomotion endpoint, before render interpolation.
  std::optional<float> completed_locomotion_frame;
  // Third-person primary weapon geometry selected for this body submission.
  std::span<const Vertex> weapon_vertices;
};
// Native high-refresh presentation policy. Endpoints are the actual clock input
// after action selection and completed output, never predicted gameplay states.
// A callback-free copy of
// the original clock verifies that no action, rebind or channel reset occurred.
// It neither advances the retained hand nor dispatches animation events.
FirstPersonRenderFrame first_person_render_frame(
    const FirstPersonViewBinding &previous, const FirstPersonViewBinding &current,
    const FirstPersonBindingDefinition &, float fraction, bool consecutive_tick);
// A fractional point between completed simulation ticks for the existing
// decoded idle presentation path. This does not choose a locomotion channel.
double animation_render_tick(std::uint32_t previous, std::uint32_t current,
                             float fraction);
} // namespace halo2
