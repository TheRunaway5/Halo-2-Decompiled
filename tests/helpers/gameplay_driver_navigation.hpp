#pragma once
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include "halo2/navigation.hpp"
#include "halo2/simulation.hpp"
#include <queue>

namespace halo2::test {
inline std::optional<Vec3> visible_driver_target(const CollisionWorld &world,
    Vec3 origin,Vec3 feet,float height) {
  for(float fraction:{.55f,.9f,.2f}) {
    const auto target=feet+Vec3{0,0,height*fraction};
    const auto delta=target-origin;const float distance=length(delta);
    const auto hit=world.raycast(origin,normalized(delta),distance);
    if(!hit||hit->distance>=distance-.15f)return target;
  }
  return {};
}
// Controller-driver routes for multiplayer caches without AI pathfinding.
// This samples actual walkable collision and never supplies engine state.
class GameplayDriverNavigation {
public:
  GameplayDriverNavigation(const LevelContent &content, const CollisionWorld &world)
      : authored_(content.navigation) {
    if (!content.navigation.empty()) return;
    const auto &biped = content.player;
    geometry_ = std::make_unique<CollisionWorld>(std::vector<CollisionTriangle>(
        world.triangles().begin(), world.triangles().end()));
    camera_height_ = biped.camera_height;
    body_height_ = biped.height;
    const float jump = biped.jump_velocity * biped.jump_velocity / (2 * retail_gravity);
    Vec3 low{INFINITY, INFINITY, INFINITY}, high{-INFINITY, -INFINITY, -INFINITY};
    for (const auto &t : world.triangles()) for (auto p : {t.a, t.b, t.c}) {
      low = {std::min(low.x, p.x), std::min(low.y, p.y), std::min(low.z, p.z)};
      high = {std::max(high.x, p.x), std::max(high.y, p.y), std::max(high.z, p.z)};
    }
    const float spacing = .5f;
    const int width = int(std::ceil((high.x - low.x) / spacing)) + 1;
    const int height = int(std::ceil((high.y - low.y) / spacing)) + 1;
    if (width <= 0 || height <= 0 || std::int64_t(width) * height > 250000) {
      geometry_.reset();
      return;
    }
    std::vector<std::vector<unsigned>> columns(std::size_t(width) * height);
    const auto clear = [&](Vec3 p) {
      return !world.overlaps(p + Vec3{0, 0, .003f}, biped.height, biped.radius);
    };
    for (const auto &t : world.triangles()) {
      const auto normal = normalized(cross(t.b - t.a, t.c - t.a));
      if (normal.z < biped.maximum_slope_cosine) continue;
      const auto ab = t.b - t.a, ac = t.c - t.a;
      const float denominator = ab.x * ac.y - ab.y * ac.x;
      if (std::abs(denominator) < 1e-8f) continue;
      const int x0 = std::max(0, int(std::ceil((std::min({t.a.x, t.b.x, t.c.x}) - low.x) / spacing)));
      const int x1 = std::min(width - 1, int(std::floor((std::max({t.a.x, t.b.x, t.c.x}) - low.x) / spacing)));
      const int y0 = std::max(0, int(std::ceil((std::min({t.a.y, t.b.y, t.c.y}) - low.y) / spacing)));
      const int y1 = std::min(height - 1, int(std::floor((std::max({t.a.y, t.b.y, t.c.y}) - low.y) / spacing)));
      for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
        const float px = low.x + spacing * x, py = low.y + spacing * y;
        const float u = ((px - t.a.x) * ac.y - (py - t.a.y) * ac.x) / denominator;
        const float v = (ab.x * (py - t.a.y) - ab.y * (px - t.a.x)) / denominator;
        if (u < 0 || v < 0 || u + v > 1) continue;
        const Vec3 p{px, py, t.a.z + u * ab.z + v * ac.z};
        auto &column = columns[std::size_t(y) * width + x];
        if (std::any_of(column.begin(), column.end(), [&](unsigned i) {
              return std::abs(nodes_[i].position.z - p.z) < .03f;
            }) || !clear(p)) continue;
        column.push_back(unsigned(nodes_.size()));
        nodes_.push_back({p, {}});
        // Keep controller planning from stalling the real-time game loop on
        // large outdoor geometry. The ordinary direct-walking driver remains
        // available; strict combat and full-match assertions stay unchanged.
        if(nodes_.size()>30000) {
          nodes_.clear();geometry_.reset();return;
        }
      }
    }
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
      for (auto i : columns[std::size_t(y) * width + x])
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
          if ((!dx && !dy) || x + dx < 0 || x + dx >= width || y + dy < 0 || y + dy >= height)
            continue;
          for (auto j : columns[std::size_t(y + dy) * width + x + dx]) {
            const auto a = nodes_[i].position, b = nodes_[j].position;
            if (b.z - a.z > jump * .8f || a.z - b.z > 1.f) continue;
            bool admitted = false;
            // A full-height arc can hit a low ceiling above usable stairs.
            // These are planner clearance samples, not simulated jump state.
            for (float lift : {0.f, .1f, .25f, .5f, jump}) {
              bool open = true;
              for (float f : {.25f, .5f, .75f}) {
                auto p = a + (b - a) * f;
                p.z += 4 * f * (1 - f) * lift;
                if (!clear(p)) { open = false; break; }
              }
              if (open) { admitted = true; break; }
            }
            if (admitted) nodes_[i].next.push_back(j);
          }
        }
  }

  std::vector<Vec3> route(Vec3 start, Vec3 goal) const {
    if (auto path = authored_.route(start, goal); !path.empty()) return path;
    const auto nearest = [&](Vec3 p) {
      unsigned result = unsigned(nodes_.size());
      float distance = 9;
      for (unsigned i = 0; i < nodes_.size(); ++i) {
        const auto d = nodes_[i].position - p;
        const float candidate = dot(d, d);
        if (candidate < distance) { result = i; distance = candidate; }
      }
      return result;
    };
    const auto from = nearest(start), to = nearest(goal), count = unsigned(nodes_.size());
    if (from == count || to == count) return {};
    std::vector<float> costs(count, INFINITY);
    std::vector<unsigned> parent(count, count);
    std::priority_queue<std::pair<float, unsigned>, std::vector<std::pair<float, unsigned>>,
                        std::greater<>> open;
    costs[from] = 0;
    open.push({0, from});
    while (!open.empty()) {
      const auto [cost, i] = open.top(); open.pop();
      if (i == to) break;
      if (cost > costs[i] + length(nodes_[i].position - nodes_[to].position) + .001f) continue;
      for (auto j : nodes_[i].next) {
        const float next = costs[i] + length(nodes_[j].position - nodes_[i].position);
        if (next >= costs[j]) continue;
        costs[j] = next; parent[j] = i;
        open.push({next + length(nodes_[j].position - nodes_[to].position), j});
      }
    }
    unsigned destination = to;
    if (from != to && parent[to] == count) {
      // Separate floors can exchange fire without sharing a walking route.
      // Find mutually visible, reachable firing positions for both controllers.
      std::vector<float> other(count, INFINITY);
      std::priority_queue<std::pair<float, unsigned>, std::vector<std::pair<float, unsigned>>,
                          std::greater<>> pending;
      other[to] = 0; pending.push({0, to});
      while (!pending.empty()) {
        const auto [cost, i] = pending.top(); pending.pop();
        if (cost > other[i]) continue;
        for (auto j : nodes_[i].next) {
          const float next = cost + length(nodes_[j].position - nodes_[i].position);
          if (next >= other[j]) continue;
          other[j] = next; pending.push({next, j});
        }
      }
      const auto candidates = [&](const std::vector<float> &distances, Vec3 target) {
        std::vector<unsigned> result;
        for (unsigned i = 0; i < count; ++i) if (std::isfinite(distances[i])) result.push_back(i);
        const auto end = result.begin() + std::min<std::size_t>(256, result.size());
        std::partial_sort(result.begin(), end, result.end(), [&](unsigned a, unsigned b) {
          return distances[a] + length(nodes_[a].position - target) <
                 distances[b] + length(nodes_[b].position - target);
        });
        result.erase(end, result.end());
        return result;
      };
      const auto visible = [&](Vec3 a, Vec3 b) {
        const auto origin = a + Vec3{0, 0, camera_height_};
        return length(a-b)<45 && visible_driver_target(*geometry_,origin,b,body_height_).has_value();
      };
      float best = INFINITY;
      destination = count;
      const auto own_candidates = candidates(costs, goal), other_candidates = candidates(other, start);
      for (auto i : own_candidates) for (auto j : other_candidates) {
        const auto a = nodes_[i].position, b = nodes_[j].position;
        const float score = costs[i] + other[j] + length(a - b) * .05f;
        if (score < best && visible(a, b) && visible(b, a)) { best = score; destination = i; }
      }
      if (destination == count) return {};
    }
    std::vector<Vec3> result{destination == to ? goal : nodes_[destination].position};
    for (auto i = destination; i != from; i = parent[i]) result.push_back(nodes_[i].position);
    std::reverse(result.begin(), result.end());
    return result;
  }
  std::size_t fallback_nodes() const { return nodes_.size(); }

private:
  struct Node { Vec3 position; std::vector<unsigned> next; };
  NavigationWorld authored_;
  std::unique_ptr<CollisionWorld> geometry_;
  float camera_height_{}, body_height_{};
  std::vector<Node> nodes_;
};
} // namespace halo2::test
