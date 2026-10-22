#pragma once
#include "halo2/physics_body_factory.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2::physics_step_detail {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
inline std::uint32_t r(const M &m, std::uint32_t a, unsigned n = 4) {
    if (n == 0 || n > 4)
        throw std::runtime_error("Invalid original physics scalar read width");
    std::array<std::byte, 4> b{}; m.read(a, std::span(b).first(n));
    std::uint32_t x = 0;
    for (unsigned i = 0; i < n; ++i) x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
    return x;
}
inline std::int32_t si(std::uint32_t x) { return std::bit_cast<std::int32_t>(x); }
inline float f(std::uint32_t x) { return std::bit_cast<float>(x); }
inline std::uint32_t u(float x) { return std::bit_cast<std::uint32_t>(x); }
inline void w(M &m, const O &o, std::uint32_t site, std::uint32_t a,
              unsigned n, std::uint64_t x) {
    // The original store writes its destination; only the optional trace
    // needs its preimage. Keep real write validation in the memory owner.
    // A synthetic read here doubles translated-memory traffic and can also
    // consume a device read that the original instruction never performed.
    if (!o && n <= 8u) {
        std::array<std::byte, 8> written{};
        for (unsigned i = 0; i < n; ++i)
            written[i] = std::byte((x >> (i * 8)) & 255);
        m.write(a, std::span<const std::byte>(written).first(n));
        return;
    }
    SourcePhysicsContactRecordStore s{site, a, {}, {}};
    s.before.resize(n); s.written.resize(n); m.read(a, s.before);
    for (unsigned i = 0; i < n; ++i) s.written[i] = std::byte((x >> (i * 8)) & 255);
    m.write(a, s.written); if (o) o(s);
}
inline __m128 v(const M &m, std::uint32_t a) {
    std::array<std::byte,16> b{};
    m.read_vector(a,b);
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i *>(b.data())));
}
inline void vw(M &m, const O &o, std::uint32_t site, std::uint32_t a, __m128 x) {
    std::array<float, 4> lanes{}; _mm_storeu_ps(lanes.data(), x);
    const auto b = std::bit_cast<std::array<std::uint64_t, 2>>(lanes);
    w(m, o, site, a, 8, b[0]); w(m, o, site, a + 8, 8, b[1]);
}
inline long double ext(float x) { return static_cast<long double>(x); }
inline long double add(long double a, long double b) { volatile long double z = a + b; return z; }
inline long double sub(long double a, long double b) { volatile long double z = a - b; return z; }
inline long double mul(long double a, long double b) { volatile long double z = a * b; return z; }
inline float spill(long double x) { volatile float z = static_cast<float>(x); return z; }
struct FpScope {
    unsigned mx; std::fenv_t env;
    explicit FpScope(FP p) : mx(_mm_getcsr()) {
        if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
            throw std::runtime_error("Unmasked source physics step FP exception owner unavailable");
        if (std::fegetenv(&env) != 0)
            throw std::runtime_error("Original physics step FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" :: "m"(p.x87_control_word) : "memory");
#else
        throw std::runtime_error("Original x87 physics step backend unavailable");
#endif
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope() { std::fesetenv(&env); _mm_setcsr(mx); }
};
} // namespace halo2::physics_step_detail
