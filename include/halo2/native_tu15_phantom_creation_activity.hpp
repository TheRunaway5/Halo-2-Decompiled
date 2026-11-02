#pragma once
#include <cstdint>

namespace halo2 {
// Executor-local diagnostics only. Never serialized, restored or consumed by
// gameplay, allocation, listener decisions or native admission.
struct NativeTu15PhantomCreationActivity {
    std::uint64_t pair_calls{},pair_notifications{},pair_completions{};
    std::uint64_t complex_calls{},simple_calls{},notification_calls{},appends{},growths{};
};
inline thread_local NativeTu15PhantomCreationActivity native_tu15_phantom_creation_counters;
inline NativeTu15PhantomCreationActivity native_tu15_phantom_creation_activity() noexcept {
    return native_tu15_phantom_creation_counters;
}
} // namespace halo2
