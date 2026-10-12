#pragma once
#include "halo2/posed_collision.hpp"

namespace halo2 {
struct ItemPointDefinition {
  std::uint32_t definition{},model{},render_model{};
  std::int32_t group=-1;
  std::vector<std::array<std::byte,36>> markers;
};
struct ItemPointState {
  std::uint32_t datum{},definition{},flags{},parent=0xffffffffu;
  std::uint8_t item_flags{};
  std::int16_t contact_instance{},contact_surface{};
  Vec3 root;
  std::span<const Transform> current_nodes;
};
struct ItemPointMarker {
  std::int16_t node{};
  Transform local,world;
  float scalar{};
  std::int16_t signed_plane{};
  CollisionPlane plane;
  float signed_distance{},shift{};
};
struct ItemPointResult {
  Vec3 point;
  std::uint32_t returned{}; // 0 root fallback still defines a valid point.
  std::optional<ItemPointMarker> marker;
};
// Bounded FBFA0: source-owned unparented state and explicit active ordinary
// collision bank. It neither settles items nor produces animated node poses.
ItemPointResult source_item_point(const ItemPointDefinition &,
    const ItemPointState &,const RawCollisionBsp &active_collision);
} // namespace halo2
