#pragma once
#include "halo2/native_tu15_activation_collector_cleanup.hpp"
namespace halo2 {
struct NativeTu15ContactManagerCleanupScratch {std::uint32_t result_byte{};};
struct NativeTu15ContactManagerCleanupServices {
    NativeTu15ActivationContactManagerServices collector;
    // Actual316AA0, RET8(result-byte DATA, collector). It writes the one-byte
    // result completely before returning; the destructor does not consume it.
    std::function<void(std::uint32_t world,std::uint32_t result_byte,
        std::uint32_t collector,NativeTu15PhysicsFrameFp&)> update_world;
};
// Actual3231B0. The result byte is ordinary caller-owned DATA only; it is
// required solely on the positive+A4 branch. No control-frame identity is used.
void native_tu15_destroy_contact_manager(SourcePhysicsContactMemory&,
    std::uint32_t manager,NativeTu15ContactManagerCleanupScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactManagerCleanupServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationContactManagerStats* = nullptr);
// Actual3232B0, normal scalar deleting result equals manager; RET4(flag).
std::uint32_t native_tu15_delete_contact_manager(SourcePhysicsContactMemory&,
    std::uint32_t manager,std::uint32_t disposing,NativeTu15ContactManagerCleanupScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactManagerCleanupServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationContactManagerStats* = nullptr);
} // namespace halo2
