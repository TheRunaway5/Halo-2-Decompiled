#include "halo2/physics_constraint_retirement.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourcePhysicsConstraintRetirementServices;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t v) {
  SourcePhysicsContactRecordStore a{site, p, {}, {}};
  a.before.resize(n);
  a.written.resize(n);
  m.read(p, a.before);
  for (unsigned i = 0; i < n; ++i)
    a.written[i] = std::byte(v >> (8 * i));
  m.write(p, a.written);
  if (o)
    o(a);
}
void required(M &m, const S &s, std::uint32_t site, std::uint32_t entry,
              std::uint32_t receiver, std::uint32_t esp,
              std::vector<std::uint32_t> args, const O &o) {
  if (!s.lower.required_owner)
    throw std::runtime_error("Required original constraint retirement owner unavailable at " +
                             std::to_string(entry));
  s.lower.required_owner(m, {site, entry, esp, receiver, std::move(args)}, o);
}
void listeners(M &m, std::uint32_t world, std::uint32_t constraint,
               std::uint32_t esp, const S &s, const O &o) {
  for (auto i = r(m, world + 0x64) - 1u; si(i) >= 0; --i)
    if (const auto listener = r(m, r(m, world + 0x60) + i * 4u))
      required(m, s, 0x31e85e, r(m, r(m, listener) + 12), listener,
               esp - 20u, {constraint}, o);
  for (auto i = r(m, world + 0x64) - 1u; si(i) >= 0; --i)
    if (!r(m, r(m, world + 0x60) + i * 4u)) {
      w(m, o, 0x31e881, world + 0x64, 4, r(m, world + 0x64) - 1u);
      for (auto j = i; si(j) < si(r(m, world + 0x64)); ++j) {
        const auto p = r(m, world + 0x60) + j * 4u;
        w(m, o, 0x31e89a, p, 4, r(m, p + 4));
      }
    }
}
} // namespace
void source_subtract_physics_constraint_requirements_1_1(
    M &m, std::uint32_t island, std::uint32_t packet, const O &o) {
  w(m, o, 0x31c578, island + 12, 4, r(m, island + 12) - r(m, packet + 4));
  const auto indices = r(m, packet + 8), old = r(m, island + 16),
             count = r(m, island + 24), delta = r(m, packet + 12);
  w(m, o, 0x31c586, island + 16, 4, old - indices);
  w(m, o, 0x31c58e, island + 24, 4, count - delta);
}
void source_retire_physics_island_constraint_1_1(
    M &m, std::uint32_t island, std::uint32_t constraint, std::uint32_t esp,
    const S &s, const O &o) {
  auto index = 0u;
  const auto count = r(m, island + 0x5c);
  while (si(index) < si(count) &&
         r(m, r(m, island + 0x58) + index * 4u) != constraint)
    ++index;
  if (si(index) >= si(count))
    index = 0xffffffffu;
  w(m, o, 0x31c64f, island + 0x5c, 4, count - 1u);
  for (; si(index) < si(r(m, island + 0x5c)); ++index) {
    const auto p = r(m, island + 0x58) + index * 4u;
    w(m, o, 0x31c65e, p, 4, r(m, p + 4));
  }
  const auto packet = esp - 16u, method = r(m, r(m, constraint) + 40);
  if (method == 0x322320)
    source_physics_constraint_requirements_1_1(m, constraint, packet, o);
  else if (method == 0x319ac0)
    source_physics_ragdoll_constraint_requirements_1_1(m, packet, o);
  else if (method == 0x319090)
    source_physics_hinge_constraint_requirements_1_1(m, constraint, packet, o);
  else
    required(m, s, 0x31c671, method, constraint, esp - 36u, {packet}, o);
  const auto subtract = r(m, r(m, island) + 12);
  if (subtract == 0x31c570)
    source_subtract_physics_constraint_requirements_1_1(m, island, packet, o);
  else
    required(m, s, 0x31c67e, subtract, island, esp - 40u,
             {constraint, packet}, o);
  w(m, o, 0x31c681, island + 0x28, 1, 1);
  w(m, o, 0x31c685, constraint + 6, 2, r(m, constraint + 6, 2) - 1u);
  w(m, o, 0x31c68e, constraint + 20, 4, 0);
  if (!r(m, constraint + 6, 2)) {
    if (source_physics_body_reference_is_resident_1_1(m, constraint))
      w(m, o, 0x31c6a4, constraint + 6, 2, 1);
    else
      required(m, s, 0x31c6b9, r(m, r(m, constraint)), constraint,
               esp - 36u, {1}, o);
  }
}
void source_remove_physics_world_constraint_1_1(
    M &m, std::uint32_t world, std::uint32_t output,
    std::uint32_t constraint, std::uint32_t esp, const S &s, const O &o) {
  if (r(m, world + 0x12c, 1) == 1) {
    const auto bank = world + 0xfc;
    if (r(m, bank + 4) == (r(m, bank + 8) & 0x7fffffffu))
      source_double_physics_body_bank_1_1(m, bank, 4, o);
    w(m, o, 0x312f56, r(m, bank) + r(m, bank + 4) * 4u, 4, constraint);
    w(m, o, 0x312f59, bank + 4, 4, r(m, bank + 4) + 1u);
    w(m, o, 0x312f5c, constraint + 6, 2, r(m, constraint + 6, 2) + 1u);
    w(m, o, 0x312f64, output, 1, 0);
    return;
  }
  auto island = r(m, r(m, constraint + 12) + 0x44);
  if (r(m, island + 0x24) == 0xffffffffu)
    island = r(m, r(m, constraint + 16) + 0x44);
  listeners(m, world, constraint, esp - 20u, s, o);
  const auto method = r(m, r(m, island) + 4);
  if (method == 0x31c620)
    source_retire_physics_island_constraint_1_1(m, island, constraint,
                                               esp - 16u, s, o);
  else
    required(m, s, 0x312f91, method, island, esp - 16u, {constraint}, o);
  w(m, o, 0x312f99, output, 1, 1);
}
} // namespace halo2
