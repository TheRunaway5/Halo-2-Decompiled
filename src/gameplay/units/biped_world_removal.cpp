#include "halo2/biped_world_removal.hpp"
#include "halo2/biped_broadphase_removal.hpp"
#include "halo2/biped_contact_retirement.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourceBipedWorldRemovalServices;
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
std::uint32_t required(M &m, const S &s, std::uint32_t site,
                       std::uint32_t entry,
                       std::optional<std::uint32_t> receiver, std::uint32_t esp,
                       std::vector<std::uint32_t> args, const O &o) {
  if (!s.lower.required_owner)
    throw std::runtime_error(
        "Required original world-removal lower owner unavailable at " +
        std::to_string(entry));
  return s.lower.required_owner(
      m, {site, entry, esp, receiver, std::move(args)}, o);
}
void free_block(M &m, std::uint32_t p, std::uint32_t bytes, std::uint32_t cls,
                std::uint32_t site, const SourcePhysicsBodyDisposalServices &s,
                const O &o) {
  const auto adapter = r(m, 0x487638);
  if (r(m, r(m, adapter) + 0x14) != 0x148a70)
    throw std::runtime_error(
        "Required original world-removal allocator unavailable");
  if (s.observe_free)
    s.observe_free({site, adapter, p, bytes, cls});
  if (!p)
    return;
  const auto backend = r(m, 0x480da0);
  if (si(p) < si(backend + 0x4340u) || si(p) > si(backend + 0xf0340u) ||
      r(m, r(m, backend) + 0x14) != 0x22eb20)
    throw std::runtime_error(
        "Required original world-removal alternate allocator unavailable");
  source_free_physics_body_small_block_1_1(m, backend, p, bytes, o);
}
void listener_remove(M &m, std::uint32_t body, std::uint32_t value,
                     bool contact, const O &o) {
  const auto offset = contact ? 0x60u : 0x70u;
  std::uint32_t i = 0;
  while (si(i) < si(r(m, body + offset + 4)) &&
         r(m, r(m, body + offset) + i * 4) != value)
    ++i;
  const bool found = si(i) < si(r(m, body + offset + 4));
  if (!found)
    i = 0xffffffffu;
  w(m, o,
    contact ? (found ? 0x3122e5 : 0x3122d6) : (found ? 0x3122a5 : 0x312296),
    r(m, body + offset) + i * 4, 4, 0);
}
void world_listeners(M &m, std::uint32_t world, std::uint32_t body,
                     std::uint32_t esp, const S &s, const O &o) {
  for (auto i = r(m, world + 0x4c) - 1u; si(i) >= 0; --i) {
    const auto listener = r(m, r(m, world + 0x48) + i * 4);
    if (listener)
      required(m, s, 0x31e69e, r(m, r(m, listener) + 8), listener, esp - 20u,
               {body}, o);
  }
  for (auto i = r(m, world + 0x4c) - 1u; si(i) >= 0; --i)
    if (!r(m, r(m, world + 0x48) + i * 4)) {
      w(m, o, 0x31e6c1, world + 0x4c, 4, r(m, world + 0x4c) - 1u);
      for (auto j = i; si(j) < si(r(m, world + 0x4c)); ++j) {
        const auto p = r(m, world + 0x48) + j * 4;
        w(m, o, 0x31e6da, p, 4, r(m, p + 4));
      }
    }
}
void body_listeners(M &m, std::uint32_t body, std::uint32_t esp, const S &s,
                    const O &o) {
  for (auto i = r(m, body + 0x80) - 1u; si(i) >= 0; --i) {
    const auto p = r(m, r(m, body + 0x7c) + i * 4);
    if (p)
      required(m, s, 0x31c26d, r(m, r(m, p) + 8), p, esp - 16u, {body}, o);
  }
  for (const auto offset : {0x88u, 0x94u})
    for (std::uint32_t i = 0; si(i) < si(r(m, body + offset + 4)); ++i) {
      const auto p = r(m, r(m, body + offset) + i * 4);
      if (p)
        required(m, s, offset == 0x88 ? 0x31c294 : 0x31c2c4,
                 r(m, r(m, p + 8) + 8), p + 8, esp - 16u, {body}, o);
    }
}
void detach_island(M &m, std::uint32_t island, std::uint32_t body,
                   std::uint32_t esp, const S &s, const O &o) {
  const auto bank = r(m, island + 0x3c), index = r(m, body + 0x58);
  w(m, o, 0x31c5d5, bank + index * 4, 4,
    r(m, bank + r(m, island + 0x40) * 4 - 4));
  w(m, o, 0x31c5e1, r(m, r(m, island + 0x3c) + r(m, body + 0x58) * 4) + 0x58, 4,
    r(m, body + 0x58));
  w(m, o, 0x31c5e4, island + 0x40, 4, r(m, island + 0x40) - 1u);
  w(m, o, 0x31c5e7, body + 0x44, 4, 0);
  w(m, o, 0x31c5ee, body + 0x58, 4, 0xffffffffu);
  const auto motion = r(m, body + 0x3c),
             motion_method = r(m, r(m, motion) + 0x14);
  const auto island_method = r(m, r(m, island) + 0x10);
  const auto amount =
      motion_method == 0x31f8a0
          ? 0x80u
          : required(m, s, 0x31c5fe, motion_method, motion, esp - 12u, {}, o);
  if (island_method == 0x31c5a0)
    w(m, o, 0x31c5a9, island + 0x14, 4, r(m, island + 0x14) - amount);
  else
    required(m, s, 0x31c606, island_method, island, esp - 16u, {0u - amount},
             o);
  w(m, o, 0x31c60a, island + 0x28, 1, 1);
}
void flush(M &m, const S &s, std::uint32_t esp, const O &o) {
  if (r(m, 0x5660b0, 1) && !r(m, 0x486563, 1))
    required(m, s, 0x1d4d05, 0x27f190, std::nullopt, esp, {}, o);
}
std::uint32_t kind(M &m, std::uint32_t body, std::uint32_t esp, const S &s,
                   const O &o) {
  const auto motion = r(m, body + 0x3c), target = r(m, r(m, motion) + 0x18);
  if (target == 0x9d900)
    return 4;
  if (target == 0x9b560)
    return 5;
  if (target == 0x31b260)
    return 6;
  if (target == 0x31b250)
    return 7;
  return required(m, s, 0x1d4cc7, target, motion, esp, {}, o);
}
std::uint32_t object(const M &m, std::uint32_t datum) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (datum & 0xffffu) * 12 + 8);
}
std::uint32_t tag(const M &m, std::uint32_t datum) {
  return r(m, r(m, 0x4eb114) + (datum & 0xffffu) * 16 + 8);
}
} // namespace
void source_compact_physics_body_listeners_1_1(M &m, std::uint32_t body,
                                               const O &o) {
  constexpr std::array<std::array<std::uint32_t, 3>, 3> rows{
      {{0x7c, 0x313989, 0x3139b4},
       {0x88, 0x3138a9, 0x3138d2},
       {0x94, 0x313919, 0x313942}}};
  for (const auto row : rows) {
    const auto bank = body + row[0], count = r(m, bank + 4);
    std::uint32_t next = 0;
    while (si(next) < si(count) && r(m, r(m, bank) + next * 4))
      ++next;
    for (auto i = next + 1u; si(i) < si(count); ++i) {
      const auto value = r(m, r(m, bank) + i * 4);
      if (value) {
        w(m, o, row[1], r(m, bank) + next * 4, 4, value);
        ++next;
      }
    }
    const auto capacity = r(m, bank + 8) & 0x7fffffffu;
    if (si(capacity) < si(next))
      source_grow_physics_body_bank_1_1(
          m, bank, si(next) < si(capacity * 2u) ? capacity * 2u : next, 4, o);
    w(m, o, row[2], bank + 4, 4, next);
  }
}
void source_destroy_physics_world_island_1_1(
    M &m, std::uint32_t p, std::uint32_t flags,
    const SourcePhysicsBodyDisposalServices &s, const O &o) {
  w(m, o, 0x31d830, p, 4, 0x41b610);
  w(m, o, 0x31d836, p + 0x1c, 4, 0x41b604);
  constexpr std::array<std::array<std::uint32_t, 4>, 7> banks{
      {{0x8c, 0x94, 3, 0x31d869},
       {0x80, 0x88, 3, 0x31d895},
       {0x74, 0x7c, 3, 0x31d8bb},
       {0x68, 0x70, 4, 0x31d8e1},
       {0x58, 0x60, 2, 0x31d907},
       {0x4c, 0x54, 2, 0x31d92d},
       {0x3c, 0x44, 2, 0x31d953}}};
  for (const auto row : banks) {
    const auto capacity = r(m, p + row[1]);
    if (si(capacity) >= 0)
      free_block(m, r(m, p + row[0]), (capacity & 0x7fffffffu) << row[2], 0x12,
                 row[3], s, o);
  }
  w(m, o, 0x31d95a, p + 0x1c, 4, 0x419ebc);
  w(m, o, 0x31d961, p, 4, 0x459d94);
  if (flags & 1u)
    free_block(m, p, r(m, p + 4, 2), 0x2d, 0x31e58f, s, o);
}
void source_detach_physics_world_body_1_1(M &m, std::uint32_t world,
                                          std::uint32_t body, std::uint32_t esp,
                                          const S &s, const O &o) {
  const auto island = r(m, body + 0x44), frame = esp - 12u;
  w(m, o, 0x31481f, body + 8, 4, 0);
  detach_island(m, island, body, frame - 8u, s, o);
  if (r(m, island + 0x24) != 0xffffffffu && !r(m, island + 0x40)) {
    const bool active = r(m, island + 0x29, 1) != 0;
    const auto bank = world + (active ? 8u : 0x14u),
               index = r(m, island + 0x24);
    w(m, o, active ? 0x31486d : 0x31484c, r(m, bank) + index * 4, 4,
      r(m, r(m, bank) + r(m, bank + 4) * 4 - 4));
    w(m, o, active ? 0x314879 : 0x314858,
      r(m, r(m, bank) + r(m, island + 0x24) * 4) + 0x24, 4,
      r(m, island + 0x24));
    w(m, o, active ? 0x31487c : 0x31485b, bank + 4, 4, r(m, bank + 4) - 1u);
    const auto target = r(m, r(m, island));
    if (target == 0x31e570)
      source_destroy_physics_world_island_1_1(m, island, 1, s.disposal, o);
    else
      required(m, s, 0x314885, target, island, frame - 8u, {1}, o);
  }
  for (std::uint32_t i = 0; si(i) < si(r(m, world + 0x118)); ++i) {
    const auto p = r(m, r(m, world + 0x114) + i * 4);
    source_compact_physics_body_listeners_1_1(m, p, o);
    const auto released = r(m, r(m, world + 0x114) + i * 4);
    w(m, o, 0x3148b7, released + 6, 2, r(m, released + 6, 2) - 1u);
    if (!r(m, released + 6, 2)) {
      if (source_physics_body_reference_is_resident_1_1(m, released))
        w(m, o, 0x3148cf, released + 6, 2, 1);
      else if (r(m, r(m, released)) == 0x317ad0)
        source_destroy_physics_rigid_body_1_1(m, released, 1, s.disposal, o);
      else
        required(m, s, 0x3148dd, r(m, r(m, released)), released, frame - 8u,
                 {1}, o);
    }
  }
  w(m, o, 0x3148eb, world + 0x118, 4, 0);
}
void source_remove_biped_world_broadphase_1_1(M &m, std::uint32_t world,
                                              std::uint32_t body,
                                              std::uint32_t esp, const S &s,
                                              const O &o) {
  const auto frame = esp - 0x54u, descriptor = frame + 0x10u;
  source_construct_physics_pair_bank_1_1(m, descriptor, r(m, world + 0x234),
                                         s.scratch, o);
  auto pairs = s.pairs;
  pairs.scratch = s.scratch;
  const auto external_pairs = pairs.required_owner;
  pairs.required_owner = [&](M &memory, const SourcePhysicsPairOwnerCall &call,
                             const O &observer) {
    if (call.entry == 0x31c6d0) {
      SourceBipedContactRetirementServices retirement;
      retirement.lower.required_owner = external_pairs;
      retirement.disposal = s.disposal;
      retirement.entry_esp = call.entry_esp;
      source_retire_physics_island_pair_1_1(
          memory, call.receiver, call.arguments.at(0), retirement, observer);
      return 0u;
    }
    if (external_pairs)
      return external_pairs(memory, call, observer);
    // This pair-owner interface owns receiver/arguments; it does not expose
    // the original caller's temporary ABI frame.
    return required(memory, s, 0, call.entry,
                    call.cdecl ? std::nullopt : std::optional(call.receiver), 0,
                    call.arguments, observer);
  };
  if (r(m, body + 0xc)) {
    const auto backend = r(m, world + 0xc4);
    const auto target = r(m, r(m, backend) + 0xc);
    if (target == 0x2e8f00)
      source_remove_physics_broadphase_body_1_1(
          m, backend, body + 0x1c, descriptor, frame - 12u, s.scratch, o);
    else
      required(m, s, 0x3146e7, target, backend, frame - 12u,
               {body + 0x1c, descriptor}, o);
    if (si(r(m, descriptor + 4)) > 0) {
      if (r(m, body + 0x40, 1)) {
        const auto list = frame + 0x24u;
        w(m, o, 0x314703, list, 4, 0x419f30);
        w(m, o, 0x31470b, list + 8, 4, frame + 0x38u);
        w(m, o, 0x31470f, list + 12, 4, 0);
        w(m, o, 0x314713, list + 16, 4, 0x80000004u);
        w(m, o, 0x31471b, list + 4, 4, body + 12u);
        w(m, o, 0x314729, r(m, world + 0x138) + 0x24, 4, list);
        const auto filter = r(m, world + 0xd0);
        source_deliver_physics_pairs_1_1(m, r(m, world + 0x138),
                                         {0, 0, r(m, descriptor),
                                          r(m, descriptor + 4),
                                          filter ? filter + 8u : 0},
                                         {frame - 24u}, pairs, o);
        const auto manager = r(m, world + 0x138);
        w(m, o, 0x314766, manager + 0x24, 4, manager + 0x100);
        for (std::uint32_t i = 0; si(i) < si(r(m, list + 12)); ++i) {
          const auto island = r(m, r(m, list + 8) + i * 4),
                     removed = island + 0x8c;
          source_update_physics_island_pairs_1_1(
              m, island, {0, 0, r(m, removed), r(m, removed + 4)}, frame - 64u,
              pairs, o);
          // The original mask makes this comparison nonnegative, so the
          // source's capacity-growth branch cannot run here.
          w(m, o, 0x3147b7, removed + 4, 4, 0);
        }
        const auto capacity = r(m, list + 16);
        if (si(capacity) >= 0)
          free_block(m, r(m, list + 8), (capacity & 0x7fffffffu) * 4u, 0x12,
                     0x314213, s.disposal, o);
        w(m, o, 0x31421a, list, 4, 0x419ebc);
      } else
        source_update_physics_island_pairs_1_1(
            m, r(m, body + 0x44),
            {0, 0, r(m, descriptor), r(m, descriptor + 4)}, frame - 64u, pairs,
            o);
    }
  }
  source_destroy_physics_pair_bank_1_1(m, descriptor, s.scratch, o);
}
void source_remove_physics_world_body_1_1(M &m, std::uint32_t world,
                                          std::uint32_t output,
                                          std::uint32_t body, std::uint32_t esp,
                                          const S &s, const O &o) {
  if (r(m, world + 0x12c, 1) == 1) {
    const auto bank = world + 0xf0;
    if (r(m, bank + 4) == (r(m, bank + 8) & 0x7fffffffu))
      source_double_physics_body_bank_1_1(m, bank, 4, o);
    w(m, o, 0x315d8c, r(m, bank) + r(m, bank + 4) * 4, 4, body);
    w(m, o, 0x315d8f, bank + 4, 4, r(m, bank + 4) + 1u);
    w(m, o, 0x315d92, body + 6, 2, r(m, body + 6, 2) + 1u);
    w(m, o, 0x315d9b, output, 1, 0);
    return;
  }
  if (si(r(m, world + 0x24)) > 0)
    required(m, s, 0x315dab, 0x315c00, world, esp - 12u, {}, o);
  source_remove_biped_world_broadphase_1_1(m, world, body, esp - 16u, s, o);
  world_listeners(m, world, body, esp - 20u, s, o);
  body_listeners(m, body, esp - 24u, s, o);
  source_detach_physics_world_body_1_1(m, world, body, esp - 16u, s, o);
  w(m, o, 0x315dd4, body + 6, 2, r(m, body + 6, 2) - 1u);
  if (!r(m, body + 6, 2)) {
    if (source_physics_body_reference_is_resident_1_1(m, body)) {
      w(m, o, 0x315df0, body + 6, 2, 1);
      w(m, o, 0x315df7, output, 1, 1);
      return;
    }
    if (r(m, r(m, body)) == 0x317ad0)
      source_destroy_physics_rigid_body_1_1(m, body, 1, s.disposal, o);
    else
      required(m, s, 0x315e04, r(m, r(m, body)), body, esp - 16u, {1}, o);
  }
  w(m, o, 0x315e0b, output, 1, 1);
}
void source_remove_biped_component_world_1_1(M &m, std::uint32_t p,
                                             std::uint32_t esp, const S &s,
                                             const O &o) {
  const auto frame = esp - 28u;
  for (std::uint32_t i = 0; si(i) < si(r(m, p + 0x80)); ++i)
    required(m, s, 0x1d4c7a, 0x312f20, r(m, 0x526234), frame - 12u,
             {frame + 0x12, r(m, r(m, p + 0x7c) + i * 12 + 8)}, o);
  for (std::uint32_t i = 0; si(i) < si(r(m, p + 0x74)); ++i) {
    const auto body = r(m, r(m, p + 0x70) + i * 0x60 + 0x40);
    listener_remove(m, body, r(m, 0x526538), false, o);
    if ((r(m, p + 4, 1) & 1u) && !r(m, body + 0x40, 1) &&
        kind(m, body, frame - 4u, s, o) != 6) {
      const auto listener = r(m, 0x526538);
      listener_remove(m, body, listener ? listener + 4 : 0, true, o);
    }
    w(m, o, 0x1d4cf3, 0x486558, 4, r(m, 0x486558) - 1u);
    source_remove_physics_world_body_1_1(m, r(m, 0x526234), frame + 0x13, body,
                                         frame - 12u, s, o);
    flush(m, s, frame - 4u, o);
  }
  if (const auto attached = r(m, p + 0x94)) {
    const auto model =
        tag(m, r(m, tag(m, r(m, object(m, r(m, p + 8)))) + 0x38));
    const auto physical = tag(m, r(m, model + 0x24));
    auto index = r(m, attached + 4) - 1u;
    w(m, o, 0x1d4d73, frame + 0x18, 4, physical);
    w(m, o, 0x1d4d77, frame + 0x14, 4, index);
    for (; si(index) >= 0; --index) {
      const auto row = r(m, r(m, p + 0x94)) + index * 8,
                 header_pool = r(m, 0x4e78d0), datum = r(m, row + 4);
      const auto local_index =
          std::uint32_t(std::int32_t(std::int8_t(r(m, row, 1))));
      const auto constraint = std::uint32_t(std::int32_t(
          std::int16_t(r(m, r(m, physical + 0x44) + local_index * 12 + 8, 2))));
      const auto definition = r(m, physical + 0x2c) + constraint * 0x68;
      if (datum != 0xffffffffu &&
          si(datum & 0xffffu) < si(r(m, header_pool + 0x38))) {
        const auto header = r(m, header_pool + 0x44) +
                            (datum & 0xffffu) * r(m, header_pool + 0x24);
        const auto salt = std::int32_t(std::int16_t(r(m, header, 2)));
        if (salt && salt == (si(datum) >> 16) &&
            (1u << (r(m, header + 3, 1) & 31u)) && r(m, header + 8)) {
          source_biped_physics_activation_1_1(m, datum, false, s.activation, o);
          const auto live_datum = r(m, row + 4),
                     live_head = r(m, r(m, 0x4e78d0) + 0x44) +
                                 (live_datum & 0xffffu) * 12;
          if ((r(m, definition) & 0x1000000u) &&
              (r(m, live_head + 3, 1) & 31u) == 0) {
            const auto unit = r(m, live_head + 8);
            if (r(m, unit + 0x3dc, 1) == 1 && r(m, unit + 0x3e4) == r(m, p + 8))
              w(m, o, 0x1d4e4f, unit + 0x3e4, 4, 0xffffffffu);
          }
        }
      }
      w(m, o, 0x1d4e5e, frame + 0x14, 4, index - 1u);
    }
    w(m, o, 0x1d4e6e, r(m, p + 0x94) + 4, 4, 0);
  }
  if (r(m, p + 4, 1) & 1u) {
    for (;;) {
      const auto datum = r(m, p + 0x20);
      if (datum == 0xffffffffu)
        break;
      const auto row = r(m, r(m, 0x526490) + 0x44) + (datum & 0xffffu) * 0x40;
      if (!r(m, row + 2, 2))
        break;
      required(m, s, 0x1d4ea8, 0x2287a0, std::nullopt, frame - 8u,
               {r(m, row + 4)}, o);
    }
    const auto capacity = r(m, p + 0x90);
    if (si(capacity) >= 0)
      free_block(m, r(m, p + 0x88), (capacity & 0x7fffffffu) * 0x48, 0x12,
                 0x1d4ed6, s.disposal, o);
    w(m, o, 0x1d4ed9, p + 0x88, 4, 0);
    w(m, o, 0x1d4ee3, p + 0x8c, 4, 0);
    w(m, o, 0x1d4eed, p + 0x90, 4, 0x80000000u);
  }
  for (const auto offset : {0x98u, 0x9cu})
    if (const auto value = r(m, p + offset))
      required(m, s, offset == 0x98 ? 0x1d4f08 : 0x1d4f1e, 0x314b30,
               r(m, 0x526234), frame - 8u, {value}, o);
  w(m, o, 0x1d4f23, p + 4, 4, r(m, p + 4) & ~0x20u);
}
} // namespace halo2
