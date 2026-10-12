#pragma once
#include "halo2/observer_motion.hpp"

namespace halo2 {
struct SourceObserverWorldConstants {
  float maximum_initial_distance{}, maximum_distance{}, aspect{}, margin{};
  float vertical_bias_threshold{}, packet_maximum_seconds{};
  double vertical_bias_test{};
  Vec3 default_forward{}, default_up{}, zero{};
};
struct SourceObserverLocation {
  std::array<std::uint32_t, 2> words{};
  std::int16_t cluster() const;
};
struct SourceObserverWorldFrame {
  std::int32_t view{}, bsp{}, cluster_count{};
  std::uint32_t current_unit = 0xffffffffu;
  std::uint16_t player_flags{};
  DeathCameraSavedClusters saved_clusters;
};
struct SourceObserverWorldServices {
  // Actual11D4A0 location and3F4F0 visibility owners. Location preserves the
  // complete eight-byte source result, including BSP and leaf identity.
  std::function<SourceObserverLocation(Vec3)> locate;
  std::function<void(std::int32_t)> request_visibility;
  std::function<bool(Vec3)> fog_side;
  DeathCameraCollision collision;
};
// Complete173F20/173FA0 validity and repair, including inactive packets.
bool source_observer_packet_valid(const DeathCameraPacket &,
    const SourceObserverConstants &, const DeathCameraConstants &,
    const SourceObserverWorldConstants &);
void source_observer_repair_packet(DeathCameraPacket &,
    const SourceObserverConstants &, const DeathCameraConstants &,
    const SourceObserverWorldConstants &);
//170F30/170E40 reset retains derivatives and unassigned render fields.
void source_observer_reset(SourceObserverState &, std::int16_t bsp,
    const SourceObserverConstants &, const SourceObserverWorldConstants &);
// Complete172A50 render-state delivery: current-state repair, screen offset,
// parent transforms, obstruction, location, saved death-view PVS and Z bias.
// The services remain actual live world owners; no collision miss is inferred.
void source_observer_world(SourceObserverState &, const SourceObserverWorldFrame &,
    const SourceObserverWorldServices &, const SourceObserverConstants &,
    const DeathCameraConstants &, const SourceObserverWorldConstants &);
} // namespace halo2
