#include "halo2/unit_local_transition.hpp"
#include "halo2/unit_action_dispatch.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated local transition backing");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::int32_t signed8(std::uint32_t v) {
  return v < 128 ? std::int32_t(v) : std::int32_t(v) - 256;
}
std::int32_t signed16(std::uint32_t v) {
  return v < 32768 ? std::int32_t(v) : std::int32_t(v) - 65536;
}
float fp(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t bits(float v) { return std::bit_cast<std::uint32_t>(v); }
void write(std::span<std::byte> b, std::size_t at, unsigned n,
           std::uint32_t value, std::uint32_t ip, UnitLocalTransitionBuffer k,
           const UnitLocalTransitionObserver &o) {
  auto old = read(b, at, n);
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((value >> (8 * i)) & 255);
  if (o)
    o({ip, k, at, n, old, read(b, at, n)});
}
SourceLocalTransitionClock clock(const SourceLocalTransitionClockProvider &p) {
  require(bool(p), "Missing original local transition clock/context owner");
  auto c = p();
  // Source SSE rounding/flush modes other than these actual captured modes
  // remain explicit unsupported context, rather than ambient host defaults.
  require((c.mxcsr & 0xe040u) == 0 && (c.mxcsr & 0x1f80u) == 0x1f80u,
          "Unsupported source SSE rounding/flush/exception context");
  require(std::fegetround() == FE_TONEAREST,
          "Unsupported host SSE rounding context");
#if defined(__i386__) || defined(__x86_64__)
  const auto host_mxcsr = _mm_getcsr();
  require((host_mxcsr & 0xe040u) == 0 && (host_mxcsr & 0x1f80u) == 0x1f80u,
          "Unsupported host SSE flush/exception context");
#else
  throw std::runtime_error("Unverified host binary32 instruction context");
#endif
  return c;
}
float mul(float a, float b) {
  volatile float v = a * b;
  return v;
}
float div(float a, float b) {
  volatile float v = a / b;
  return v;
}
std::int32_t integer(long double value) {
  if (!std::isfinite(value) ||
      value < std::numeric_limits<std::int32_t>::min() ||
      value > std::numeric_limits<std::int32_t>::max())
    return std::numeric_limits<std::int32_t>::min();
  return static_cast<std::int32_t>(value);
}
std::int32_t fistp(float value, std::uint16_t fpcw) {
  require((fpcw & 0x3f) == 0x3f,
          "Unmasked source x87 conversion exceptions require an owner");
  long double v = value, rounded;
  switch ((fpcw >> 10) & 3) {
  case 0: {
    auto lo = std::floor(v), part = v - lo;
    rounded = part < 0.5L                ? lo
              : part > 0.5L              ? lo + 1
              : std::fmod(lo, 2.0L) == 0 ? lo
                                         : lo + 1;
    break;
  }
  case 1:
    rounded = std::floor(v);
    break;
  case 2:
    rounded = std::ceil(v);
    break;
  default:
    rounded = std::trunc(v);
    break;
  }
  return integer(rounded);
}
std::span<std::byte> body(std::uint32_t unit,
                          const UnitConstructorObjectPool &p) {
  auto i = unit & 65535u;
  require(i < p.capacity && bool(p.body), "Missing original transition object");
  auto at = std::size_t(i) * 12;
  require(at <= p.headers.size() && 12 <= p.headers.size() - at,
          "Truncated transition header pool");
  auto b = p.body(std::uint16_t(i));
  auto n = read(p.headers, at + 6, 2);
  require(n <= b.size(), "Truncated retained transition body");
  return b.first(n);
}
std::size_t span_offset(std::span<const std::byte> b, std::uint32_t raw,
                        std::size_t extent) {
  auto offset = signed16(raw);
  require(offset >= 0 && std::size_t(offset) <= b.size() &&
              extent <= b.size() - std::size_t(offset),
          "Missing source-owned transition bank");
  return std::size_t(offset);
}
} // namespace

std::uint32_t source_transition_remaining_seconds(
    std::span<const std::byte> c, const SourceLocalTransitionClockProvider &p) {
  require(c.size() >= 4, "Missing original transition counter");
  auto duration = read(c, 1, 1);
  if (!duration)
    return 0;
  auto current = signed8(read(c, 0, 1));
  auto ticks = (read(c, 3, 1) & 1) ? signed8(duration) - current : current;
  auto time = clock(p);
  return bits(mul(float(ticks), fp(read(time.record, 4))));
}
void start_source_transition_counter(
    std::span<std::byte> c, std::uint32_t seconds, std::uint8_t direction,
    std::uint32_t one, const SourceLocalTransitionClockProvider &p,
    const UnitLocalTransitionObserver &o) {
  require(c.size() >= 4, "Missing original transition counter");
  auto time = clock(p);
  auto ticks = fistp(mul(float(signed16(read(time.record, 2, 2))), fp(seconds)),
                     time.fpcw);
  ticks = ticks < 1 ? 1 : ticks > 127 ? 127 : ticks;
  auto duration = ticks - 1;
  auto put = [&](std::size_t at, std::uint32_t v, std::uint32_t ip) {
    write(c, at, 1, v, ip, UnitLocalTransitionBuffer::Counter, o);
  };
  if (duration <= 0) {
    put(1, 0, 0x1dccff);
    put(0, 0, 0x1dcd03);
    put(3, 0, 0x1dcd06);
    return;
  }
  auto previous = read(c, 1, 1);
  if (previous && direction == (read(c, 3, 1) & 1) &&
      duration == signed8(previous))
    return;
  if (previous) {
    auto denominator = float(signed8(previous));
    if (!(denominator > fp(one)))
      denominator = fp(one);
    auto rescaled = mul(div(float(signed8(read(c, 0, 1))), denominator),
                        float(duration - 1));
    auto current = integer(std::trunc(static_cast<long double>(rescaled)));
    put(1, std::uint32_t(duration), 0x1dccd3);
    put(0, std::uint32_t(current), 0x1dccd9);
    put(3, direction != 0, 0x1dccdb);
  } else {
    put(1, std::uint32_t(duration), 0x1dccf0);
    put(0, direction ? 1 : std::uint32_t(duration - 1), 0x1dccf6);
    put(3, direction != 0, 0x1dccf8);
  }
}
bool save_source_unit_local_transition(
    std::span<std::byte> m, std::int32_t nodes, std::uint32_t seconds,
    std::span<std::byte> saved, std::span<const std::byte> current,
    const SourceUnitLocalTransitionServices &s,
    const UnitLocalTransitionObserver &o) {
  if (nodes <= 0 || !(fp(seconds) > fp(s.zero_bits)))
    return false;
  require(m.size() >= 0x90, "Missing retained local transition manager");
  auto c = m.subspan(0x64, 4);
  if (read(c, 1, 1) && !(read(c, 3, 1) & 2)) {
    auto remaining = source_transition_remaining_seconds(c, s.clock);
    if (!(fp(seconds) >= fp(remaining)))
      return false; // Original JB rejects unordered too.
  }
  auto n = std::uint64_t(std::uint32_t(nodes)) * 32;
  require(n <= saved.size() && n <= current.size(),
          "Truncated source current/saved local bank");
  write(m, 0x65, 1, 0, 0x1cf02d, UnitLocalTransitionBuffer::Manager, o);
  write(m, 0x64, 1, 0, 0x1cf030, UnitLocalTransitionBuffer::Manager, o);
  write(m, 0x67, 1, 0, 0x1cf033, UnitLocalTransitionBuffer::Manager, o);
  // Original forward REP MOVSD semantics, including any actual overlap.
  for (std::size_t i = 0; i < n; i += 4)
    write(saved, i, 4, read(current, i), 0x1cf036,
          UnitLocalTransitionBuffer::Saved, o);
  auto counter_observer = [&](const UnitLocalTransitionStore &v) {
    if (o)
      o({v.instruction, UnitLocalTransitionBuffer::Manager, 0x64 + v.offset,
         v.bytes, v.before, v.after});
  };
  start_source_transition_counter(c, seconds, 1, s.one_bits, s.clock,
                                  counter_observer);
  return true;
}
void capture_source_object_transition(
    std::uint32_t unit, std::uint32_t seconds,
    const SourceUnitLocalTransitionServices &s,
    const UnitLocalTransitionObserver &o) {
  auto b = body(unit, s.objects);
  if (read(b, 0x112, 2) == 65535)
    return;
  require(bool(s.tags.address) && bool(s.tags.read),
          "Missing original transition definition owner");
  auto d = s.tags.address(read(b, 0));
  require(d <= none - 0x38, "Escaped transition definition address");
  auto field = s.tags.read(d + 0x38, 4);
  require(field.size() == 4, "Truncated original transition definition");
  if (read(field, 0) == none)
    return;
  // Original MOVSX then logical SHR, not a fabricated node count or extent.
  auto signed_bytes = signed16(read(b, 0x110, 2));
  auto nodes = std::bit_cast<std::uint32_t>(signed_bytes) >> 5;
  auto n = std::uint64_t(nodes) * 32;
  require(n <= b.size(), "Unsupported escaped transition local extent");
  auto m = span_offset(b, read(b, 0x12a, 2), 0x90),
       saved = span_offset(b, read(b, 0x10e, 2), std::size_t(n)),
       current = span_offset(b, read(b, 0x112, 2), std::size_t(n));
  auto observer = [&](const UnitLocalTransitionStore &v) {
    if (!o)
      return;
    auto at = v.buffer == UnitLocalTransitionBuffer::Manager ? m + v.offset
                                                             : saved + v.offset;
    o({v.instruction, UnitLocalTransitionBuffer::Body, at, v.bytes, v.before,
       v.after});
  };
  (void)save_source_unit_local_transition(
      b.subspan(m, 0x90), std::int32_t(nodes), seconds,
      b.subspan(saved, std::size_t(n)), b.subspan(current, std::size_t(n)), s,
      observer);
  mark_source_object_for_update(unit, s.objects, [&](const UnitActionStore &v) {
    if (o)
      o({v.instruction, UnitLocalTransitionBuffer::Headers,
         std::size_t(v.identity & 65535) * 12 + v.offset, v.bytes, v.before,
         v.after});
  });
}
} // namespace halo2
