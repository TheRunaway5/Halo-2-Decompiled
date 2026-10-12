#include "halo2/source_lift_membership.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t r(const SourcePhysicsContactMemory &m, std::uint32_t p,
                unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v{};
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
void w(SourcePhysicsContactMemory &m, std::uint32_t p, std::uint32_t v,
       unsigned n = 4) {
  std::array<std::byte, 4> b{};
  for (unsigned i = 0; i < n; ++i)
    b[i] = std::byte((v >> (8 * i)) & 255u);
  m.write(p, std::span(b).first(n));
}
std::int32_t signed8(std::uint32_t v) {
  return std::bit_cast<std::int8_t>(std::uint8_t(v));
}
std::uint32_t property(const SourcePhysicsContactMemory &m,
                       std::uint32_t entity, std::uint32_t key) {
  const auto count = std::bit_cast<std::int32_t>(r(m, entity + 0x34));
  if (count > 0) {
    const auto entries = r(m, entity + 0x30);
    for (std::int32_t i = 0; i < count; ++i)
      if (r(m, entries + std::uint32_t(i) * 8) == key)
        return r(m, entries + std::uint32_t(i) * 8 + 4);
  }
  return 0; // Original missing-property value, distinct from NONE.
}
std::uint32_t components(const SourcePhysicsContactMemory &m,
                         SourcePhysicsContactImage image) {
  return r(m,
           r(m, image == SourcePhysicsContactImage::TitleUpdate15 ? 0x52be5cu
                                                                  : 0x526248u) +
               0x44);
}
std::uint32_t objects(const SourcePhysicsContactMemory &m,
                      SourcePhysicsContactImage image) {
  return r(m,
           r(m, image == SourcePhysicsContactImage::TitleUpdate15 ? 0x4ecbd0u
                                                                  : 0x4e78d0u) +
               0x44);
}
std::uint32_t rule(const SourcePhysicsContactMemory &m, std::uint32_t own,
                   std::int32_t shape, SourcePhysicsContactImage image) {
  auto p = r(m, objects(m, image) + (r(m, own + 8) & 0xffffu) * 12 + 8);
  const auto tags =
      r(m, image == SourcePhysicsContactImage::TitleUpdate15 ? 0x4f0414u
                                                             : 0x4eb114u);
  p = r(m, tags + (r(m, p) & 0xffffu) * 16 + 8);
  p = r(m, tags + (r(m, p + 0x38) & 0xffffu) * 16 + 8);
  p = r(m, tags + (r(m, p + 0x24) & 0xffffu) * 16 + 8);
  const auto index = std::bit_cast<std::int16_t>(
      std::uint16_t(r(m, r(m, p + 0x44) + std::uint32_t(shape) * 12 + 8, 2)));
  return r(m, p + 0x2c) + std::uint32_t(std::int32_t(index) * 0x68);
}
bool matches(const SourcePhysicsContactMemory &m, std::uint32_t entry,
             std::uint32_t object, std::uint32_t body,
             const SourceLiftMembershipInput &i) {
  return r(m, entry + 4) == object &&
         signed8(r(m, entry + 3, 1)) == i.own_body &&
         signed8(r(m, entry + 1, 1)) == std::bit_cast<std::int32_t>(body) &&
         signed8(r(m, entry, 1)) == i.phantom_shape;
}
void free_empty(SourcePhysicsContactMemory &m, std::uint32_t own,
                const SourceLiftMembershipServices &s) {
  const auto descriptor = r(m, own + 0x94);
  if (descriptor) {
    if (!s.release)
      throw FormatError("Lift contact requires original release owner");
    const auto capacity = r(m, descriptor + 8);
    if (!(capacity & 0x80000000u))
      s.release(r(m, descriptor), (capacity & 0x7fffffffu) * 8, 0x12);
    s.release(descriptor, 12, 0x12);
  }
  w(m, own + 0x94, 0);
}
} // namespace
void source_lift_contact_add(SourcePhysicsContactMemory &m,
                             const SourceLiftMembershipInput &i,
                             SourcePhysicsContactImage image,
                             const SourceLiftMembershipServices &s) {
  const auto other = property(m, i.entity, 0x2001);
  const auto body = property(m, i.entity, 0x2002);
  if (other == 0xffffffffu || other == i.own_component_datum ||
      !source_lift_contact_allowed(m, std::uint32_t(i.phantom_shape),
                                   i.own_component_datum, other, image))
    return;
  const auto object = r(m, components(m, image) + (other & 0xffffu) * 0xa0 + 8);
  if (!r(m, i.own_component_address + 0x94)) {
    if (!s.allocate)
      throw FormatError("Lift contact requires original allocation owner");
    const auto allocated = s.allocate(12, 0x12);
    if (allocated) {
      w(m, allocated, 0);
      w(m, allocated + 4, 0);
      w(m, allocated + 8, 0x80000000u);
    }
    w(m, i.own_component_address + 0x94, allocated);
  }
  const auto descriptor = r(m, i.own_component_address + 0x94);
  const auto count = std::bit_cast<std::int32_t>(r(m, descriptor + 4));
  auto entry = r(m, descriptor);
  for (std::int32_t n = 0; n < count; ++n, entry += 8)
    if (matches(m, entry, object, body, i)) {
      w(m, entry + 2, (r(m, entry + 2, 1) + 1) & 255u, 1);
      return;
    }
  const auto rule_address =
      rule(m, i.own_component_address, i.phantom_shape, image);
  if (std::uint32_t(count) == (r(m, descriptor + 8) & 0x7fffffffu)) {
    if (!s.grow)
      throw FormatError("Lift contact requires original array-growth owner");
    s.grow(descriptor, 8);
  }
  const auto next = r(m, descriptor + 4);
  entry = r(m, descriptor) + next * 8;
  w(m, descriptor + 4, next + 1);
  w(m, entry + 3, std::uint32_t(i.own_body), 1);
  w(m, entry, std::uint32_t(i.phantom_shape), 1);
  w(m, entry + 1, body, 1);
  w(m, entry + 2, 0, 1);
  w(m, entry + 4, object);
  if (r(m, rule_address) & 0x01000000u) {
    const auto header = objects(m, image) + (object & 0xffffu) * 12;
    if ((1u << (r(m, header + 3, 1) & 31u)) & 1u) {
      const auto unit = r(m, header + 8);
      if (r(m, unit + 0x3e4) == 0xffffffffu)
        w(m, unit + 0x3e4, r(m, i.own_component_address + 8));
    }
  }
  w(m, entry + 2, (r(m, entry + 2, 1) + 1) & 255u, 1);
}
void source_lift_contact_remove(SourcePhysicsContactMemory &m,
                                const SourceLiftMembershipInput &i,
                                SourcePhysicsContactImage image,
                                const SourceLiftMembershipServices &s) {
  const auto other = property(m, i.entity, 0x2001);
  const auto body = property(m, i.entity, 0x2002);
  if (other == 0xffffffffu || other == i.own_component_datum)
    return;
  const auto descriptor = r(m, i.own_component_address + 0x94);
  if (!descriptor)
    return;
  const auto object = r(m, components(m, image) + (other & 0xffffu) * 0xa0 + 8);
  const auto count = std::bit_cast<std::int32_t>(r(m, descriptor + 4));
  auto entry = r(m, descriptor);
  for (std::int32_t n = 0; n < count; ++n, entry += 8)
    if (matches(m, entry, object, body, i)) {
      const auto refs = (r(m, entry + 2, 1) - 1) & 255u;
      w(m, entry + 2, refs, 1);
      if (!refs) {
        const auto rule_address =
            rule(m, i.own_component_address, i.phantom_shape, image);
        const auto next = r(m, descriptor + 4) - 1;
        const auto entries = r(m, descriptor);
        w(m, descriptor + 4, next);
        w(m, entries + std::uint32_t(n) * 8, r(m, entries + next * 8));
        w(m, entries + std::uint32_t(n) * 8 + 4, r(m, entries + next * 8 + 4));
        if (r(m, rule_address) & 0x01000000u) {
          const auto header = objects(m, image) + (object & 0xffffu) * 12;
          if ((1u << (r(m, header + 3, 1) & 31u)) & 1u) {
            const auto unit = r(m, header + 8);
            if (r(m, unit + 0x3e4) == r(m, i.own_component_address + 8))
              w(m, unit + 0x3e4, 0xffffffffu);
          }
        }
      }
      break;
    }
  if (!r(m, descriptor + 4))
    free_empty(m, i.own_component_address, s);
}
} // namespace halo2
