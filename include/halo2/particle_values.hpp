#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Completed owner state consumed by 248DF0. Unknown lifecycle fields retain
// their original offsets; this module does not manufacture emission/physics.
struct ParticleSystemValueState {
  float field04{},random_a{},random_b{};
  std::optional<std::array<float,2>> effect_scales; // NONE -> zero in slots 9/10.
};
struct ParticleLocationValueState { std::uint8_t field03{};float field1c{}; };
struct ParticleValueState {
  float field08{},field10{},field38{};
  Vec3 position{};
  std::array<std::uint16_t,4> random{}; // +14,+16,+18,+1A.
};
struct ParticleValueSources {
  std::optional<ParticleSystemValueState> system;
  std::optional<ParticleLocationValueState> location;
  std::optional<ParticleValueState> particle;
  std::int32_t game_tick{};
  float game_rate{};
  Vec3 camera_position{},camera_forward{},camera_up{}; // g_485618.
};
struct ParticleValueCache {
  std::array<float,17> values{};
  std::uint32_t evaluated{};
};
// 248DF0 and the 173BA0 masked writer. The caller ORs the requested mask,
// preserving already-computed slots exactly as 224550 does.
float particle_source_value(unsigned,const ParticleValueSources &);
void complete_particle_values(ParticleValueCache &,std::uint32_t requested,const ParticleValueSources &);
} // namespace halo2
