#pragma once
#include "halo2/native_tu15_body_deactivation_policy.hpp"
namespace halo2 {
struct NativeTu15GameplayDeactivationPolicyStats {
    std::uint64_t calls{},blocked{},vehicle_tests{},biped_tests{},body_policies{};
};
// Actual1D9B20/1D9B70 and their closed gameplay eligibility chain
//1D9BC0/F20C0/E7790/E65A0. The two output bytes are caller-owned DATA;
// there is no guest CPU state, captured result or original-execution service.
bool native_tu15_gameplay_deactivation_policy(SourcePhysicsContactMemory&,
    std::uint32_t policy,NativeTu15PhysicsBody,NativeTu15DeactivationPeriod,
    std::uint32_t output,std::uint32_t temporary_output,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15GameplayDeactivationPolicyStats* = nullptr);
} // namespace halo2
