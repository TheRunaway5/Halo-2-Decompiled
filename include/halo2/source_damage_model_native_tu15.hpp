#pragma once
#include <cstdint>

namespace halo2 {
// Process-local diagnostics only; these counters never enter gameplay state.
struct SourceDamageModelNativeActivity {
    std::uint64_t calls{},null_object{},null_model{},no_damage_information{},resolved{};
};
SourceDamageModelNativeActivity source_damage_model_native_activity();
}
