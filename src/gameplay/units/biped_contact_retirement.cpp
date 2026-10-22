#include "halo2/biped_contact_retirement.hpp"
#include "halo2/physics_constraint_retirement.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourceBipedContactRetirementServices;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t v) {
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
void required(M &m, const S &s, std::uint32_t entry, std::uint32_t receiver,
              std::vector<std::uint32_t> args, const O &o) {
  if (!s.lower.required_owner)
    throw std::runtime_error(
        "Required original pair retirement owner unavailable at " +
        std::to_string(entry));
  s.lower.required_owner(m, {entry, receiver, std::move(args), false}, o);
}
void free_block(M &m, std::uint32_t p, std::uint32_t bytes, std::uint32_t cls,
                std::uint32_t site, const S &s, const O &o) {
  const auto adapter = r(m, 0x487638), backend = r(m, 0x480da0);
  if (r(m, r(m, adapter) + 0x14) != 0x148a70)
    throw std::runtime_error(
        "Required original contact retirement allocator unavailable");
  if (s.disposal.observe_free)
    s.disposal.observe_free({site, adapter, p, bytes, cls});
  if (!p)
    return;
  if (si(p) < si(backend + 0x4340u) || si(p) > si(backend + 0xf0340u) ||
      r(m, r(m, backend) + 0x14) != 0x22eb20)
    throw std::runtime_error(
        "Required original contact retirement alternate allocator unavailable");
  source_free_physics_body_small_block_1_1(m, backend, p, bytes, o);
}
void queue(M &m, std::uint32_t world, std::uint32_t body, const O &o) {
  w(m, o, 0x313246, body + 6, 2, r(m, body + 6, 2) + 1u);
  const auto d = world + 0x114;
  if (r(m, d + 4) == (r(m, d + 8) & 0x7fffffffu))
    source_double_physics_body_bank_1_1(m, d, 4, o);
  w(m, o, 0x313272, r(m, d) + r(m, d + 4) * 4u, 4, body);
  w(m, o, 0x31327a, d + 4, 4, r(m, d + 4) + 1u);
}
void remove_listener(M &m, std::uint32_t body, std::uint32_t listener,
                     const O &o) {
  auto i = 0u;
  while (si(i) < si(r(m, body + 0x8c)) &&
         r(m, r(m, body + 0x88) + i * 4u) != listener)
    ++i;
  if (si(i) >= si(r(m, body + 0x8c)))
    i = 0xffffffffu;
  w(m, o, 0x31224e, r(m, body + 0x88) + i * 4u, 4, 0);
  if (const auto world = r(m, body + 8))
    queue(m, world, body, o);
}
void release_body(M &m, std::uint32_t body, std::uint32_t site,
                  std::uint32_t resident, const S &s, const O &o) {
  w(m, o, site, body + 6, 2, r(m, body + 6, 2) - 1u);
  if (r(m, body + 6, 2))
    return;
  if (source_physics_body_reference_is_resident_1_1(m, body))
    w(m, o, resident, body + 6, 2, 1);
  else if (r(m, r(m, body)) == 0x317ad0)
    source_destroy_physics_rigid_body_1_1(m, body, 1, s.disposal, o);
  else
    required(m, s, r(m, r(m, body)), body, {1}, o);
}
void contact_listeners(M &m, std::uint32_t owner, std::uint32_t packet,
                       bool world, const S &s, const O &o) {
  const auto offset = world ? 0xb4u : 0x60u;
  w(m, o, world ? 0x31e94b : 0x31c35b, packet + 16, 4, world ? 0u : owner);
  for (auto i = r(m, owner + offset + 4) - 1u; si(i) >= 0; --i)
    if (const auto listener = r(m, r(m, owner + offset) + i * 4u)) {
      const auto method = r(m, r(m, listener) + 4);
      // The original27E760 is literally RET4.
      if (method != 0x27e760)
        required(m, s, method, listener, {packet}, o);
    }
  for (auto i = r(m, owner + offset + 4) - 1u; si(i) >= 0; --i)
    if (!r(m, r(m, owner + offset) + i * 4u)) {
      w(m, o, world ? 0x31e997 : 0x31c391, owner + offset + 4, 4,
        r(m, owner + offset + 4) - 1u);
      for (auto j = i; si(j) < si(r(m, owner + offset + 4)); ++j) {
        const auto p = r(m, owner + offset) + j * 4u;
        w(m, o, world ? 0x31e9ae : 0x31c3aa, p, 4, r(m, p + 4));
      }
    }
}
void remove_contact(M &m, std::uint32_t constraint, std::uint32_t id,
                    std::uint32_t esp, const S &s, const O &o) {
  const auto ids = r(m, constraint + 0x20), index = r(m, ids + (id & 0xffffu), 1),
             count = r(m, constraint + 0x94);
  w(m, o, 0x322235, ids + (id & 0xffffu), 1, 0xff);
  const auto packet = esp - 16u;
  w(m, o, 0x322243, esp + 4u, 4, index);
  for (unsigned i = 0; i < 4; ++i)
    w(m, o, std::array{0x322247u, 0x32224bu, 0x32224fu, 0x322253u}[i],
      packet + i * 4u, 4, 0);
  if (count == 2) {
    w(m, o, 0x322259, packet + 8, 4, 4);
    w(m, o, 0x322261, packet + 4, 4, 32);
    w(m, o, 0x322269, packet + 12, 4, 1);
  }
  for (auto i = index; si(i) < si(count - 1u); ++i)
    for (unsigned word = 0; word < 7; ++word) {
      const auto dst = r(m, constraint + 0x90) + i * 28u + word * 4u;
      w(m, o, 0x322295, dst, 4, r(m, dst + 28));
    }
  w(m, o, 0x3222ab, r(m, constraint + 0x90) + index * 28u + 24u, 2, 0);
  w(m, o, 0x3222b0, constraint + 0x94, 4, count - 1u);
  w(m, o, 0x3222b6, constraint + 0x38, 4, r(m, constraint + 0x38) - 1u);
  for (auto i = r(m, constraint + 0x24) - 1u; si(i) >= 0; --i) {
    const auto p = r(m, constraint + 0x20) + i, value = r(m, p, 1);
    if (value != 0xff && si(value) > si(index))
      w(m, o, 0x3222d4, p, 1, value - 1u);
  }
  w(m, o, 0x3222eb, packet + 4, 4, r(m, packet + 4) + 48u);
  w(m, o, 0x3222ef, packet + 8, 4, r(m, packet + 8) + 4u);
  w(m, o, 0x3222fc, packet + 12, 4, r(m, packet + 12) + 1u);
  const auto island = r(m, constraint + 20), method = r(m, r(m, island) + 12);
  if (method == 0x31c570)
    source_subtract_physics_constraint_requirements_1_1(m, island, packet, o);
  else
    required(m, s, method, island, {constraint, packet}, o);
  w(m, o, 0x32230e, constraint + 0x50, 4, r(m, constraint + 0x50) | 5u);
}
void retire_contact(M &m, std::uint32_t manager, std::uint32_t id,
                    std::uint32_t esp, const S &s, const O &o) {
  if (!esp)
    throw std::runtime_error("Required original contact retirement ABI unavailable");
  const auto packet = esp - 20u, first = r(m, manager + 0x1c),
             second = r(m, manager + 0x20);
  w(m, o, 0x31f41c, packet, 2, id);
  w(m, o, 0x31f421, packet + 4, 4, manager);
  w(m, o, 0x31f425, packet + 8, 4, first);
  w(m, o, 0x31f429, packet + 12, 4, second);
  contact_listeners(m, r(m, manager + 8), packet, true, s, o);
  if (r(m, first + 0x64))
    contact_listeners(m, first, packet, false, s, o);
  if (r(m, second + 0x64))
    contact_listeners(m, second, packet, false, s, o);
  remove_contact(m, manager + 16, id, esp - 44u, s, o);
  if (!r(m, manager + 0xa4)) {
    const auto old = r(m, r(m, manager + 8) + 0x12c, 1);
    w(m, o, 0x31f483, r(m, manager + 8) + 0x12c, 1, 0);
    SourcePhysicsConstraintRetirementServices services;
    services.disposal = s.disposal;
    services.lower.required_owner = [&](M &memory,
        const SourcePhysicsContactBatchOwnerCall &call, const O &observer) {
      required(memory, s, call.entry, call.ecx.value_or(0), call.arguments, observer);
      return 0u;
    };
    source_remove_physics_world_constraint_1_1(
        m, r(m, manager + 8), esp + 4u, manager + 16, esp - 48u, services, o);
    w(m, o, 0x31f496, r(m, manager + 8) + 0x12c, 1, old);
  }
}
void destroy_agent(M &m, std::uint32_t p, std::uint32_t flags, const S &s,
                   const O &o) {
  const auto target = r(m, r(m, p));
  if (target == 0x2f3700) {
    w(m, o, 0x2e00f0, p, 4, 0x459d94);
    if (flags & 1u)
      free_block(m, p, r(m, p + 4, 2), 0x1a, 0x2f371f, s, o);
    return;
  }
  if (target != 0x2f2070) {
    required(m, s, target, p, {flags}, o);
    return;
  }
  w(m, o, 0x2efadd, p, 4, 0x4190d8);
  const auto cap = r(m, p + 0x14);
  if (si(cap) >= 0)
    free_block(m, r(m, p + 0xc), (cap & 0x7fffffffu) * 12u, 0x12, 0x2efb0c, s,
               o);
  w(m, o, 0x2efb13, p, 4, 0x459d94);
  if (flags & 1u)
    free_block(m, p, r(m, p + 4, 2), 0x1a, 0x2f208f, s, o);
}
void retire_agent(M &m, std::uint32_t agent, std::uint32_t target,
                  std::uint32_t esp, const S &s, const O &o) {
  if (target == 0x2ec8d0) {
    const auto child = r(m, agent + 12);
    retire_agent(m, child, r(m, r(m, child) + 0x14), esp ? esp - 8u : 0u, s, o);
    destroy_agent(m, agent, 1, s, o);
  } else if (target == 0x2ef640) {
    auto nested = s;
    nested.entry_esp = esp;
    source_retire_physics_pair_agent_1_1(m, agent, nested, o);
  } else if (target == 0x2f4870) {
    const auto manager = r(m, agent + 8);
    while (r(m, agent + 24)) {
      const auto index = r(m, agent + 24) - 1u;
      w(m, o, 0x308f8e, agent + 24, 4, index);
      const auto id = r(m, agent + 0x30u + index * 48u + 28u, 2),
                 method = r(m, r(m, manager) + 8);
      if (method == 0x31f400)
        retire_contact(m, manager, id, esp ? esp - 40u : 0u, s, o);
      else
        required(m, s, method, manager, {id}, o);
    }
    destroy_agent(m, agent, 1, s, o);
  }
  else
    required(m, s, target, agent, {}, o);
}
void dispose_pair(M &m, std::uint32_t row, const S &s, const O &o) {
  const auto agent = r(m, row + 8);
  if (!agent)
    return;
  const auto manager = r(m, agent + 8), target = r(m, r(m, agent) + 0x14);
  retire_agent(m, agent, target, s.entry_esp ? s.entry_esp - 28u : 0u, s, o);
  const auto mt = r(m, r(m, manager) + 0x10);
  if (mt == 0x31f6b0) {
    if (manager) {
      const auto dt = r(m, r(m, manager));
      if (dt == 0x31f740)
        source_destroy_physics_pair_contact_manager_1_1(m, manager, 1, s, o);
      else
        required(m, s, dt, manager, {1}, o);
    }
  } else
    required(m, s, mt, manager, {}, o);
}
bool match(M &m, std::uint32_t row, std::uint32_t a, std::uint32_t b) {
  return r(m, r(m, row) + 4) == r(m, a + 4) &&
         r(m, r(m, row + 4) + 4) == r(m, b + 4);
}
} // namespace
void source_retire_physics_pair_agent_1_1(M &m, std::uint32_t p, const S &s,
                                          const O &o) {
  const auto end = r(m, p + 0xc) + r(m, p + 0x10) * 12u;
  for (auto row = r(m, p + 0xc); row != end; row += 12u)
    if (const auto child = r(m, row + 8))
      retire_agent(m, child, r(m, r(m, child) + 0x14),
                   s.entry_esp ? s.entry_esp - 16u : 0u, s, o);
  destroy_agent(m, p, 1, s, o);
}
void source_destroy_physics_pair_contact_manager_1_1(M &m, std::uint32_t p,
                                                     std::uint32_t flags,
                                                     const S &s, const O &o) {
  w(m, o, 0x31f65f, p, 4, 0x41b6b4);
  if (r(m, p + 0xa4)) {
    if (!s.attached_constraint_output)
      throw std::runtime_error("Required original attached contact constraint "
                               "ABI output unavailable");
    required(m, s, 0x312f20, r(m, p + 8),
             {s.attached_constraint_output, p + 0x10}, o);
  }
  const auto q = p + 0x10;
  for (const auto row : std::array<std::array<std::uint32_t, 4>, 3>{
           {{0x90, 0x98, 28, 0x31bb1a},
            {0x34, 0x3c, 32, 0x31bb40},
            {0x20, 0x28, 1, 0x31bb63}}}) {
    const auto cap = r(m, q + row[1]);
    if (si(cap) >= 0)
      free_block(m, r(m, q + row[0]), (cap & 0x7fffffffu) * row[2], 0x12,
                 row[3], s, o);
  }
  w(m, o, 0x321fa0, q, 4, 0x41b9a0);
  w(m, o, 0x321fa6, q + 8, 4, 0x41b990);
  for (const auto row : std::array<std::array<std::uint32_t, 3>, 2>{
           {{0xc, 0x321fc5, 0x321fdd}, {0x10, 0x321ffd, 0x322015}}})
    if (const auto body = r(m, q + row[0])) {
      remove_listener(m, body, q, o);
      release_body(m, r(m, q + row[0]), row[1], row[2], s, o);
    }
  w(m, o, 0x322027, q + 8, 4, 0x41b980);
  w(m, o, 0xdd97b, q, 4, 0x459d98);
  if (r(m, q + 4, 2) & 0x8000u) {
    w(m, o, 0xdd98a, q + 4, 2, r(m, q + 4, 2) & 0x7fffu);
    if (!s.disposal.clear_reference_flag)
      throw std::runtime_error(
          "Required original pair constraint reference flag owner unavailable");
    s.disposal.clear_reference_flag(m, q, o);
  }
  w(m, o, 0xdd993, q, 4, 0x459d94);
  w(m, o, 0x31f699, p, 4, 0x459d94);
  if (flags & 1u)
    free_block(m, p, r(m, p + 4, 2), 0x1d, 0x31f75f, s, o);
}
void source_retire_physics_island_pair_1_1(M &m, std::uint32_t list,
                                           std::uint32_t pair, const S &s,
                                           const O &o) {
  w(m, o, 0x31c6eb, list + 12, 1, 1);
  auto row = r(m, list + 0x80);
  const auto end = r(m, list + 0x84), a = r(m, pair), b = r(m, pair + 4);
  while (row < end) {
    const auto key = r(m, r(m, row) + 4), own = r(m, a + 4);
    if (key > own || (key == own && r(m, r(m, row + 4) + 4) >= r(m, b + 4)))
      break;
    row += 16u;
  }
  if (row != end && match(m, row, a, b)) {
    while (r(m, list + 0x80) < row) {
      const auto src = r(m, list + 0x80), dst = r(m, list + 0x88);
      for (unsigned i = 0; i < 4; ++i)
        w(m, o, std::array{0x31c76eu, 0x31c773u, 0x31c779u, 0x31c77fu}[i],
          dst + i * 4u, 4, r(m, src + i * 4u));
      w(m, o, 0x31c79b, list + 0x88, 4, dst + 16u);
      w(m, o, 0x31c7a1, list + 0x80, 4, src + 16u);
    }
    dispose_pair(m, r(m, list + 0x80), s, o);
    w(m, o, 0x31c7c5, list + 0x80, 4, r(m, list + 0x80) + 16u);
    return;
  }
  const auto island_for = [&](std::uint32_t handle) {
    return r(m, r(m, (handle ? handle - 16u : 0u) + 0x20) + 0x44);
  };
  for (const auto island : {island_for(a), island_for(b)}) {
    const auto d = island + 0x74;
    for (std::uint32_t i = 0; si(i) < si(r(m, d + 4)); ++i)
      if (match(m, r(m, d) + i * 8u, a, b)) {
        w(m, o, 0x31c848, d + 4, 4, r(m, d + 4) - 1u);
        const auto bank = r(m, d), last = r(m, d + 4);
        w(m, o, 0x31c84f, bank + i * 8u, 4, r(m, bank + last * 8u));
        w(m, o, 0x31c858, bank + i * 8u + 4, 4, r(m, bank + last * 8u + 4));
        return;
      }
  }
  auto island = island_for(a);
  if (island == list - 0x1cu)
    island = island_for(b);
  for (std::uint32_t i = 0; si(i) < si(r(m, island + 0x6c)); ++i)
    if (match(m, r(m, island + 0x68) + i * 16u, a, b)) {
      dispose_pair(m, r(m, island + 0x68) + i * 16u, s, o);
      w(m, o, 0x31c93c, island + 0x6c, 4, r(m, island + 0x6c) - 1u);
      for (auto j = i; si(j) < si(r(m, island + 0x6c)); ++j)
        for (unsigned k = 0; k < 4; ++k) {
          const auto dst = r(m, island + 0x68) + j * 16u;
          w(m, o, std::array{0x31c94fu, 0x31c954u, 0x31c95au, 0x31c960u}[k],
            dst + k * 4u, 4, r(m, dst + 16u + k * 4u));
        }
      return;
    }
}
} // namespace halo2
