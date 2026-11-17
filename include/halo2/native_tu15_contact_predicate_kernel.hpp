#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <bit>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_contact_predicate_detail {
struct Pair { __m128 first, second; };
struct Dot { __m128 product, y_sum, z_sum; };

inline __m128 product(__m128 left, __m128 right) {
    asm volatile("mulps %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}
inline __m128 difference(__m128 left, __m128 right) {
    asm volatile("subps %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}
inline __m128 sum_low(__m128 left, __m128 right) {
    asm volatile("addss %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}
inline Dot dot(__m128 lanes) {
    const auto y = sum_low(_mm_shuffle_ps(lanes, lanes, 0x55), lanes);
    return {lanes, y, sum_low(_mm_shuffle_ps(lanes, lanes, 0xaa), y)};
}
inline bool even_status(std::uint16_t status, unsigned mask) {
    return std::popcount((unsigned(status) >> 8u) & mask) % 2u == 0;
}

// Actual303800's typed predicate. The view owns real reads, scratch
// publication and floating-point compare boundaries; it cannot replace a
// scratch reload with the value previously supplied to the store.
template<class View> bool reject(View& view) {
    if (!even_status(view.distance_limit(), 0x05u)) return true;
    const auto normals = view.normals();
    const auto agreement = dot(product(normals.first, normals.second));
    if (even_status(view.normal_threshold(agreement), 0x05u)) return true;
    const auto positions = view.positions();
    const auto normal = [&] {
        if constexpr (requires { view.separation_normal(); }) return view.separation_normal();
        else return normals.first;
    }();
    const auto separation = dot(product(difference(positions.second, positions.first), normal));
    return even_status(view.separation_threshold(separation), 0x41u);
}
} // namespace halo2::native_contact_predicate_detail
#endif
