#include "halo2/particle_collision.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
float evaluate(const ParticleProperty &p, const ParticleValueCache &cache,
               const ShaderFunctionTables &tables) {
  if (p.input < 0 || p.input >= 17 || p.range < 0 || p.range >= 17 ||
      p.modifier_input >= 17 || p.curve.size() < 2)
    throw FormatError("Particle collider property escapes source cache");
  const auto require = [&](unsigned index) {
    if (!(cache.evaluated & (1u << index)))
      throw FormatError("Particle collider value has no source producer");
  };
  if (std::to_integer<unsigned>(p.curve[0]) != 1)
    require(p.input);
  if (std::to_integer<unsigned>(p.curve[1]) & 1)
    require(p.range);
  if (p.modifier_input >= 0 && (p.modifier == 1 || p.modifier == 2))
    require(p.modifier_input);
  float value = shader_function_value(p.curve, cache.values[p.input],
                                      cache.values[p.range], tables);
  if (p.modifier_input >= 0) {
    if (p.modifier == 1)
      value = cache.values[p.modifier_input] + value;
    if (p.modifier == 2)
      value = cache.values[p.modifier_input] * value;
  }
  return value;
}
} // namespace
float source_particle_collision_child_gain(Vec3 velocity) {
  // Original caller FCW023F selects 53-bit x87 arithmetic before the float
  // store. Keep those operations separate from the source SSE bounce.
  const double x = velocity.x, y = velocity.y, z = velocity.z;
  const float value = float(std::sqrt((x * x + y * y) + z * z) - .5);
  return value < 0 ? 0 : (value > 1 ? 1 : value);
}
void source_particle_collision_response(ParticleSimulationState &p,
                                        const ParticleCollisionHit &hit,
                                        float friction, float restitution) {
  // Ordered SSE products/stores at 2BF6E7..2BF824. Constants .005 and .8
  // are immutable source words in both verified update images.
  const float projection =
      (p.velocity.x * hit.normal.x + hit.normal.z * p.velocity.z) +
      hit.normal.y * p.velocity.y;
  const Vec3 normal{projection * hit.normal.x, projection * hit.normal.y,
                    projection * hit.normal.z};
  const Vec3 tangent{p.velocity.x - normal.x, p.velocity.y - normal.y,
                     p.velocity.z - normal.z};
  const float factor = 1.f - friction;
  p.position = {hit.normal.x * .005f + hit.position.x,
                hit.normal.y * .005f + hit.position.y,
                hit.normal.z * .005f + hit.position.z};
  p.velocity = {tangent.x * factor - normal.x * restitution,
                tangent.y * factor - normal.y * restitution,
                tangent.z * factor - normal.z * restitution};
  p.rotation_rate = factor * p.rotation_rate;
}
void apply_particle_collider_controller(
    const ParticleMovementController &controller, std::uint32_t movement_flags,
    std::uint32_t particle_flags, std::uint32_t head, float elapsed,
    const ParticleValueSources &source, const ShaderFunctionTables &tables,
    const ParticleMovementPool &pool, const ParticleCollisionQuery &query,
    const ParticleCollisionChild &child) {
  if (controller.type != 1 || controller.parameters.size() < 2 ||
      !pool.particle || !pool.capacity || !query)
    throw FormatError("Original collider1 requires its parameters, retained "
                      "pool and collision owner");
  ParticleValueCache cache;
  auto values = source;
  values.location.reset();
  values.particle.reset();
  complete_particle_values(cache, controller.required_values & 0x107f0u,
                           values);
  std::array<float, 2> parameters{};
  for (unsigned i = 0; i < 2; ++i)
    if (controller.constant_parameters & (1u << i))
      parameters[i] = evaluate(controller.parameters[i], cache, tables);
  std::uint32_t flags = (movement_flags & 2) ? 0x800005u : 0x800000u;
  if (movement_flags & 4)
    flags |= 2;
  if (movement_flags & 0x10)
    flags |= 0x20;
  if (movement_flags & 8)
    flags |= 0x400;
  if (movement_flags & 0x20)
    flags |= 0x10;
  if (flags & 0x430)
    flags |= 8;
  std::size_t traversed{};
  while (head != 0xffffffffu) {
    if (++traversed > pool.capacity || std::uint16_t(head) >= pool.capacity)
      throw FormatError("Particle collider chain escapes pool or cycles");
    auto &p = pool.particle(head);
    head = p.next_particle;
    if ((p.flags & 9) || !(p.age <= 1.f))
      continue;
    const unsigned countdown = p.flags >> 13;
    if (countdown) {
      p.flags = std::uint16_t(((countdown - 1) << 13) | (p.flags & 0x1fff));
      continue;
    }
    cache.evaluated &= 0xffff07f0u;
    values.particle = ParticleValueState{p.age, p.system_fraction, p.phase,
                                         p.position, p.random};
    complete_particle_values(cache, controller.required_values & 0xf80fu,
                             values);
    if (!(movement_flags & 0xf)) {
      p.flags &= ~2u;
      continue;
    }
    const Vec3 step = p.velocity * elapsed;
    const auto hit = query(flags, p.position - step, step * 12.f);
    if (!hit) {
      p.flags = std::uint16_t((p.flags & 0x1ffd) | 0xa000);
      continue;
    }
    // CRT37EF30 initializes5661CC from source465118 (3E2AAAAB); u5
    // CRT3829C0 does the same. This accepts the first two of twelve steps.
    if (!(hit->fraction < std::bit_cast<float>(0x3e2aaaabu))) {
      p.flags &= 0x1fff;
      continue;
    }
    for (unsigned i = 0; i < 2; ++i)
      if (!(controller.constant_parameters & (1u << i)))
        parameters[i] = evaluate(controller.parameters[i], cache, tables);
    source_particle_collision_response(p, *hit, parameters[0], parameters[1]);
    if (!(p.flags & 2) && child)
      child(p, *hit);
    p.flags |= 2;
    if (hit->normal.z > .8f)
      p.flags |= 0x10;
    else
      p.flags &= ~0x10u;
    if (source_particle_collision_dies(particle_flags,
        hit->type>=0?std::uint32_t(hit->type):(hit->structure?1u:2u)))
      p.flags |= 1;
  }
}
} // namespace halo2
