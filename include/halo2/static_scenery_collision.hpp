#pragma once
#include "halo2/posed_collision.hpp"
#include "halo2/model.hpp"
#include "halo2/scene.hpp"
#include "halo2/native_authority_boundary.hpp"
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
  struct AuthorityOwner {std::size_t placement{};Transform root;bool active{};std::optional<std::vector<Transform>> solved_nodes;};
  struct AuthorityCheckpoint {
    std::uint32_t version=1;
    std::array<std::byte,32> scene_binding{},definition_binding{};
    std::map<std::uint32_t,AuthorityOwner> owners;
    std::vector<CollisionTriangle> triangles;
  };
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const StaticSceneryCollision &)const noexcept;
  private:
    friend class StaticSceneryCollision;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Compose under the root's barrier with its admitted scene and immutable
  // image/map/schema. Cached triangle values/order survive and are checked
  // against a target-content rebuild. Content pointers never cross import.
  AuthorityCheckpoint authority_checkpoint(const SceneObjects::AuthorityCheckpoint &)const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &,const SceneObjects::AuthorityCheckpoint &);
  bool commit_authority(PreparedAuthority &&) noexcept;
private:
  NativeAuthorityBoundary authority_;
  const LevelContent* content_;
  using Owner=AuthorityOwner;
  std::map<std::uint32_t,Owner> owners_;
  std::vector<CollisionTriangle> triangles_;
};
}
