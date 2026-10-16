#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <array>
#include <functional>
#include <optional>

namespace halo2 {
class SourceRuntimeMemory;
// Read-only provenance of the actual17D730 -> C2380 call. No typed-pool
// identity mapping or new light/cache publication is performed here.
struct SourceEffectLightBirthTu15 {
    std::uint32_t callsite{},source_tick{},definition{},light=0xffffffffu;
    std::uint32_t parent_effect{},parent_effect_body{},part_address{},part_ordinal{},event_ordinal{};
    std::optional<std::uint32_t> location,location_address;
    std::uint32_t owner{},node{},position_address{},forward_address{},up_address{},control_word{};
    std::uint32_t caller_stack{},light_record{};
    std::array<std::uint32_t,68> light_words{};
};
SourceEffectLightBirthTu15 source_effect_light_birth_before_tu15(const SourceRuntimeMemory&,
    std::uint32_t callsite,const SourcePhysicsWorldRuntimeAbi&);
SourceEffectLightBirthTu15 source_effect_light_birth_after_tu15(const SourceRuntimeMemory&,
    const SourceEffectLightBirthTu15&,const SourcePhysicsWorldRuntimeAbi&);
using SourceEffectLightBirthObserverTu15=std::function<void(bool,
    const SourceEffectLightBirthTu15&,const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory&)>;
}
