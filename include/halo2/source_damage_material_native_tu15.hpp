#pragma once
#include <cstdint>

namespace halo2 {
// Diagnostics only; these counters never enter saved or replicated state.
struct SourceDamageMaterialNativeActivity {
    std::uint64_t calls{},groups_found{},modifiers_found{};
};
SourceDamageMaterialNativeActivity source_damage_material_native_activity();
}
