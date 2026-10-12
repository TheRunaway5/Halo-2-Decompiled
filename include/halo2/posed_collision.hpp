#pragma once
#include "halo2/transform.hpp"
#include "halo2/original_data.hpp"
#include <limits>

namespace halo2 {
struct CollisionPlane { Vec3 normal; float distance{}; };
struct CollisionNode3 { std::int16_t plane{}; std::array<std::int32_t,2> children{}; };
struct CollisionLeaf { std::uint8_t flags{}, count{}; std::uint16_t first{}; };
struct CollisionReference2 { std::int16_t plane{}, node{}; };
struct CollisionNode2 { Vec2 normal; float distance{}; std::array<std::int16_t,2> children{}; };
struct CollisionSurface {
  std::int16_t plane{}; std::uint16_t first_edge{};
  std::uint8_t flags{}, breakable{}; std::int16_t material{};
};
struct CollisionEdge {
  std::array<std::uint16_t,2> vertices{}, next{}, surfaces{};
};
struct RawCollisionBsp {
  std::int16_t node{};
  std::vector<CollisionNode3> nodes;
  std::vector<CollisionPlane> planes;
  std::vector<CollisionLeaf> leaves;
  std::vector<CollisionReference2> references;
  std::vector<CollisionNode2> nodes2;
  std::vector<CollisionSurface> surfaces;
  std::vector<CollisionEdge> edges;
  std::vector<Vec3> vertices;
  // Literal vertex+0C incident edge;245400 uses its first surface's metadata.
  std::vector<std::uint16_t> vertex_edges;
};
struct TerrainCollision {
  std::vector<RawCollisionBsp> bsps;
  std::vector<std::int16_t> global_materials;
  std::vector<std::int16_t> leaf_clusters; // sbsp+2C/+30, stride8, signed cluster+0.
};
struct RawCollisionHit {
  float fraction{};
  std::int32_t crossing_plane{}, leaf{}, surface{}, signed_plane{};
  std::int16_t material{};
  std::uint8_t flags{}, breakable{};
};
struct RawCollisionResult {
  std::optional<RawCollisionHit> hit;
  std::vector<std::int32_t> visited_leaves;
  std::int32_t prior_leaf=-1, prior_class{}, prior_plane=-1;
};
// TU1E2020/1E20C0 lower query, without model transforms or the outer world
// dispatcher. Runtime breakable state is explicit. Misses have no hit fields.
RawCollisionResult raw_collision(const RawCollisionBsp &, const OriginalData &,
    Vec3 origin, Vec3 delta, unsigned lower_flags,
    std::span<const std::uint32_t> breakable_bits, unsigned breakable_count,
    float previous_fraction=std::numeric_limits<float>::max());
struct ModelCollision {
  std::uint32_t model{}, collision{};
  struct Material { std::int16_t damage_section{}, global_material{}; };
  struct Region { std::uint8_t collision{}; std::vector<std::uint8_t> permutations; };
  using Permutation=std::vector<RawCollisionBsp>;
  std::vector<Material> materials;
  std::vector<Region> model_regions;
  std::vector<std::vector<Permutation>> regions;
  std::vector<Transform> authored_inverse;
};
struct PosedCollisionHit {
  float fraction{};
  CollisionPlane plane;
  std::uint32_t packed_bsp{};
  std::int32_t leaf{}, surface{}, signed_plane{};
  std::int16_t region{}, node{}, model_material{}, global_material{};
  std::uint8_t flags{}, breakable{};
};
// TU1E25C0, with an explicit projection row and optional breakable bitmap.
bool collision_surface_contains(const RawCollisionBsp &, unsigned surface,
    Vec2 point, std::array<std::int16_t,3> projection,
    std::span<const std::uint32_t> breakable_bits={}, unsigned breakable_count=0);
// TU16AF90/1F3900. Current node transforms and active model permutations must
// come from a separately established live producer. No static-pose substitution.
std::optional<PosedCollisionHit> posed_collision(const ModelCollision &,
    std::span<const Transform> nodes, std::span<const std::uint8_t> active,
    const OriginalData &, Vec3 origin, Vec3 delta, unsigned lower_flags,
    float previous_fraction=1.f);
}
