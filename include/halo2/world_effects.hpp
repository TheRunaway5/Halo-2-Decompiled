#pragma once
#include "halo2/source_effect_random.hpp"
#include "halo2/source_effect_location.hpp"
#include "halo2/bullet_impact.hpp"
#include "halo2/melee_impact.hpp"
#include "halo2/collision.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/effect_light_registration.hpp"
#include "halo2/effect_light_view.hpp"
#include "halo2/effect_light_receiver.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_attached_effect.hpp"
#include "halo2/source_effect_replica_render_tu15.hpp"
#include <memory>
#include <map>

namespace halo2 {
class PhysicsLiveWorldTu15;
struct WorldEffectFrame {
  Vec3 position, normal, incoming;
  Vec3 forward{1, 0, 0}, up{0, 0, 1};
  bool terrain{};
  float scale_a = 1, scale_b = 1;
};
struct WorldEffectDiagnostics {
  std::size_t effects{}, particles{}, decals{}, systems{}, lights{};
  std::vector<std::string> unavailable;
};
enum class WorldEffectSoundKind { Part, Material, Collision, Death };
struct WorldEffectSound {
  std::uint32_t sound = 0xffffffffu, owner = 0xffffffffu;
  std::uint64_t tick{};
  Vec3 position;
  float gain = 1;
  WorldEffectSoundKind kind = WorldEffectSoundKind::Part;
};
struct WorldEffectLight {
  std::uint32_t definition=0xffffffffu,owner=0xffffffffu,lens=0xffffffffu,flags{};
  std::uint64_t tick{};
  Vec3 position,forward,up;
  float radius{},specular_radius{};
  EffectLightState state;
  // Transport/effect identity is distinct from C0350's attached object fields.
  // Standalone world parts have owner NONE even when the effect was delivered
  // by a retained projectile/contact event.
  std::uint32_t effect_owner=0xffffffffu;
  SourceEffectLightRegistration registration;
  std::uint32_t identity=0xffffffffu,effect_definition=0xffffffffu;
};
struct WorldEffectLightReceiver {
  std::uint32_t identity{},epoch{};
  float selected_fade{};
  SourceEffectLightReceiverView view;
};
struct WorldEffectLensRequest {
  SourceEffectReplicaRenderTu15 source;
  std::uint32_t lens{};
  unsigned ordinal{};
  Vec3 position,forward;
  float opacity{};
};
// Authored effe/prt3/pmov/deca playback over the native triangle backend.
// All textures are made resident before Renderer construction. Simulation is
// once per game tick; per-view drawing is read-only and resolution independent.
class NativeWorldEffects {
  struct Impl;
public:
  NativeWorldEffects(const Cache &, Mesh &, const CollisionWorld &,
                     std::uint32_t visual_seed);
  NativeWorldEffects(const Cache &, Mesh &, const CollisionWorld &,
                     SourceEffectRandom);
  ~NativeWorldEffects();
  // Owning local presentation cut. No native world, renderer, playing audio or
  // room/generation proof is implied. Definitions and raw internal links are
  // copied/rebound; GPU material identities remain resident and unchanged.
  // Immutable BSP light geometry shares residency; mutable light pools,
  // spatial registration, fades and query/selection state remain independent.
  class Checkpoint {
  public:
    std::optional<std::uint64_t> tick()const noexcept;
    std::array<std::uint32_t,2> random()const;
  private:
    friend class NativeWorldEffects;
    struct Data;
    std::shared_ptr<const Data> data_;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();
    PreparedRestore(PreparedRestore &&)noexcept;
    PreparedRestore &operator=(PreparedRestore &&)noexcept;
  private:
    friend class NativeWorldEffects;
    PreparedRestore();
    std::unique_ptr<Impl> replacement_;
    std::shared_ptr<const int> owner_;
  };
  Checkpoint checkpoint()const;
  // Same local owner/assets only, at a completed frame. Allocate every owner
  // before publishing any. The enclosing coordinator restores shared visual
  // RNG atomically with the other consumers; preparation never writes it.
  // Supplied RNG words outlive the restored owner and preserve their captured
  // alias relation. Supply accepted-world collision callbacks explicitly, and
  // keep this owner quiescent until commit.
  PreparedRestore prepare_restore(const Checkpoint &,SourceEffectRandom,
      ParticleCollisionQuery)const;
  bool commit_restore(PreparedRestore &&)noexcept;
  bool restore_ready(const PreparedRestore &)const noexcept;
  // Completed source cache ownership replaces only procedural decal parts.
  // A present empty authoritative frame still owns and clears this output.
  void source_decal_cache(bool authoritative);
  void impact(const BulletImpact &);
  void melee_impact(const MeleeImpact &);
  void spawn(std::uint32_t effect, std::uint32_t owner, std::uint64_t tick,
             const WorldEffectFrame &);
  // Completed source location frames override every authored marker by its
  // actual string ID. Native presentation supplies no replacement direction.
  void spawn_source(std::uint32_t effect,std::uint32_t owner,std::uint64_t tick,
      std::span<const SourceEffectLocation>,SourceEffectColors={},SourceEffectControls={1,1},
      std::optional<SourceEffectReplicaRenderTu15> replica_render={});
  // Read-only requests from actual imported render provenance and the same
  // native original event clock as the effect's other presentation parts.
  std::vector<WorldEffectLensRequest> replica_lenses()const;
  // Preload native particle/light/sound/decal components before renderer
  // construction. Other part owners are reported separately; this alone is
  // not proof that every authored part can play.
  void preload_attached(std::uint32_t effect);
  // Strict graph admission for callers requiring every authored part.
  // Retained source lens/damage/object owners cannot be omitted by this
  // presentation-only runtime. Existing partial map-component delivery uses
  // preload_attached and reports its independent source owners explicitly.
  // Passing does not establish GPU/audio resources or consumption.
  void preload_complete(std::uint32_t effect);
  // Completed source instances render their particles through1770C0.
  // Keep this graph's separate light/sound owners without duplicate sprites.
  void source_particle_effect(std::uint32_t effect);
  using ObjectMarkers=std::map<std::string,std::vector<WorldEffectFrame>>;
  void object_markers(std::uint32_t full_handle,ObjectMarkers);
  void remove_object(std::uint32_t full_handle);
  void spawn_attached(std::uint32_t effect,std::uint32_t full_handle,
      std::uint32_t owner,std::uint64_t tick,SourceEffectColors={},SourceEffectControls={});
  void request_weapon(std::uint32_t effect,std::uint32_t weapon,std::uint64_t tick,
      SourceEffectColors,Vec3 velocity,float scale_a,float scale_b,std::uint32_t source_instance,
      bool sounds,const ObjectMarkers *source_locations=nullptr);
  void update_weapon_locations(std::uint32_t weapon,std::uint32_t instance,const ObjectMarkers &);
  //1766B0/175FA0 object attachment branch. The caller supplies current posed
  //markers and original function values; marker aliases are per attachment.
  void spawn_looping_attached(std::uint32_t effect,std::uint32_t full_handle,
      std::uint64_t tick,std::string marker,float scale_a,float scale_b,
      const SourceAttachedEffect *source=nullptr,const ObjectMarkers *locations=nullptr);
  // Completed source flags retain independent function pause (4), PVS hide
  // (8) and draining retirement (40). Values alone never decide admission.
  bool update_looping_attached(std::uint32_t full_handle,std::uint32_t instance,
      const SourceAttachedEffect &,const ObjectMarkers &locations={});
  // Original object attachment creator1766B0 sets the retained repeat flag.
  // Import static scenery's actual effe attachments and root transforms.
  std::size_t attach_scenery(const LevelContent &);
  void advance(std::uint64_t tick, float seconds);
  void collision_query(ParticleCollisionQuery);
  // Consume once after advancing the simulation, independently of view count.
  std::vector<WorldEffectSound> take_sounds();
  // Tick-produced source light state. GPU admission/submission is separate.
  std::vector<WorldEffectLight> lights() const;
  // Stable authored definitions admitted during preload, before GPU resources
  // are created. This list supplies no births, fades or visibility decisions.
  std::vector<std::uint32_t> light_definitions() const;
  // C15A0 precedes the main visibility query. View results snapshot the fades
  // and selection while the global query epoch and local caches persist.
  SourceEffectLightView light_view(Vec3 camera_position,
      const SourceEffectLightCollectionContext&,const SourceEffectLightCandidateContext&,
      std::int32_t frame,bool cinematic=false);
  // Completed live simulation pool, after the current view's originalC35D0.
  // Read actual occupied identities/spatial lists and original owner/shape
  // admission. The private standalone allocator is not merged or re-registered.
  SourceEffectLightView source_light_view(PhysicsLiveWorldTu15&,unsigned bsp,
      const SourceEffectLightCollectionContext&,const SourceEffectLightCandidateContext&,
      std::int32_t frame,bool cinematic=false);
  std::vector<WorldEffectLightReceiver> source_light_receivers(PhysicsLiveWorldTu15&,
      unsigned bsp,const SourceEffectLightView&,const SourceBspAdmissionBsp&,
      std::span<const std::uint8_t> containing_fog,const SourceBspAdmission* main_view);
  //31C80's selected-light order, actual main-view BSP auxiliary/fog and
  // nested query epochs. Geometry collection/culling/submission stay separate.
  std::vector<WorldEffectLightReceiver> light_receivers(const SourceEffectLightView&,
      const SourceBspAdmissionBsp&,std::span<const std::uint8_t> containing_fog,
      const SourceBspAdmission* main_view);
  std::vector<ParticleDraw> draw(const MuzzleEffectView &, float fraction,
                                 float seconds) const;
  std::vector<Vertex> decals(std::uint64_t tick, float fraction,
                             float seconds) const;
  WorldEffectDiagnostics diagnostics() const;

private:
  std::unique_ptr<Impl> impl_;
  std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
};
} // namespace halo2
