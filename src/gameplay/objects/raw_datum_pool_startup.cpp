#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Owner {
  const SourceRawDatumPoolServices &s;
  const SourceRawDatumPoolObserver &o;

public:
  Owner(const SourceRawDatumPoolServices &services,
        const SourceRawDatumPoolObserver &observer)
      : s(services), o(observer) {}
  std::uint32_t r(std::uint32_t p, unsigned n = 4) const {
    if (!s.read && !s.memory)
      throw std::runtime_error("Original raw pool startup storage unavailable");
    const std::span<const std::byte> b = s.read ? s.read(p, n) : s.memory(p, n);
    if (b.size() != n)
      throw std::runtime_error("Original raw pool startup extent unavailable");
    std::uint32_t v = 0;
    for (unsigned i = 0; i < n; ++i)
      v |= std::to_integer<unsigned>(b[i]) << (8 * i);
    return v;
  }
  void w(std::uint32_t pc, std::uint32_t p, std::uint32_t value,
         unsigned n = 4) const {
    const auto before = r(p, n);
    if (n < 4)
      value &= (1u << (8 * n)) - 1;
    auto b = s.memory(p, n);
    for (unsigned i = 0; i < n; ++i)
      b[i] = std::byte(value >> (8 * i));
    if (o)
      o({pc, p, before, value, n});
  }
  // Complete326970 bounded copy, including aligned-word, trailing-byte and
  // zero-fill stores. Both pool names and the two-byte salt use this owner.
  void copy(std::uint32_t to, std::uint32_t from, std::uint32_t count) const {
    if (!count)
      return;
    std::uint32_t remaining = count;
    if (from & 3u) {
      do {
        const auto v = r(from++, 1);
        w(0x3269a1, to++, v, 1);
        if (!--count)
          return;
        if (!v) {
          while (to & 3u) {
            w(0x3269e6, to++, 0, 1);
            if (!--count)
              return;
          }
          remaining = count;
          for (std::uint32_t i = 0; i < (count >> 2); ++i) {
            w(0x326a79, to, 0);
            to += 4;
          }
          for (std::uint32_t i = 0; i < (remaining & 3); ++i)
            w(0x326a03, to++, 0, 1);
          return;
        }
      } while (from & 3u);
      remaining = count;
    }
    auto words = count >> 2;
    while (words) {
      const auto value = r(from);
      from += 4;
      unsigned zero = 4;
      for (unsigned i = 0; i < 4; ++i)
        if (!((value >> (8 * i)) & 255u)) {
          zero = i;
          break;
        }
      if (zero == 4) {
        w(0x326a15, to, value);
        to += 4;
        --words;
        continue;
      }
      constexpr std::uint32_t pcs[]{0x326a6b, 0x326a65, 0x326a5b, 0x326a51};
      w(pcs[zero], to, zero ? value & ((1u << (8 * zero)) - 1) : 0);
      to += 4;
      --words;
      while (words) {
        w(0x326a79, to, 0);
        to += 4;
        --words;
      }
      for (std::uint32_t i = 0; i < (remaining & 3); ++i)
        w(0x326a03, to++, 0, 1);
      return;
    }
    auto tail = remaining & 3;
    while (tail) {
      const auto v = r(from++, 1);
      w(0x3269c8, to++, v, 1);
      --tail;
      if (!v) {
        while (tail) {
          w(0x326a03, to++, 0, 1);
          --tail;
        }
        return;
      }
    }
  }
  static std::int32_t signed_word(std::uint32_t v) {
    return std::bit_cast<std::int32_t>(v);
  }
  static std::uint32_t sar5(std::uint32_t v) {
    return (v >> 5) | ((v & 0x80000000u) ? 0xf8000000u : 0);
  }
  void fill(std::uint32_t p, std::uint32_t bytes, std::uint32_t value,
            std::uint32_t word_pc, std::uint32_t byte_pc) const {
    for (std::uint32_t i = 0; i < (bytes >> 2); ++i) {
      w(word_pc, p, value);
      p += 4;
    }
    for (std::uint32_t i = 0; i < (bytes & 3); ++i)
      w(byte_pc, p++, value, 1);
  }
};
} // namespace
void source_raw_datum_header(std::uint32_t pool, std::uint32_t name,
                             std::uint32_t capacity, std::uint32_t stride,
                             std::uint8_t alignment, std::uint32_t context,
                             std::uint32_t bitmap,
                             const SourceRawDatumPoolServices &s,
                             const SourceRawDatumPoolObserver &o) {
  Owner a(s, o);
  for (unsigned i = 0; i < 19; ++i)
    a.w(0x16d0da, pool + 4 * i, 0);
  a.copy(pool, name, 32);
  a.w(0x16d0f1, pool + 0x2a, a.r(pool + 0x2a, 1) | 3u, 1);
  a.w(0x16d0f7, pool + 0x1f, 0, 1);
  a.w(0x16d0fa, pool + 0x20, capacity);
  a.w(0x16d101, pool + 0x24, stride);
  a.w(0x16d108, pool + 0x44, 0);
  a.w(0x16d10b, pool + 0x29, 0, 1);
  a.w(0x16d115, pool + 0x28, alignment, 1);
  a.w(0x16d118, pool + 0x2c, 0x64407440);
  a.w(0x16d11f, pool + 0x48, bitmap);
  a.w(0x16d122, pool + 0x30, context);
}
void source_raw_datum_initialize_inplace(
    std::uint32_t pool, std::uint32_t name, std::uint32_t capacity,
    std::uint32_t stride, std::uint8_t alignment, std::uint32_t context,
    const SourceRawDatumPoolServices &s, const SourceRawDatumPoolObserver &o) {
  const auto mask = (1u << (alignment & 31u)) - 1u,
             elements = (pool + mask + 0x4cu) & ~mask;
  source_raw_datum_header(pool, name, capacity, stride, alignment, context,
                          elements + capacity * stride, s, o);
  Owner a(s, o);
  a.w(0x16d0a0, pool + 0x2a, a.r(pool + 0x2a, 1) & 0xfcu, 1);
  const auto size = Owner::sar5(capacity + 31u) * 4u;
  a.w(0x16d0af, pool + 0x44, elements);
  a.fill(a.r(pool + 0x48), size, 0, 0x16d0ba, 0x16d0c1);
}
void source_raw_datum_reset(std::uint32_t pool,
                            const SourceRawDatumPoolServices &s,
                            const SourceRawDatumPoolObserver &o) {
  Owner a(s, o);
  a.w(0x16d228, pool + 0x38, 0);
  a.w(0x16d22f, pool + 0x3c, 0);
  a.w(0x16d236, pool + 0x34, 0);
  a.copy(pool + 0x40, pool, 2);
  a.w(0x16d245, pool + 0x40, a.r(pool + 0x40, 2) | 0x8000u, 2);
  if (a.r(pool + 0x2a, 1) & 8u)
    a.fill(a.r(pool + 0x44), a.r(pool + 0x24) * a.r(pool + 0x20), 0xbabababa,
           0x16d268, 0x16d26f);
  for (std::uint32_t i = 0;
       Owner::signed_word(i) < Owner::signed_word(a.r(pool + 0x20)); ++i)
    a.w(0x16d289, a.r(pool + 0x44) + i * a.r(pool + 0x24), 0, 2);
  a.fill(a.r(pool + 0x48), Owner::sar5(a.r(pool + 0x20) + 31u) * 4u, 0,
         0x16d2ad, 0x16d2b4);
}
void source_raw_datum_make_valid(std::uint32_t pool,
                                 const SourceRawDatumPoolServices &s,
                                 const SourceRawDatumPoolObserver &o) {
  Owner(s, o).w(0x16d213, pool + 0x29, 1, 1);
  source_raw_datum_reset(pool, s, o);
}
void source_raw_datum_bind_existing(std::uint32_t pool, std::uint32_t elements,
                                    std::uint32_t capacity,
                                    const SourceRawDatumPoolServices &s,
                                    const SourceRawDatumPoolObserver &o) {
  Owner a(s, o);
  a.w(0x16d130, pool + 0x2a, a.r(pool + 0x2a, 1) & 0xfdu, 1);
  a.w(0x16d13f, pool + 0x20, capacity);
  a.w(0x16d142, pool + 0x44, elements);
  a.w(0x16d145, pool + 0x29, 1, 1);
  a.w(0x16d149, pool + 0x38, 0);
  a.w(0x16d14c, pool + 0x3c, 0);
  a.w(0x16d14f, pool + 0x34, capacity);
  a.copy(pool + 0x40, pool, 2);
  a.w(0x16d15a, pool + 0x40, a.r(pool + 0x40, 2) | 0x8000u, 2);
  for (std::uint32_t i = 0;
       Owner::signed_word(i) < Owner::signed_word(a.r(pool + 0x20)); ++i) {
    const auto stride = a.r(pool + 0x24),
               element = a.r(pool + 0x44) + stride * i;
    const bool used = a.r(element, 2) != 0;
    if (!used && (a.r(pool + 0x2a, 1) & 8u)) {
      a.fill(element, stride, 0xbabababa, 0x16d195, 0x16d19c);
      a.w(0x16d19e, element, 0, 2);
    }
    const auto bitmap = a.r(pool + 0x48) + Owner::sar5(i) * 4u,
               mask = 1u << (i & 31u);
    if (used) {
      a.w(0x16d1f7, bitmap, a.r(bitmap) | mask);
      a.w(0x16d1fe, pool + 0x38, i + 1);
      a.w(0x16d201, pool + 0x3c, a.r(pool + 0x3c) + 1);
    } else {
      a.w(0x16d1c0, bitmap, a.r(bitmap) & ~mask);
      if (Owner::signed_word(i) < Owner::signed_word(a.r(pool + 0x34)))
        a.w(0x16d1c7, pool + 0x34, i);
    }
  }
}
} // namespace halo2
