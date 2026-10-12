#include "halo2/biped_physics_activation.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourceBipedPhysicsActivationServices;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> raw{};
  m.read(p, std::span(raw).first(n));
  std::uint32_t result = 0;
  for (unsigned i = 0; i < n; ++i)
    result |= std::to_integer<std::uint32_t>(raw[i]) << (i * 8);
  return result;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p,
       std::uint32_t value, unsigned n = 4) {
  SourcePhysicsContactRecordStore store{site, p, {}, {}};
  store.before.resize(n);
  store.written.resize(n);
  m.read(p, store.before);
  for (unsigned i = 0; i < n; ++i)
    store.written[i] = std::byte(value >> (i * 8));
  m.write(p, store.written);
  if (o)
    o(store);
}
template <class F, class... A>
decltype(auto) need(const F &f, const char *message, A... a) {
  if (!f)
    throw std::runtime_error(message);
  return f(a...);
}
std::int32_t si(std::uint32_t x) { return std::bit_cast<std::int32_t>(x); }
std::uint32_t header(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12u;
}
std::uint32_t component(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x526248) + 0x44) + (d & 0xffffu) * 0xa0u;
}
void mark(M &m, std::uint32_t d, const O &o) {
  // LiteralB8250 initial short circuit. The original low16 header read even
  // precedes the NONE guard; callers own the validity of their source datum.
  if (r(m, header(m, d) + 2, 1) & 2u)
    return;
  while (d != 0xffffffffu) {
    const auto h = header(m, d);
    w(m, o, 0xb8288, h + 2, r(m, h + 2, 1) | 2u, 1);
    d = r(m, r(m, h + 8) + 0x14);
  }
}
} // namespace
void source_remove_biped_deferred_body_1_1(M &m, std::uint32_t world,
                                          std::uint32_t body, const S &s,
                                          const O &o) {
  for (std::uint32_t i = 0; si(i) < si(r(m, world + 0x10c)); ++i) {
    if (r(m, r(m, world + 0x108) + i * 4) != body)
      continue;
    auto index = 0xffffffffu;
    const auto count = r(m, world + 0x10c);
    for (std::uint32_t j = 0; si(j) < si(count); ++j)
      if (r(m, r(m, world + 0x108) + j * 4) == body) {
        index = j;
        break;
      }
    const auto last = r(m, world + 0x10c) - 1;
    w(m, o, 0x312ae6, world + 0x10c, last);
    const auto bank = r(m, world + 0x108);
    w(m, o, 0x312af7, bank + index * 4, r(m, bank + last * 4));
    w(m, o, 0x312afa, body + 6, r(m, body + 6, 2) - 1, 2);
    if (r(m, body + 6, 2) == 0) {
      if (need(s.deferred_delete,
               "Required original27F9F0 body lifetime owner unavailable", body))
        w(m, o, 0x312b12, body + 6, 1, 2);
      else
        need(s.destroy_body,
             "Required original body deleting virtual owner unavailable", body);
    }
    // Deliberately do not revisit the element swapped into this index. This
    // is the original312B2C forward loop, not a host erase-all operation.
  }
}
void source_activate_biped_body_1_1(M &m, std::uint32_t body, const S &s,
                                   const O &o) {
  const auto world = r(m, body + 8);
  if (!world)
    return;
  source_remove_biped_deferred_body_1_1(m, world, body, s, o);
  const auto island = r(m, body + 0x44);
  if (!r(m, island + 0x29, 1) && r(m, island + 0x24) != 0xffffffffu)
    need(s.activate_island,
         "Required original3131C0 world/island activation owner unavailable",
         r(m, body + 8), island);
}
void source_activate_biped_component_1_1(M &m, std::uint32_t c, const S &s,
                                        const O &o) {
  if (!(r(m, c + 4) & 0x20u))
    return;
  for (std::uint32_t i = 0; si(i) < si(r(m, c + 0x74)); ++i)
    source_activate_biped_body_1_1(
        m, r(m, r(m, c + 0x70) + i * 0x60 + 0x40), s, o);
}
void source_biped_physics_activation_1_1(M &m, std::uint32_t d, bool request,
                                        const S &s, const O &o) {
  const auto h = header(m, d), p = r(m, h + 8);
  const auto mask = 1u << (r(m, h + 3, 1) & 31u);
  if ((mask & 0x1883u) && (r(m, p + 0xc0, 1) & 0x40u) &&
      r(m, p + 0xb4) != 0xffffffffu) {
    if (!request) {
      source_activate_biped_component_1_1(m, component(m, r(m, p + 0xb4)), s, o);
      w(m, o, 0xbaaf8, p + 0xc1, r(m, p + 0xc1, 1) & 0xfeu, 1);
      mark(m, d, o);
    }
  } else if (request)
    w(m, o, 0xbab16, p + 0xc1, r(m, p + 0xc1, 1) | 1u, 1);
  else {
    w(m, o, 0xbaaf8, p + 0xc1, r(m, p + 0xc1, 1) & 0xfeu, 1);
    mark(m, d, o);
  }
  if (mask & 0x3cu) {
    const auto parent = r(m, r(m, header(m, d) + 8) + 0xd4);
    if (parent != 0xffffffffu)
      need(s.mark_parent, "Required originalB6760 parent mark owner unavailable",
           parent, 0x400u);
  }
}
void source_remove_biped_object_physics_1_1(
    M &m, std::uint32_t d, const std::function<void(std::uint32_t)> &remove,
    const O &o) {
  const auto p = r(m, header(m, d) + 8), physics = r(m, p + 0xb4);
  if (physics != 0xffffffffu)
    need(remove, "Required original1D4C50 component removal owner unavailable",
         component(m, physics));
  w(m, o, 0xb976b, p + 0xc0, r(m, p + 0xc0, 1) & 0xbfu, 1);
}
} // namespace halo2
