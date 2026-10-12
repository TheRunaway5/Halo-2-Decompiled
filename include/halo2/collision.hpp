#pragma once
#include "halo2/content.hpp"
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
  explicit CollisionWorld(std::vector<CollisionTriangle> triangles);
  std::optional<RayHit> raycast(Vec3 origin, Vec3 direction,
                                float distance) const;
  void move(Body &, float dt) const;
  bool overlaps(Vec3 feet, float height, float radius) const;
  std::size_t size() const { return triangles_.size(); }
  std::span<const CollisionTriangle> triangles() const { return triangles_; }

private:
  struct Node {
    Vec3 low, high;
    std::size_t begin{}, end{};
    int left = -1, right = -1;
  };
  std::vector<CollisionTriangle> triangles_;
  std::vector<std::size_t> order_;
  std::vector<Node> nodes_;
  int build(std::size_t begin, std::size_t end);
  std::vector<std::size_t> query(Vec3 low, Vec3 high) const;
  bool resolve(Body &) const;
};
} // namespace halo2
