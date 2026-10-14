#pragma once
#include "halo2/fp_registration.hpp"
#include <array>
#include <functional>
#include <map>
#include <tuple>

namespace halo2 {
struct Unit;
struct LevelContent;
struct PropAreaDamageDefinition;
class SourceRuntimeResources;
struct NativeMeleeConstants {
  std::array<std::array<std::uint32_t,3>,4> candidates;
  std::uint32_t frame_seconds_bits;
  std::uint32_t impact_scale_bits{},moving_threshold_bits{},special_seconds_bits{};
};
struct NativeMeleeAction {
  std::uint32_t activity=0xffffffffu,started{};
  std::int8_t progress{},impact=-1,recovery=-1,duration{};
  std::uint8_t modifiers{},speed_modifier{};
  bool active()const{return activity!=0xffffffffu;}
};
// EB000's ordered available candidates and single shared LCG consumption.
int select_source_melee_candidate(std::array<bool,4>,std::uint32_t &rng);
// Ordinary strike branch of EB700. Returns the D0830 request boundary;
// this arithmetic helper neither invents contacts nor applies damage.
bool advance_source_melee_animation(NativeMeleeAction &);
// EB080 semantic state selection; availability and airborne classification
// are explicit inputs, never animation-name heuristics.
std::uint32_t source_melee_activity(std::uint32_t current,int kind,bool airborne,
    std::uint32_t ordinary,bool weapon_fallback);
bool valid_native_melee_activity(std::uint32_t);
struct NativeMeleeContact {
  std::uint32_t object=0xffffffffu;
  std::uint8_t type{}; // Original object kind;0 is biped.
  float fraction{};
  Vec3 point,normal;
  std::int16_t material=-1;
};
struct NativeMeleeHit {
  std::optional<NativeMeleeContact> target;
  Vec3 point,normal;
  std::int16_t material=-1;
};
// D0830's literal ordered5x5 query and object preference. The host supplies
// real contacts for mask2480000F, excluding the attacking full object datum.
NativeMeleeHit query_native_melee(Vec3 origin,Vec3 aim,Vec3 pyramid,
    const std::function<std::optional<NativeMeleeContact>(Vec3,Vec3,std::uint32_t)> &);
class NativeMeleeAnimation {
public:
  NativeMeleeAnimation(std::shared_ptr<const Cache>,const LevelContent &,NativeMeleeConstants);
  void update(Unit &,bool pressed,std::uint32_t tick,std::uint32_t &rng,
      const std::function<void(std::uint32_t)> &activity,
      const std::function<void(const NativeMeleeAction&)> &impact={},int kind=1);
  Vec3 pyramid(const Unit &)const;
  std::shared_ptr<const PropAreaDamageDefinition> damage_definition(const Unit &,std::uint32_t activity);
  // Only immutable decoded memo tables live here; action/RNG live in Unit/
  // Simulation. Root restore reconstructs this owner against its pinned local
  // resource snapshots, retaining the same authenticated immutable bytes.
  using AuthorityDigest=std::array<std::byte,32>;
  AuthorityDigest authority_binding()const;
  std::shared_ptr<NativeMeleeAnimation> prepare_authority_owner(const LevelContent &,AuthorityDigest)const;
private:
  NativeMeleeAnimation(std::shared_ptr<const Cache>,const LevelContent &,NativeMeleeConstants,
      std::shared_ptr<const SourceRuntimeResources>);
  std::shared_ptr<const Cache> cache_;
  std::shared_ptr<const SourceRuntimeResources> resources_;
  NativeMeleeConstants constants_;
  std::map<std::uint32_t,int> representations_;
  std::map<std::uint32_t,std::uint32_t> graphs_;
  std::map<std::tuple<std::uint32_t,int,bool>,FirstPersonBindingDefinition> bindings_;
  std::map<std::uint32_t,std::shared_ptr<const PropAreaDamageDefinition>> damage_;
  float movement_forward_{};
  bool begin(Unit &,int kind,std::uint32_t tick,std::uint32_t &rng,
      const std::function<void(std::uint32_t)> &);
};
} // namespace halo2
