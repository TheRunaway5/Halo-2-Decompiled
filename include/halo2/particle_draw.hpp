#pragma once
#include "halo2/particle_material.hpp"
#include "halo2/particle_model.hpp"

namespace halo2 {
// Completed original draw state. Emission/simulation and 23E460 camera/fog
// setup must supply these values; the graphics path invents no particle state.
struct ParticleVertexState {
  std::array<ShaderVector,16> attributes{};
  std::array<ShaderVector,192> constants{};
  ShaderVector clip_range{},surface_fog{};
};
struct ParticleDepthState {
  ShaderVector clip_range{},parameters{};
};
struct ParticleDraw {
  // Retained native effect owner for read-only submission diagnostics.
  // GPU attributes and source particle state never depend on this field.
  std::uint32_t effect_owner=0xffffffffu;
  std::uint32_t effect_definition=0xffffffffu;
  std::shared_ptr<const ParticleMaterial> material;
  ParticleVertexState vertex;
  ParticleDepthState depth;
  // Caller-provided transparent order: the number of preceding glow batches.
  // Zero precedes the first batch; the final boundary follows all batches.
  unsigned before_light_volume{};
  // Live world-location collector's negative camera depth (175EA0/1E4C0).
  // Captured diagnostic submissions can retain their explicit boundary.
  std::optional<float> scene_sort_depth;
  std::shared_ptr<const ParticleModelDefinition> model;
  unsigned model_first{},model_count{};
};
} // namespace halo2
