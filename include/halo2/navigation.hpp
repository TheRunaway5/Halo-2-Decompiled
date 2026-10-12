#pragma once
#include "halo2/content.hpp"
#include <optional>
namespace halo2 {
// Uses authored sectors and portals. Route cost is the host's geometric cost;
// retail actor movement, jump hints, and dynamic obstacle costs remain
// separate.
class NavigationWorld {
public:
  explicit NavigationWorld(const std::vector<NavigationSector> &sectors)
      : sectors_(sectors) {}
  std::optional<std::size_t> locate(Vec3, float maximum_distance = 3) const;
  std::vector<Vec3> route(Vec3 start, Vec3 goal,
                          bool allow_crouch = true) const;

private:
  const std::vector<NavigationSector> &sectors_;
};
} // namespace halo2
