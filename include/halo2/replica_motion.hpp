#pragma once
#include "halo2/multiplayer.hpp"
#include <array>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace halo2 {
// Non-host dead reckoning of replicated physics objects and projectiles.
// Loose weapons, unoccupied vehicles, source dynamic objects, scene props,
// weapon projectiles and released grenades advance from the newest accepted
// snapshot with their replicated (or snapshot-derived) linear and angular
// velocity, plus gravity where the original applies it: a projectile's own
// gravity, and Havok's world gravity for a body in free flight. Completed
// host point impulses (MultiplayerState::physics_impulses) replace a body's
// motion at once. A newer snapshot's disagreement with the presented path is
// blended out continuously at the original client's per-tick object rates;
// large errors snap. Players and occupied vehicles are never dead-reckoned.
// Presentation only: the host never runs this, and no snapshot, gameplay or
// authority value is changed.
struct ReplicaMotionSettings {
  float extrapolation_seconds=.25f; // Longest dead-reckoned interval past a snapshot.
  // Per-tick share of the remaining error removed, as the 2004 client's own
  // object presentation blend (retail A9A70 position, A9B40 orientation),
  // applied continuously between frames instead of once per tick.
  float position_blend=.45f,orientation_blend=.35f;
  float snap_distance=2.f;          // World units; larger position errors snap.
  float snap_angle=1.f;             // Radians; larger orientation errors snap.
  float gravity=retail_gravity;     // Havok world gravity along -Z (world units/s^2).
};
class ReplicaMotion {
public:
  enum class Kind : std::uint8_t {weapon_projectile,grenade,world_weapon,vehicle,dynamic_object,scene_root};
  // Authored projectile gravity (proj+164 scale times the retail gravity),
  // when the presentation owner knows the definition. Otherwise projectile
  // gravity is measured from consecutive authoritative velocities.
  using ProjectileGravity=std::function<std::optional<float>(std::uint32_t definition)>;
  // Optional scene-root admission (the runtime admits physics props, not
  // device-driven machines/controls or static fixtures). Empty admits all.
  using SceneRootAdmission=std::function<bool(const SourceSceneRoot &)>;
  struct Statistics {std::uint64_t receipts{},impulses{},corrections{},snaps{};};
  explicit ReplicaMotion(ReplicaMotionSettings={},ProjectileGravity={},SceneRootAdmission={});
  // One accepted authoritative snapshot (including a same-tick refresh) at
  // its local receipt time, in seconds on any monotonic clock.
  void receive(const MultiplayerState &,double seconds);
  // Replace each tracked root in a presentation copy of the newest snapshot.
  void present(MultiplayerState &shown,double seconds)const;
  // Rigid world motions for posed objects, composed onto their newest
  // completed WORLD nodes (SourceWorldInterpolation/SceneInterpolation).
  std::vector<std::pair<std::uint32_t,Transform>> node_motions(double seconds)const;
  // Presented root of one tracked object (diagnostics and tests).
  std::optional<Transform> root(Kind,std::uint32_t object,double seconds)const;
  void reset();
  bool active()const{return !tracks_.empty();}
  std::size_t tracked()const{return tracks_.size();}
  std::uint32_t impulse_cursor()const{return impulse_serial_;}
  Statistics statistics()const{return statistics_;}
private:
  struct Sample {
    std::uint32_t tick{};
    Vec3 position;
    std::array<float,4> rotation{0,0,0,1};
    std::optional<Vec3> velocity;
  };
  struct Track {
    Kind kind{};
    std::uint32_t object{},definition{};
    std::array<Sample,3> history{};
    unsigned samples{};
    Transform root;           // Newest authoritative root.
    bool rotates{};           // Rigid right-handed basis: orientation can advance.
    double base_time{};
    Vec3 position,velocity,angular,acceleration;
    std::array<float,4> rotation{0,0,0,1};
    bool moving{};            // Newest sample differs from the one before it.
    Vec3 error_position;
    std::array<float,4> error_rotation{0,0,0,1};
    bool error{};
    std::uint64_t generation{};
  };
  struct Pose {Vec3 position;std::array<float,4> rotation{0,0,0,1};};
  static std::uint64_t key(Kind kind,std::uint32_t object){return (std::uint64_t(kind)<<32)|object;}
  void observe(Kind,std::uint32_t object,std::uint32_t definition,const Transform &root,
      std::optional<Vec3> velocity,std::uint32_t tick,double seconds);
  Pose evaluate(const Track &,double seconds)const;
  std::pair<float,float> correction_weights(double elapsed)const;
  std::optional<Transform> motion(const Track &,double seconds)const;
  ReplicaMotionSettings settings_;
  ProjectileGravity projectile_gravity_;
  SceneRootAdmission scene_root_admission_;
  std::unordered_map<std::uint64_t,Track> tracks_;
  std::uint64_t generation_{};
  std::uint32_t last_tick_{},impulse_serial_{};
  bool have_tick_{},have_serial_{};
  Statistics statistics_;
};
}
