#pragma once
#include "halo2/source_effect_random.hpp"
#include "halo2/particle_creation.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/transform.hpp"
#include "halo2/weapon_effect.hpp"
#include "halo2/effect_sound.hpp"
#include "halo2/fixed_effect_event.hpp"
#include "halo2/source_effect_event.hpp"
#include "halo2/source_effect_location.hpp"
#include "halo2/particle_movement.hpp"
#include "halo2/particle_collision.hpp"
#include <memory>

namespace halo2 {
struct AuthoredMuzzleEffect {
  std::uint32_t marker{};
  std::uint32_t flags{};
  std::int16_t loop_start=-1;
  float initial_advance{};
  std::vector<FixedEffectEventDefinition> events;
  std::vector<SourceEffectEventDefinition> clocks;
  std::vector<EffectSoundCue> sounds;
  bool light_parts{},damage_parts{},acceleration_parts{};
  struct System {
    std::uint32_t marker{};
    unsigned event{},source_index{};
    ParticleSystemRenderDefinition definition;
    std::vector<ParticleMovementDefinition> movement;
    ParticleSpriteDefinition sprite;
    std::shared_ptr<const ParticleMaterial> material;
    std::uint32_t collision_child=0xffffffffu,collision_group{};
  };
  std::vector<System> systems;
};
struct MuzzleEffectView {
  std::array<ShaderVector,12> camera;
  std::array<ShaderVector,6> fog;
  std::array<float,9> orientation;
  Vec3 position,forward,up;
  ShaderVector clip_range;
  unsigned width{},height{};
};
// Native FP residency for the admitted camera1/3 fixed event systems.
// Markers are current model-order poses, never an estimated camera offset.
// Authored emitters, RNG, birth and sprite kernels use retained native owners.
// World views, fluid transitions and the original allocator remain separate work.
class NativeMuzzleEffects {
  struct Impl;
public:
  explicit NativeMuzzleEffects(const Cache &,std::uint32_t visual_seed);
  NativeMuzzleEffects(const Cache &,SourceEffectRandom);
  ~NativeMuzzleEffects();
  NativeMuzzleEffects(const NativeMuzzleEffects &)=delete;
  // Local completed-frame presentation state, not a network/native-world
  // checkpoint. The enclosing coordinator must bind its tick to the accepted
  // world and restore shared visual RNG together with every other consumer.
  class Checkpoint {
  public:
    std::optional<std::uint64_t> tick()const noexcept;
    std::array<std::uint32_t,2> random()const;
  private:
    friend class NativeMuzzleEffects;
    struct Data;
    std::shared_ptr<const Data> data_;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();
    PreparedRestore(PreparedRestore &&)noexcept;
    PreparedRestore &operator=(PreparedRestore &&)noexcept;
  private:
    friend class NativeMuzzleEffects;
    PreparedRestore();
    std::unique_ptr<Impl> replacement_;
    std::shared_ptr<const int> owner_;
  };
  Checkpoint checkpoint()const;
  // Same local owner/resident assets only. Preparation allocates privately and
  // never writes the caller's RNG. Both words must outlive the restored owner;
  // their captured alias relation must match. Collision callbacks must address
  // the accepted world, not its predecessor.
  PreparedRestore prepare_restore(const Checkpoint &,SourceEffectRandom,
      ParticleCollisionQuery)const;
  // Caller keeps the owner quiescent from preparation through publication.
  // Allocation-free single-use publication; foreign/consumed tokens reject.
  bool commit_restore(PreparedRestore &&)noexcept;
  bool restore_ready(const PreparedRestore &)const noexcept;
  std::uint32_t request(const WeaponEffectInput &,std::uint32_t tick);
  std::uint32_t request(const WeaponEffectInput &,std::uint32_t tick,SourceEffectColors);
  std::uint32_t request(const WeaponEffectInput &,std::uint32_t tick,SourceEffectColors,Vec3 inherited_velocity);
  std::uint32_t request(const WeaponEffectInput &,std::uint32_t tick,SourceEffectColors,Vec3 inherited_velocity,bool light_parts_owned);
  std::uint32_t request(const WeaponEffectInput &,std::uint32_t tick,SourceEffectColors,Vec3 inherited_velocity,
      bool light_parts_owned,bool source_gameplay_parts_owned);
  using Marker=std::function<Transform(std::uint32_t)>;
  using Markers=std::function<std::vector<Transform>(std::uint32_t)>;
  // Completed FP marker bank at a simulation tick. Birth never depends on a draw.
  void pose(std::uint32_t weapon,const Marker &,const MuzzleEffectView &,std::uint32_t tick,float tick_seconds);
  void pose(std::uint32_t weapon,const Markers &,const MuzzleEffectView &,std::uint32_t tick,float tick_seconds);
  // Retained object attachment: repeat flag2, zero initial interval, and
  // current completed object-function scalars. It owns a separate lifetime.
  std::uint32_t attachment(std::uint32_t effect,std::uint32_t weapon,std::uint32_t tick,SourceEffectControls,bool enabled=true);
  std::uint32_t attachment(std::uint32_t effect,std::uint32_t weapon,std::uint32_t tick,
      SourceEffectControls,bool enabled,std::uint32_t authored_marker_alias);
  std::uint32_t attachment(std::uint32_t effect,std::uint32_t weapon,std::uint32_t tick,
      SourceEffectControls,bool enabled,std::uint32_t authored_marker_alias,bool source_gameplay_parts_owned);
  void retire_attachments_except(std::span<const std::uint32_t> weapons);
  void advance(std::uint32_t tick,float tick_seconds);
  void collision_query(ParticleCollisionQuery);
  std::uint32_t marker_name(std::uint32_t effect) const;
  std::vector<std::uint32_t> marker_names(std::uint32_t effect) const;
  struct Sound {EffectSoundCue cue;std::uint32_t owner{};std::optional<Vec3> position;};
  std::vector<Sound> take_sounds();
  std::vector<ParticleDraw> draw(std::uint32_t weapon,const MuzzleEffectView &,
      std::uint32_t tick,float fraction,float tick_seconds);
  std::vector<ParticleDraw> draw(std::uint32_t weapon,const Marker &,
      const MuzzleEffectView &,std::uint32_t tick,float fraction,float tick_seconds);
  std::vector<ParticleDraw> draw(std::uint32_t weapon,const Transform &camera_local_marker,
      const MuzzleEffectView &,std::uint32_t tick,float fraction,float tick_seconds);
  std::size_t particle_count() const;
  std::size_t location_count() const;
private:
  std::unique_ptr<Impl> impl_;
  std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
};
} // namespace halo2
