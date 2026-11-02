#pragma once
#include "halo2/native_tu15_overlap_query_binding.hpp"
#include "halo2/native_tu15_convex_simplex_solver_binding.hpp"
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
struct NativeTu15OverlapQueryHandoffPolicy {
    std::uint32_t stack_first{},stack_end{},data_first{},data_bytes{};
    bool observers_read_only{};
    const bool*first_trace_active{};
    const bool*second_trace_active{};
    std::function<void(std::uint32_t)> instruction;
};
struct NativeTu15OverlapQueryHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15OverlapQueryBindingStats queries;
    NativeTu15ConvexSimplexBindingActivity simplex;
};
// Actual2F4D3C virtual+4 ->2F73C0, thiscall4/RET16. The adapter reads ordinary
// current inputs and updates only the void-call continuation and native FP
// fields. Kernels own their DATA; no original executor/result is available.
bool native_tu15_handoff_overlap_query(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    NativeTu15OverlapQueryHandoffPolicy,
    const std::function<std::uint64_t()>&timestamp_counter={},
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15OverlapQueryHandoffActivity* = nullptr);
} // namespace halo2
