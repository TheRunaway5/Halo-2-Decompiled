#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete paired direct dispatcher D9180, amount DB160, target-player D90E0,
// player scale 15F520, actor scale 1CBE30, campaign/team scaling, registered
// Slayer/King strength callbacks, and actual BBCB0/D7160 lookups.
// The caller owns one real request, header/body/tag/player/actor/options/RNG
// bank and the full original CPU/stack state. Unimplemented region, effect,
// reaction, physics and delivery owners retain their actual pending call ABI.
void source_run_direct_damage_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
