#pragma once

#include <bit>
#include <cstdint>
#include <emmintrin.h>

namespace halo2::native_pool_pressure_metric {

inline constexpr std::uint32_t slot_count = 16u;
inline constexpr std::uint32_t slot_size = 4u;
inline constexpr std::uint32_t totals_offset = 0xf0384u;
inline constexpr std::uint32_t occupied_offset = 0x44u;
inline constexpr std::uint32_t stride_offset = 0xfff0fd24u;
inline constexpr std::uint32_t scale_address = 0x46a134u;

inline std::uint32_t product(std::uint32_t total, std::uint32_t occupied, std::uint32_t stride) {
    return (total - occupied) * stride;
}

inline __m128 convert(__m128 lanes, std::uint32_t bits) {
    const auto value = std::bit_cast<std::int32_t>(bits);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("cvtsi2ss %1,%0" : "+x"(lanes) : "r"(value) : "memory");
    return lanes;
#else
    return _mm_cvtsi32_ss(lanes,value);
#endif
}

inline __m128 scale(__m128 lanes, std::uint32_t bits) {
    const auto value = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0" : "+x"(lanes) : "x"(value) : "memory");
    return lanes;
#else
    return _mm_mul_ss(lanes,value);
#endif
}

} // namespace halo2::native_pool_pressure_metric
