#pragma once
#include "halo2/posed_collision.hpp"
#include "halo2/model.hpp"
#include "halo2/scene.hpp"
#include <map>

namespace halo2 {
struct StaticSceneryCollisionDefinition {
  ModelCollision collision;
  RenderModel hierarchy;
  std::vector<std::uint8_t> permutations;
};
// Restricted fixed owner: scen, real coll, deterministic regions, one node,
// no animation graph, physics model or damage transitions. Missing ownership
// stays unresolved; this never freezes a dynamic object into a static mesh.
std::shared_ptr<const StaticSceneryCollisionDefinition> load_static_scenery_collision_definition(
    const Cache&,const ScenarioObject&);
// Authored polygons/current WORLD nodes -> the existing host triangle backend.
// This does not implement Havok broadphase, motion, contact or shape dispatch.
std::vector<CollisionTriangle> model_collision_triangles(const ModelCollision&,
    std::span<const Transform>,std::span<const std::uint8_t>,std::size_t placement);
class StaticSceneryCollision {
public:
  explicit StaticSceneryCollision(const LevelContent&);
  bool synchronize(const SceneObjects&);
  std::span<const CollisionTriangle> triangles()const{return triangles_;}
private:
  const LevelContent* content_;
  struct Owner {std::size_t placement{};Transform root;bool active{};std::optional<std::vector<Transform>> solved_nodes;};
  std::map<std::uint32_t,Owner> owners_;
  std::vector<CollisionTriangle> triangles_;
};
}
