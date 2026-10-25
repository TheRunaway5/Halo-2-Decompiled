#pragma once
#include <atomic>
#include <cstdint>

namespace halo2 {
struct SourceDifficultyParameterNativeActivity {
    std::uint64_t parameter_calls{}, current_difficulty_calls{};
};
namespace difficulty_parameter_native_detail {
// Process diagnostics only; never serialized or consumed by gameplay.
inline std::atomic<std::uint64_t> parameter_calls{}, current_difficulty_calls{};
}
inline SourceDifficultyParameterNativeActivity source_difficulty_parameter_native_activity() {
    using namespace difficulty_parameter_native_detail;
    return {parameter_calls.load(std::memory_order_relaxed),
        current_difficulty_calls.load(std::memory_order_relaxed)};
}
}
