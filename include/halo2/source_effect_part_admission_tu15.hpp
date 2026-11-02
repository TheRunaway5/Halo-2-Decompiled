#pragma once
#include <cstdint>
#include <cstddef>
#include <optional>
#include <span>
namespace halo2 {
// Original17D266..17D292: a signed location index must name an authored
// location and the part reference must be present. This precedes iteration,
// environment/position queries, gain production and downstream dispatch.
bool source_effect_part_reference_admitted_tu15(std::int16_t location,
    std::uint32_t location_count,std::uint32_t reference)noexcept;
// Original17D298..17D2BA: part+2 is selected by actual effect+2 bit5.
// Part+0's separate environment query is not replaced by this predicate.
bool source_effect_part_selector_admitted_tu15(std::uint16_t selector,
    std::uint16_t effect_flags)noexcept;
struct SourceEffectSoundPartTu15 {
    std::uint32_t sound{},scale_a{},scale_b{};
    std::int16_t location{};
    std::uint16_t selector{};
};
// Read the complete literal56-byte part used by both native sound consumers.
// This admits only its snd! identity/location/reference. Environment, current
// selector flags, location iteration and gain/voice ownership remain callers.
std::optional<SourceEffectSoundPartTu15> source_effect_sound_part_tu15(
    std::span<const std::byte>,std::uint32_t location_count);
}
