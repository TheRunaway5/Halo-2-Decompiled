#include "halo2/navigation.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <limits>
#include <queue>
namespace halo2 {
namespace {
Vec3 center(const NavigationSector &s) {
  Vec3 p{};
  for (auto v : s.polygon)
    p = p + v;
  return p / float(s.polygon.size());
}
float distance_to_polygon(Vec3 p, const NavigationSector &s) {
  // Authored sectors are convex. Project onto the sector plane, then test the
  // edge half spaces; outside points use the nearest boundary segment.
  if (s.polygon.size() < 3)
    return INFINITY;
  Vec3 normal{};
  for (std::size_t i = 1; i + 1 < s.polygon.size() && length(normal) < .5f; ++i)
    normal = normalized(
        cross(s.polygon[i] - s.polygon[0], s.polygon[i + 1] - s.polygon[0]));
  if (length(normal) < .5f)
    return INFINITY;
  const float height = dot(p - s.polygon[0], normal);
  const auto q = p - normal * height;
  bool positive = false, negative = false;
  float boundary = INFINITY;
  for (std::size_t i = 0; i < s.polygon.size(); ++i) {
    const auto a = s.polygon[i], b = s.polygon[(i + 1) % s.polygon.size()];
    const auto edge = b - a;
    const float side = dot(cross(edge, q - a), normal);
    positive |= side > 1e-5f;
    negative |= side < -1e-5f;
    const float t = std::clamp(
        dot(q - a, edge) / std::max(dot(edge, edge), 1e-12f), 0.f, 1.f);
    boundary = std::min(boundary, length(p - (a + edge * t)));
  }
  return positive && negative ? boundary : std::abs(height);
}
} // namespace
std::optional<std::size_t> NavigationWorld::locate(Vec3 p,
                                                   float maximum) const {
  std::optional<std::size_t> result;
  for (std::size_t i = 0; i < sectors_.size(); ++i) {
    const auto &s = sectors_[i];
    if (!(s.flags & 1) || (s.flags & 4))
      continue;
    const auto d = distance_to_polygon(p, s);
    if (d < maximum) {
      maximum = d;
      result = i;
    }
  }
  return result;
}
std::vector<Vec3> NavigationWorld::route(Vec3 start, Vec3 goal,
                                         bool crouch) const {
  const auto from = locate(start), to = locate(goal);
  if (!from || !to)
    return {};
  if (*from == *to)
    return {goal};
  const auto count = sectors_.size();
  std::vector<float> costs(count, INFINITY);
  std::vector<std::size_t> parent(count, count);
  std::vector<Vec3> entrances(count);
  using Entry = std::pair<float, std::size_t>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
  costs[*from] = 0;
  open.push({length(center(sectors_[*from]) - goal), *from});
  std::vector<bool> closed(count);
  while (!open.empty()) {
    const auto i = open.top().second;
    open.pop();
    if (closed[i])
      continue;
    closed[i] = true;
    if (i == *to)
      break;
    for (const auto &portal : sectors_[i].portals) {
      const auto j = portal.sector;
      if (j >= count)
        throw FormatError("navigation portal index outside sectors");
      if (!(sectors_[j].flags & 1) || (sectors_[j].flags & 4) ||
          (!crouch && ((sectors_[j].flags & 0x400) || (portal.flags & 0x100))))
        continue;
      const float candidate =
          costs[i] + length(center(sectors_[i]) - center(sectors_[j]));
      if (candidate >= costs[j])
        continue;
      costs[j] = candidate;
      parent[j] = i;
      entrances[j] = portal.midpoint;
      open.push(
          {candidate + length(center(sectors_[j]) - center(sectors_[*to])), j});
    }
  }
  if (parent[*to] == count)
    return {};
  std::vector<Vec3> result{goal};
  for (auto i = *to; i != *from; i = parent[i]) {
    if (i >= count || result.size() > count)
      throw FormatError("cyclic navigation route");
    result.push_back(entrances[i]);
  }
  std::reverse(result.begin(), result.end());
  return result;
}
} // namespace halo2
