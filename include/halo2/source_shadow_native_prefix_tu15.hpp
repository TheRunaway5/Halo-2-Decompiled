#pragma once
#include <cstdint>

namespace halo2 {
// Diagnostic counts for the current executor thread. These are not source
// memory, gameplay counters, or a substitute for original-query acceptance.
struct SourceShadowNativePrefixTu15Stats {
    std::uint64_t attempts{},admitted{},declined_reference{},
        declined_instruction{},declined_view{},declined_numeric{};
};
SourceShadowNativePrefixTu15Stats source_shadow_native_prefix_tu15_stats() noexcept;
void source_reset_shadow_native_prefix_tu15_stats() noexcept;
}
