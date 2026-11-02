#pragma once
#include <cstdint>

namespace halo2 {
// Diagnostics only; these counters never enter saved or replicated state.
struct SourceDamageMaterialNativeActivity {
    std::uint64_t calls{},groups_found{},modifiers_found{};
    std::uint64_t search_calls{},comparator_calls{};
};
SourceDamageMaterialNativeActivity source_damage_material_native_activity();
}
