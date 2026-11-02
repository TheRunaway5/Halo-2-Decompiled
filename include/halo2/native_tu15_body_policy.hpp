#pragma once
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_shape_bounds.hpp"
namespace halo2 {
struct NativeTu15BodyPolicyScratch {NativeTu15BoundsOutput bounds;};
struct NativeTu15BodyPolicyServices {
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ShapeBoundsServices bounds;
    std::function<std::uint32_t(std::uint32_t policy,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsFrameFp&)>policy_kind;
    std::function<void(std::uint32_t policy,NativeTu15PhysicsVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&)>destroy_policy;
    std::function<void(std::uint32_t object,std::uint32_t flags,
        NativeTu15PhysicsFrameFp&)>release_base_lock;
};
struct NativeTu15BodyPolicyStats {
    std::uint64_t calls{},same_kind{},native_type_queries{},unconverted_type_queries{},
        constructors{},shape_bounds_calls{},installs{},default_installs{},
        protected_references{},native_destructions{},unconverted_destructions{},
        storage_releases{},base_lock_releases{};
    std::uint64_t gameplay_destructions{};
};
// Actual31B730,31E820/31E760,1E10A0 and31E7F0. The caller retains32 aligned
// ordinary bound bytes across the real shape service. Native SIMD/x87 own
// the constructor extent computation; no GP/SIMD frame enters this kernel.
void native_tu15_select_body_policy(SourcePhysicsContactMemory&,NativeTu15PhysicsBody,
    std::uint32_t kind,NativeTu15BodyPolicyScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15BodyPolicyServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BodyPolicyStats* = nullptr);
} // namespace halo2
