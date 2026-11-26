#pragma once

#include <cstdint>

namespace halo2::native_animation_fade_start {
inline constexpr std::uint32_t clock = 0x51df54u;
inline constexpr std::uint32_t one = 0x46a0d0u;
inline constexpr std::uint32_t tick_rate = 2u;
inline constexpr std::uint32_t position = 0u;
inline constexpr std::uint32_t length = 1u;
inline constexpr std::uint32_t direction = 3u;
inline constexpr std::uint32_t max_ticks = 127u;

enum class Access {
    clock_pointer, clock_rate, previous_length, previous_direction, matching_length,
    rescale_length, unit_scale, previous_position,
    scaled_length, scaled_position, scaled_direction,
    initial_length, initial_position, initial_direction,
    clear_length, clear_position, clear_direction,
};

// Keep the original counter in place when both its length and direction
// match. Otherwise preserve its fractional progress, or start a new fade.
template<class Memory, class Frame>
void start(Memory &memory, Frame &frame, std::uint32_t fade, std::uint32_t active) {
    using enum Access;
    const auto time = memory.load32(clock_pointer, clock);
    const auto rate = memory.load16(clock_rate, time + tick_rate);
    const auto duration = frame.duration(rate);
    if (!duration) {
        memory.store8(clear_length, fade + length, 0u);
        memory.store8(clear_position, fade + position, 0u);
        memory.store8(clear_direction, fade + direction, 0u);
        return;
    }

    const auto previous = memory.load8(previous_length, fade + length);
    if (frame.has_previous(previous) &&
        frame.same_direction(active, memory.load8(previous_direction, fade + direction)) &&
        frame.same_length(duration, memory.load8(matching_length, fade + length))) {
        frame.retained();
        return;
    }

    // The source rereads the length before rescaling its position.
    const auto old_length = memory.load8(rescale_length, fade + length);
    if (frame.has_previous(old_length)) {
        const auto unit = memory.load32(unit_scale, one);
        frame.select_divisor(old_length, unit);
        const auto previous_frame = memory.load8(previous_position, fade + position);
        const auto adjusted = frame.rescale(previous_frame, duration);
        memory.store8(scaled_length, fade + length, duration);
        memory.store8(scaled_position, fade + position, adjusted);
        memory.store8(scaled_direction, fade + direction, active != 0u);
    } else {
        const auto first = frame.initial_position(duration, active);
        memory.store8(initial_length, fade + length, duration);
        memory.store8(initial_position, fade + position, first);
        memory.store8(initial_direction, fade + direction, active != 0u);
    }
}
} // namespace halo2::native_animation_fade_start
