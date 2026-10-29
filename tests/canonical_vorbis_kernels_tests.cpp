#include "canonical_vorbis_kernels.h"
#include "fixtures/canonical_vorbis_mdct_v1.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

std::uint32_t word(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

std::uint64_t output_hash(const float *output, std::size_t count) {
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t i = 0; i < count; ++i) {
        const auto bits = word(output[i]);
        for (unsigned shift = 0; shift < 32; shift += 8) {
            hash ^= (bits >> shift) & 0xffu;
            hash *= 1099511628211ull;
        }
    }
    return hash;
}

template <std::size_t TrigCount, std::size_t BitCount>
void mdct_fixture(const std::array<std::uint32_t, TrigCount> &trig_bits,
                  const std::array<int, BitCount> &bitrev,
                  std::uint64_t expected_hash,
                  const std::array<std::uint32_t, 8> &expected_words) {
    const int n = static_cast<int>(BitCount * 4);
    std::vector<float> trig;
    for (const auto bits : trig_bits)
        trig.push_back(std::bit_cast<float>(bits));
    std::vector<int> permutation(bitrev.begin(), bitrev.end());
    H2CanonicalVorbisMdctLookup lookup{
        n, static_cast<int>(std::countr_zero(static_cast<unsigned>(n))),
        trig.data(), permutation.data(), 0.f};

    constexpr std::uint32_t guard_word = 0x7f7fffffu;
    const float guard = std::bit_cast<float>(guard_word);
    std::vector<float> input(n / 2 + 2, guard);
    std::uint32_t seed = 0x13579bdfu;
    for (int i = 0; i < n / 2; ++i) {
        seed = 1664525u * seed + 1013904223u;
        const auto mantissa = static_cast<std::int32_t>(seed >> 8) - 0x800000;
        input[i + 1] = static_cast<float>(mantissa) * 0x1p-24f;
    }
    const auto original_input = input;
    const std::array<int, 8> indices{
        0, 1, n / 4 - 1, n / 4, n / 2 - 1, n / 2, n - 2, n - 1};
    for (const bool in_place : {false, true}) {
        std::vector<float> output(n + 2, guard);
        if (in_place)
            for (int i = 0; i < n / 2; ++i)
                output[i + 1] = input[i + 1];
        h2_canonical_vorbis_mdct_backward(
            &lookup, in_place ? output.data() + 1 : input.data() + 1,
            output.data() + 1);
        if (word(output.front()) != guard_word ||
            word(output.back()) != guard_word || input != original_input)
            throw std::runtime_error("MDCT modified an input or output guard");
        if (output_hash(output.data() + 1, n) != expected_hash)
            throw std::runtime_error("Canonical MDCT complete output differs");
        for (std::size_t i = 0; i < indices.size(); ++i)
            if (word(output[indices[i] + 1]) != expected_words[i])
                throw std::runtime_error("Canonical MDCT output word differs");
    }
    for (std::size_t i = 0; i < trig.size(); ++i)
        if (word(trig[i]) != trig_bits[i])
            throw std::runtime_error("MDCT modified a coefficient");
    if (permutation != std::vector<int>(bitrev.begin(), bitrev.end()))
        throw std::runtime_error("MDCT modified a permutation entry");
}

void overlap_rounding_fixture() {
    const std::array<float, 2> window{1.f, 0x1.fffffep-1f};
    const std::array<float, 2> incoming{-1.f, 1.f};
    std::array<float, 2> pcm{0x1.000002p+0f, -0x1.000002p+0f};

    // Separate binary32 multiply/add loses the nonzero cancellation residual.
    volatile float unfused_product = pcm[0] * window[1];
    volatile float unfused = unfused_product - 1.f;
    if (word(unfused) != 0)
        throw std::runtime_error("Overlap RED control did not round separately");
    h2_canonical_vorbis_overlap(pcm.data(), incoming.data(), window.data(), 2);
    if (word(pcm[0]) != 0x337ffffeu || word(pcm[1]) != 0xb4400000u)
        throw std::runtime_error("Canonical overlap single rounding differs");
    const auto before_empty = pcm;
    h2_canonical_vorbis_overlap(pcm.data(), incoming.data(), window.data(), 0);
    if (pcm != before_empty)
        throw std::runtime_error("Empty overlap changed PCM");
}

} // namespace

int main() try {
    using namespace canonical_vorbis_fixture;
    mdct_fixture(trig64, bitrev64, output_hash64, output_words64);
    mdct_fixture(trig128, bitrev128, output_hash128, output_words128);
    mdct_fixture(trig256, bitrev256, output_hash256, output_words256);
    mdct_fixture(trig2048, bitrev2048, output_hash2048, output_words2048);
    overlap_rounding_fixture();
    std::cout << "PASS canonical binary32 MDCT64/128/256/2048, separate and "
                 "in-place buffers, guards and overlap FMA rounding\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
