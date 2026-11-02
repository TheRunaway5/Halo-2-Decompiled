#pragma once
#include "halo2/native_tu15_activation_contact_manager.hpp"
namespace halo2 {
// Actual31F660 through the existing semantic collector owner. The appended
// definition in activation_contact_manager.cpp is staged until direct proof;
// it does not duplicate collector storage/body/listener destruction logic.
void native_tu15_destroy_activation_collector(SourcePhysicsContactMemory&,
    std::uint32_t collector,NativeTu15PhysicsFrameFp&,
    const NativeTu15ActivationContactManagerServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationContactManagerStats* = nullptr);
} // namespace halo2
