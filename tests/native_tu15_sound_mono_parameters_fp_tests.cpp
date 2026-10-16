#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>

using namespace halo2::tu15_mono_parameter_conversion;
namespace {
void require(bool condition, const char* why) {
    if (!condition) throw std::runtime_error(why);
}
std::uint16_t cw() { std::uint16_t value; asm volatile("fnstcw %0":"=m"(value)); return value; }
std::uint16_t sw() { std::uint16_t value; asm volatile("fnstsw %0":"=m"(value)); return value; }
std::array<std::uint16_t, 2> tag_top() {
    alignas(16) std::array<unsigned char, 32> state{};
    asm volatile("fnstenv %0; fldenv %0":"=m"(state)::"memory");
    return {std::uint16_t(state[8] | unsigned(state[9]) << 8), std::uint16_t(sw() & 0x3800)};
}
}
int main() try {
    const std::array modes{FpMode::source_pretable_53, FpMode::double_53, FpMode::extended_64};
    const std::array<std::uint32_t, 7> values{
        std::bit_cast<std::uint32_t>(0.25f), std::bit_cast<std::uint32_t>(1600.f),
        std::bit_cast<std::uint32_t>(-5.f), std::bit_cast<std::uint32_t>(0.7f),
        std::bit_cast<std::uint32_t>(5000.f), std::bit_cast<std::uint32_t>(-5.f),
        std::bit_cast<std::uint32_t>(1.f)};
    std::size_t dyadic = 0, restored = 0, stack_rejects = 0, q23_edges = 0;
    for (auto mode : modes) {
        // Exact dyadic inputs give independent integer expected words, including
        // both sides of Q23 integer boundaries and the negative asymmetry.
        for (std::uint32_t i = 0; i < 131072; ++i) {
            const auto numerator = (i * 131071u) & 0xffffffu;
            const double positive = double(numerator) / 16777216.0;
            require(quantize_finite_fraction(positive, mode).bits == numerator / 2, "Positive dyadic Q23 differs");
            const auto negative_expected = numerator ? 0xffffffu - numerator / 2 : 0u;
            require(quantize_finite_fraction(-positive, mode).bits == negative_expected, "Negative dyadic Q23 differs");
            dyadic += 2;
        }
        for (double input : {-2., -1., 1., 2.}) {
            require(quantize_finite_fraction(input, mode).bits == (input < 0 ? 0x800000u : 0x7fffffu), "Saturation differs");
            ++q23_edges;
        }
        for (std::uint64_t bits : {UINT64_C(0x0000000000000001), UINT64_C(0x8000000000000001), UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000)}) {
            const auto expected = bits == UINT64_C(0x8000000000000001) ? 0xffffffu : 0u;
            require(quantize_finite_fraction(std::bit_cast<double>(bits), mode).bits == expected, "Subnormal/signed zero differs");
            ++q23_edges;
        }
        const auto baseline = convert_bounded_values(values, mode);
        for (std::uint16_t precision : {std::uint16_t(0), std::uint16_t(0x200), std::uint16_t(0x300)})
            for (std::uint16_t rounding : {std::uint16_t(0), std::uint16_t(0x400), std::uint16_t(0x800), std::uint16_t(0xc00)})
                for (std::uint32_t mx : {0x1f80u, 0x3f80u, 0x5f80u, 0x7f80u, 0x9fc0u})
                    for (int flags : {0, FE_INEXACT, FE_INVALID | FE_DIVBYZERO, FE_UNDERFLOW | FE_OVERFLOW}) {
                        std::fenv_t environment;
                        require(std::fegetenv(&environment) == 0, "Cannot retain test FP environment");
                        const auto incoming = std::uint16_t(0x7f | precision | rounding);
                        asm volatile("fldcw %0"::"m"(incoming));
                        std::feclearexcept(FE_ALL_EXCEPT);
                        _mm_setcsr(mx);
                        if (flags) std::feraiseexcept(flags);
                        const auto before_cw = cw(), before_sw = sw();
                        const auto before_mx = _mm_getcsr();
                        const auto before_tag = tag_top();
                        const auto actual = convert_bounded_values(values, mode);
                        require(cw() == before_cw && sw() == before_sw && _mm_getcsr() == before_mx && tag_top() == before_tag, "Caller FP environment was not restored");
                        require(actual == baseline, "Explicit source mode depends on caller FP settings");
                        require(std::fesetenv(&environment) == 0, "Cannot restore test FP environment");
                        ++restored;
                    }
    }
    for (unsigned depth = 1; depth <= 7; ++depth) {
        for (unsigned i = 0; i < depth; ++i) asm volatile("fld1");
        const auto before_cw = cw(), before_sw = sw();
        const auto before_mx = _mm_getcsr();
        const auto before_tag = tag_top();
        for (auto mode : modes) {
            bool rejected = false;
            try { (void)convert_bounded_values(values, mode); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected && cw() == before_cw && sw() == before_sw && _mm_getcsr() == before_mx && tag_top() == before_tag, "Occupied x87 stack was accepted or changed");
            ++stack_rejects;
        }
        for (unsigned i = 0; i < depth; ++i) {
            long double retained;
            asm volatile("fstpt %0":"=m"(retained));
            require(retained == 1, "Rejected conversion damaged caller x87 stack");
        }
    }
    std::cout << "{\"passed\":true,\"independent_exact_dyadic_quantizations\":" << dyadic
              << ",\"saturation_subnormal_signed_zero_controls\":" << q23_edges
              << ",\"caller_FP_restoration_and_result_controls\":" << restored
              << ",\"occupied_stack_no_mutation_rejects\":" << stack_rejects << "}\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
