#pragma once
#include "halo2/native_tu15_activation_collector_cleanup.hpp"

namespace halo2 {
// Actual3261D0, table41F478 virtual0, RET4(disposing). Reuses the sole native
//31F660 collector lifetime and returns the same ordinary collector identity.
std::uint32_t native_tu15_delete_contact_collector(SourcePhysicsContactMemory&,
    std::uint32_t collector,std::uint32_t disposing,NativeTu15PhysicsFrameFp&,
    const NativeTu15ActivationContactManagerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ActivationContactManagerStats* = nullptr);
} // namespace halo2
