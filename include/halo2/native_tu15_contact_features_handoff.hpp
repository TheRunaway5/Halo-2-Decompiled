#pragma once
#include "halo2/native_tu15_contact_features.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <functional>
#include <thread>
namespace halo2 {
struct NativeTu15ContactFeaturesHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{};
    std::uint64_t first_shape_calls{},second_shape_calls{};
    std::uint64_t swept_calls{},soa_calls{},box_calls{},capsule_calls{},polygon_calls{},sphere_calls{};
};
struct NativeTu15ContactFeaturesHandoffPolicy {
    std::uint32_t stack_first{},stack_end{};
    bool observers_read_only{};
    const bool*first_trace_active{};
    const bool*second_trace_active{};
    std::function<void(std::uint32_t)> instruction;
};
// This registry owns only the memory/executor and immutable admission policy.
// It retains no GP/SIMD postimage, stack image, physics outcome or TLS state.
class NativeTu15ContactFeaturesHandoffBinding {
    SourcePhysicsContactMemory*memory_;
    NativeTu15ContactFeaturesHandoffPolicy policy_;
    NativeTu15ContactFeaturesHandoffActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_handoff_contact_features(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
        const SourcePhysicsContactRecordObserver&);
public:
    NativeTu15ContactFeaturesHandoffBinding(SourcePhysicsContactMemory&,
        NativeTu15ContactFeaturesHandoffPolicy,
        NativeTu15ContactFeaturesHandoffActivity* = nullptr);
    ~NativeTu15ContactFeaturesHandoffBinding();
    NativeTu15ContactFeaturesHandoffBinding(const NativeTu15ContactFeaturesHandoffBinding&)=delete;
    NativeTu15ContactFeaturesHandoffBinding&operator=(const NativeTu15ContactFeaturesHandoffBinding&)=delete;
};
// Only the two TU1.5 30E930 virtual34 calls admit this void RET12 adapter.
// False means no explicit binding exists. Once bound, unsupported inputs fail
// explicitly and never select an original implementation as a fallback.
bool native_tu15_handoff_contact_features(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
