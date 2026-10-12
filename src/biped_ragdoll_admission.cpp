#include "halo2/biped_ragdoll_admission.hpp"
#include "halo2/physics_shape_wrapper.hpp"
#include "halo2/unit_animation_resolution.hpp"
#include <bit>
#include <cfenv>
#include <limits>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return x;
}
std::int32_t si(std::uint32_t x) { return std::bit_cast<std::int32_t>(x); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n) {
  const auto x = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t x) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(x >> (i * 8));
  m.write(p, s.written);
  if (o)
    o(s);
}
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
template <class F, class... A>
decltype(auto) need(const F &f, const char *s, A &&...a) {
  if (!f)
    missing(s);
  return f(std::forward<A>(a)...);
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12u + 8);
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 0xffffu) * 16u + 8);
}
std::uint32_t component(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x526248) + 0x44) +
         (r(m, object(m, d) + 0xb4) & 0xffffu) * 160u;
}
float f(std::uint32_t x) { return std::bit_cast<float>(x); }
std::uint32_t u(float x) { return std::bit_cast<std::uint32_t>(x); }
__m128 scalar(std::uint32_t x) { return _mm_set_ss(f(x)); }
std::uint32_t word(__m128 x) { return u(_mm_cvtss_f32(x)); }
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
      missing("Required original admission FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(p.x87_control_word) : "memory");
#else
    missing("Required original admission x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::int32_t rounded(__m128 v) {
  float x = _mm_cvtss_f32(v);
  std::int32_t out;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfistpl %0" : "=m"(out) : "m"(x) : "memory");
#else
  missing("Required original admission FISTP backend unavailable");
#endif
  return out;
}
std::uint32_t property(const M &m, std::uint32_t body, std::uint32_t key) {
  const auto count = si(r(m, body + 0x34));
  if (count > 0) {
    const auto bank = r(m, body + 0x30);
    for (std::int32_t i = 0; i < count; ++i)
      if (r(m, bank + std::uint32_t(i) * 8) == key)
        return r(m, bank + std::uint32_t(i) * 8 + 4);
  }
  return 0;
}
std::uint32_t
animation_definition(const M &m, std::uint32_t graph, std::uint32_t packed,
                     const SourceBipedAnimationResourceOwner &owner) {
  const auto index =
      std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(packed >> 16)));
  if (index == -1)
    return 0;
  const auto inherited =
      std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(packed)));
  auto p = tag(m, graph);
  if (inherited != -1) {
    std::uint32_t next = 0;
    if (inherited < si(r(m, p + 0x4c))) {
      const auto d = r(m, r(m, p + 0x50) + std::uint32_t(inherited) * 0x20 + 4);
      if (d != 0xffffffffu)
        next = tag(m, d);
    }
    p = next;
  }
  p = r(m, p + 0x30) + std::uint32_t(index) * 0x6c;
  if (p)
    need(owner, "Required original1E13C0 graph resource owner unavailable",
         graph, packed);
  return p;
}
std::uint8_t place(const SourceBipedPlacementServices &services,
                    const SourceBipedPlacementRequest &request,
                    std::uint32_t entry) {
  if (services.place_body_abi) {
    if (!entry)
      missing("OriginalE3270 caller frame required");
    return services.place_body_abi(request, entry);
  }
  return need(services.place_body,
              "Required originalE3270 physical-body placement owner unavailable",
              request);
}
void traverse(M &m, std::uint32_t d, std::uint32_t anchor, std::uint32_t height,
              const SourceBipedPlacementScratch &s,
              const SourceBipedPlacementServices &services, std::uint32_t slot,
              std::uint8_t valid, const O &o, std::uint32_t entry = 0) {
  const auto c = component(m, d);
  const auto first_output = entry ? entry - 0xc : s.constraint_first;
  const auto second_output = entry ? entry - 8 : s.constraint_second;
  const auto result_output = entry ? entry - 4 : s.child_result;
  for (std::uint32_t i = 0; si(i) < si(r(m, c + 0x80)); ++i) {
    source_biped_constraint_slots_1_1(m, c, i, first_output,
                                      second_output, o);
    const auto first = r(m, first_output), second = r(m, second_output);
    auto next = 0xffffffffu;
    if (first == slot)
      next = second;
    if (second == slot)
      next = first;
    if (next == 0xffffffffu)
      continue;
    const auto mask = 1u << (next & 31u),
               address = s.visited_mask + std::uint32_t(si(next) >> 5) * 4;
    if (r(m, address) & mask)
      continue;
    const auto child = place(services,
        SourceBipedPlacementRequest{d, anchor, height, next, i, slot, valid},
        entry ? entry - 0x40 : 0);
    w(m, o, 0xe3ad1, result_output, 1, child);
    w(m, o, 0xe3ad9, address, 4, r(m, address) | mask);
    traverse(m, d, anchor, height, s, services, next, child, o,
              entry ? entry - 0x40 : 0);
  }
}
} // namespace
void source_biped_animation_resource_1_1(
    const M &m, std::uint32_t graph, std::uint32_t packed,
    const std::function<void(std::uint32_t, std::uint8_t)> &request) {
  const auto first = std::uint8_t(r(m, 0x518410, 1));
  const std::array<std::uint8_t, 2> flags{
      first, first ? std::uint8_t(r(m, 0x518411, 1)) : std::uint8_t(0)};
  if (!flags[0] || !flags[1] || (packed >> 16) == 0xffffu)
    return;
  SourceAnimationGraphMemory memory;
  std::vector<std::byte> buffer;
  memory.graph_address = [&](std::uint32_t d) { return tag(m, d); };
  memory.read = [&](std::uint32_t p, std::size_t n) {
    buffer.resize(n);
    m.read(p, buffer);
    return std::span<const std::byte>(buffer);
  };
  SourceAnimationResourceServices resources;
  resources.state = [&] { return flags; };
  resources.request = request;
  demand_source_animation(tag(m, graph), packed, memory, resources);
}
void source_biped_rigid_node_slots_1_1(M &m, std::uint32_t d, std::uint32_t out,
                                       const O &o) {
  const auto p = object(m, d), model = r(m, tag(m, r(m, p)) + 0x38);
  w(m, o, 0x183684, out, 4, 0);
  w(m, o, 0x18368b, out + 0x104, 1, 0xff);
  if (model == 0xffffffffu)
    return;
  const auto phmo = r(m, tag(m, model) + 0x24);
  if (phmo == 0xffffffffu)
    return;
  const auto def = tag(m, phmo), c = component(m, d);
  w(m, o, 0x1836e1, out, 4, r(m, def + 0xc8));
  for (unsigned i = 0; i < 64; ++i)
    w(m, o, 0x1836ef, out + 4 + i * 4, 4, 0xffffffffu);
  for (std::uint32_t i = 0; si(i) < si(r(m, c + 0x74)); ++i) {
    for (std::uint32_t j = 0;
         si(j) < si(r(m, r(m, c + 0x70) + i * 0x60 + 0x4c)); ++j) {
      const auto id = sx(m, r(m, r(m, c + 0x70) + i * 0x60 + 0x48) + j, 1);
      if (id == -1)
        continue;
      const auto node = sx(m, r(m, def + 0x3c) + std::uint32_t(id) * 0x90, 2);
      if (node == -1)
        w(m, o, 0x183739, out + 0x104, 1, i);
      else
        w(m, o, 0x183733, out + 4 + std::uint32_t(node), 1, i);
    }
  }
}
void source_biped_constraint_slots_1_1(M &m, std::uint32_t c,
                                       std::uint32_t index, std::uint32_t first,
                                       std::uint32_t second, const O &o) {
  const auto joint = r(m, r(m, c + 0x7c) + index * 12 + 8),
             a = r(m, joint + 12), b = r(m, joint + 16);
  w(m, o, 0xdd9d8, first, 4, property(m, a, 0x2002));
  if (property(m, b, 0x2001) == 0xffffffffu) {
    w(m, o, 0xdda3a, second, 4, 0xffffffffu);
    return;
  }
  const auto count = si(r(m, b + 0x34));
  if (count > 0) {
    const auto bank = r(m, b + 0x30);
    for (std::int32_t i = 0; i < count; ++i)
      if (r(m, bank + std::uint32_t(i) * 8) == 0x2002) {
        w(m, o, 0xdda33, second, 4, r(m, bank + std::uint32_t(i) * 8 + 4));
        return;
      }
  }
  w(m, o, 0xdda28, second, 4, 0);
}
bool source_biped_ragdoll_budget_1_1(const M &m, bool restricted) {
  const auto pool = r(m, 0x4f01f4);
  std::uint32_t count = 0;
  for (std::uint32_t i = 0; si(i) < si(r(m, pool + 0x38)); ++i)
    if ((r(m, r(m, pool + 0x48) + (i >> 5) * 4) & (1u << (i & 31u))) &&
        !(r(m, r(m, pool + 0x44) + i * r(m, pool + 0x24) + 2, 1) & 2u))
      ++count;
  if (count > 8)
    return false;
  const auto current = si(r(m, r(m, 0x4edf18) + 0x1130));
  if (current >= 3)
    return false;
  return !restricted || current < 2;
}
std::uint8_t source_biped_animation_death_gate_1_1(
    const M &m, std::uint32_t d, bool first, FP fp,
    const SourceBipedAnimationResourceOwner &owner) {
  const auto p = object(m, d), manager = p + std::uint32_t(sx(m, p + 0x12a, 2));
  if (r(m, manager + 0x68) == 0xffffffffu || r(m, manager) == 0xffffffffu ||
      sx(m, manager + 6, 2) == -1)
    return 0;
  const auto def =
      animation_definition(m, r(m, manager), r(m, manager + 4), owner);
  std::int32_t frame = -1;
  for (std::uint32_t i = 0; si(i) < si(r(m, def + 0x4c)); ++i) {
    const auto event = r(m, def + 0x50) + i * 4;
    if (sx(m, event, 2) == (first ? 0 : 1)) {
      frame = sx(m, event + 2, 2);
      break;
    }
  }
  if (frame == -1)
    return 0;
  Scope scope(fp);
  auto phase = _mm_setzero_ps();
  if (r(m, manager) != 0xffffffffu && sx(m, manager + 6, 2) != -1)
    phase = _mm_mul_ss(scalar(r(m, manager + 0x1c)), scalar(r(m, 0x4650dc)));
  return std::uint8_t(
      1 + (rounded(_mm_mul_ss(phase, scalar(r(m, 0x46516c)))) < frame));
}
bool source_biped_recent_contact_1_1(M &m, std::uint32_t p, std::uint32_t body,
                                     std::uint32_t normal, FP fp, const O &o) {
  if (r(m, p + 0x60) == 0xffffffffu)
    return false;
  Scope scope(fp);
  const auto clock = r(m, 0x518444);
  const auto limit =
      rounded(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, clock + 2, 2)),
                         scalar(r(m, 0x4650d8))));
  if (si(r(m, clock + 8) - r(m, p + 0x60)) > limit)
    return false;
  w(m, o, 0x1f0e66, body, 4, r(m, p + 0x64));
  constexpr std::array sites{0x1f0e6du, 0x1f0e72u, 0x1f0e78u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], normal + i * 4, 4, r(m, p + 0x68 + i * 4));
  return true;
}
void source_biped_world_position_1_1(M &m, std::uint32_t d, std::uint32_t out,
                                     FP fp, const O &o) {
  const auto p = object(m, d);
  const auto parent = r(m, p + 0x14);
  if (parent == 0xffffffffu) {
    constexpr std::array sites{0xbace4u, 0xbace9u, 0xbacefu};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, sites[i], out + i * 4, 4, r(m, p + 0x64 + i * 4));
    return;
  }
  Scope scope(fp);
  const auto owner = object(m, parent),
             matrix = owner + std::uint32_t(sx(m, owner + 0x116, 2)) +
                      std::uint32_t(sx(m, p + 0x18, 1)) * 0x34;
  __m128 v[3]{scalar(r(m, p + 0x64)), scalar(r(m, p + 0x68)),
                          scalar(r(m, p + 0x6c))};
  if (f(r(m, matrix)) != f(r(m, 0x465030)))
    for (auto &x : v)
      x = _mm_mul_ss(scalar(r(m, matrix)), x);
  constexpr std::array sites{0xbad7cu, 0xbada8u, 0xbadd5u};
  for (unsigned i = 0; i < 3; ++i) {
    auto x = _mm_add_ss(_mm_mul_ss(scalar(r(m, matrix + 0x1c + i * 4)), v[2]),
                        _mm_mul_ss(scalar(r(m, matrix + 0x10 + i * 4)), v[1]));
    x = _mm_add_ss(x, _mm_mul_ss(scalar(r(m, matrix + 4 + i * 4)), v[0]));
    x = _mm_add_ss(x, scalar(r(m, matrix + 0x28 + i * 4)));
    w(m, o, sites[i], out + i * 4, 4, word(x));
  }
}
std::uint8_t source_place_biped_ragdoll_1_1(
    M &m, std::uint32_t d, std::uint32_t anchor, std::uint32_t height,
    const SourceBipedPlacementScratch &s,
    const SourceBipedPlacementServices &services, const O &o) {
  source_biped_rigid_node_slots_1_1(m, d, s.node_slots, o);
  const auto count = si(r(m, component(m, d) + 0x74));
  w(m, o, 0xe31e3, s.visited_mask, 4, 0);
  // Original E3190 leaves AL=0 on these guard returns.
  if (count < 1 || count > 32 || si(r(m, s.node_slots)) < 1)
    return 0;
  const auto slot = sx(m, s.node_slots + 4, 1);
  if (slot < 0 || slot >= count)
    return 0;
  const auto recursive = s.entry_esp ? s.entry_esp - 0x13c : 0;
  const auto valid = place(services,
      SourceBipedPlacementRequest{d, anchor, height, std::uint32_t(slot),
                                   0xffffffffu, 0xffffffffu, 0}, recursive);
  w(m, o, 0xe3236, s.result_word, 1, valid);
  const auto address = s.visited_mask + std::uint32_t(slot >> 5) * 4;
  w(m, o, 0xe324a, address, 4, r(m, address) | (1u << (slot & 31)));
  traverse(m, d, anchor, height, s, services, std::uint32_t(slot), valid, o,
            recursive);
  return 1;
}
std::uint32_t source_biped_body_extent_1_1(
    M &m, std::uint32_t c, std::uint32_t slot, std::uint32_t esp, FP fp,
    const O &o) {
  const auto body = r(m, r(m, c + 0x70) + slot * 0x60 + 0x40);
  return source_physics_convex_extents_1_1(
      m, r(m, body + 0xc), {esp - 4, esp - 8, esp - 0x54, 0}, fp, o);
}
void source_maintain_biped_ragdoll_1_1(
    M &m, std::uint32_t d, std::uint32_t esp,
    const SourceBipedPlacementScratch &s, FP fp,
    const SourceBipedPlacementServices &services, const O &o) {
  const auto p = object(m, d);
  if (r(m, p + 0x34b, 1) != 1 || r(m, p + 0xb4) == 0xffffffffu)
    return;
  const auto c = component(m, d);
  const auto root = sx(m, c + 0x18, 1);
  if (!(r(m, c + 4) & 0x800u) || root < 0 || root >= si(r(m, c + 0x74)))
    return;
  const auto height = source_biped_body_extent_1_1(
      m, c, std::uint32_t(root), esp - 0x20, fp, o);
  const auto motion = r(m, r(m, r(m, c + 0x70) +
                                  std::uint32_t(root) * 0x60 + 0x40) + 0x3c);
  w(m, o, 0xe3152, esp - 0x10, 4, height);
  // Source loads all three COM words before assigning its anchor packet.
  const std::array xyz{r(m, motion + 0x70), r(m, motion + 0x74),
                       r(m, motion + 0x78)};
  constexpr std::array sites{0xe3167u, 0xe316du, 0xe3173u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], esp - 12 + i * 4, 4, xyz[i]);
  source_place_biped_ragdoll_1_1(m, d, esp - 12, height, s, services, o);
}
void source_maintain_biped_ragdoll_1_1(
    M &m, std::uint32_t d, std::uint32_t esp, FP fp,
    const SourceBipedPlacementServices &services, const O &o) {
  source_maintain_biped_ragdoll_1_1(
      m, d, esp, source_biped_placement_scratch_1_1(esp - 0x28), fp,
      services, o);
}
std::uint8_t source_rebuild_biped_ragdoll_1_1(
    M &m, std::uint32_t d, const SourceBipedRagdollRebuildScratch &scratch,
    FP fp, const SourceBipedRagdollRebuildServices &services, const O &o) {
  Scope scope(fp);
  const auto p = object(m, d),
             capsule =
                 r(m, tag(m, r(m, p)) + 0x288) + r(m, p + 0x3ff, 1) * 0x80,
             c = component(m, d);
  const auto height = r(m, capsule + 0x2c);
  const bool member = r(m, p + 0xc0, 1) & 0x40u;
  if (member)
    need(services.remove_object,
         "Required originalB9730 object world-removal owner unavailable", d);
  source_biped_world_position_1_1(m, d, scratch.anchor, fp, o);
  constexpr std::array sites{0xe2d8cu, 0xe2d9du, 0xe2db0u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], scratch.anchor + i * 4, 4,
      word(_mm_add_ss(scalar(r(m, capsule + 0x70 + i * 4)),
                      scalar(r(m, scratch.anchor + i * 4)))));
  if (r(m, p + 0x348, 2) & 0x200u) {
    const auto current = object(m, d);
    const auto n = std::uint32_t(sx(m, current + 0x114, 2)) / 0x34;
    const auto amount = scalar(r(m, 0x465138));
    w(m, o, 0xe2e02, p + 0x6c, 4,
      word(_mm_add_ss(scalar(r(m, p + 0x6c)), amount)));
    if (si(n) > 0)
      for (std::uint32_t i = 0; i < n; ++i) {
        const auto node = current + std::uint32_t(sx(m, current + 0x116, 2)) +
                          0x30 + i * 0x34;
        w(m, o, 0xe2e18, node, 4, word(_mm_add_ss(scalar(r(m, node)), amount)));
      }
  }
  source_rebuild_object_physics_1_1(m, d, 0, services.physics, o);
  std::uint8_t result = 0;
  if (r(m, c + 4) & 0x800u)
    result = source_place_biped_ragdoll_1_1(
        m, d, scratch.anchor, height, scratch.placement, services.placement, o);
  if (member) {
    const auto current = object(m, d);
    if (r(m, current + 0xb4) != 0xffffffffu)
      need(services.insert_component,
           "Required original1D4F30 component insertion owner unavailable",
           component(m, d));
    w(m, o, 0xe2e97, current + 0xc0, 1, r(m, current + 0xc0, 1) | 0x40u);
  }
  if (result) {
    if (r(m, p + 0x3dc, 1) == 3) {
      w(m, o, 0xe2eb6, p + 0x448, 4, r(m, r(m, 0x518444) + 8));
      source_update_biped_death_constraints_1_1(m, d, scratch.requirements, fp,
                                                o);
    }
    if (d != 0xffffffffu) {
      const auto current = object(m, d);
      if (sx(m, current + 0x112, 2) != -1)
        w(m, o, 0xe2ee5, current + 4, 4, r(m, current + 4) | 0x20000000u);
    }
  }
  return result;
}
std::uint8_t source_rebuild_biped_ragdoll_1_1(
    M &m, std::uint32_t d, std::uint32_t esp, FP fp,
    const SourceBipedRagdollRebuildServices &services, const O &o) {
  return source_rebuild_biped_ragdoll_1_1(
      m, d, source_biped_ragdoll_rebuild_scratch_1_1(esp), fp, services, o);
}
std::uint8_t source_admit_biped_death_1_1(
    M &m, std::uint32_t d, const SourceBipedDeathAdmissionScratch &scratch,
    FP fp, const SourceBipedDeathAdmissionServices &s, const O &o) {
  Scope scope(fp);
  const auto p = object(m, d);
  const auto mode = r(m, p + 0x34b, 1);
  if (mode == 1) {
    if (!r(m, 0x517fdc, 1) && (r(m, p + 0xc1, 1) & 1u)) {
      if (d != 0xffffffffu && sx(m, p + 0x112, 2) != -1)
        w(m, o, 0xe259e, p + 4, 4, r(m, p + 4) | 0x20000000u);
      w(m, o, 0xe25ad, p + 0x34b, 1, 2);
      need(s.set_velocity,
           "Required originalB86C0 biped velocity owner unavailable", d,
           r(m, 0x46fc44));
      need(s.rebuild_controller,
           "Required original1C70D0 biped controller rebuild unavailable", d);
      const auto count = r(m, 0x4edf18) + 0x1130;
      w(m, o, 0xe25cf, count, 4, r(m, count) - 1);
      return 1;
    }
    const auto clock = r(m, 0x518444);
    const auto divisor = rounded(
        _mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, clock + 2, 2)),
                   scalar(r(m, 0x452000))));
    const auto dividend = si(r(m, clock + 8) + (d & 0xffffu));
    if (!divisor ||
        (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1))
      missing("OriginalE2520 IDIV exception owner unavailable");
    if (dividend % divisor == 0)
      need(s.maintain_placement,
           "Required originalE30B0 ragdoll placement maintenance owner "
           "unavailable",
           d);
    source_update_biped_death_constraints_1_1(m, d, scratch.requirements, fp,
                                              o);
    return 1;
  }
  if (mode != 0) {
    if (!(r(m, p + 0xc1, 1) & 1u))
      need(s.recover_controller,
           "Required originalE2F00 controller recovery owner unavailable", d);
    return 0;
  }
  bool admit = false;
  if (r(m, p + 0x3fd, 1) && !r(m, p + 0x3fe, 1))
    admit = !source_biped_airborne_restriction_1_1(m, d, fp) ||
            (r(m, p + 0xc1, 1) & 1u);
  if (!admit)
    admit = source_biped_contact_ragdoll_request_1_1(m, p + 0x3ec, fp);
  if (!admit)
    return 0;
  const auto def = tag(m, r(m, p));
  if (r(m, tag(m, r(m, def + 0x38)) + 0x24) == 0xffffffffu ||
      (r(m, p + 0x348, 2) & 0x400u))
    return 0;
  if (r(m, 0x517fdd, 1)) {
    w(m, o, 0xe2c80, p + 0x349, 1, r(m, p + 0x349, 1) | 1u);
    return 1;
  }
  if (r(m, def + 0x1f0) & 0x800u)
    return 1;
  const bool special =
      (r(m, p + 0x134) & 0x8000000u) && !(r(m, p + 0x348, 2) & 0x200u);
  if (!source_biped_ragdoll_budget_1_1(m, !special)) {
    w(m, o, 0xe2c6c, p + 0x349, 1, r(m, p + 0x349, 1) | 2u);
    return 1;
  }
  const auto first = source_biped_animation_death_gate_1_1(
                 m, d, true, fp, s.animation_resource),
             second = source_biped_animation_death_gate_1_1(
                 m, d, false, fp, s.animation_resource);
  if (!(r(m, p + 0x134) & 0x8000000u) && (first == 2 || second == 2))
    return 1;
  w(m, o, 0xe2742, p + 0x34b, 1, 1);
  need(s.update_activation,
       "Required originalBAA80 biped activation owner unavailable", d);
  if (r(m, 0x517fdc, 1))
    w(m, o, 0xe275c, p + 0xc0, 2, r(m, p + 0xc0, 2) | 0x100u);
  if (!need(s.rebuild_ragdoll,
            "Required originalE2CE0 ragdoll rebuild owner unavailable", d)) {
    w(m, o, 0xe2c55, p + 0x34b, 1, 0);
    w(m, o, 0xe2c5c, p + 0x348, 2, r(m, p + 0x348, 2) | 0x100u);
    return 1;
  }
  const auto count = r(m, 0x4edf18) + 0x1130;
  w(m, o, 0xe2788, count, 4, r(m, count) + 1);
  bool applied = false;
  if (r(m, p + 0xb4) != 0xffffffffu) {
    const auto c = component(m, d);
    if ((r(m, c + 4) & 0x800u) &&
        source_biped_recent_contact_1_1(m, p + 0x3ec, scratch.contact_body,
                                        scratch.contact_normal, fp, o)) {
      if (scratch.contact_normal != scratch.impulse - 0x18u)
        missing("OriginalE2520 contact/delta caller scratch layout required");
      applied = source_biped_contact_velocity_1_1(
          m, d, si(r(m, scratch.contact_body)), scratch.impulse - 0x50u, fp,
          s.impulse, o);
    }
    if (!applied && si(r(m, c + 0x74)) > 0) {
      const auto axis = r(m, 0x46fc5c);
      constexpr std::array sites{0xe2c03u, 0xe2c12u, 0xe2c23u};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, sites[i], scratch.impulse + i * 4, 4,
          word(_mm_mul_ss(scalar(r(m, axis + i * 4)), scalar(r(m, 0x46507c)))));
      auto slot = sx(m, c + 0x18, 1);
      if (slot < 0 || slot >= si(r(m, c + 0x74)))
        slot = -1;
      need(s.impulse,
           "Required original1D4A00 physical-body impulse owner unavailable", c,
           slot, scratch.impulse);
    }
  }
  w(m, o, applied ? 0xe2bd2 : 0xe2c3d, p + 0x349, 1,
    r(m, p + 0x349, 1) & 0xfeu);
  return 1;
}
std::uint8_t source_admit_biped_death_1_1(
    M &m, std::uint32_t d, std::uint32_t entry, FP fp,
    const SourceBipedDeathAdmissionServices &services, const O &observer) {
  return source_admit_biped_death_1_1(
      m, d, source_biped_death_admission_scratch_1_1(entry), fp, services,
      observer);
}
} // namespace halo2
