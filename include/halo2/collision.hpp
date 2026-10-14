#pragma once
#include "halo2/content.hpp"
#include <memory>
#include <optional>
namespace halo2 {
// Existing host triangle backend's closest-point calculation.
Vec3 collision_triangle_closest_point(Vec3,const CollisionTriangle&);
struct RayHit {
  float distance{};
  Vec3 position, normal;
  std::size_t triangle{};
  CollisionSurfaceIdentity source;
  std::uint8_t surface_flags{};
  bool backface{}; // Authored plane orientation before host normal flipping.
  std::uint16_t material{}; // Global material, resolved by the triangle importer.
};
struct Body {
  Vec3 position, velocity;
  float height{}, radius{};
  bool grounded{};
  float maximum_slope_cosine = .65f;
  Vec3 support_normal{0, 0, 1};
  // Actual native solver contact. This is not a retail Havok material query.
  std::optional<std::size_t> support_triangle;
  float landing_speed{}; // Native incoming downward speed at a support contact.
};
// Host triangle BVH/capsule solver, not a port of Havok or retail contact code.
class CollisionWorld {
public:
  struct Layers {
    std::span<const CollisionTriangle> scenery, devices, props;
  };
  struct PreviousLayers {
    std::shared_ptr<const CollisionWorld> fixed, all;
    bool fixed_changed = true;
  };
  struct Composition;
  explicit CollisionWorld(std::vector<CollisionTriangle> triangles);
  // All three views share one triangle bank. Each retains its original BVH,
  // triangle indices and candidate order. Retained views force copy-on-write.
  // Recomposition invalidates borrowed spans; layer inputs must be independent.
  static Composition compose(const std::shared_ptr<const CollisionWorld> &bsp,
                             Layers, PreviousLayers);
  std::optional<RayHit> raycast(Vec3 origin, Vec3 direction,
                                float distance) const;
  void move(Body &, float dt) const;
  bool overlaps(Vec3 feet, float height, float radius) const;
  std::size_t size() const { return count_; }
  std::span<const CollisionTriangle> triangles() const {
    return std::span<const CollisionTriangle>(*geometry_).first(count_);
  }

private:
  struct Node {
    Vec3 low, high;
    std::size_t begin{}, end{};
    int left = -1, right = -1;
  };
  struct Acceleration {
    std::vector<std::size_t> order;
    std::vector<Node> nodes;
  };
  std::shared_ptr<std::vector<CollisionTriangle>> geometry_;
  std::size_t count_{};
  std::shared_ptr<const Acceleration> acceleration_;
  CollisionWorld(std::shared_ptr<std::vector<CollisionTriangle>>, std::size_t,
                 std::shared_ptr<const Acceleration> = {});
  static std::shared_ptr<const Acceleration> accelerate(std::span<const CollisionTriangle>);
  static int build(std::span<const CollisionTriangle>, Acceleration &,
                   std::size_t begin, std::size_t end);
  std::vector<std::size_t> query(Vec3 low, Vec3 high) const;
  bool resolve(Body &) const;
};
struct CollisionWorld::Composition {
  std::shared_ptr<const CollisionWorld> bsp, fixed, all;
};
} // namespace halo2
