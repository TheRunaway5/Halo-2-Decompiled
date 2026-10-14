#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include <limits>
#include <numeric>
namespace halo2 {
namespace {
float component(Vec3 p, int axis) {
  return axis == 0 ? p.x : axis == 1 ? p.y : p.z;
}
std::optional<float> intersection(Vec3 o, Vec3 d, const CollisionTriangle &t,
                                  float limit) {
  const auto e = t.b - t.a, f = t.c - t.a, h = cross(d, f);
  const float det = dot(e, h);
  if (std::abs(det) < 1e-8f)
    return {};
  const float inv = 1 / det;
  const auto q = o - t.a;
  const float u = dot(q, h) * inv;
  if (u < 0 || u > 1)
    return {};
  const auto r = cross(q, e);
  const float v = dot(d, r) * inv;
  if (v < 0 || u + v > 1)
    return {};
  const float distance = dot(f, r) * inv;
  return distance >= 0 && distance <= limit ? std::optional<float>(distance)
                                            : std::nullopt;
}
Vec3 closest(Vec3 p, const CollisionTriangle &t) {
  const auto ab = t.b - t.a, ac = t.c - t.a, ap = p - t.a;
  const float d1 = dot(ab, ap), d2 = dot(ac, ap);
  if (d1 <= 0 && d2 <= 0)
    return t.a;
  const auto bp = p - t.b;
  const float d3 = dot(ab, bp), d4 = dot(ac, bp);
  if (d3 >= 0 && d4 <= d3)
    return t.b;
  const float vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0)
    return t.a + ab * (d1 / (d1 - d3));
  const auto cp = p - t.c;
  const float d5 = dot(ab, cp), d6 = dot(ac, cp);
  if (d6 >= 0 && d5 <= d6)
    return t.c;
  const float vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0)
    return t.a + ac * (d2 / (d2 - d6));
  const float va = d3 * d6 - d5 * d4;
  if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0)
    return t.b + (t.c - t.b) * ((d4 - d3) / (d4 - d3 + d5 - d6));
  const float inv = 1 / (va + vb + vc);
  return t.a + ab * (vb * inv) + ac * (vc * inv);
}
std::pair<Vec3, Vec3> segments(Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
  const auto u = b - a, v = d - c, w = a - c;
  const float aa = dot(u, u), bb = dot(u, v), cc = dot(v, v), dd = dot(u, w),
              ee = dot(v, w);
  float s = 0, t = 0;
  if (aa < 1e-12f && cc < 1e-12f)
    return {a, c};
  if (aa < 1e-12f)
    t = std::clamp(ee / cc, 0.f, 1.f);
  else if (cc < 1e-12f)
    s = std::clamp(-dd / aa, 0.f, 1.f);
  else {
    const float den = aa * cc - bb * bb;
    if (den > 1e-12f)
      s = std::clamp((bb * ee - cc * dd) / den, 0.f, 1.f);
    t = (bb * s + ee) / cc;
    if (t < 0) {
      t = 0;
      s = std::clamp(-dd / aa, 0.f, 1.f);
    } else if (t > 1) {
      t = 1;
      s = std::clamp((bb - dd) / aa, 0.f, 1.f);
    }
  }
  return {a + u * s, c + v * t};
}
std::pair<Vec3, Vec3> capsule_distance(Vec3 a, Vec3 b,
                                       const CollisionTriangle &t) {
  if (auto hit = intersection(a, b - a, t, 1)) {
    const auto p = a + (b - a) * *hit;
    return {p, p};
  }
  std::pair<Vec3, Vec3> best{a, closest(a, t)};
  float dist = dot(best.first - best.second, best.first - best.second);
  auto consider = [&](std::pair<Vec3, Vec3> pair) {
    const auto d = pair.first - pair.second;
    const float n = dot(d, d);
    if (n < dist) {
      dist = n;
      best = pair;
    }
  };
  consider({b, closest(b, t)});
  consider(segments(a, b, t.a, t.b));
  consider(segments(a, b, t.b, t.c));
  consider(segments(a, b, t.c, t.a));
  return best;
}
} // namespace
CollisionWorld::CollisionWorld(std::vector<CollisionTriangle> triangles)
    : geometry_(std::make_shared<std::vector<CollisionTriangle>>(std::move(triangles))),
      count_(geometry_->size()), acceleration_(accelerate(this->triangles())) {}
CollisionWorld::CollisionWorld(std::shared_ptr<std::vector<CollisionTriangle>> geometry,
    std::size_t count, std::shared_ptr<const Acceleration> acceleration)
    : geometry_(std::move(geometry)), count_(count),
      acceleration_(acceleration ? std::move(acceleration) : accelerate(triangles())) {}
std::shared_ptr<const CollisionWorld::Acceleration> CollisionWorld::accelerate(
    std::span<const CollisionTriangle> triangles) {
  if(triangles.size()>std::numeric_limits<std::uint32_t>::max())
    throw FormatError("Collision triangle indices exceed their 32-bit storage");
  auto acceleration = std::make_shared<Acceleration>();
  acceleration->order.resize(triangles.size());
  std::iota(acceleration->order.begin(), acceleration->order.end(), std::uint32_t{0});
  if (!triangles.empty()) build(triangles, *acceleration, 0, triangles.size());
  return acceleration;
}
CollisionWorld::Composition CollisionWorld::compose(
    const std::shared_ptr<const CollisionWorld> &bsp, Layers layers, PreviousLayers previous) {
  if (!bsp) throw FormatError("Collision composition requires its BSP owner");
  const auto base_count = bsp->size();
  auto fixed_count = base_count;
  for (const auto layer : {layers.scenery, layers.devices}) {
    if (layer.size() > bsp->geometry_->max_size() - fixed_count)
      throw FormatError("Collision layer bank exceeds its capacity");
    fixed_count += layer.size();
  }
  if (layers.props.size() > bsp->geometry_->max_size() - fixed_count)
    throw FormatError("Collision layer bank exceeds its capacity");
  const auto all_count = fixed_count + layers.props.size();
  std::shared_ptr<const Acceleration> fixed_acceleration;
  if (!previous.fixed_changed && previous.fixed && previous.fixed->size() == fixed_count)
    fixed_acceleration = previous.fixed->acceleration_;
  // Release superseded views before testing exclusive ownership. A checkpoint
  // or a copied collision view still retains its own geometry and BVH.
  previous.all.reset();
  previous.fixed.reset();
  if (all_count == base_count) return {bsp, bsp, bsp};
  std::shared_ptr<std::vector<CollisionTriangle>> geometry;
  // Multiple owners of this same BSP view protect only the immutable BSP
  // prefix. Its suffix may be rebuilt even when a checkpoint retains the view.
  // A distinct collision view/value protects the backing and forces a copy.
  if (bsp->geometry_.use_count() == 1)
    geometry = bsp->geometry_;
  else {
    geometry = std::make_shared<std::vector<CollisionTriangle>>();
    geometry->reserve(all_count);
    const auto base = bsp->triangles();
    geometry->insert(geometry->end(), base.begin(), base.end());
  }
  geometry->reserve(all_count);
  geometry->resize(base_count);
  for (const auto layer : {layers.scenery, layers.devices, layers.props})
    geometry->insert(geometry->end(), layer.begin(), layer.end());
  const auto view = [&](std::size_t count, std::shared_ptr<const Acceleration> acceleration = {}) {
    return std::shared_ptr<const CollisionWorld>(
        new CollisionWorld(geometry, count, std::move(acceleration)));
  };
  Composition result;
  result.bsp = geometry == bsp->geometry_ ? bsp : view(base_count, bsp->acceleration_);
  result.fixed = fixed_count == base_count ? result.bsp : view(fixed_count, std::move(fixed_acceleration));
  result.all = all_count == fixed_count ? result.fixed : view(all_count);
  return result;
}
int CollisionWorld::build(std::span<const CollisionTriangle> triangles, Acceleration &acceleration,
                          std::size_t begin, std::size_t end) {
  auto &order = acceleration.order;
  auto &nodes = acceleration.nodes;
  Node n;
  n.begin = std::uint32_t(begin);
  n.end = std::uint32_t(end);
  n.low = {INFINITY, INFINITY, INFINITY};
  n.high = {-INFINITY, -INFINITY, -INFINITY};
  for (auto i = begin; i < end; ++i) {
    const auto &t = triangles[order[i]];
    n.low = minimum(n.low, minimum(t.a, minimum(t.b, t.c)));
    n.high = maximum(n.high, maximum(t.a, maximum(t.b, t.c)));
  }
  const int index = int(nodes.size());
  nodes.push_back(n);
  if (end - begin > 8) {
    const auto extent = n.high - n.low;
    const int axis = extent.x > extent.y && extent.x > extent.z ? 0
                     : extent.y > extent.z                      ? 1
                                                                : 2;
    const auto mid = (begin + end) / 2;
    std::nth_element(order.begin() + begin, order.begin() + mid,
                     order.begin() + end, [&](auto a, auto b) {
                       const auto &x = triangles[a];
                       const auto &y = triangles[b];
                       return component(x.a + x.b + x.c, axis) <
                              component(y.a + y.b + y.c, axis);
                     });
    const int left = build(triangles, acceleration, begin, mid), right = build(triangles, acceleration, mid, end);
    nodes[index].left = left;
    nodes[index].right = right;
  }
  return index;
}
std::vector<std::size_t> CollisionWorld::query(Vec3 low, Vec3 high) const {
  std::vector<std::size_t> result;
  if (acceleration_->nodes.empty())
    return result;
  std::vector<int> stack{0};
  while (!stack.empty()) {
    const auto n = acceleration_->nodes[stack.back()];
    stack.pop_back();
    if (n.high.x < low.x || n.low.x > high.x || n.high.y < low.y ||
        n.low.y > high.y || n.high.z < low.z || n.low.z > high.z)
      continue;
    if (n.left < 0)
      result.insert(result.end(), acceleration_->order.begin() + n.begin,
                    acceleration_->order.begin() + n.end);
    else {
      stack.push_back(n.left);
      stack.push_back(n.right);
    }
  }
  return result;
}
std::optional<RayHit> CollisionWorld::raycast(Vec3 origin, Vec3 dir,
                                              float distance) const {
  if (!std::isfinite(distance) || distance < 0 || length(dir) < 1e-8f)
    return {};
  dir = normalized(dir);
  const auto end = origin + dir * distance;
  std::optional<RayHit> best;
  for (const auto i : query(minimum(origin, end), maximum(origin, end))) {
    const auto &t = (*geometry_)[i];
    if (auto hit = intersection(origin, dir, t, distance)) {
      auto n = normalized(cross(t.b - t.a, t.c - t.a));
      if (dot(n, dir) > 0)
        n = n * -1;
      distance = *hit;
      const auto authored=t.source.bsp>=0?t.source_normal:cross(t.b-t.a,t.c-t.a);
      best = RayHit{*hit, origin + dir * *hit, n, i, t.source,t.flags,dot(authored,dir)>0,t.material};
    }
  }
  return best;
}
bool CollisionWorld::resolve(Body &b) const {
  bool touched = false;
  for (int iteration = 0; iteration < 8; ++iteration) {
    const auto a = b.position + Vec3{0, 0, b.radius},
               c = b.position +
                   Vec3{0, 0, std::max(b.radius, b.height - b.radius)};
    const auto margin =
        Vec3{b.radius + 0.001f, b.radius + 0.001f, b.radius + 0.001f};
    bool changed = false;
    for (auto i : query(minimum(a, c) - margin, maximum(a, c) + margin)) {
      const auto &t = (*geometry_)[i];
      const auto current_a = b.position + Vec3{0, 0, b.radius};
      const auto current_c = b.position +
          Vec3{0, 0, std::max(b.radius, b.height - b.radius)};
      const auto pair = capsule_distance(current_a, current_c, t);
      const auto delta = pair.first - pair.second;
      const float distance = length(delta);
      if (distance >= b.radius)
        continue;
      auto normal = distance > 1e-6f ? delta / distance
                                     : normalized(cross(t.b - t.a, t.c - t.a));
      if (distance <= 1e-6f && dot(normal, b.velocity) > 0)
        normal = normal * -1;
      b.position = b.position + normal * (b.radius - distance + 0.0001f);
      const float inward = dot(b.velocity, normal);
      if(normal.z>=b.maximum_slope_cosine && inward<0)
        b.landing_speed=std::max(b.landing_speed,-b.velocity.z);
      if (inward < 0)
        b.velocity = b.velocity - normal * inward;
      if (normal.z >= b.maximum_slope_cosine) {
        b.grounded = true;
        b.support_normal = normal;
        b.support_triangle = i;
      }
      touched = changed = true;
      // Process every candidate with the corrected capsule. Repeated wall
      // contacts must not consume the entire pass before floor support runs.
    }
    if (!changed)
      break;
  }
  return touched;
}
Vec3 collision_triangle_closest_point(Vec3 p,const CollisionTriangle& t){return closest(p,t);}
void CollisionWorld::move(Body &b, float dt) const {
  if (!(b.radius > 0) || !(b.height >= 2 * b.radius) || !(dt >= 0) ||
      !std::isfinite(dt) || !std::isfinite(b.maximum_slope_cosine) ||
      b.maximum_slope_cosine < 0 || b.maximum_slope_cosine > 1)
    throw FormatError("invalid capsule movement");
  const float subdivisions =
      std::ceil(length(b.velocity) * dt / (b.radius * .35f));
  if (!std::isfinite(subdivisions) || subdivisions > 4096)
    throw FormatError("capsule movement exceeds subdivision limit");
  const int steps = std::max(1, int(subdivisions));
  b.grounded = false;
  b.landing_speed=0;
  b.support_triangle.reset();
  for (int n = 0; n < steps; ++n) {
    b.position = b.position + b.velocity * (dt / steps);
    resolve(b);
  }
  if (!b.grounded && b.velocity.z <= 0) {
    const auto support =
        raycast(b.position + Vec3{0, 0, .001f}, {0, 0, -1}, .006f);
    if (support && support->normal.z >= b.maximum_slope_cosine) {
      // This is also an admitted support contact, even without triangle
      // penetration. Preserve its incoming speed for the native landing owner.
      b.landing_speed=std::max(b.landing_speed,-b.velocity.z);
      b.grounded = true;
      b.support_normal = support->normal;
      b.support_triangle = support->triangle;
    }
  }
}
bool CollisionWorld::overlaps(Vec3 feet, float height, float radius) const {
  const auto a = feet + Vec3{0, 0, radius},
             b = feet + Vec3{0, 0, height - radius},
             r = Vec3{radius, radius, radius};
  for (auto i : query(minimum(a, b) - r, maximum(a, b) + r)) {
    const auto p = capsule_distance(a, b, (*geometry_)[i]);
    if (length(p.first - p.second) < radius - 0.0001f)
      return true;
  }
  return false;
}
} // namespace halo2
