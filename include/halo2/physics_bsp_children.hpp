#pragma once
#include "halo2/physics_current_broadphase.hpp"

namespace halo2 {
// Original nonstandard register ABIs are explicit. An unavailable owner must
// stop; a callback cannot admit geometry, keys or contacts with a default result.
struct SourcePhysicsBspOwnerCall {
    std::uint32_t instruction,entry,ecx,entry_esp;
    // Only source-owned register arguments are present. Unrepresented preserved
    // registers are absent, never fabricated zero/default state.
    std::optional<std::uint32_t> eax,edx,esi,edi;
    std::vector<std::uint32_t> arguments;
};
struct SourcePhysicsBspChildServices {
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsBspOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
};
// Complete2E0A60 relative transform. All four lanes and source load/store
// aliasing/order are retained; neither an inverse nor a pose is fabricated.
void source_physics_relative_transform_1_1(SourcePhysicsContactMemory &,
    std::uint32_t output,std::uint32_t left,std::uint32_t right,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete305E40 resident MOPP bytecode traversal. Actual classifier/local
// backing, MOPP bytes, query context and real output bank are required. Reserved
// opcodes retain a required original diagnostic owner, never a no-hit fallback.
void source_query_physics_mopp_1_1(SourcePhysicsContactMemory &,
    std::uint32_t context,std::uint32_t classifier,std::uint32_t bytecode,
    std::uint32_t entry_esp,const SourcePhysicsBspChildServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
//3066E0 quantizes real source bounds with original x87 spill/FISTP ordering
//and initializes the MOPP query. entry_esp owns temporary classifier aliases.
void source_initialize_physics_mopp_query_1_1(SourcePhysicsContactMemory &,
    std::uint32_t context,std::uint32_t mopp,std::uint32_t bounds,
    std::uint32_t output_descriptor,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsBspChildServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete2E5BC0 wrapper; no additional query state or keys are supplied.
void source_query_physics_bsp_mopp_1_1(SourcePhysicsContactMemory &,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t output_descriptor,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsBspChildServices & = {},const SourcePhysicsContactRecordObserver & = {});
//1F2DB0 stored-state encoded child eligibility, including source switch bank
//and object header/remap reads. Instanced switch185AA0 remains required.
bool source_physics_bsp_child_enabled_1_1(SourcePhysicsContactMemory &,
    std::uint32_t key,std::uint32_t entry_esp,
    const SourcePhysicsBspChildServices & = {},const SourcePhysicsContactRecordObserver & = {});
// Complete1F26B0 preserves original query order and removes only source-disabled
//keys. This is not sorting, broadphase admission or a native chosen filter.
void source_filter_physics_bsp_children_1_1(SourcePhysicsContactMemory &,
    std::uint32_t shape,std::uint32_t output_descriptor,std::uint32_t entry_esp,
    const SourcePhysicsBspChildServices & = {},const SourcePhysicsContactRecordObserver & = {});
// Complete1F16F0 triangle constructor follows authored BSP surface/edge/vertex
//banks. The source optional transform is explicit; no collision fallback exists.
std::uint32_t source_construct_physics_bsp_triangle_1_1(SourcePhysicsContactMemory &,
    std::uint32_t output,std::uint32_t transform,
    const std::array<std::uint32_t,5> &arguments,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Measured1F3320 class1 terrain child path, plus its actual disabled/null path.
//Other original classes stop at their unmeasured branch; no borrowed shape.
std::uint32_t source_physics_bsp_child_shape_1_1(SourcePhysicsContactMemory &,
    std::uint32_t container,std::uint32_t key,std::uint32_t output,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsBspChildServices & = {},const SourcePhysicsContactRecordObserver & = {});
// Measured1F2420 terrain query path composes the real MOPP producer/filter.
//Its instanced-geometry branch stops before unported source branch stores.
void source_query_physics_bsp_children_1_1(SourcePhysicsContactMemory &,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t output_descriptor,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsBspChildServices & = {},const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
