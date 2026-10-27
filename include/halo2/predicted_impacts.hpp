#pragma once
#include "halo2/bullet_impact.hpp"
#include "halo2/content.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/netcode_mode.hpp"
#include <array>
#include <deque>
#include <map>
#include <optional>
#include <span>
#include <tuple>
#include <vector>

namespace halo2 {
class Cache;
struct BspSurfaceLighting;
struct NativeGrenadeDetonation;
// Predicted impact sparks for the local non-host player's own precision
// hitscan shots (docs/NETCODE_ENHANCED_GOAL.md item 6). Balanced and Full.
// Presentation only: no hit, damage or result is decided here, and nothing
// is ever predicted on a player. A predicted round is traced from the local
// view against the structure BSP with the native query of the lag-compensation
// cover veto (original_map_vector_contact, 04800005); the world surface it
// reaches gets the weapon projectile's own authored impact effects (the
// material response and material-effect selection NativeWorldEffects::impact
// applies to the host's terrain contacts). The host's replicated copy of the
// same impact is then recognized (shooter, tick window, position tolerance,
// the same effect tag) and suppressed, so it does not spawn twice.
inline constexpr bool predicted_impacts_enabled(NetcodeMode mode){return mode!=NetcodeMode::original;}

// A weapon whose local presses are predicted: the precision classification of
// lag compensation (instantaneous projectile, latching rather than spew
// primary trigger) with magazine rounds. Energy (heat) weapons are excluded:
// their overheat state is not replicated.
struct PredictedShotWeapon {
  std::uint32_t weapon=null_datum,projectile=null_datum;
  unsigned rounds_per_press=1;  // Authored barrel shots per fire (the battle rifle's burst).
  double round_seconds{};       // Spacing of rounds within one press (barrel rate).
  double cycle_seconds{};       // Press to the barrel's next certain readiness (rounds + recovery).
  double ready_seconds{};       // Authored ready time after the weapon becomes current.
  int rounds_per_shot=1;
  float range=100;
};
std::optional<PredictedShotWeapon> predicted_shot_weapon(const WeaponDefinition &);

// Local fire gating: only a press that certainly fires on the host is
// predicted. A missed prediction leaves the host's impact to spawn as before;
// a wrong one would show a spark for a shot that never happened.
class PredictedShotGate {
public:
  struct Settings {
    double tick_seconds=1.0/30.0;
    double margin_seconds=.1;        // Spacing beyond the weapon's cycle between predicted presses.
    double action_hold_seconds=1.0;  // No prediction after a reload, melee, grenade or switch input.
    double ready_margin_seconds=.1;  // Beyond the authored ready time of a newly current weapon.
    double ammunition_seconds=.6;    // Predicted rounds not yet reflected in replicated rounds.
  };
  // The newest replicated state of the local player.
  struct Player {
    bool armed{};  // Connected, alive, on foot, single wielding, idle magazine, no unit action.
    std::uint32_t player=null_datum,unit=null_datum,weapon_object=null_datum,weapon=null_datum;
    int loaded{};
    std::uint32_t tick{},shot_tick{};
  };
  struct Controls {bool fire{},reload{},melee{},grenade{},switch_weapon{};};
  struct Round {
    double seconds{};          // Local time the round is due.
    std::uint32_t tick_floor{};// Newest host tick known at the press.
    std::uint32_t player=null_datum,unit=null_datum,weapon=null_datum,projectile=null_datum;
    unsigned index{};          // Round within its press.
    float range=100;
  };
  struct Statistics {std::uint64_t presses{},admitted{},rounds{},refused{};};
  PredictedShotGate():PredictedShotGate(Settings{}){}
  explicit PredictedShotGate(Settings);
  // One delivered input tick of this view (local monotonic seconds). Returns
  // the number of rounds scheduled; the first is due at `seconds`.
  unsigned input(double seconds,const Controls &,const Player &,const PredictedShotWeapon *);
  // Rounds due by `seconds`, oldest first. They leave the gate.
  std::vector<Round> take_due(double seconds);
  void reset();
  const Statistics &statistics()const noexcept{return stats_;}
private:
  Settings settings_;
  std::deque<Round> pending_;
  std::deque<double> recent_rounds_;
  bool fire_held_{};
  double last_press_=-1e9,last_admitted_=-1e9,hold_until_=-1e9,weapon_since_=-1e9;
  std::uint32_t weapon_object_=null_datum;
  Statistics stats_;
};

// The gate's view of a local player's newest replicated state at `tick`:
// armed when connected, alive, on foot, single wielding, with an idle
// magazine and no melee, grenade or other unit action in progress.
PredictedShotGate::Player predicted_shot_player(const MultiplayerState::Player &,std::uint32_t tick);

// Pending predictions and the host-copy match.
class PredictedImpactLedger {
public:
  struct Settings {
    double expiry_seconds=1.0;       // A prediction the host never confirmed is forgotten.
    std::uint32_t early_ticks=1;     // Host ticks before the press's newest known tick.
    std::uint32_t window_ticks=30;   // Host ticks after it (round trip plus input buffering).
    float tolerance=.35f;            // World units, plus...
    float tolerance_per_unit=.03f;   // ...this much per world unit of shot distance.
  };
  struct Prediction {
    std::uint32_t player=null_datum,unit=null_datum,weapon=null_datum,projectile=null_datum;
    std::uint32_t tick_floor{};
    double seconds{};
    Vec3 position;
    float distance{};
    std::int16_t material=-1;
    std::array<std::uint32_t,4> effects{null_datum,null_datum,null_datum,null_datum};
  };
  // One replicated host contact. Unknown fields stay NONE / -1.
  struct HostImpact {
    std::uint32_t player=null_datum,unit=null_datum;   // Shooter, when the host names one.
    std::uint32_t weapon=null_datum,projectile=null_datum;
    std::uint32_t effect=null_datum;                   // The host's selected effect, when known.
    std::int16_t material=-1;
    bool world=true;                                   // False: a contact on an object or unit.
    std::uint64_t tick{};
    Vec3 position;
  };
  struct Statistics {std::uint64_t predicted{},suppressed{},expired{};};
  PredictedImpactLedger():PredictedImpactLedger(Settings{}){}
  explicit PredictedImpactLedger(Settings);
  void predict(const Prediction &);
  // True when `host` is the copy of a pending prediction (which it consumes):
  // the caller must not spawn it again. Player hits never match: they are not
  // world contacts and do not select a predicted world effect.
  bool suppress(const HostImpact &host,double seconds);
  void expire(double seconds);
  std::size_t pending()const noexcept{return pending_.size();}
  void reset(){pending_.clear();}
  const Statistics &statistics()const noexcept{return stats_;}
private:
  Settings settings_;
  std::deque<Prediction> pending_;
  Statistics stats_;
};
// The two replicated contact journals: TU1.5 projectile contacts (in the
// detonation journal, naming the shooter and the completed effect) and the
// legacy native bullet impacts (terrain flag and global material).
PredictedImpactLedger::HostImpact predicted_host_impact(const NativeGrenadeDetonation &);
PredictedImpactLedger::HostImpact predicted_host_impact(const BulletImpact &,std::uint32_t projectile);

// A body that blocks a predicted round before it reaches the structure: the
// round is then not predicted (the host would hit the body, not the wall).
struct PredictedImpactOccluder {Vec3 base;float height{},radius{};}; // Vertical capsule; height 0 is a sphere.
struct PredictedWorldImpact {
  BulletImpact impact;       // NativeWorldEffects::impact input (a terrain contact).
  std::array<std::uint32_t,4> effects{null_datum,null_datum,null_datum,null_datum};
  float distance{};
};
class PredictedImpactTracer {
public:
  // Borrows all three; they must outlive the tracer.
  PredictedImpactTracer(const Cache &,const LevelContent &,const BspSurfaceLighting &);
  std::optional<PredictedWorldImpact> trace(const WeaponDefinition &,Vec3 origin,Vec3 direction,float range,
      std::span<const PredictedImpactOccluder>,std::uint64_t tick,std::uint32_t &random)const;
  // Effects NativeWorldEffects::impact spawns for this terrain contact, in
  // its request order (material primary, sweetener, secondary, then the
  // projectile's own material-contact effect).
  std::array<std::uint32_t,4> effects(const WeaponDefinition &,const BulletImpact &)const;
private:
  std::pair<std::uint32_t,std::uint32_t> projectile_effects(std::uint32_t projectile)const;
  const Cache &cache_;
  const LevelContent &content_;
  const BspSurfaceLighting &structure_;
  // (record, instance, surface) -> global material and authored plane normal.
  std::map<std::tuple<int,int,int>,std::pair<std::int16_t,Vec3>> surfaces_;
  mutable std::map<std::uint32_t,std::pair<std::uint32_t,std::uint32_t>> projectiles_;
};
} // namespace halo2
