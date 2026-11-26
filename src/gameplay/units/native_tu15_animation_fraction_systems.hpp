#pragma once

#include "gameplay/units/native_tu15_animation_clamp_systems.hpp"

namespace halo2::native_animation_fraction {
namespace layout = native_animation_clamp;

enum class Access {
    animation_index,
    graph_handle,
    tag_instances,
    graph_address,
    animation_key,
    frame_count,
    frame_margin,
    fraction,
    store_zero,
    store_limit,
    store_scaled,
    reload_argument,
    store_argument
};

enum class Exit { negative, nonnegative };

// Convert a normalized animation position to a frame argument and hand it to
// the frame clamp. The separate stores retain the writes made to the caller's
// argument slot at each instruction.
template<class Memory, class Frame>
Exit position(Memory &memory, Frame &frame, std::uint32_t animation_state) {
    using enum Access;
    std::uint32_t animation = 0;
    if (memory.load16(animation_index, animation_state + layout::state::animation_index) != 0xffffu) {
        const auto graph = memory.load32(graph_handle, animation_state + layout::state::graph);
        const auto tags = memory.load32(tag_instances, layout::globals::tag_instances);
        const auto definition = memory.load32(graph_address,
            tags + ((graph & 0xffffu) << layout::instance_shift) + layout::tag_address);
        const auto key = memory.load32(animation_key, animation_state + layout::state::animation);
        animation = frame.lookup_animation(definition, key);
    }

    const auto count = memory.load16(frame_count, animation + layout::frame_count);
    frame.convert_count(layout::widen16(count));
    const auto margin = memory.load32(frame_margin, layout::globals::frame_margin);
    const auto limit = frame.frame_limit(margin);
    const auto argument = frame.argument_address();
    const auto input = memory.load32(fraction, argument);
    const auto scaled = frame.scale_fraction(input);
    const auto exit = frame.scaled_is_negative() ? Exit::negative : Exit::nonnegative;
    if (exit == Exit::negative) {
        memory.store32(store_zero, argument, 0u);
    } else {
        const bool fits_or_unordered = frame.scaled_does_not_exceed_limit();
        memory.store32(store_limit, argument, limit);
        if (fits_or_unordered)
            memory.store32(store_scaled, argument, scaled);
    }
    const auto result = memory.load32(reload_argument, argument);
    memory.store32(store_argument, argument, result);
    return exit;
}
} // namespace halo2::native_animation_fraction
