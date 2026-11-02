#pragma once
#include "halo2/native_tu15_simplex_triangle.hpp"
#include "halo2/native_tu15_closest_segments.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <functional>
#include <thread>
namespace halo2 {
struct NativeTu15SimplexHelpersHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{};
    std::uint64_t triangle_calls{},segment_calls{};
    std::uint64_t triangle_30dad9_calls{},triangle_30e3b6_calls{},triangle_30eb85_calls{},triangle_30ece2_calls{},segment_30eacc_calls{};
};
struct NativeTu15SimplexHelpersHandoffPolicy {
    std::uint32_t stack_first{},stack_end{};
    bool observers_read_only{};
    const bool*first_trace_active{};
    const bool*second_trace_active{};
    std::function<void(std::uint32_t)> instruction;
};
// This registry owns only the memory/executor and immutable admission policy.
// It retains no GP/SIMD postimage, stack image, physics outcome or TLS state.
class NativeTu15SimplexHelpersHandoffBinding {
    SourcePhysicsContactMemory*memory_;
    NativeTu15SimplexHelpersHandoffPolicy policy_;
    NativeTu15SimplexHelpersHandoffActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_handoff_simplex_helpers(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
        const SourcePhysicsContactRecordObserver&);
public:
    NativeTu15SimplexHelpersHandoffBinding(SourcePhysicsContactMemory&,
        NativeTu15SimplexHelpersHandoffPolicy,
        NativeTu15SimplexHelpersHandoffActivity* = nullptr);
    ~NativeTu15SimplexHelpersHandoffBinding();
    NativeTu15SimplexHelpersHandoffBinding(const NativeTu15SimplexHelpersHandoffBinding&)=delete;
    NativeTu15SimplexHelpersHandoffBinding&operator=(const NativeTu15SimplexHelpersHandoffBinding&)=delete;
};
// Only five qualified TU1.5 solver calls admit this typed EAX adapter.
// Triangle calls complete RET12; the cdecl segment call completes RET0.
// False means no explicit binding exists. Once bound, unsupported inputs fail
// explicitly and never select an original implementation as a fallback.
bool native_tu15_handoff_simplex_helpers(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
