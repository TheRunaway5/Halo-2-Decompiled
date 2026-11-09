#pragma once
#include "halo2/physics_contact_record.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <emmintrin.h>

namespace halo2::prt_helpers_detail {
inline std::uint32_t read(const SourcePhysicsContactMemory& memory,
    std::uint32_t address, unsigned width = 4) {
    std::array<std::byte, 4> bytes{};
    memory.read(address, std::span(bytes).first(width));
    std::uint32_t value{};
    for (unsigned i = 0; i < width; ++i)
        value |= std::uint32_t(std::to_integer<unsigned>(bytes[i])) << (8 * i);
    return value;
}
inline void store(SourcePhysicsContactMemory& memory, std::uint32_t address,
    std::uint32_t value, unsigned width, std::uint32_t site,
    const SourcePhysicsContactRecordObserver& observe) {
    std::array<std::byte, 4> bytes{};
    for (unsigned i = 0; i < width; ++i) bytes[i] = std::byte(value >> (8 * i));
    SourcePhysicsContactRecordStore event{site, address, {}, {}};
    if (observe) {
        event.before.resize(width);
        memory.read(address, event.before);
        event.written.assign(bytes.begin(), bytes.begin() + width);
    }
    memory.write(address, std::span(bytes).first(width));
    if (observe) observe(event);
}

struct SectionOffsets { std::uint32_t coefficients{}, last_count{}; };
// Actual23520: cap each section at two coefficients and publish byte offsets.
// Argument slots remain addresses because outputs can alias those slots.
inline SectionOffsets section_offsets(SourcePhysicsContactMemory& memory,
    std::uint32_t sections, unsigned count, std::uint32_t output, std::uint32_t output_slot,
    std::uint32_t stride_slot, const SourcePhysicsContactRecordObserver& observe) {
    SectionOffsets result;
    auto offset = output + 12u;
    for (unsigned section = 0; section < count; ++section) {
        store(memory, offset, result.coefficients * read(memory, stride_slot),
            4, 0x23547u, observe);
        result.last_count = std::min(read(memory, sections + section * 4u + 6u, 2), 2u);
        result.coefficients += result.last_count;
        store(memory, read(memory, output_slot) + 0x4cu + section,
            result.last_count, 1, 0x2355cu, observe);
        offset += 4;
    }
    return result;
}

struct PackedTransform { std::uint32_t penultimate{}, last{}, scale{}; };
// Actual23580: transpose the twelve raw transform words into a scaled matrix.
// Keep loads/stores interleaved: the original permits overlapping transforms.
inline PackedTransform pack_transform(SourcePhysicsContactMemory& memory,
    std::uint32_t input, std::uint32_t output,
    const SourcePhysicsContactRecordObserver& observe) {
    constexpr std::array offsets{4u, 16u, 28u, 40u, 8u, 20u, 32u, 44u, 12u, 24u, 36u, 48u};
    constexpr std::array sites{0x23582u, 0x23588u, 0x2358eu, 0x2359cu,
        0x235a2u, 0x235a8u, 0x235aeu, 0x235b4u, 0x235bau, 0x235c0u, 0x235c6u, 0x235ccu};
    PackedTransform result;
    for (unsigned word = 0; word < offsets.size(); ++word) {
        const auto value = read(memory, input + word * 4u);
        if (word == 3) result.scale = read(memory, 0x46a0d0u);
        store(memory, output + offsets[word], value, 4, sites[word], observe);
        if (word == 10) result.penultimate = value;
        if (word == 11) result.last = value;
    }
    store(memory, output, result.scale, 4, 0x235cfu, observe);
    return result;
}

inline bool reference() {
    static const bool enabled = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_MODEL_PRT_HELPERS");
        return value && *value;
    }();
    return enabled;
}
// Narrow caller transport only: kernels above never receive virtual registers.
template<class State> void bridge(State& state, std::uint32_t entry) {
    const auto stack = state.gp[4];
    if (entry == 0x23580u) {
        const auto result = pack_transform(state.memory, state.gp[1], state.gp[0], state.observe);
        state.gp[1] = result.last;
        state.gp[2] = result.penultimate;
        state.xmm[0] = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(result.scale)));
        (void)read(state.memory, stack);
        state.gp[4] = stack + 4;
        return;
    }
    const auto count = std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(state.gp[3])));
    state.gp[3] = std::uint32_t(count);
    state.gp[0] = state.gp[2] = 0;
    if (count > 0) {
        store(state.memory, stack - 4, state.gp[5], 4, 0x2352bu, state.observe);
        store(state.memory, stack - 8, state.gp[6], 4, 0x2352cu, state.observe);
        //2352D reads the first argument before the final nonvolatile save.
        const auto output = read(state.memory, stack + 4);
        store(state.memory, stack - 12, state.gp[7], 4, 0x23531u, state.observe);
        const auto result = section_offsets(state.memory, state.gp[1], unsigned(count),
            output, stack + 4, stack + 8, state.observe);
        state.gp[0] = result.coefficients;
        state.gp[1] = result.last_count;
        state.gp[2] = std::uint32_t(count);
        state.gp[7] = read(state.memory, stack - 12);
        state.gp[6] = read(state.memory, stack - 8);
        state.gp[5] = read(state.memory, stack - 4);
        state.flags.minus(std::uint32_t(count), std::uint32_t(count), 4);
    } else {
        state.flags.logic(std::uint32_t(count), 4);
    }
    (void)read(state.memory, stack);
    state.gp[4] = stack + 12;
}
}
