#pragma once
#include "halo2/native_tu15_object_activation_residuals.hpp"
#include "halo2/physics_contact_runtime.hpp"
namespace halo2 {
struct NativeTu15ObjectVelocityTransportPolicy {
    std::uint32_t caller_stack_begin{},caller_stack_end{};
    std::uint32_t scratch_first{},scratch_bytes{};
    bool observation_only_stores{};
    const bool*first_trace{};const bool*second_trace{};
    std::function<void(std::uint32_t actual_instruction)>publish_phase;
};
using NativeTu15ObjectVelocityOwnerExecutor=std::function<void(std::uint32_t owner,
    SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&)>;
struct NativeTu15ObjectVelocityTransportActivity {
    std::uint64_t attempts{},completed{},refusals{},unqualified_owner_calls{};
    NativeTu15ObjectActivationBindingActivity binding;
};
// The legacy ABI adapter is outside the semantic kernels. It accepts only the
// authenticated F2091 packet, uses the caller's declared unused stack arena,
// executes genuine remaining owners, and preserves outer GP/SIMD values.
// Only real FP state and source RET4/SP+8 are published at the void boundary.
void native_tu15_run_object_velocity_handoff(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const NativeTu15ObjectVelocityTransportPolicy&,
    const NativeTu15ObjectVelocityOwnerExecutor&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectVelocityTransportActivity* = nullptr);
} // namespace halo2
