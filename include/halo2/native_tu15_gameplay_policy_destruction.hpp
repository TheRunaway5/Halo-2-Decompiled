#pragma once
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
struct NativeTu15GameplayPolicyDestructionServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<void(std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)>release_base_lock;
};
// Actual1D9C80: inline base retirement, optional source lock release, and the
// actual allocator's sized release. The ordinary return value is the policy.
std::uint32_t native_tu15_destroy_gameplay_body_policy(SourcePhysicsContactMemory&,
    std::uint32_t policy,std::uint32_t disposing,NativeTu15PhysicsFrameFp&,
    const NativeTu15GameplayPolicyDestructionServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
