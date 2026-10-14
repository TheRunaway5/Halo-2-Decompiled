#pragma once
#include "halo2/shader_animation.hpp"

namespace halo2 {
struct SourceParticleSamplerWrite {
  std::uint32_t stage{},state{},value{};
  bool operator==(const SourceParticleSamplerWrite &) const = default;
};
// Original18540's admitted constant scalar overlays, in authored write order.
// Other function/input/transform paths require their completed source owner.
std::vector<SourceParticleSamplerWrite> source_constant_particle_sampler_writes(
    const ShaderAnimation &,ShaderRange);
// Logical SDK states22..25. Initial zero bytes are the original SDK image's
// resident bank and agree with the pinned TU1.5 neutral live bank. This does
// not assert execution of the complete original device constructor.
using SourceParticleSamplerBank=std::array<std::array<std::uint32_t,4>,4>;
void source_apply_particle_sampler_writes(SourceParticleSamplerBank &,
    std::span<const SourceParticleSamplerWrite>);
// Original3FFF10 emits states22..25 in order; NV097's method consumer
// swizzles00/01/11/10, and the GLSL uniform reader publishes00/01/10/11.
ShaderVector source_particle_bump_matrix(const SourceParticleSamplerBank &,unsigned stage);
} // namespace halo2
