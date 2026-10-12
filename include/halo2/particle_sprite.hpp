#pragma once
#include "halo2/cache.hpp"
#include "halo2/math.hpp"
#include "halo2/particle_values.hpp"

namespace halo2 {
struct ParticleProperty {
  std::int16_t input{},range{},modifier_input{};
  std::uint16_t modifier{};
  std::vector<std::byte> curve;
};
struct ParticleSpriteFrame {
  // Retail box2f: x0, x1, y0, y1. These are atlas bounds, not XYWH.
  std::array<float,4> bounds{};
  Vec2 pivot{};
};
struct ParticleSpriteDefinition {
    std::uint32_t flags{},bitmap{};
    std::uint32_t required_values{};
    std::int16_t billboard_style{}; // GPU classification is a separate admission.
  std::int16_t first_sequence{},sequence_count{},bitmap_type{};
  std::array<ParticleProperty,5> properties; // color, alpha, size, angle, frame
  std::array<float,4> geometry{},bounds{};
  std::vector<std::vector<ParticleSpriteFrame>> sequences;
};
struct ParticleSpriteInput {
  // 173BA0/248DF0's values are supplied by the simulation owner. This module
  // does not invent particle ages, emission times, random values, or locations.
  std::array<float,17> values{};
  std::uint16_t random1{},random2{};
  std::uint32_t seed_color{};
  Vec3 position{},direction{};
  float scale{1},width{1};
  std::array<float,4> color{1,1,1,1}; // RGB, alpha
  std::int32_t selected_sequence{-1}; // caller's initialized context sequence
};
struct ParticleSpriteSubmission {
  Vec3 position{},direction{};
  float size{},angle{};
  std::uint32_t argb{};
  float blend{};
  std::array<float,4> geometry{},uv_a{},uv_b{};
};
// Retail 224550 -> 23ED30 parameters, before the authored vertex shader.
// Exact particle simulation and GPU submission are separate requirements.
ParticleSpriteSubmission particle_sprite_submission(const ParticleSpriteDefinition &,
    const ParticleSpriteInput &,const ShaderFunctionTables &);
ParticleSpriteSubmission particle_sprite_submission(const ParticleSpriteDefinition &,
    ParticleSpriteInput,ParticleValueCache &,const ParticleValueSources &,const ShaderFunctionTables &);
// Retail 23ED30's complete NV2A submission. Bindings are supplied explicitly
// because 23ECD0 initializes them separately from the sprite calculation.
// The actual executable passes geometry in EBX and UV A/B on the stack.
std::array<std::uint32_t,46> particle_sprite_pushbuffer(const ParticleSpriteSubmission &,
    const std::array<std::uint32_t,8> &bindings);
// Generic NV2A attributes after 23ED30's immediate uploads. The packed color
// uses SET_VERTEX_DATA4UB byte order; the authored vertex program swizzles it.
std::array<ShaderVector,16> particle_sprite_vertex_attributes(const ParticleSpriteSubmission &);
} // namespace halo2
