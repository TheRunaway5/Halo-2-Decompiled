#pragma once
#include "halo2/physics_bsp_children.hpp"

namespace halo2 {
// Real source storage/virtual dependencies. No geometry, contact or successful
// callback is supplied by this module when its owning producer is unavailable.
using SourcePhysicsChildAgentServices = SourcePhysicsBspChildServices;
// Complete307FE0 returns the original retained null-agent singleton identity.
std::uint32_t source_null_physics_child_agent_1_1();
void source_sort_physics_child_keys_1_1(SourcePhysicsContactMemory &,
    std::uint32_t keys,std::uint32_t left,std::uint32_t right,std::uint32_t entry_esp,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_child_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,std::uint32_t entry_esp,
    const SourcePhysicsChildAgentServices & = {},const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_child_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t entry_esp,
    const SourcePhysicsChildAgentServices & = {},const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_filter_physics_child_layers_1_1(SourcePhysicsContactMemory &,
    std::uint32_t filter,std::uint32_t output,std::uint32_t a,std::uint32_t b,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_filter_physics_child_1_1(SourcePhysicsContactMemory &,
    std::uint32_t filter,const std::array<std::uint32_t,6> &arguments,
    std::uint32_t entry_esp,const SourcePhysicsChildAgentServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete305560. Real shape center/support/type methods are dispatched from
// retained vtables. Convex SoA, wrapper, capsule and BSP triangle methods are represented;
// other actual virtuals require their own producer. No substitute support point.
void source_initialize_physics_support_state_1_1(SourcePhysicsContactMemory &,
    std::uint32_t state,const std::array<std::uint32_t,3> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsChildAgentServices & = {},const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_child_agent_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,3> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsChildAgentServices & = {},const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_allocate_physics_child_agent_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,4> &arguments,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsChildAgentServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_dispatch_physics_child_agent_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,4> &arguments,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsChildAgentServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
//2EFC30 query/cache orchestration. Bounds, child definitions, layer filtering,
//allocations and retained cache rows are produced by the original owner chain.
//The alternate incremental mode remains an explicit unmeasured owner boundary.
void source_update_physics_child_cache_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,const std::array<std::uint32_t,4> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsChildAgentServices & = {},const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
