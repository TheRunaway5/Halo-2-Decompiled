#include "halo2/native_tu15_object_motion.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using FP = SourcePhysicsFactoryFloatingPoint;

// TU1.5 globals and owners (u5English default.xbe).
constexpr std::uint32_t object_headers = 0x4ecbd0u;   // TU1.1 0x4E78D0
constexpr std::uint32_t components = 0x52be5cu;       // TU1.1 0x526248
constexpr std::uint32_t network_state = 0x4dc04cu;    // B7E50 simulation/network owner
constexpr std::uint32_t game_time = 0x51df54u;        // E65A0 clock (+2 ticks per second)
constexpr std::uint32_t zero_vector_pointer = 0x474d04u; // BC200/1D8C80 zero vector
constexpr std::uint32_t pi_value = 0x556554u, two_pi_value = 0x556558u; // 120150
constexpr std::uint32_t linear_setter = 0x3239f0u, angular_setter = 0x323a00u; // motion +50/+54

std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return x;
}
void w(M &m, std::uint32_t p, std::uint32_t x, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  for (unsigned i = 0; i < n; ++i)
    b[i] = std::byte(x >> (8 * i));
  m.write(p, std::span<const std::byte>(b).first(n));
}
float f(const M &m, std::uint32_t p) { return std::bit_cast<float>(r(m, p)); }
Vec3 v3(const M &m, std::uint32_t p) { return {f(m, p), f(m, p + 4), f(m, p + 8)}; }
void w3(M &m, std::uint32_t p, Vec3 v) {
  w(m, p, std::bit_cast<std::uint32_t>(v.x));
  w(m, p + 4, std::bit_cast<std::uint32_t>(v.y));
  w(m, p + 8, std::bit_cast<std::uint32_t>(v.z));
}
// MOVAPS: two ordered eight-byte stores of x,y,z,0.
void w4(M &m, std::uint32_t p, Vec3 v) {
  std::array<std::byte, 16> b{};
  const std::array<std::uint32_t, 4> lanes{std::bit_cast<std::uint32_t>(v.x), std::bit_cast<std::uint32_t>(v.y),
                                           std::bit_cast<std::uint32_t>(v.z), 0u};
  for (unsigned i = 0; i < 16; ++i)
    b[i] = std::byte(lanes[i / 4] >> (8 * (i % 4)));
  m.write(p, std::span<const std::byte>(b).first(8));
  m.write(p + 8, std::span<const std::byte>(b).subspan(8, 8));
}
std::int32_t sign(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }

// Single-precision SSE operations under the source MXCSR.
float add(float a, float b) { return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float sub(float a, float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float mul(float a, float b) { return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float div(float a, float b) { return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a), _mm_set_ss(b))); }
// COMISS a,b then JA: a > b (unordered is false).
bool above(float a, float b) { return _mm_comigt_ss(_mm_set_ss(a), _mm_set_ss(b)) != 0; }

// The source x87 control word and MXCSR for the duration of one owner.
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error("Native TU1.5 object motion requires the x87 backend");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};
// x87 operations at the source control word's precision and rounding.
long double x87_sqrt(long double x) {
  long double result;
  asm volatile("fsqrt" : "=t"(result) : "0"(x));
  return result;
}
long double x87_add(long double a, long double b) {
  volatile long double r = a + b;
  return r;
}
long double x87_sub(long double a, long double b) {
  volatile long double r = a - b;
  return r;
}
long double x87_mul(long double a, long double b) {
  volatile long double r = a * b;
  return r;
}
long double x87_div(long double a, long double b) {
  volatile long double r = a / b;
  return r;
}
long double x87_atan2(long double y, long double x) {
  long double result;
  asm volatile("fpatan" : "=t"(result) : "0"(x), "u"(y) : "st(1)");
  return result;
}
float x87_store(long double x) {
  volatile float r = static_cast<float>(x);
  return r;
}
std::int32_t x87_fistp(long double x) {
  std::int32_t result;
  asm volatile("fistpl %0" : "=m"(result) : "t"(x) : "st");
  return result;
}

std::uint32_t header(const M &m, std::uint32_t object) { return r(m, r(m, object_headers) + 0x44) + (object & 0xffffu) * 12u; }
std::uint32_t data(const M &m, std::uint32_t object) { return r(m, header(m, object) + 8); }
std::uint32_t type_mask(const M &m, std::uint32_t object) { return 1u << (r(m, header(m, object) + 3, 1) & 31u); }
std::uint32_t component_of(const M &m, std::uint32_t object) {
  const auto datum = r(m, data(m, object) + 0xb4);
  return datum == 0xffffffffu ? 0u : r(m, r(m, components) + 0x44) + (datum & 0xffffu) * 0xa0u;
}
std::uint32_t body(const M &m, std::uint32_t component, std::uint32_t slot) {
  return r(m, r(m, component + 0x70) + slot * 0x60u + 0x40);
}
std::uint32_t motion_type(const M &m, std::uint32_t b) {
  const auto motion = r(m, b + 0x3c), method = r(m, r(m, motion) + 0x18);
  switch (method) { // TU1.5 motion-class +18 owners (TU1.1 in parentheses)
  case 0x31d520u: return 2; // (9D5A0)
  case 0x31e2f0u: return 3; // (320B00)
  case 0x2e8cf0u: return 4; // (9D900)
  case 0x31dd70u: return 5; // (9B560)
  case 0x31edf0u: return 6; // (31B260)
  case 0x31ede0u: return 7; // (31B250)
  default:
    throw std::runtime_error("Native TU1.5 motion class is outside the ported motion owners");
  }
}
// Root slot selected by component +18 (signed, bounded by +74), or -1.
std::int32_t root_slot(const M &m, std::uint32_t component) {
  const auto slot = std::bit_cast<std::int8_t>(std::uint8_t(r(m, component + 0x18, 1)));
  return slot >= 0 && slot < sign(r(m, component + 0x74)) ? slot : -1;
}
bool island_active(const M &m, std::uint32_t b) { // 315C90
  const auto island = r(m, b + 0x44);
  return island && r(m, island + 0x29, 1) != 0;
}

// ---- 315CB0 activation subtree ----
// Unported branches are refused by check_activation before any write.
void check_activation(const M &m, std::uint32_t b) {
  const auto world = r(m, b + 8);
  if (!world)
    return;
  const auto count = sign(r(m, world + 0x10c));
  const auto bank = r(m, world + 0x108);
  for (std::int32_t i = 0; i < count; ++i)
    if (r(m, bank + std::uint32_t(i) * 4u) == b && r(m, b + 6, 2) == 1)
      throw std::runtime_error("Native TU1.5 activation would release a deferred body (283450/destructor)");
  const auto island = r(m, b + 0x44);
  if (!island || r(m, island + 0x29, 1) || r(m, island + 0x24) == 0xffffffffu)
    return;
  if (r(m, world + 0xc) == (r(m, world + 0x10) & 0x7fffffffu))
    throw std::runtime_error("Native TU1.5 island activation would grow the active bank (2E3250)");
  for (std::int32_t i = sign(r(m, world + 0x7c)) - 1; i >= 0; --i)
    if (r(m, r(m, world + 0x78) + std::uint32_t(i) * 4u))
      throw std::runtime_error("Native TU1.5 island activation has a world listener (322660)");
  const auto entities = r(m, island + 0x3c);
  for (std::int32_t e = 0; e < sign(r(m, island + 0x40)); ++e) {
    const auto entity = r(m, entities + std::uint32_t(e) * 4u);
    for (std::int32_t i = sign(r(m, entity + 0x74)) - 1; i >= 0; --i)
      if (r(m, r(m, entity + 0x70) + std::uint32_t(i) * 4u))
        throw std::runtime_error("Native TU1.5 island activation has an entity listener (322660)");
  }
}
// 322660 compaction of null listener entries (no listener is called here).
void compact(M &m, std::uint32_t array_field) {
  for (std::int32_t i = sign(r(m, array_field + 4)) - 1; i >= 0; --i) {
    if (r(m, r(m, array_field) + std::uint32_t(i) * 4u))
      continue;
    const auto count = r(m, array_field + 4) - 1u;
    w(m, array_field + 4, count);
    for (std::int32_t j = i; j < sign(r(m, array_field + 4)); ++j)
      w(m, r(m, array_field) + std::uint32_t(j) * 4u, r(m, r(m, array_field) + std::uint32_t(j + 1) * 4u));
  }
}
void activate(M &m, std::uint32_t b) { // 315CB0
  const auto world = r(m, b + 8);
  if (!world)
    return;
  // 3165D0: remove every deferred reference to this body from the world.
  for (std::int32_t i = 0; i < sign(r(m, world + 0x10c)); ++i) {
    const auto bank = r(m, world + 0x108);
    if (r(m, bank + std::uint32_t(i) * 4u) != b)
      continue;
    std::int32_t found = -1;
    for (std::int32_t j = 0; j < sign(r(m, world + 0x10c)); ++j)
      if (r(m, bank + std::uint32_t(j) * 4u) == b) {
        found = j;
        break;
      }
    const auto last = r(m, world + 0x10c) - 1u;
    w(m, world + 0x10c, last);
    w(m, bank + std::uint32_t(found) * 4u, r(m, bank + last * 4u));
    const auto references = std::uint16_t(r(m, b + 6, 2) - 1u);
    w(m, b + 6, references, 2);
    if (!references)
      throw std::runtime_error("Native TU1.5 activation released a deferred body (283450/destructor)");
  }
  const auto island = r(m, b + 0x44);
  if (r(m, island + 0x29, 1) || r(m, island + 0x24) == 0xffffffffu)
    return;
  // 316D40: move the island from the inactive to the active bank.
  const auto active = world + 8;
  if (r(m, world + 0xc) == (r(m, world + 0x10) & 0x7fffffffu))
    throw std::runtime_error("Native TU1.5 island activation would grow the active bank (2E3250)");
  const auto count = r(m, active + 4);
  w(m, r(m, active) + count * 4u, island);
  w(m, active + 4, count + 1u);
  const auto inactive = r(m, world + 0x14), inactive_count = r(m, world + 0x18);
  const auto moved = r(m, inactive + (inactive_count - 1u) * 4u);
  const auto index = r(m, island + 0x24);
  w(m, inactive + index * 4u, moved);
  w(m, r(m, r(m, world + 0x14) + index * 4u) + 0x24, index);
  w(m, world + 0x18, inactive_count - 1u);
  w(m, island + 0x24, r(m, world + 0xc) - 1u);
  w(m, island + 0x29, 1, 1);
  w(m, island + 0x34, 0);
  w(m, island + 0x38, 0);
  // 322660 with no listeners: only its null-entry compaction remains.
  compact(m, world + 0x78);
  for (std::int32_t e = 0; e < sign(r(m, island + 0x40)); ++e)
    compact(m, r(m, r(m, island + 0x3c) + std::uint32_t(e) * 4u) + 0x70);
  w(m, island + 0x30, 0, 1);
}
void activate_component(M &m, std::uint32_t component) { // 1D9F10
  if (!(r(m, component + 4) & 0x20u))
    return;
  for (std::uint32_t i = 0; sign(i) < sign(r(m, component + 0x74)); ++i)
    activate(m, body(m, component, i));
}

// ---- 1CD6A0 component velocity delivery ----
void deliver(M &m, std::uint32_t component, std::uint32_t slot, const Vec3 &v, bool angular) {
  if (angular && (r(m, component + 4) & 2u)) // 1D9100's static-component gate
    return;
  const auto b = body(m, component, slot);
  if (r(m, b + 0x40, 1))
    return;
  if (!island_active(m, b) && r(m, b + 0x44))
    activate(m, b);
  const auto motion = r(m, b + 0x3c);
  const auto setter = r(m, r(m, motion) + (angular ? 0x54u : 0x50u));
  if (setter != (angular ? angular_setter : linear_setter))
    throw std::runtime_error("Native TU1.5 motion velocity virtual is outside the ported owners");
  w4(m, motion + (angular ? 0x50u : 0x40u), v);
}
template <class F> void each_delivered_body(const M &m, std::uint32_t object, std::uint32_t component, F &&visit) {
  if (type_mask(m, object) & 2u) {
    const auto slot = root_slot(m, component);
    if (slot < 0)
      return;
    const auto b = body(m, component, std::uint32_t(slot));
    if (r(m, b + 0x40, 1) || motion_type(m, b) == 6) // 1D8EB0
      return;
    visit(std::uint32_t(slot), b);
    return;
  }
  for (std::uint32_t i = 0; sign(i) < sign(r(m, component + 0x74)); ++i) {
    const auto b = body(m, component, i);
    if (r(m, b + 0x40, 1) || motion_type(m, b) == 6)
      continue;
    visit(i, b);
  }
}

// ---- B9390 / BDA50 / BE7E0 update propagation ----
void propagate(M &m, std::uint32_t object) { // B9390
  if (r(m, header(m, object) + 2, 1) & 2u)
    return;
  for (auto o = object; o != 0xffffffffu; o = r(m, data(m, o) + 0x14))
    w(m, header(m, o) + 2, r(m, header(m, o) + 2, 1) | 2u, 1);
}
void mark_tree(M &m, std::uint32_t object) { // BE7E0
  if (type_mask(m, object) & 0x1003u)
    propagate(m, object);
  for (auto child = r(m, data(m, object) + 0x10); child != 0xffffffffu; child = r(m, data(m, child) + 0xc))
    mark_tree(m, child);
}
void update_root(M &m, std::uint32_t object) { // BDA50
  if (r(m, header(m, object) + 2, 1) & 0x80u) {
    auto root = 0xffffffffu; // BCFB0: ultimate parent
    for (auto o = object; o != 0xffffffffu; o = r(m, data(m, o) + 0x14))
      root = o;
    object = root;
  }
  if (r(m, header(m, object) + 2, 1) & 1u) {
    propagate(m, object);
    w(m, header(m, object) + 2, r(m, header(m, object) + 2, 1) | 4u, 1);
  }
  mark_tree(m, object);
}
void network_dirty(M &m, std::uint32_t entry, std::uint32_t flags) { // B7E50
  const auto owner = r(m, network_state);
  const auto mode = r(m, owner + 8);
  if (mode != 4) // only mode 4 reaches the mark (4/5 admitted, 3/5 rejected)
    return;
  const auto state = r(m, owner + 4);
  const auto index = entry & 0x3ffu;
  if (!(r(m, r(m, state + 0x20a4) + index * 8u + 0x44, 1) & 4u))
    return;
  const auto at = state + 0x2098u + index * 36u + 0x14u + 0xcu;
  w(m, at, r(m, at) | flags);
}
void wake(M &m, std::uint32_t object) { // BBBC0 with DL=0
  const auto mask = type_mask(m, object);
  const auto o = data(m, object);
  if ((mask & 0x1883u) && ((r(m, o + 0xc0, 1) >> 6) & 1u) && r(m, o + 0xb4) != 0xffffffffu)
    activate_component(m, component_of(m, object));
  w(m, o + 0xc1, r(m, o + 0xc1, 1) & 0xfeu, 1);
  propagate(m, object);
  if ((mask & 0x1cu) || (mask & 0x20u)) {
    const auto entry = r(m, o + 0xd4);
    if (entry != 0xffffffffu)
      network_dirty(m, entry, 0x400u);
  }
}
float squared(Vec3 v) { return add(add(mul(v.x, v.x), mul(v.y, v.y)), mul(v.z, v.z)); }

// ---- D2B60 / D2BD0 rider iterator ----
// Both read the object type from data +AA (not the header byte) and shift by
// it with the x86 five-bit count.
bool unit_type(const M &m, std::uint32_t d) { return ((1u << (r(m, d + 0xaa, 1) & 31u)) & 3u) != 0; }
bool rides(const M &m, std::uint32_t d) { return ((r(m, d + 4) >> 26) & 1u) != 0; }
struct Riders {
  std::uint32_t root{}, current{}, next{};
};
Riders first_rider(const M &m, std::uint32_t object) { // D2B60
  auto o = object;
  for (;;) {
    const auto d = data(m, o);
    const auto parent = r(m, d + 0x14);
    if (parent == 0xffffffffu || !rides(m, d) || !unit_type(m, data(m, parent)))
      break;
    o = parent;
  }
  return {o, o, r(m, data(m, o) + 0x10)};
}
// The next unit child of the sibling list at `it.next` (advancing it), if any.
std::optional<std::uint32_t> next_unit_sibling(const M &m, Riders &it) {
  while (it.next != 0xffffffffu) {
    const auto child = it.next;
    const auto d = data(m, child);
    it.next = r(m, d + 0xc);
    if (unit_type(m, d))
      return child;
  }
  return std::nullopt;
}
// From `first` along its sibling list: the first unit with +4 bit 26.
std::uint32_t descend(const M &m, std::uint32_t first) {
  auto o = first;
  while (o != 0xffffffffu) {
    const auto d = data(m, o);
    if (unit_type(m, d) && rides(m, d))
      break;
    o = r(m, d + 0xc);
  }
  return o;
}
std::optional<std::uint32_t> next_rider(const M &m, Riders &it) { // D2BD0
  if (it.root == it.current) {
    if (const auto unit = next_unit_sibling(m, it))
      return unit;
    it.current = descend(m, r(m, data(m, it.root) + 0x10));
    if (it.current != 0xffffffffu)
      it.next = r(m, data(m, it.current) + 0x10);
  }
  if (it.root == it.current || it.current == 0xffffffffu)
    return std::nullopt;
  for (;;) {
    if (const auto unit = next_unit_sibling(m, it))
      return unit;
    it.current = descend(m, r(m, data(m, it.current) + 0xc));
    if (it.current == 0xffffffffu)
      return std::nullopt;
    it.next = r(m, data(m, it.current) + 0x10);
  }
}

// ---- transform math (BC190 / 144BF0 / 144090 / 145560 / 144A60 / 120150) ----
NativeTu15Transform basis(Vec3 position, Vec3 forward, Vec3 up) { // 144BF0
  NativeTu15Transform t;
  t.scale = 1;
  t.forward = forward;
  t.left = {sub(mul(forward.z, up.y), mul(up.z, forward.y)), sub(mul(up.z, forward.x), mul(forward.z, up.x)),
            sub(mul(forward.y, up.x), mul(forward.x, up.y))};
  t.up = up;
  t.position = position;
  return t;
}
NativeTu15Transform product(const NativeTu15Transform &a, const NativeTu15Transform &b) { // 145560
  const auto combine = [&](Vec3 v) {
    return Vec3{add(mul(v.x, a.forward.x), add(mul(v.y, a.left.x), mul(v.z, a.up.x))),
                add(mul(v.x, a.forward.y), add(mul(v.y, a.left.y), mul(v.z, a.up.y))),
                add(mul(v.x, a.forward.z), add(mul(v.y, a.left.z), mul(v.z, a.up.z)))};
  };
  NativeTu15Transform out;
  const auto &p = b.position;
  const auto lane = [&](float fx, float lx, float ux, float px) {
    return add(mul(add(add(mul(p.x, fx), mul(p.y, lx)), mul(p.z, ux)), a.scale), px);
  };
  out.position = {lane(a.forward.x, a.left.x, a.up.x, a.position.x), lane(a.forward.y, a.left.y, a.up.y, a.position.y),
                  lane(a.forward.z, a.left.z, a.up.z, a.position.z)};
  out.up = combine(b.up);
  out.left = combine(b.left);
  out.forward = combine(b.forward);
  out.scale = mul(b.scale, a.scale);
  return out;
}
NativeTu15Transform inverse(const NativeTu15Transform &in) { // 144090
  NativeTu15Transform out;
  if (in.scale == 0.f) { // UCOMISS/LAHF/TEST 44/JNP: ordered equality
    out.scale = 0;
    out.forward = out.left = out.up = out.position = {};
    return out;
  }
  float nx = sub(0.f, in.position.x), ny = sub(0.f, in.position.y), nz = sub(0.f, in.position.z);
  if (in.scale == 1.f) {
    out.scale = 1.f;
  } else {
    const float k = div(1.f, in.scale);
    out.scale = k;
    nx = mul(k, nx);
    ny = mul(k, ny);
    nz = mul(k, nz);
  }
  out.forward = {in.forward.x, in.left.x, in.up.x};
  out.left = {in.forward.y, in.left.y, in.up.y};
  out.up = {in.forward.z, in.left.z, in.up.z};
  out.position = {add(add(mul(out.up.x, nz), mul(out.forward.x, nx)), mul(out.left.x, ny)),
                  add(add(mul(out.up.y, nz), mul(out.forward.y, nx)), mul(ny, out.left.y)),
                  add(add(mul(out.forward.z, nx), mul(out.up.z, nz)), mul(out.left.z, ny))};
  return out;
}
std::array<float, 4> quaternion(const NativeTu15Transform &t) { // 144A60 over forward/left/up rows
  const std::array<float, 9> m{t.forward.x, t.forward.y, t.forward.z, t.left.x, t.left.y,
                               t.left.z,    t.up.x,      t.up.y,      t.up.z};
  std::array<float, 4> q{};
  const float trace = add(add(m[4], m[0]), m[8]);
  if (above(trace, 0.f)) {
    const auto s = x87_mul(x87_sqrt(x87_add(trace, 1.f)), .5f);
    const float stored = x87_store(s);
    const float c = div(.25f, stored);
    q[3] = stored;
    q[0] = mul(sub(m[5], m[7]), c);
    q[1] = mul(sub(m[6], m[2]), c);
    q[2] = mul(sub(m[1], m[3]), c);
  } else {
    int i = above(m[4], m[0]) ? 1 : 0;
    if (above(m[8], m[i * 4]))
      i = 2;
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    const auto s = x87_mul(x87_sqrt(x87_add(x87_sub(x87_sub(m[i * 4], m[j * 4]), m[k * 4]), 1.f)), .5f);
    const float stored = x87_store(s);
    const float c = div(.25f, stored);
    q[i] = stored;
    q[3] = mul(sub(m[j * 3 + k], m[k * 3 + j]), c);
    q[j] = mul(add(m[i * 3 + j], m[j * 3 + i]), c);
    q[k] = mul(add(m[i * 3 + k], m[k * 3 + i]), c);
  }
  if (above(0.f, q[3]))
    for (auto *c : {&q[3], &q[0], &q[1], &q[2]})
      *c = sub(0.f, *c);
  return q;
}
float normalize(Vec3 &v) { // 30BD0
  const long double x = v.x, y = v.y, z = v.z;
  const auto length = x87_sqrt(x87_add(x87_add(x87_mul(x, x), x87_mul(y, y)), x87_mul(z, z)));
  const float stored = x87_store(length);
  // FCOMIP: retained 0.0001 above |length| (unordered falls through).
  if (static_cast<long double>(std::bit_cast<float>(0x38d1b717u)) > std::fabs(length))
    return 0.f;
  const float k = div(1.f, stored);
  v.x = mul(v.x, k);
  v.y = mul(k, v.y);
  v.z = mul(k, v.z);
  return stored;
}
} // namespace

NativeTu15Transform native_tu15_object_transform(const M &m, std::uint32_t object) {
  const auto o = data(m, object);
  auto t = basis(v3(m, o + 0x64), v3(m, o + 0x70), v3(m, o + 0x7c));
  const auto parent = r(m, o + 0x14);
  if (parent != 0xffffffffu) {
    const auto p = data(m, parent);
    const auto node = p + std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(r(m, o + 0x18, 1)))) * 0x34) +
                      std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(m, p + 0x116, 2)))));
    NativeTu15Transform a;
    a.scale = f(m, node);
    a.forward = v3(m, node + 4);
    a.left = v3(m, node + 0x10);
    a.up = v3(m, node + 0x1c);
    a.position = v3(m, node + 0x28);
    t = product(a, t);
  }
  return t;
}

NativeTu15Velocities native_tu15_object_velocities(const M &m, std::uint32_t object) {
  auto root = object;
  for (auto o = object; o != 0xffffffffu; o = r(m, data(m, o) + 0x14))
    root = o;
  const auto o = data(m, root);
  const auto component = component_of(m, root);
  const std::int32_t slot = component ? root_slot(m, component) : -1;
  if (slot < 0)
    return {v3(m, o + 0x88), v3(m, o + 0x94)};
  const auto zero = v3(m, r(m, zero_vector_pointer));
  if (r(m, o + 0xc1, 1) & 1u)
    return {zero, zero};
  const auto b = body(m, component, std::uint32_t(slot));
  if (r(m, b + 0x40, 1))
    return {zero, zero};
  const auto motion = r(m, b + 0x3c);
  return {v3(m, motion + 0x40), v3(m, motion + 0x50)};
}

bool native_tu15_motion_held(const M &m, std::uint32_t object, FP fp) {
  if (r(m, header(m, object) + 3, 1))
    return false; // not a biped
  const auto o = data(m, object);
  const auto restricted = [&] { // E65A0
    if (r(m, o + 0xc1, 1) & 1u)
      return false;
    if (!(r(m, o + 0x348, 1) & 1u))
      return false;
    Scope scope(fp);
    const auto rate = std::bit_cast<std::int16_t>(std::uint16_t(r(m, r(m, game_time) + 2, 2)));
    const float scaled = mul(static_cast<float>(rate), f(m, 0x46a458u));
    const auto ticks = x87_fistp(scaled);
    if (std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(r(m, o + 0x399, 1)))) < ticks)
      return false;
    const auto state = r(m, o + 0x3dc, 1);
    if (state != 1 && state != 3)
      return false;
    return r(m, o + 0x14) == 0xffffffffu;
  };
  const auto animated = [&] { // CE8F0
    const auto at = o + std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(m, o + 0x12a, 2)))));
    if (r(m, at + 0x68) == 0xffffffffu || r(m, at) == 0xffffffffu || r(m, at + 6, 2) == 0xffffu)
      return false;
    const auto id = r(m, at + 0x7c);
    return id == 0xd000042u || id == 0xc000043u;
  };
  if ((r(m, o + 0x10a, 1) >> 2) & 1u)
    if (!restricted())
      return true;
  if (animated())
    return true;
  return ((r(m, o + 0x134) >> 27) & 1u) != 0;
}

NativeTu15Velocities native_tu15_nudge_correction(const M &m, std::uint32_t object, Vec3 position, Vec3 forward,
                                                  Vec3 up, Vec3 linear, Vec3 angular, FP fp) {
  Scope scope(fp);
  const auto current = native_tu15_object_transform(m, object);
  const auto desired = basis(position, forward, up);
  const auto difference = product(desired, inverse(current));
  const auto q = quaternion(difference);
  // 120150: axis/angle, with the angle folded at the retained pi.
  Vec3 axis{q[0], q[1], q[2]};
  const float length = normalize(axis);
  const auto doubled = [&] {
    const auto half = x87_atan2(length, q[3]);
    return x87_add(half, half);
  }();
  float angle = x87_store(doubled);
  if (doubled > static_cast<long double>(f(m, pi_value))) {
    axis = {sub(0.f, axis.x), sub(0.f, axis.y), sub(0.f, axis.z)};
    angle = sub(f(m, two_pi_value), angle);
  }
  // Angular: axis * angle * 5, capped to length 1 (x87 1/sqrt).
  const float scale = mul(angle, 5.f);
  Vec3 a{mul(axis.x, scale), mul(axis.y, scale), mul(axis.z, scale)};
  const float as = add(add(mul(a.z, a.z), mul(a.y, a.y)), mul(a.x, a.x));
  if (above(as, 1.f)) {
    const auto k = x87_div(1.f, x87_sqrt(as));
    a = {x87_store(x87_mul(a.x, k)), x87_store(x87_mul(a.y, k)), x87_store(x87_mul(a.z, k))};
  }
  // Linear: 0.9 * dpos, capped to length 0.05.
  const float g = std::bit_cast<float>(0x3f666666u);
  Vec3 l{mul(difference.position.x, g), mul(difference.position.y, g), mul(difference.position.z, g)};
  const float ls = add(add(mul(l.z, l.z), mul(l.y, l.y)), mul(l.x, l.x));
  if (above(ls, std::bit_cast<float>(0x3b23d70bu))) {
    const auto k = x87_div(std::bit_cast<float>(0x3d4ccccdu), x87_sqrt(ls));
    l = {x87_store(x87_mul(l.x, k)), x87_store(x87_mul(l.y, k)), x87_store(x87_mul(l.z, k))};
  }
  return {{add(linear.x, l.x), add(linear.y, l.y), add(linear.z, l.z)},
          {add(angular.x, a.x), add(angular.y, a.y), add(angular.z, a.z)}};
}

std::string native_tu15_object_velocities_unsupported(const M &m, std::uint32_t object, const Vec3 *linear,
                                                     const Vec3 *angular) {
  try {
    const auto component = component_of(m, object);
    if (!component)
      return {};
    each_delivered_body(m, object, component, [&](std::uint32_t, std::uint32_t b) {
      if (!island_active(m, b) && r(m, b + 0x44))
        check_activation(m, b);
      const auto motion = r(m, b + 0x3c);
      if (linear && r(m, r(m, motion) + 0x50) != linear_setter)
        throw std::runtime_error("Native TU1.5 motion velocity virtual is outside the ported owners");
      if (angular && !(r(m, component + 4) & 2u) && r(m, r(m, motion) + 0x54) != angular_setter)
        throw std::runtime_error("Native TU1.5 motion velocity virtual is outside the ported owners");
    });
    if (r(m, component + 4) & 0x20u)
      for (std::uint32_t i = 0; sign(i) < sign(r(m, component + 0x74)); ++i)
        check_activation(m, body(m, component, i));
  } catch (const std::exception &error) {
    return error.what();
  }
  return {};
}

void native_tu15_set_object_velocities(M &m, std::uint32_t object, const Vec3 *linear, const Vec3 *angular,
                                       FP fp) {
  Scope scope(fp);
  const auto o = data(m, object);
  const auto component = component_of(m, object);
  const bool wakes = (linear && above(squared(*linear), std::bit_cast<float>(0x38d1b717u))) ||
                     (angular && above(squared(*angular), std::bit_cast<float>(0x38d1b717u)));
  // Refuse unported owners before any store.
  if (const auto reason = native_tu15_object_velocities_unsupported(m, object, linear, angular); !reason.empty())
    throw std::runtime_error(reason);
  // B9770.
  std::uint32_t flags = 0;
  if (linear) {
    w3(m, o + 0x88, *linear);
    flags = 0x10;
  }
  if (angular) {
    w3(m, o + 0x94, *angular);
    flags |= 0x20;
  }
  if (flags && r(m, o + 0xd4) != 0xffffffffu)
    network_dirty(m, r(m, o + 0xd4), flags);
  // 1CD6A0 with force=1.
  if (component) {
    each_delivered_body(m, object, component, [&](std::uint32_t slot, std::uint32_t) {
      if (linear)
        deliver(m, component, slot, *linear, false);
      if (angular)
        deliver(m, component, slot, *angular, true);
    });
    activate_component(m, component);
  }
  if (wakes) {
    wake(m, object);
    propagate(m, object);
    update_root(m, object);
  }
}

std::vector<std::uint32_t> native_tu15_vehicle_riders(const M &m, std::uint32_t object) {
  std::vector<std::uint32_t> riders;
  auto it = first_rider(m, object);
  while (const auto unit = next_rider(m, it))
    riders.push_back(*unit);
  return riders;
}

void native_tu15_vehicle_child_update(M &m, std::uint32_t object, const Vec3 *position, const Vec3 *linear, FP fp) {
  if (r(m, header(m, object) + 3, 1) != 1)
    return; // B4C5C: vehicles only
  Scope scope(fp);
  const auto o = data(m, object);
  const float seconds = f(m, r(m, game_time) + 4);
  const auto accumulate = [&](std::uint32_t at, Vec3 d) { // MOVSS/ADDSS/MOVSS per lane, x first
    w(m, at, std::bit_cast<std::uint32_t>(add(f(m, at), d.x)));
    w(m, at + 4, std::bit_cast<std::uint32_t>(add(f(m, at + 4), d.y)));
    w(m, at + 8, std::bit_cast<std::uint32_t>(add(f(m, at + 8), d.z)));
  };
  if (position)
    accumulate(o + 0x270, {sub(f(m, o + 0x64), position->x), sub(f(m, o + 0x68), position->y),
                           sub(f(m, o + 0x6c), position->z)});
  Vec3 change;
  if (linear)
    change = {mul(sub(linear->x, f(m, o + 0x88)), seconds), mul(sub(linear->y, f(m, o + 0x8c)), seconds),
              mul(sub(linear->z, f(m, o + 0x90)), seconds)};
  auto it = first_rider(m, object);
  while (const auto unit = next_rider(m, it)) {
    const auto u = data(m, *unit);
    if (linear)
      accumulate(u + 0x270, change);
    w(m, u + 0x134, r(m, u + 0x134) | 0x10000000u);
  }
}

bool native_tu15_nudge_object(M &m, std::uint32_t object, std::uint32_t mode, const Vec3 *position,
                              const Vec3 *forward, const Vec3 *up, const Vec3 *linear, const Vec3 *angular, FP fp) {
  bool take_linear = linear != nullptr, take_angular = angular != nullptr;
  if (native_tu15_motion_held(m, object, fp) || (!take_linear && !take_angular))
    return false;
  const auto current = native_tu15_object_transform(m, object);
  const auto pose_position = position ? *position : current.position;
  const auto pose_forward = forward ? *forward : current.forward;
  const auto pose_up = up ? *up : current.up;
  auto velocities = native_tu15_object_velocities(m, object);
  {
    Scope scope(fp);
    if (take_linear) {
      const Vec3 d{sub(linear->x, velocities.linear.x), sub(linear->y, velocities.linear.y),
                   sub(linear->z, velocities.linear.z)};
      if (mode == 2 && !above(add(add(mul(d.z, d.z), mul(d.x, d.x)), mul(d.y, d.y)), std::bit_cast<float>(0x3d75c28fu)))
        take_linear = false;
      else
        velocities.linear = *linear;
    }
    if (take_angular) {
      const Vec3 d{sub(angular->x, velocities.angular.x), sub(angular->y, velocities.angular.y),
                   sub(angular->z, velocities.angular.z)};
      if (mode == 2 && !above(add(add(mul(d.z, d.z), mul(d.y, d.y)), mul(d.x, d.x)), std::bit_cast<float>(0x3c23d70au)))
        take_angular = false;
      else
        velocities.angular = *angular;
    }
  }
  const auto out = native_tu15_nudge_correction(m, object, pose_position, pose_forward, pose_up, velocities.linear,
                                                velocities.angular, fp);
  if (!take_linear && !take_angular)
    return false;
  const Vec3 *linear_out = take_linear ? &out.linear : nullptr, *angular_out = take_angular ? &out.angular : nullptr;
  // Refuse an unported B9800 owner before B4C40 writes anything.
  if (const auto reason = native_tu15_object_velocities_unsupported(m, object, linear_out, angular_out);
      !reason.empty())
    throw std::runtime_error(reason);
  if (take_linear) // B46BD: B4C40 with no position and the corrected linear velocity
    native_tu15_vehicle_child_update(m, object, nullptr, linear_out, fp);
  native_tu15_set_object_velocities(m, object, linear_out, angular_out, fp);
  return true;
}
} // namespace halo2
