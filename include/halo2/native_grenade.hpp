#pragma once
#include "halo2/model.hpp"
#include "halo2/unit_grenade.hpp"
#include "halo2/native_grenade_constants.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_effect_location.hpp"
#include <functional>
#include <map>

namespace halo2 {
struct Unit;
struct SourceProjectileDetonationTu15;
struct DamageOwner;
struct PropAreaDamageDefinition;
class CollisionWorld;
// Immutable selected-image words, not mutable projectile/contact state.
struct NativeGrenadeAction {
  SourceGrenadeAction source;
  std::uint32_t graph=0xffffffffu,clip=0xffffffffu,activity=0xffffffffu;
  float frame{};
  bool animation_active{};
  NativeGrenadeAction();
  std::uint32_t active_actions()const;
  std::uint8_t phase()const;
  std::uint8_t counter()const;
  std::uint32_t held()const;
};
struct NativeGrenadeProjectile {
  std::uint32_t datum=0xffffffffu,definition=0xffffffffu,parent=0xffffffffu;
  std::uint32_t player=0xffffffffu,unit=0xffffffffu;
  std::int16_t designator=-1,type=-1;
  int native_owner=-2;
  Vec3 position,velocity;
  // Authored creation frame retained through attachment/detach, never inferred
  // from flight velocity. Released angular integration remains unavailable.
  Vec3 forward,up;
  std::uint32_t flags=2;
  std::int16_t state{},age{};
  float timer{},timer_step{},arming{},arming_step{};
  float traveled{};
};
struct NativeGrenadeEvent {
  enum Kind { Begin,Prepared,Released,Contact,Detonated,Unavailable } kind;
  std::uint32_t unit=0xffffffffu,projectile=0xffffffffu,definition=0xffffffffu,effect=0xffffffffu;
  Vec3 position,velocity;
  std::uint32_t activity=0xffffffffu,graph=0xffffffffu,clip=0xffffffffu;
  std::uint8_t counter{},phase{};
};
// Native presentation retention of actual detonation callbacks. The64-entry
// bound is a transport design, not original gameplay timing or losslessness.
struct NativeGrenadeDetonation {
  std::uint64_t serial{},tick{};
  std::uint32_t projectile=0xffffffffu,definition=0xffffffffu,effect=0xffffffffu;
  std::uint32_t player=0xffffffffu,unit=0xffffffffu;
  Vec3 position,forward,up;
  std::vector<SourceEffectLocation> source_locations;
  SourceEffectColors colors;
  SourceEffectControls controls{1,1};
};
// FCF90 entry through FD0F8, before any world query. Source constructor
// supplies timer_step/arming_step; contact flags are actual backend inputs.
void advance_native_grenade_timer(NativeGrenadeProjectile&,std::int16_t timer_start,float one);
// Native residency/world adapter over actual imported tags and decoded clips.
// It runs the retained E99B0 action and ordinary source release/timer/bounce
// arithmetic; full Xbox body/Havok/audio/effect admission is not asserted.
class NativeGrenadeSandbox {
public:
  NativeGrenadeSandbox(std::shared_ptr<const Cache>,std::uint32_t unit_definition,
      NativeGrenadeConstants);
  using Allocate=std::function<std::uint32_t(std::uint8_t)>;
  using Constructed=std::function<void(std::uint32_t,std::uint32_t)>;
  using Activity=std::function<void(const Unit&,std::uint32_t)>;
  using Area=std::function<void(const PropAreaDamageDefinition&,Vec3,
      std::uint32_t player,std::uint32_t unit,std::int16_t designator,int native_owner,
      std::uint32_t projectile_definition)>;
  // The triangle owner retains scenario placement identities for object hits.
  // Resolve their actual type only when the source response gate needs it.
  using CollisionObjectType=std::function<std::uint8_t(std::size_t placement)>;
  void update_unit(Unit&,bool pressed,int native_owner,const Allocate&,const Activity&,
      const Constructed& = {});
  void cancel_unit(Unit&);
  // Presentation adoption of completed source physics bodies. Motion, fuse,
  // contact and retirement stay with that world, never this native sandbox.
  struct PresentationOwner {const Unit *unit;int native_owner;};
  void adopt_source_projectiles(std::vector<NativeGrenadeProjectile>,std::span<const PresentationOwner>);
  void adopt_source_detonations(std::uint64_t tick,std::span<const SourceProjectileDetonationTu15>);
  void update_projectiles(std::uint64_t tick,const CollisionWorld&,const Area&,
      const std::function<void(std::uint32_t)>& release,const CollisionObjectType&);
  std::span<const NativeGrenadeProjectile> projectiles()const{return projectiles_;}
  std::vector<NativeGrenadeEvent> take_events();
  std::span<const NativeGrenadeDetonation> detonations()const{return detonations_;}
  const AnimationClip* action_clip(std::uint32_t graph,std::uint32_t clip)const;
private:
  struct Definition;
  struct UnitDefinition;
  struct WeaponClips;
  std::shared_ptr<const Cache> cache_;
  std::shared_ptr<const Definition> definition_;
  std::map<std::uint32_t,std::shared_ptr<const UnitDefinition>> units_;
  std::map<std::pair<std::uint32_t,std::uint32_t>,std::shared_ptr<const WeaponClips>> clips_;
  mutable std::map<std::pair<std::uint32_t,std::uint32_t>,AnimationClip> selected_clips_;
  std::vector<NativeGrenadeProjectile> projectiles_;
  std::vector<NativeGrenadeEvent> events_;
  std::vector<NativeGrenadeDetonation> detonations_;
  std::uint64_t detonation_serial_{};
  std::map<std::uint32_t,std::pair<unsigned,ObjectAttachmentFrame>> attachments_;
  const UnitDefinition &unit_definition(std::uint32_t);
  std::shared_ptr<const WeaponClips> weapon_clips(std::uint32_t unit,std::uint32_t weapon);
  void prepare(Unit&,int,const Allocate&,const Constructed&);
  void release(Unit&);
};
} // namespace halo2
