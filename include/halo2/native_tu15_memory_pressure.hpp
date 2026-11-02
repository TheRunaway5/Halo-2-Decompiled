#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15MemoryPressureCandidate {
    std::uint32_t mode{},excluded{0xffffffffu};
    bool final_pass{},aggressive{},recent_cleanup{};
};
struct NativeTu15MemoryPressureServices {
    std::function<void(bool,NativeTu15PhysicsFrameFp&)> trim_objects; //1CD130
    std::function<bool(NativeTu15MemoryPressureCandidate,NativeTu15PhysicsFrameFp&)> remove_candidate; //1CCBE0
    std::function<void(NativeTu15PhysicsFrameFp&)> begin_cleanup; //1CB330
    std::function<void(NativeTu15PhysicsFrameFp&)> finish_cleanup; //1CB220
};
struct NativeTu15MemoryPressureStats {
    std::uint64_t calls{},reentrant_skips{},level_queries{},pool_queries{},restorations{},
        trims{},candidate_calls{},candidate_removals{},cleanup_batches{},unbound_services{};
};
// Typed149AD0/2335B0. Preserve actual counter reads, modular integer sums,
// scalar SSE operations/comparisons and MXCSR; live x87 values are untouched.
std::uint32_t native_tu15_memory_pressure_level(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,NativeTu15MemoryPressureStats* = nullptr);
std::uint32_t native_tu15_pool_pressure_metric_bits(SourcePhysicsContactMemory&,
    std::uint32_t pool,NativeTu15PhysicsFrameFp&,NativeTu15MemoryPressureStats* = nullptr);
// Actual149680 returns the independently constructed emergency allocator.
std::uint32_t native_tu15_restore_emergency_allocator(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MemoryPressureStats* = nullptr);
// Actual1498A0 and1496F0. Remaining object services are explicit typed owners;
// absent services throw at their source boundary without a parent fallback.
void native_tu15_collect_memory_pressure_batch(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,const NativeTu15MemoryPressureServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MemoryPressureStats* = nullptr);
void native_tu15_collect_memory_pressure(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,const NativeTu15MemoryPressureServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MemoryPressureStats* = nullptr);
}
