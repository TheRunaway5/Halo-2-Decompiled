#pragma once

#include "gameplay/units/native_tu15_unit_animation_update_systems.hpp"
#include "physics/runtime/native_tu15_record_iterator_systems.hpp"
#include "physics/runtime/native_tu15_transition_fraction_systems.hpp"

namespace halo2::native_keyed_marker_pose {
namespace animation = native_unit_animation_update;
using native_object_update::at;
using native_object_update::ObjectHeader;
using native_object_update::widen16;
using native_record_iterator::Iterator;

inline constexpr std::uint32_t least_weight = 0x46a0ecu;

enum class Access {
    object_table, object_rows, other_object, object_data, channels_offset, state_offset,
    animation_tag, graph, channel, duration, weight_store, weight_read, margin,
    matrices, poses, weight_argument, record_first, record_second, record_index,
    record_stage, record_first_key, record_second_key
};
enum class Local : std::uint32_t {
    other_object = 0x1c, weight = 0x24, matrices = 0x2c, poses = 0x28,
    weight_after_matrices_push = 0x28
};

// Pose the keyed marker chains against the other object once the transition
// has enough weight. The iterator starts at its first stage with empty keys.
template<class Memory, class Frame>
void pose(Memory &memory, Frame &frame, std::uint32_t handle) {
    using enum Access;
    const auto header = native_object_update::header_of(memory, handle, object_table, object_rows);
    frame.reserve_frame();
    const auto other = memory.load32(other_object, frame.local(Local::other_object));
    const auto object = memory.load32(object_data, at(header, offsetof(ObjectHeader, object)));
    const auto channels = object + widen16(memory.load16(channels_offset, at(object, animation::unit::channels)));
    const auto state = object + widen16(memory.load16(state_offset, at(object, animation::unit::state)));

    if (memory.load32(animation_tag, at(state, animation::state::animation)) == animation::none ||
        memory.load32(graph, at(state, animation::state::graph)) == animation::none ||
        memory.load16(channel, at(state, animation::state::mode)) == 0xffffu ||
        !frame.has_other_object(other))
        return;
    const auto transition = at(channels, animation::channels::second);
    if (memory.load8(duration, at(transition, native_transition_fraction::transition::duration)) == 0)
        return;

    const auto weight = frame.transition_weight(transition);
    memory.store32(weight_store, frame.local(Local::weight), weight);
    frame.load_absolute_weight(memory.load32(weight_read, frame.local(Local::weight)));
    if (!frame.passes_margin(memory.load32(margin, least_weight))) return;

    const auto matrices_address = memory.load32(matrices, frame.local(Local::matrices));
    const auto poses_address = memory.load32(poses, frame.local(Local::poses));
    frame.push_matrices(matrices_address);
    const auto weight_for_pose = memory.load32(weight_argument, frame.local(Local::weight_after_matrices_push));
    const auto iterator = frame.push_pose_arguments(poses_address, weight_for_pose);
    const auto empty = frame.iterator_sentinel();
    memory.store32(record_first, at(iterator, offsetof(Iterator, record)), empty);
    memory.store32(record_second, at(iterator, offsetof(Iterator, record) + 4u), empty);
    memory.store16(record_index, at(iterator, offsetof(Iterator, index)), empty & 0xffffu);
    memory.store16(record_stage, at(iterator, offsetof(Iterator, stage)), 0u);
    memory.store32(record_first_key, at(iterator, offsetof(Iterator, first_key)), empty);
    memory.store32(record_second_key, at(iterator, offsetof(Iterator, second_key)), empty);
    frame.pose_chains();
}
} // namespace halo2::native_keyed_marker_pose
