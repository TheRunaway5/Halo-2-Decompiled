#pragma once

#include <cstdint>

namespace halo2::native_authored_control_point::systems {
inline constexpr std::uint32_t first_control_point_kind = 4u;
inline constexpr std::uint32_t last_contiguous_kind = 8u;
inline constexpr std::uint32_t powered_cubic_kind = 10u;
inline constexpr std::uint32_t stride_table = 0x44b884u;
inline constexpr std::uint32_t first_point_offset = 0x14u;
inline constexpr std::uint32_t words_per_point = 2u;

// Keep the second kind read after the range checks. A watched or aliased
// reference may select a different stride by the time its kind is read again.
template<class Frame>
void resolve(Frame &frame) {
    frame.load_blob();
    if (!frame.supports_control_points()) {
        frame.finish_without_point();
        return;
    }
    frame.load_layer_stride();
    frame.select_layer();
    frame.select_point();
    frame.finish_with_point();
}
} // namespace halo2::native_authored_control_point::systems
