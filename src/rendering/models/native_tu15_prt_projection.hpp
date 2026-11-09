#pragma once
#include "rendering/models/native_tu15_prt_helpers.hpp"
#include <cstring>

namespace halo2::prt_projection_detail {
using prt_helpers_detail::read;
using prt_helpers_detail::store;
inline std::uint32_t signed_word(const SourcePhysicsContactMemory& memory, std::uint32_t at) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory, at, 2)))));
}
inline __m128 scalar(std::uint32_t value) {
    return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));
}
inline std::uint32_t bits(__m128 value) {
    return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));
}
// The source operand owns its NaN payload. Keep the SSE destination explicit
// even when register allocation would prefer swapping commutative operands.
inline __m128 add(__m128 a, __m128 b, bool packed) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (packed) asm("addps %1,%0" : "+x"(a) : "x"(b));
    else asm("addss %1,%0" : "+x"(a) : "x"(b));
    return a;
#else
    return packed ? _mm_add_ps(a, b) : _mm_add_ss(a, b);
#endif
}
inline __m128 multiply(__m128 a, __m128 b, bool packed) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (packed) asm("mulps %1,%0" : "+x"(a) : "x"(b));
    else asm("mulss %1,%0" : "+x"(a) : "x"(b));
    return a;
#else
    return packed ? _mm_mul_ps(a, b) : _mm_mul_ss(a, b);
#endif
}
struct Dot9 {
    __m128 sum{}, ninth{}, high{}, right_high{};
};
// Nine-coefficient SH projection, with the source's exact SSE grouping and
// eight-byte load order. Upper lanes escape through the caller's ABI too.
inline Dot9 dot9(const SourcePhysicsContactMemory& memory,
    std::uint32_t left, std::uint32_t right) {
    std::array<std::byte, 16> a{}, b{}, c{}, d{};
    memory.read(left, std::span(a).first(8));
    memory.read(right, std::span(b).first(8));
    memory.read(left + 16u, std::span(c).first(8));
    memory.read(right + 16u, std::span(d).first(8));
    memory.read(left + 8u, std::span(a).last(8));
    memory.read(right + 8u, std::span(b).last(8));
    memory.read(left + 24u, std::span(c).last(8));
    memory.read(right + 24u, std::span(d).last(8));
    Dot9 result;
    __m128 lo, rhs;
    std::memcpy(&lo, a.data(), 16); std::memcpy(&rhs, b.data(), 16);
    std::memcpy(&result.high, c.data(), 16); std::memcpy(&result.right_high, d.data(), 16);
    lo = multiply(lo, rhs, true);
    result.high = multiply(result.high, result.right_high, true);
    result.sum = add(lo, result.high, true);
    result.ninth = scalar(read(memory, left + 32u));
    result.high = _mm_movehl_ps(result.high, result.sum);
    result.ninth = multiply(result.ninth, scalar(read(memory, right + 32u)), false);
    result.sum = add(result.sum, result.high, true);
    result.ninth = add(result.ninth, result.sum, false);
    result.sum = _mm_shuffle_ps(result.sum, result.sum, 85);
    result.sum = add(result.sum, result.ninth, false);
    return result;
}

struct Caller {
    std::uint32_t descriptor, source, stack, saved_bx, saved_si, saved_di, saved_bp;
};
struct Projection {
    std::uint32_t last_source{}, last_rows{};
    Dot9 last_dot;
    bool projected{};
    std::uint32_t last_scalar{};
};
// Complete23C80 row/component traversal. Stack scratch remains published so
// output aliases and ordered-store observers see the original transaction.
inline Projection project(SourcePhysicsContactMemory& memory, const Caller& caller,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto order = signed_word(memory, caller.descriptor);
    const auto coefficients = order * order;
    store(memory, caller.stack - 0x48u, caller.saved_bx, 4, 0x23c89u, observe);
    const auto components = signed_word(memory, caller.descriptor + 4u);
    store(memory, caller.stack - 0x4cu, caller.saved_si, 4, 0x23c8eu, observe);
    store(memory, caller.stack - 0x50u, caller.saved_di, 4, 0x23c90u, observe);
    Projection result{coefficients, caller.source, {}, false};
    if (std::int32_t(signed_word(memory, caller.descriptor + 2u)) <= 0) return result;
    store(memory, caller.stack - 0x54u, caller.saved_bp, 4, 0x23ca3u, observe);
    const auto frame = caller.stack - 0x54u;
    const auto frame_read = [&](std::uint32_t offset) { return read(memory, frame + offset); };
    const auto frame_store = [&](std::uint32_t offset, std::uint32_t value, std::uint32_t site) {
        store(memory, frame + offset, value, 4, site, observe);
    };
    const auto channel_bytes = coefficients * 4u;
    const auto component_bytes = channel_bytes * 3u;
    const auto row_bytes = (components + 1u) * component_bytes;
    frame_store(0x50, component_bytes, 0x23cb2u);
    frame_store(0x2c, caller.source + component_bytes, 0x23cb8u);
    frame_store(0x24, caller.source + channel_bytes * 2u, 0x23cbfu);
    frame_store(0x20, caller.source + channel_bytes, 0x23cc6u);
    frame_store(0x1c, caller.source, 0x23cd3u);
    frame_store(0x30, caller.source + channel_bytes * 4u, 0x23cdau);
    frame_store(0x28, caller.source + channel_bytes * 5u, 0x23cdeu);
    for (std::uint32_t row = 0;;) {
        constexpr std::array left_slots{0x1cu, 0x20u, 0x24u};
        constexpr std::array prepare_sites{0x23ce6u, 0x23d3fu, 0x23d98u};
        constexpr std::array dot_sites{0x23d35u, 0x23d8eu, 0x23de7u};
        for (unsigned channel = 0; channel < 3; ++channel) {
            frame_store(0x18, frame_read(left_slots[channel]), prepare_sites[channel]);
            const auto left = frame_read(0x18);
            const auto right = frame_read(0x58 + channel * 4);
            result.last_dot = dot9(memory, left, right);
            frame_store(0x34 + channel * 4, bits(result.last_dot.sum), dot_sites[channel]);
        }
        auto current_components = signed_word(memory, caller.descriptor + 4u);
        auto output = frame_read(0x64);
        constexpr std::array main_sites{0x23e02u, 0x23e18u, 0x23e2fu};
        for (unsigned channel = 0; channel < 3; ++channel) {
            if (channel) current_components = signed_word(memory, caller.descriptor + 4u);
            const auto sample = scalar(frame_read(0x34 + channel * 4));
            result.last_scalar = bits(sample);
            store(memory, output + ((3u * current_components + 4u) * row) * 4u + channel * 4u,
                bits(sample), 4, main_sites[channel], observe);
        }
        current_components = signed_word(memory, caller.descriptor + 4u);
        output += ((3u * current_components + 4u) * row) * 4u + 16u;
        if (std::int32_t(current_components) > 0) {
            const auto initial_red = frame_read(0x2c);
            const auto initial_blue = frame_read(0x28);
            frame_store(0x10, initial_red, 0x23e58u);
            const auto initial_green = frame_read(0x30);
            frame_store(0x18, initial_blue, 0x23e60u);
            frame_store(0x14, initial_green, 0x23e64u);
            for (std::uint32_t component = 0;;) {
                constexpr std::array component_left{0x10u, 0x14u, 0x18u};
                constexpr std::array component_prepare{0x23e74u, 0x23ecdu, 0x23f26u};
                constexpr std::array component_dots{0x23ec3u, 0x23f1cu, 0x23f75u};
                for (unsigned channel = 0; channel < 3; ++channel) {
                    frame_store(0x40, frame_read(component_left[channel]), component_prepare[channel]);
                    const auto left = frame_read(0x40);
                    const auto right = frame_read(0x58 + channel * 4);
                    result.last_dot = dot9(memory, left, right);
                    frame_store(0x44 + channel * 4, bits(result.last_dot.sum), component_dots[channel]);
                }
                store(memory, output + component * 4u, frame_read(0x44), 4, 0x23f81u, observe);
                current_components = signed_word(memory, caller.descriptor + 4u);
                store(memory, output + (current_components + component) * 4u,
                    frame_read(0x48), 4, 0x23f92u, observe);
                current_components = signed_word(memory, caller.descriptor + 4u);
                const auto last_value = frame_read(0x4c);
                result.last_scalar = last_value;
                const auto advance = frame_read(0x50);
                store(memory, output + (component + 2u * current_components) * 4u,
                    last_value, 4, 0x23fa8u, observe);
                frame_store(0x10, frame_read(0x10) + advance, 0x23fadu);
                frame_store(0x14, frame_read(0x14) + advance, 0x23fb1u);
                const auto next_blue = frame_read(0x18) + advance;
                current_components = signed_word(memory, caller.descriptor + 4u);
                ++component;
                frame_store(0x18, next_blue, 0x23fc2u);
                if (std::int32_t(component) >= std::int32_t(current_components)) break;
            }
        }
        const auto next_green = frame_read(0x20) + row_bytes;
        const auto current_red = frame_read(0x1c);
        const auto current_blue = frame_read(0x24);
        const auto current_component_blue = frame_read(0x28);
        frame_store(0x20, next_green, 0x23fdeu);
        const auto current_component_green = frame_read(0x30);
        frame_store(0x1c, current_red + row_bytes, 0x23feau);
        const auto current_component_red = frame_read(0x2c);
        frame_store(0x30, current_component_green + row_bytes, 0x23ff2u);
        result.last_rows = signed_word(memory, caller.descriptor + 2u);
        ++row;
        result.last_source = current_blue + row_bytes;
        frame_store(0x24, result.last_source, 0x24003u);
        frame_store(0x28, current_component_blue + row_bytes, 0x24007u);
        frame_store(0x2c, current_component_red + row_bytes, 0x2400bu);
        if (std::int32_t(row) >= std::int32_t(result.last_rows)) break;
    }
    result.projected = true;
    return result;
}

template<class State> void bridge(State& state) {
    const Caller caller{state.gp[0], state.gp[2], state.gp[4], state.gp[3], state.gp[6], state.gp[7], state.gp[5]};
    const auto result = project(state.memory, caller, state.observe);
    state.gp[1] = result.last_source;
    state.gp[2] = result.last_rows;
    if (result.projected) {
        state.gp[5] = read(state.memory, caller.stack - 0x54u);
        //23F7B/23E22 reload the last channel's low scalar after projection.
        state.xmm[0] = scalar(result.last_scalar);
        state.xmm[1] = result.last_dot.ninth;
        state.xmm[2] = result.last_dot.high;
        state.xmm[3] = result.last_dot.right_high;
    }
    state.gp[7] = read(state.memory, caller.stack - 0x50u);
    state.gp[6] = read(state.memory, caller.stack - 0x4cu);
    state.gp[3] = read(state.memory, caller.stack - 0x48u);
    state.flags.plus(caller.stack - 0x44u, 0x44u, 4);
    (void)read(state.memory, caller.stack);
    state.gp[4] = caller.stack + 20u;
}
}
