#pragma once
#include <cstdint>
#include <cstdlib>
#include <xmmintrin.h>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_contact_predicate_source_frame.hpp"
#endif

namespace halo2::native_contact_predicate_owner_detail {
struct Activity {
    std::uint64_t completed{}, continuations{};
    std::uint32_t last_next{};
};
inline Activity& activity() {
    static thread_local Activity value{};
    return value;
}
inline bool force_reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_CONTACT_PREDICATE");
        return value && *value;
    }();
    return enabled;
}
template<bool Literal, class State> std::uint32_t bridge(State& s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (force_reference()) return 0u;
    using native_contact_predicate_detail::SourceFormat;
    constexpr auto format=Literal ? SourceFormat::literal : SourceFormat::direct;
    const auto result=native_contact_predicate_detail::run_source_private<format>(s);
    if (result==1u) ++activity().completed;
    else if (result) { ++activity().continuations; activity().last_next=result; }
    return result;
#else
    (void)s;
    return 0u;
#endif
}
} // namespace halo2::native_contact_predicate_owner_detail

namespace halo2::native_contact_predicate_ordered_detail {
// Keep the authored destination/source operand order in the retained body.
// In particular, a compiler may exchange the two packed multiply operands
// and change the escaped NaN payload without changing finite arithmetic.
inline __m128 multiply(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_contact_predicate_detail::product(a,b);
#else
    return _mm_mul_ps(a,b);
#endif
}
inline __m128 subtract(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_contact_predicate_detail::difference(a,b);
#else
    return _mm_sub_ps(a,b);
#endif
}
inline __m128 sum_low(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_contact_predicate_detail::sum_low(a,b);
#else
    return _mm_add_ss(a,b);
#endif
}
} // namespace halo2::native_contact_predicate_ordered_detail
