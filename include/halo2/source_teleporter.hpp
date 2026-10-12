#pragma once
#include "halo2/content.hpp"
#include <functional>

namespace halo2 {
struct SourceTeleporterPair {
  std::int32_t entrance = -1, exit = -1, cooldown{}, blocked_notice{};
};
struct SourceTeleporterBank {
  std::array<SourceTeleporterPair, 16> pairs;
  std::int32_t count{};
};
struct SourceTeleporterPlayer {
  std::uint32_t datum = null_datum, unit = null_datum;
  std::uint16_t flags{};
  std::int16_t local_controller = -1;
};
struct SourceTeleporterContext {
  std::int16_t tick_rate{};
  float cooldown_seconds{}, entrance_distance_squared{};
  Vec3 default_up; // Selected image's [46FC50]/[474D10] vector.
};
// These are the actual dependency owners of1A0580/1A0790, not optional
// notifications. In particular destination_busy must run1A0400's collection,
// deepest contact, player flag and dirty-entity work. A false constant or host
// triangle overlap does not implement that service. The span is the occupied
// player pool in slot order;1A2C60 additionally excludes current-unitNONE.
struct SourceTeleporterServices {
  std::function<Vec3(std::uint32_t)> root, forward;
  std::function<std::uint32_t(std::uint32_t)> parent;
  std::function<bool(const SourceTeleporterPair &, std::uint32_t)>
      destination_busy;
  std::function<void(std::uint32_t)> traversal_event, blocked_local_notice,
      blocked_unit_notice;
  std::function<void(std::uint32_t)> detach_from_parent;
  std::function<void(std::uint32_t, Vec3, Vec3, Vec3)> set_position;
  std::function<void(std::int16_t, Vec3)> set_local_view;
};
// Complete1A0340/1A0390 with the no-position, max-one1A25A0 selection.
// Failed channels write the current bank slot too, as in the original.
void source_teleporter_initialize(SourceTeleporterBank &,
                                  std::span<const NetgameFlag>);
// Complete1A0580,1A0680 and1A0790 control. Destination queries, authored
// events, object registration/pose writes and local camera delivery retain
// required explicit owners. This function alone is not a live teleporter.
// Caller supplies the original nearest-even SSE environment.
void source_teleporter_step(SourceTeleporterBank &,
                            std::span<const NetgameFlag>,
                            std::span<SourceTeleporterPlayer>,
                            const SourceTeleporterContext &,
                            const SourceTeleporterServices &);
// Literal x87 FPATAN/FADD/FSUB/FCOS/FSIN, then original30BD0 normalize.
// Uses the source nearest-even/53-bit precision environment and restores it.
// Unsupported CPU/compiler combinations reject, rather than use host libm.
Vec3 source_teleporter_forward(Vec3, float entrance_facing, float exit_facing);
std::int32_t source_teleporter_cooldown(std::int16_t tick_rate, float seconds);
} // namespace halo2
