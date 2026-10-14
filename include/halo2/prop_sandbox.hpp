#pragma once
#include "halo2/collision.hpp"
#include "halo2/model.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/posed_collision.hpp"
#include "halo2/projectile_material_response.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_model_regions.hpp"
#include <map>

namespace halo2 {
struct PropDamageResponse {
  std::uint32_t flags{}, effect=null_datum, damage=null_datum,
      marker{}, delay_effect=null_datum;
  std::int16_t state{}, region=-1;
  float threshold{}, delay{}, skip_fraction{}, total_threshold{};
  std::uint32_t effect_marker{};
};
struct PropDamageSection {
  std::uint32_t flags{};
  float vitality{};
  std::vector<PropDamageResponse> responses;
};
struct PropSandboxDefinition {
  std::uint32_t datum=null_datum;
  RenderModel model; // Authored hierarchy only; no render resources retained.
  ModelCollision collision;
  SourceInitialModelDefinition region_definition;
  SourceInitialModelSelection initial_regions;
  std::map<std::uint32_t,std::vector<ObjectAttachmentMarker>> markers;
  VitalityDefinition vitality;
  std::vector<PropDamageSection> sections;
  // Admit the contact material consumer only when every authored entry
  // agrees with the default. Distinct hit-region selection remains an owner.
  bool uniform_impact_material{},uniform_impact_region_absorption{};
  bool impact_region_absorbs{};
};
struct PropAreaDamageDefinition {
  WeaponDefinition damage;
  float radius_minimum{},radius_maximum{},core_radius{},acceleration{};
  // Literal secondary inputs. Their acceleration/camouflage owners are
  // unavailable; importing them never fabricates those side effects.
  float source_58{},source_64{},source_68{};
  std::uint32_t flags{}; // jpt!+0C, distinct from definition flags+14.
};
std::shared_ptr<const PropAreaDamageDefinition> load_prop_area_damage_definition(
    const Cache&,std::uint32_t);
struct PropDamageOwner {
  std::uint32_t player=null_datum,unit=null_datum;
  std::int16_t designator=-1;
  int native_owner=-2;
};
std::shared_ptr<const PropSandboxDefinition> load_prop_sandbox_definition(
    const Cache&,const ScenarioObject&);
struct PropSandboxEvent {
  std::uint32_t handle{}, effect=null_datum, damage=null_datum;
  Vec3 position, forward;
  bool remove{};
  PropDamageOwner owner;
  Vec3 effect_position, effect_forward;
};
// Native authoritative lifetime replication, not Xbox wire/object storage.
struct PropRemoval {
  std::uint32_t handle{},placement{},definition{};
  // Literal emitted visual request, retained at the completed native tick.
  // Clients never reconstruct a response from the object's definition.
  std::uint32_t effect=null_datum;
  Vec3 effect_position{},effect_forward{};
  std::uint64_t event_tick{};
  bool operator==(const PropRemoval& b)const {
    return handle==b.handle && placement==b.placement && definition==b.definition &&
        effect==b.effect && event_tick==b.event_tick &&
        effect_position.x==b.effect_position.x && effect_position.y==b.effect_position.y && effect_position.z==b.effect_position.z &&
        effect_forward.x==b.effect_forward.x && effect_forward.y==b.effect_forward.y && effect_forward.z==b.effect_forward.z;
  }
};
struct PropSandboxState {
  std::uint32_t handle{}, definition{};
  std::size_t placement{};
  RechargeState vitality;
  PropDamageOwner owner;
  struct Section {std::uint16_t destroyed{}, pending{};std::uint8_t damage{};};
  std::vector<Section> sections;
  SourceModelRegionBank regions;
};
// Completed native region state for this exact prop generation. Late joiners
// receive the authoritative choice; they never re-run damage or consume RNG.
struct PropRegionSnapshot {
  std::uint32_t handle{},placement{},definition{};
  SourceModelRegionBank regions;
};
// Actual authored collision surfaces/current scene roots, composed into the
// existing host triangle mover. This is not a Havok solver implementation.
class PropSandbox {
public:
  explicit PropSandbox(const LevelContent&);
  bool synchronize(const SceneObjects&);
  std::vector<CollisionTriangle> triangles()const;
  struct Hit {std::uint32_t handle{};PosedCollisionHit collision;};
  std::optional<Hit> raycast(Vec3 origin,Vec3 delta,float maximum_fraction=1,
      std::uint32_t ignored=null_datum,std::uint32_t ignored_other=null_datum)const;
  bool damage(std::uint32_t,const WeaponDefinition&,float amount,PropDamageOwner = {},bool radius=false,
      const SourceModelRegionOwners & = {});
  struct DamageContactResult {
    bool changed{};
    std::optional<SourceProjectileDamagePresentation> presentation;
  };
  DamageContactResult damage_contact(std::uint32_t,const WeaponDefinition&,float amount,
      PropDamageOwner = {},bool radius=false,const SourceModelRegionOwners & = {});
  // Host geometry adapter over the same exact posed authored triangles.
  // Retail185380's physical point-shape/Havok dispatcher remains separate.
  Vec3 closest_point(std::uint32_t,Vec3)const;
  std::vector<PropSandboxEvent> tick(const std::function<void(const PropSandboxEvent&)>& owner={},
      const SourceModelRegionOwners & = {});
  const SourceInitialModelSelection *region_selection(std::uint32_t)const;
  std::vector<PropRegionSnapshot> region_snapshot()const;
  void apply_regions(std::span<const PropRegionSnapshot>);
  const PropSandboxState& state(std::uint32_t)const;
  const std::vector<PropSandboxState> states()const;
private:
  struct Prop {PropSandboxState state;const PropSandboxDefinition* definition{};
    Transform root;std::vector<Transform> nodes;bool active{};};
  const LevelContent* content_;
  std::map<std::uint32_t,Prop> props_;
  std::vector<PropSandboxEvent> events_;
  bool collision_dirty_{};
  void deplete(Prop&,const SourceModelRegionOwners &);
  void dispatch(Prop&,unsigned section,unsigned response,const SourceModelRegionOwners &);
};
// D8AE0 ordinary spherical falloff, after genuine current target-point input.
// Zero fraction suppresses damage; lower damage is not applied outside range.
float prop_area_fraction(const PropAreaDamageDefinition&,float distance);
// D8280 candidate radius includes acceleration/camouflage radii. Their
// independent consumers remain unavailable; health still uses +0/+4 only.
float prop_area_candidate_radius(const PropAreaDamageDefinition&);
struct PropAreaCastResult {bool hit{};Vec3 point;};
using PropAreaQuery=std::function<PropAreaCastResult(Vec3,Vec3,std::uint32_t)>;
// Complete D8590 ordinary and biped/core branches; each clipping query's
// corrected endpoint is consumed by the following cast. World query is explicit.
bool prop_area_occluded(Vec3 origin,Vec3 target,bool biped,float core_radius,
    const PropAreaQuery&);
}
