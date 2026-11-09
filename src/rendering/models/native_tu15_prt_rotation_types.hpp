#pragma once
#include "rendering/models/native_tu15_prt_matrix_vector.hpp"
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::prt_rotation_detail {
// Native scalar operators keep the first SSE operand's NaN ownership and
// round at the same single-precision boundaries as the authored polynomial.
inline float multiply(float a, float b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("mulss %1,%0" : "+x"(a) : "x"(b)); return a;
#else
    return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
}
inline float add(float a, float b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("addss %1,%0" : "+x"(a) : "x"(b)); return a;
#else
    return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
}
inline float subtract(float a, float b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("subss %1,%0" : "+x"(a) : "x"(b)); return a;
#else
    return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
}
struct Caller {
    std::uint32_t matrix, coefficients, output, stack, saved_bx, saved_bp, saved_di;
    std::uint16_t x87_c1;
};
struct Continuation {
    std::uint32_t output, source;
    float first, second, third, fourth, fifth, sixth, seventh, eighth;
    std::uint16_t x87_c1;
};
struct SquareResult {
    std::uint32_t output, source, saved_bp, saved_si, saved_di;
    float first, second;
};
// Published source-bank transport lives here; the generated numerical body
// uses only typed native scalar locals and three structured degree exits.
struct Frame {
    SourcePhysicsContactMemory& memory;
    const SourcePhysicsContactRecordObserver& observe;
    std::uint32_t word(std::uint32_t address) const { return prt_helpers_detail::read(memory, address); }
    float scalar(std::uint32_t address) const { return std::bit_cast<float>(word(address)); }
    long double extended_double(std::uint32_t address) const { return prt_basis_detail::double_value(memory, address); }
    long double extended_single(std::uint32_t address) const { return prt_basis_detail::single(memory, address); }
    void publish(std::uint32_t site, std::uint32_t address, std::uint32_t value) {
        prt_helpers_detail::store(memory, address, value, 4, site, observe);
    }
    SquareResult square(std::uint32_t stack, std::uint32_t size, std::uint32_t output,
        std::uint32_t saved_bp, std::uint32_t saved_si, std::uint32_t saved_di, float previous_second) {
        const prt_matrix_vector_detail::Caller caller{size, output, stack, saved_bp, saved_si, saved_di};
        const auto result = prt_matrix_vector_detail::multiply_square(memory, caller, observe);
        // These pops happen before the parent resumes its next degree.
        const auto di = word(stack - 20u), si = word(stack - 16u), bp = word(stack - 12u);
        (void)word(stack);
        return {result.output, result.vector, bp, si, di,
            std::bit_cast<float>(prt_projection_detail::bits(result.sum)),
            result.used_bulk ? std::bit_cast<float>(prt_projection_detail::bits(result.partial)) : previous_second};
    }
};
}
