#pragma once
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// E5680 and1F08E0 consume the actual biped/contact packet and source clock.
// Native grounded flags do not substitute for either source predicate.
bool source_biped_airborne_restriction_1_1(const SourcePhysicsContactMemory &,
                                           std::uint32_t object_datum,
                                           SourcePhysicsFactoryFloatingPoint);
bool source_biped_contact_ragdoll_request_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t contact_packet,
    SourcePhysicsFactoryFloatingPoint);
// Original1EFFe0 returns XMM0: elapsed source-clock fraction, clamped0..1.
std::uint32_t
source_biped_constraint_elapsed_1_1(const SourcePhysicsContactMemory &,
                                    std::uint32_t physical_state,
                                    SourcePhysicsFactoryFloatingPoint);
// Complete1D6FC0 authored-property search, limits, friction and3190E0 island
// requirement removal/re-admission. Scratch is its real16-byte caller packet.
void source_update_biped_ragdoll_constraint_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t constraint_index, std::uint32_t scale_word,
    std::uint32_t requirements_scratch, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// CompleteE2430 over the currently admitted body/constraint bank and source
// death clock. The scalar selection uses the object's actual datum low2 bits.
void source_update_biped_death_constraints_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t requirements_scratch, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
