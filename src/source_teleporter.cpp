#include "halo2/source_teleporter.hpp"
#include "halo2/pickup_visibility.hpp"
#include <bit>

namespace halo2 {
namespace {
float sub(float a, float b) {
  volatile float r = a - b;
  return r;
}
float add(float a, float b) {
  volatile float r = a + b;
  return r;
}
float mul(float a, float b) {
  volatile float r = a * b;
  return r;
}
template <class F, class... A> decltype(auto) owner(const F &f, A &&...a) {
  if (!f)
    throw FormatError("Teleporter requires original dependency owner");
  return f(std::forward<A>(a)...);
}
const NetgameFlag &flag(std::span<const NetgameFlag> f, std::int32_t i) {
  if (i < 0 || std::size_t(i) >= f.size())
    throw FormatError("Invalid original teleporter flag index");
  return f[std::size_t(i)];
}
float distance(Vec3 p, Vec3 q, bool nearest) {
  const auto x = sub(p.x, q.x), y = sub(p.y, q.y), z = sub(p.z, q.z);
  // 1A0680 z*x-order differs from1A0790's z*y-order. Each SSE
  // operation rounds separately; FMA/reassociation changes boundary admission.
  return nearest ? add(add(mul(z, z), mul(y, y)), mul(x, x))
                 : add(add(mul(z, z), mul(x, x)), mul(y, y));
}
#if (defined(__i386__) || defined(__x86_64__)) &&                              \
    (defined(__GNUC__) || defined(__clang__))
struct SourceX87 {
  unsigned short previous{};
  SourceX87() {
    asm volatile("fnstcw %0" : "=m"(previous));
    const unsigned short source = 0x027f;
    asm volatile("fldcw %0" ::"m"(source));
  }
  ~SourceX87() { asm volatile("fldcw %0" ::"m"(previous)); }
};
#endif
} // namespace
void source_teleporter_initialize(SourceTeleporterBank &bank,
                                  std::span<const NetgameFlag> flags) {
  bank.count = 0;
  for (std::int16_t channel = 0; channel < 16; ++channel) {
    SourceTeleporterPair pair;
    for (std::size_t i = 0; i < flags.size(); ++i) {
      const auto &f = flags[i];
      if (f.channel != channel)
        continue; // selector12=-1 is a wildcard.
      if (f.type == 7 && pair.entrance == -1)
        pair.entrance = std::int32_t(i);
      if (f.type == 8 && pair.exit == -1)
        pair.exit = std::int32_t(i);
    }
    bank.pairs.at(std::size_t(bank.count)) = pair;
    if (pair.entrance != -1 && pair.exit != -1)
      ++bank.count;
  }
}
Vec3 source_teleporter_forward(Vec3 v, float entrance, float exit) {
#if (defined(__i386__) || defined(__x86_64__)) &&                              \
    (defined(__GNUC__) || defined(__clang__))
  SourceX87 precision;
  Vec3 out{};
  asm volatile(
      "flds %[y]\n\tflds %[x]\n\tfpatan\n\tfadds %[exit]\n\tfsubs "
      "%[entrance]\n\t"
      "fld %%st(0)\n\tfcos\n\tfstps %[ox]\n\tfsin\n\tfstps %[oy]"
      : [ox] "=m"(out.x), [oy] "=m"(out.y)
      : [x] "m"(v.x), [y] "m"(v.y), [entrance] "m"(entrance), [exit] "m"(exit)
      : "st", "st(1)");
  return pickup_side_normalizer(out).output;
#else
  throw FormatError(
      "Original teleporter x87 heading owner is unavailable on this platform");
#endif
}
std::int32_t source_teleporter_cooldown(std::int16_t tick_rate, float seconds) {
#if (defined(__i386__) || defined(__x86_64__)) &&                              \
    (defined(__GNUC__) || defined(__clang__))
  SourceX87 precision;
  const float product = mul(float(tick_rate), seconds);
  std::int32_t result;
  asm volatile("flds %1\n\tfistpl %0" : "=m"(result) : "m"(product) : "st");
  return result;
#else
  throw FormatError(
      "Original teleporter x87 cooldown owner is unavailable on this platform");
#endif
}
void source_teleporter_step(SourceTeleporterBank &bank,
                            std::span<const NetgameFlag> flags,
                            std::span<SourceTeleporterPlayer> players,
                            const SourceTeleporterContext &context,
                            const SourceTeleporterServices &services) {
  if (bank.count < 0 || bank.count > 16)
    throw FormatError("Invalid original teleporter pair count");
  for (auto &p : players)
    if (p.unit != null_datum)
      p.flags &= 0xff7fu;
  for (std::int32_t n = 0; n < bank.count; ++n) {
    const auto &pair = bank.pairs[std::size_t(n)];
    if (pair.cooldown)
      continue;
    const auto entrance = flag(flags, pair.entrance).position;
    for (auto &p : players)
      if (p.unit != null_datum) {
        const auto root = owner(services.root, p.unit);
        if (owner(services.parent, p.unit) == null_datum &&
            distance(root, entrance, false) < context.entrance_distance_squared)
          p.flags |= 0x80;
      }
  }
  for (auto &p : players)
    if (p.unit != null_datum && !(p.flags & 0x80u))
      p.flags &= 0xfeffu;
  for (std::int32_t n = 0; n < bank.count; ++n) {
    auto &pair = bank.pairs[std::size_t(n)];
    if (pair.cooldown) {
      pair.cooldown =
          std::bit_cast<std::int32_t>(std::uint32_t(pair.cooldown) - 1u);
      continue;
    }
    const auto &entrance = flag(flags, pair.entrance);
    float best = context.entrance_distance_squared;
    SourceTeleporterPlayer *selected = nullptr;
    for (auto &p : players)
      if (p.unit != null_datum) {
        const auto root = owner(services.root, p.unit);
        const auto d = distance(root, entrance.position, true);
        if (owner(services.parent, p.unit) == null_datum && best > d) {
          best = d;
          selected = &p;
        }
      }
    if (!selected || (selected->flags & 0x100u)) {
      pair.blocked_notice = 0;
      continue;
    }
    auto &p = *selected;
    if (owner(services.destination_busy, pair, p.datum)) {
      if (pair.blocked_notice > 0) {
        --pair.blocked_notice;
        continue;
      }
      pair.blocked_notice = std::int32_t(context.tick_rate) * 4;
      if (p.local_controller != -1)
        owner(services.blocked_local_notice, p.datum);
      owner(services.blocked_unit_notice, p.unit);
      continue;
    }
    const auto forward = owner(services.forward, p.unit);
    const auto &exit = flag(flags, pair.exit);
    p.flags |= 0x100;
    owner(services.traversal_event, p.datum);
    const auto heading =
        source_teleporter_forward(forward, entrance.facing, exit.facing);
    owner(services.detach_from_parent, p.unit);
    owner(services.set_position, p.unit, exit.position, heading,
          context.default_up);
    if (p.local_controller != -1)
      owner(services.set_local_view, p.local_controller, heading);
    pair.cooldown =
        source_teleporter_cooldown(context.tick_rate, context.cooldown_seconds);
    pair.blocked_notice = 0;
  }
}
} // namespace halo2
