#pragma once

#include "gameplay/vehicles/native_tu15_zoom_factor_systems.hpp"

namespace halo2::native_weapon_magnification {
namespace layout = native_zoom_factor;
inline constexpr std::uint32_t level_count_offset = 0x1feu;
inline constexpr std::uint32_t minimum_offset = 0x200u;
inline constexpr std::uint32_t maximum_offset = 0x204u;

enum class Access {
    objects, headers, one, weapon, tag, definitions, definition, level_count,
    store_fraction, minimum, store_minimum, maximum, store_maximum,
    ratio_maximum, ratio_minimum, ratio_fraction, result_minimum, store_result, result
};
enum class Local : std::uint32_t { minimum = 0, maximum = 4, fraction = 8 };

// A valid zoom level samples the weapon's magnification range geometrically.
// The first level uses the minimum and the last uses the maximum. A missing
// level returns unit magnification. Replace nonpositive endpoints with unit
// magnification before interpolation.
template<class Memory, class Frame>
void compute(Memory &memory, Frame &frame, std::uint32_t handle, std::uint32_t level) {
    using enum Access;
    const auto array = memory.load32(objects, layout::globals::objects);
    const auto rows = memory.load32(headers, array + layout::datum_data);
    const auto default_scale = memory.load32(one, layout::globals::one);
    const auto object = memory.load32(weapon, rows + (handle & layout::handle_mask) * layout::header_size
        + layout::header_body);
    const auto weapon_tag = memory.load32(tag, object + layout::weapon_field::tag);
    const auto tags = memory.load32(definitions, layout::globals::definitions);
    const auto weapon_definition = memory.load32(definition,
        tags + (weapon_tag & layout::handle_mask) * layout::definition_size + layout::definition_body);

    frame.reserve_locals();
    if (frame.level_is_negative(level)) return;
    const auto count = memory.load16(level_count, weapon_definition + level_count_offset);
    if (!frame.level_is_available(level, count)) return;

    const auto fraction = frame.fraction(level, count);
    memory.store32(store_fraction, frame.local(Local::fraction), fraction);
    const auto min_scale = memory.load32(minimum, weapon_definition + minimum_offset);
    const auto minimum_value = frame.minimum_is_positive() ? min_scale : default_scale;
    memory.store32(store_minimum, frame.local(Local::minimum), minimum_value);
    const auto max_scale = memory.load32(maximum, weapon_definition + maximum_offset);
    memory.store32(store_maximum, frame.local(Local::maximum), frame.maximum_or_default(max_scale, default_scale));

    frame.push_float(memory.load32(ratio_maximum, frame.local(Local::maximum)));
    frame.divide_by(memory.load32(ratio_minimum, frame.local(Local::minimum)));
    frame.push_float(memory.load32(ratio_fraction, frame.local(Local::fraction)));
    frame.raise_ratio();
    frame.multiply_by(memory.load32(result_minimum, frame.local(Local::minimum)));
    memory.store32(store_result, frame.local(Local::fraction), frame.round_result());
    frame.pop_result();
    (void)memory.load32(result, frame.local(Local::fraction));
}
} // namespace halo2::native_weapon_magnification
