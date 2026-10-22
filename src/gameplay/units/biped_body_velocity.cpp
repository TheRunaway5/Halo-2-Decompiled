#include "halo2/biped_body_velocity.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
std::int32_t sign(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t v,
       unsigned n = 4) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
std::uint32_t body(const M &m, std::uint32_t component, std::uint32_t slot) {
  return r(m, r(m, component + 0x70) + slot * 0x60 + 0x40);
}
std::uint32_t active(M &m, std::uint32_t b, std::uint32_t scratch, const O &o) {
  const auto island = r(m, b + 0x44);
  const auto value = island ? r(m, island + 0x29, 1) : 0;
  w(m, o, island ? 0x3121b8 : 0x3121ab, scratch, value, 1);
  return value;
}
std::uint32_t motion_type(const M &m, std::uint32_t b,
                          const SourceBipedVelocityServices &s) {
  const auto motion = r(m, b + 0x3c), method = r(m, r(m, motion) + 0x18);
  switch (method) {
  case 0x9d900:
    return 4;
  case 0x9b560:
    return 5;
  case 0x31b250:
    return 7;
  case 0x31b260:
    return 6;
  default:
    if (!s.motion_type)
      throw std::runtime_error(
          "Required original velocity motion type unavailable");
    return s.motion_type(motion, method);
  }
}
} // namespace

void source_biped_body_angular_velocity_1_1(
    M &m, std::uint32_t component, std::uint32_t slot, std::uint32_t velocity,
    std::uint32_t entry, const SourceBipedBodyTransformServices &services,
    const O &o) {
  if (r(m, component + 4) & 2u)
    return;
  const auto b = body(m, component, slot);
  if (r(m, b + 0x40, 1))
    return;
  const auto frame = ((entry - 4) & ~15u) - 0x20;
  constexpr std::array sites{0x1d4871u, 0x1d487cu, 0x1d4887u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], frame + 0x10 + 4 * i, r(m, velocity + 4 * i));
  w(m, o, 0x1d4890, frame + 0x1c, 0);
  if (!active(m, b, frame + 0xf, o) && r(m, b + 0x44)) {
    if (!services.activate)
      throw std::runtime_error(
          "Required original angular velocity activation unavailable");
    services.activate(b);
  }
  const auto motion = r(m, b + 0x3c);
  if (r(m, r(m, motion) + 0x54) != 0x31fea0)
    throw std::runtime_error(
        "Required original angular velocity virtual unavailable");
  std::array<std::byte, 16> bytes{};
  m.read(frame + 0x10, bytes);
  for (unsigned i = 0; i < 2; ++i) {
    SourcePhysicsContactRecordStore s{0x31fea7, motion + 0x50 + i * 8, {}, {}};
    s.before.resize(8);
    s.written.assign(bytes.begin() + i * 8, bytes.begin() + i * 8 + 8);
    m.read(s.address, s.before);
    m.write(s.address, s.written);
    if (o)
      o(s);
  }
}

bool source_biped_component_active_1_1(M &m, std::uint32_t component,
                                       std::uint32_t entry, const O &o) {
  for (std::uint32_t i = 0; sign(i) < sign(r(m, component + 0x74)); ++i)
    if (active(m, body(m, component, i), entry - 1, o))
      return true;
  return false;
}

void source_biped_object_velocity_1_1(
    M &m, std::uint32_t object, std::uint32_t linear, std::uint32_t angular,
    std::uint8_t force, std::uint32_t entry,
    const SourceBipedVelocityServices &services, const O &o) {
  const auto header = r(m, r(m, 0x4e78d0) + 0x44) + (object & 0xffffu) * 12;
  const auto datum = r(m, r(m, header + 8) + 0xb4);
  if (datum == 0xffffffffu)
    return;
  const auto component = r(m, r(m, 0x526248) + 0x44) + (datum & 0xffffu) * 0xa0;
  if (!force && !source_biped_component_active_1_1(m, component, entry - 12, o))
    return;
  const bool root_only = (r(m, header + 3, 1) & 31u) == 1;
  const auto deliver = [&](std::uint32_t slot) {
    const auto b = body(m, component, slot);
    if (r(m, b + 0x40, 1) || motion_type(m, b, services) == 6)
      return;
    const auto child_entry = entry - (root_only ? 16u : 20u);
    if (linear)
      source_biped_body_velocity_1_1(m, component, slot, linear, child_entry,
                                     services.body, o);
    if (angular)
      source_biped_body_angular_velocity_1_1(m, component, slot, angular,
                                             child_entry, services.body, o);
  };
  if (root_only) {
    const auto slot =
        std::bit_cast<std::int8_t>(std::uint8_t(r(m, component + 0x18, 1)));
    if (slot >= 0 && slot < sign(r(m, component + 0x74)))
      deliver(std::uint32_t(slot));
  } else {
    for (std::uint32_t i = 0; sign(i) < sign(r(m, component + 0x74)); ++i)
      deliver(i);
  }
  if (force && (r(m, component + 4) & 0x20u))
    source_activate_biped_component_1_1(m, component, services.activation, o);
}
} // namespace halo2
