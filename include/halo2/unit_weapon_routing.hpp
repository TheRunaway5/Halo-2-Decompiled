#pragma once
#include "halo2/physics_contact_runtime.hpp"
namespace halo2 {
// Complete TU1.1 inventory, equip/put-away, primary/secondary reload action,
// animation request and item-owner/drop routing bodies. The caller must supply
// genuinely retained unit/item/manager/extension/local/world banks and every
// reached lower owner. This does not construct a live Unit or admit a callback
// by default. All registers, stack, flags and floating state are retained.
void source_run_unit_weapon_routing_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
