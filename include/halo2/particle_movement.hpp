#pragma once
#include "halo2/particle_creation.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <functional>

namespace halo2 {
struct ParticleMovementController {
  std::uint16_t type{};
  std::uint32_t constant_parameters{},required_values{};
  std::vector<ParticleProperty> parameters;
};
struct ParticleMovementDefinition {
  std::uint32_t datum{},flags{};
  std::vector<ParticleMovementController> controllers;
};
// Literal pmov rows and forwarded function curves. The supplied retained tag
// provider must own every referenced span; no missing-controller fallback.
ParticleMovementDefinition load_particle_movement_definition(
    std::uint32_t datum,const SourceTagMetadataProviders &);
ParticleMovementDefinition load_particle_movement_definition(
    const Cache &,std::uint32_t datum);

struct ParticleMovementStore {
  std::uint32_t particle{},source_instruction{};
  std::uint16_t offset{};
  std::uint32_t value_bits{};
};
using ParticleMovementObserver=std::function<void(const ParticleMovementStore &)>;
struct ParticleMovementPool {
  // Source2BF130 indexes the low16 bits and does not check the salt. The
  // storage owner resolves that index; the full incoming datum is observable.
  std::function<ParticleSimulationState &(std::uint32_t datum)> particle;
  std::size_t capacity{};
};
// Complete original TU1.1 2BF130 / u5 2C2B40 physics0 controller. This changes
// velocity/angular rate only. Original249210's preceding position/age/phase
// schedule and collider1/swarm2/wind3 dispatch remain separate owners.
// gravity is the actual current526254[0]; one is the selected image's literal
// source word. Values not requested by the authored mask have no source owner.
void apply_particle_physics_controller(const ParticleMovementController &,
    std::uint32_t head,float elapsed,float gravity,float one,
    const ParticleValueSources &,const ShaderFunctionTables &,
    const ParticleMovementPool &,const ParticleMovementObserver & = {});
} // namespace halo2
