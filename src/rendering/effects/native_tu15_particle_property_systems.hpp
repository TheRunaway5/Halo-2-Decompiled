#pragma once

#include <cstdint>

namespace halo2::native_particle_property {
inline constexpr std::uint32_t lower_input = 0u;
inline constexpr std::uint32_t upper_input = 2u;
inline constexpr std::uint32_t modifier_mode = 4u;
inline constexpr std::uint32_t modifier_input = 6u;
inline constexpr std::uint32_t curve_reference = 8u;
inline constexpr std::uint32_t curve_definition = 4u;
inline constexpr std::uint32_t curve_flags = 1u;
inline constexpr std::uint32_t minimum = 4u;
inline constexpr std::uint32_t maximum = 8u;
inline constexpr std::uint32_t curve_type_mask = 0xf0u;
inline constexpr std::uint32_t one = 0x46a0d0u;

enum class Access {
    upper_index, lower_index, curve, curve_type, minimum_value, maximum_value,
    unit_value, extra_index, extra_mode, extra_add, extra_multiply,
};
enum class Modifier { none, add, multiply };

// Evaluate the property curve over its two input values. A curve without an
// authored type returns a fraction that the caller expands to its range.
template<class Memory, class Frame>
void evaluate(Memory &memory, Frame &frame) {
    using enum Access;
    const auto property = frame.property();
    const auto upper = memory.load16(upper_index, property + upper_input);
    const auto lower = memory.load16(lower_index, property + lower_input);
    frame.evaluate_curve(upper, lower);

    const auto definition = memory.load32(curve, frame.curve_address() + curve_definition);
    const auto flags = memory.load8(curve_type, definition + curve_flags);
    if (frame.has_range(flags)) {
        const bool below = frame.below_zero();
        memory.load32(minimum_value, definition + minimum);
        memory.load32(maximum_value, definition + maximum);
        if (below) frame.use_zero();
        else frame.limit_to_one(memory.load32(unit_value, one));
        frame.expand_range();
    }

    const auto current = frame.property();
    const auto index = memory.load16(extra_index, current + modifier_input);
    if (!frame.restore_and_check_modifier(index)) return;
    const auto mode = memory.load16(extra_mode, current + modifier_mode);
    const auto modifier = frame.choose_modifier(mode);
    if (modifier == Modifier::add) {
        const auto value = memory.load32(extra_add, frame.input_address(index));
        frame.modify(value, modifier);
    } else if (modifier == Modifier::multiply) {
        const auto value = memory.load32(extra_multiply, frame.input_address(index));
        frame.modify(value, modifier);
    }
}
} // namespace halo2::native_particle_property
