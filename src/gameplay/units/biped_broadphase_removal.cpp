#include "halo2/biped_broadphase_removal.hpp"
#include <bit>
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
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint64_t v) {
  SourcePhysicsContactRecordStore x{site, p, {}, {}};
  x.before.resize(n);
  x.written.resize(n);
  m.read(p, x.before);
  for (unsigned i = 0; i < n; ++i)
    x.written[i] = std::byte(v >> (8 * i));
  m.write(p, x.written);
  if (o)
    o(x);
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::int16_t sw(std::uint32_t v) {
  return std::bit_cast<std::int16_t>(std::uint16_t(v));
}
} // namespace
void source_remove_physics_broadphase_axis_1_1(M &m, std::uint32_t axis,
                                               std::uint32_t minimum,
                                               std::uint32_t maximum,
                                               const O &o) {
  auto p = r(m, axis) + minimum * 4u;
  const auto first_end = r(m, axis) + maximum * 4u - 4u;
  while (p < first_end) {
    w(m, o, 0x2e654b, p, 4, r(m, p + 4));
    p += 4;
  }
  const auto count = r(m, axis + 4) - 2u, end = r(m, axis) + count * 4u;
  w(m, o, 0x2e655b, axis + 4, 4, count);
  while (p < end) {
    w(m, o, 0x2e656c, p, 4, r(m, p + 8));
    p += 4;
  }
}
void source_repair_removed_physics_broadphase_indices_1_1(M &m,
                                                          std::uint32_t bank,
                                                          std::uint32_t count,
                                                          std::uint32_t removed,
                                                          const O &o) {
  const std::array<unsigned, 4> lo{0, 2, 0, 2}, hi{4, 6, 4, 6};
  std::array<std::uint32_t, 4> minima{}, maxima{};
  for (unsigned j = 0; j < 4; ++j) {
    minima[j] = r(m, removed + lo[j], 2);
    maxima[j] = r(m, removed + hi[j], 2);
  }
  const auto min_x = r(m, removed + 8, 2), max_x = r(m, removed + 10, 2);
  for (auto p = bank; p < bank + count * 16u; p += 16) {
    std::uint64_t a = 0, b = 0;
    for (unsigned j = 0; j < 4; ++j) {
      const auto value = r(m, p + j * 2u, 2);
      const auto delta =
          std::uint16_t((sw(value) > sw(minima[j]) ? 0xffffu : 0u) +
                        (sw(value) > sw(maxima[j]) ? 0xffffu : 0u));
      a |= std::uint64_t(std::uint16_t(value + delta)) << (16 * j);
      const auto x = r(m, p + 8 + j * 2u, 2);
      const auto d = j < 2 ? std::uint16_t((sw(x) > sw(min_x) ? 0xffffu : 0u) +
                                           (sw(x) > sw(max_x) ? 0xffffu : 0u))
                           : std::uint16_t(0);
      b |= std::uint64_t(std::uint16_t(x + d)) << (16 * j);
    }
    w(m, o, 0x2e641c, p, 8, a);
    w(m, o, 0x2e6426, p + 8, 8, b);
  }
}
void source_remove_physics_broadphase_body_1_1(
    M &m, std::uint32_t backend, std::uint32_t handle, std::uint32_t pairs,
    std::uint32_t esp, const SourcePhysicsBroadphaseServices &s, const O &o) {
  const auto frame = ((esp - 4u) & ~15u) - 0x60u;
  const auto count = r(m, backend + 0x44), index = r(m, handle),
             bank = backend + 0x40;
  const auto own = r(m, bank) + index * 16u, descriptor = frame + 0x3c;
  const auto n = std::uint32_t((si(count) >> 5) + 4);
  source_construct_physics_overlap_bitset_1_1(m, descriptor, n, s, o);
  const auto bitset = r(m, descriptor), table = r(m, backend + 0x4c);
  const auto minimum = r(m, own + 8, 2), maximum = r(m, own + 10, 2);
  const auto lower_value = r(m, table + minimum * 4u, 2);
  for (std::uint32_t j = 0; si(j) <= si(count) >> 7; ++j) {
    w(m, o, 0x2e8f97, bitset + j * 16u, 8, 0);
    w(m, o, 0x2e8f97, bitset + j * 16u + 8, 8, 0);
  }
  const auto toggle = [&](std::uint32_t id, std::uint32_t site) {
    const auto p = bitset + (id >> 5) * 4;
    w(m, o, site, p, 4, r(m, p) ^ (1u << (id & 31u)));
  };
  auto start = table + 4u;
  if (r(m, backend + 0x70)) {
    const auto bucket = si(lower_value) >> ((16u - r(m, backend + 0x74)) & 31u);
    if (bucket > 0) {
      const auto slot =
                     r(m, backend + 0x78) + std::uint32_t(bucket) * 16u - 16u,
                 id = r(m, slot, 2);
      toggle(id, 0x2e8ff3);
      for (std::uint32_t j = 0; si(j) < si(r(m, slot + 8)); ++j)
        toggle(r(m, r(m, slot + 4) + j * 2u, 2), 0x2e9026);
      const auto record = r(m, bank) + id * 16u;
      for (auto p = table + r(m, record + 8, 2) * 4u + 4u;
           p < table + r(m, record + 10, 2) * 4u; p += 4u)
        if (!(r(m, p, 1) & 1u)) {
          const auto other = r(m, p + 2, 2), word = bitset + (other >> 5) * 4u;
          w(m, o, 0x2e909a, word, 4, r(m, word) & ~(1u << (other & 31u)));
        }
      start = table + r(m, record + 8, 2) * 4u + 4u;
    }
  }
  for (; start < table + minimum * 4u; start += 4u)
    toggle(r(m, start + 2, 2), 0x2e90eb);
  for (start += 4u; start < table + maximum * 4u; start += 4u)
    if (!(r(m, start, 1) & 1u))
      toggle(r(m, start + 2, 2), 0x2e9130);
  for (auto p = bitset; p < bitset + (std::uint32_t(si(count) >> 5) * 4u) + 4u;
       p += 4u) {
    auto mask = r(m, p), candidate = r(m, bank) + (p - bitset) * 128u;
    while (mask) {
      if (!(mask & 255u)) {
        mask >>= 8;
        candidate += 128u;
        continue;
      }
      if (mask & 1u) {
        const auto overlap = ((r(m, own + 4) - r(m, candidate)) |
                              (r(m, candidate + 4) - r(m, own))) &
                             0x80008000u;
        if (!overlap) {
          const auto code = r(m, candidate + 12);
          if (!(code & 1u)) {
            if (r(m, pairs + 4) == (r(m, pairs + 8) & 0x7fffffffu))
              source_double_physics_body_bank_1_1(m, pairs, 8, o);
            const auto dst = r(m, pairs) + r(m, pairs + 4) * 8u;
            w(m, o, 0x2e91fc, dst, 4, handle);
            w(m, o, 0x2e91ff, dst + 4, 4, code);
            w(m, o, 0x2e920e, pairs + 4, 4, r(m, pairs + 4) + 1u);
          } else {
            const auto slot = r(m, backend + 0x78) + (code & ~1u);
            auto j = 0u;
            while (si(j) < si(r(m, slot + 8)) &&
                   r(m, r(m, slot + 4) + j * 2u, 2) != std::uint16_t(index))
              ++j;
            if (si(j) >= si(r(m, slot + 8)))
              j = 0xffffffffu;
            w(m, o, 0x2e9255, slot + 8, 4, r(m, slot + 8) - 1u);
            w(m, o, 0x2e925f, r(m, slot + 4) + j * 2u, 2,
              r(m, r(m, slot + 4) + r(m, slot + 8) * 2u, 2));
          }
        }
      }
      mask >>= 1;
      candidate += 16;
    }
  }
  source_destroy_physics_overlap_bitset_1_1(m, descriptor, s, o);
  const auto current_bank = r(m, bank);
  source_remove_physics_broadphase_axis_1_1(m, backend + 0x4c, r(m, own + 8, 2),
                                            r(m, own + 10, 2), o);
  source_remove_physics_broadphase_axis_1_1(m, backend + 0x58, r(m, own, 2),
                                            r(m, own + 4, 2), o);
  source_remove_physics_broadphase_axis_1_1(m, backend + 0x64, r(m, own + 2, 2),
                                            r(m, own + 6, 2), o);
  source_repair_removed_physics_broadphase_indices_1_1(m, current_bank, count,
                                                       own, o);
  if (index < count - 1u) {
    const auto last = r(m, bank) + count * 16u - 16u;
    constexpr std::array<std::uint32_t, 4> copy_sites{0x2e9328, 0x2e932d,
                                                      0x2e9333, 0x2e9339};
    for (unsigned j = 0; j < 4; ++j)
      w(m, o, copy_sites[j], own + j * 4u, 4, r(m, last + j * 4u));
    for (const auto [offset, site] :
         std::array<std::array<std::uint32_t, 2>, 2>{
             {{8, 0x2e9346}, {10, 0x2e9351}}})
      w(m, o, site, r(m, backend + 0x4c) + r(m, own + offset, 2) * 4u + 2, 2,
        index);
    const auto code = r(m, own + 12);
    if (!(code & 1u)) {
      for (const auto row :
           std::array<std::array<std::uint32_t, 3>, 4>{{{0, 0x58, 0x2e9364},
                                                        {4, 0x58, 0x2e9370},
                                                        {2, 0x64, 0x2e937c},
                                                        {6, 0x64, 0x2e9388}}})
        w(m, o, row[2], r(m, backend + row[1]) + r(m, own + row[0], 2) * 4u + 2,
          2, index);
      w(m, o, 0x2e9390, code, 4, index);
    } else
      w(m, o, 0x2e93a7, r(m, backend + 0x78) + (code & ~1u), 2, index);
    if (r(m, backend + 0x70) && !(r(m, own + 12, 1) & 1u)) {
      const auto shift = (std::uint16_t(16u - r(m, backend + 0x74, 2))) & 31u;
      const auto own_min = r(m, own + 8, 2), own_max = r(m, own + 10, 2);
      auto first = r(m, r(m, backend + 0x4c) + own_min * 4u, 2) >> shift;
      if (si(first) > 0) {
        const auto id = r(m, r(m, backend + 0x78) + first * 16u - 16u, 2);
        if (r(m, r(m, bank) + id * 16u + 10, 2) > own_min)
          --first;
      }
      const auto end =
          (r(m, r(m, backend + 0x4c) + own_max * 4u, 2) >> shift) - 1u;
      for (auto k = first; si(k) <= si(end); ++k) {
        const auto slot = r(m, backend + 0x78) + k * 16u;
        auto j = 0u;
        while (si(j) < si(r(m, slot + 8)) &&
               r(m, r(m, slot + 4) + j * 2u, 2) != std::uint16_t(count - 1u))
          ++j;
        if (si(j) >= si(r(m, slot + 8)))
          j = 0xffffffffu;
        w(m, o, 0x2e9460, r(m, slot + 4) + j * 2u, 2, index);
      }
    }
  }
  const auto fresh = count - 1u, capacity = r(m, bank + 8) & 0x7fffffffu;
  if (si(capacity) < si(fresh))
    source_grow_physics_body_bank_1_1(
        m, bank, si(fresh) < si(capacity * 2u) ? capacity * 2u : fresh, 16, o);
  w(m, o, 0x2e94a7, bank + 4, 4, fresh);
}
} // namespace halo2
