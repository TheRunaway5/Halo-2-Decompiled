#pragma once
#include "halo2/physics_child_agents.hpp"

namespace halo2 {
using SourcePhysicsNarrowphaseServices = SourcePhysicsChildAgentServices;
// Complete2E1430 reads the retained shape-pair factory table. Its alternate
// 2FFE20 owner is required; no native chosen admission result is available.
std::uint32_t source_admit_physics_narrowphase_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,4> &,std::uint32_t entry_esp,
    const SourcePhysicsNarrowphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete2E0C70 rotation and309860 result-packet owner. Real original workspace,
// support-state, result vector and output aliases are explicit input storage.
void source_rotate_physics_contact_vector_1_1(SourcePhysicsContactMemory &,
    std::uint32_t output,std::uint32_t matrix,std::uint32_t vector,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_emit_physics_narrowphase_result_1_1(SourcePhysicsContactMemory &,
    std::uint32_t workspace,const std::array<std::uint32_t,3> &,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete2F4BD0 separation-cache/result orchestration.30AE40 is a genuine
// required GJK owner. This module supplies neither its result nor a contact.
std::uint32_t source_update_physics_separation_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,5> &,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsNarrowphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// The2F4D0A continuation consumes the actual completed30AE40 workspace, support
// state, result-vector storage and returned EAX. It does not construct these
// inputs. This boundary permits original-output tests without a replay service.
std::uint32_t source_finish_physics_separation_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,5> &,
    std::uint32_t original_entry_esp,std::uint32_t gjk_result,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete2E14E0 wrapper-motion/support orchestration. Motion/property admission,
// unsupported shape methods and unported narrowphase remain required owners.
std::uint32_t source_update_physics_motion_narrowphase_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,5> &,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsNarrowphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
//2E17ED continuation requires the actual completed child separation owner's
// retained frame byte/flags and saved step byte. It supplies no lower result.
std::uint32_t source_finish_physics_motion_narrowphase_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,5> &,
    std::uint32_t original_entry_esp,const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
