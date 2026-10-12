#pragma once
#include "halo2/physics_broadphase.hpp"
#include <optional>

namespace halo2 {
struct SourcePhysicsPairOwnerCall {
    std::uint32_t entry,receiver;
    std::vector<std::uint32_t> arguments;
    bool cdecl;
    // Actual source callback ABI frame, when supplied by its owning caller.
    std::uint32_t entry_esp{};
};
struct SourcePhysicsPairManagerServices {
    SourcePhysicsBroadphaseServices scratch;
    // Actual unrepresented virtual/direct owner, with its literal arguments.
    // This must execute a real provider, never choose filter/agent success or
    // return a recorded contact. Absent ownership throws at the actual call.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsPairOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
    std::optional<SourcePhysicsFactoryFloatingPoint> floating_point;
};
struct SourcePhysicsPairManagerScratch {
    // Original routine entry ESP, backed by the caller's temporary ABI region.
    //378EB0 uses+4/+14;31D360 validator uses+4. No body/contact identity.
    std::uint32_t entry_esp;
};
// Complete2E5F50 layer/group predicate, from actual stored collidable keys and
// filter rows. It writes its source byte; no candidate/contact is supplied.
std::uint32_t source_filter_physics_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t filter,std::uint32_t output,std::uint32_t a,std::uint32_t b,
    const SourcePhysicsContactRecordObserver & = {});
void source_append_physics_static_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t list,std::uint32_t pair,
    const SourcePhysicsContactRecordObserver & = {});
void source_remove_physics_static_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t list,std::uint32_t pair,const SourcePhysicsContactRecordObserver & = {});
void source_deliver_physics_pairs_1_1(SourcePhysicsContactMemory &,
    std::uint32_t manager,const std::array<std::uint32_t,5> &arguments,
    const SourcePhysicsPairManagerScratch &,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_notify_physics_collidable_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t pair,bool removed,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_add_physics_entity_collidable_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entity,std::uint32_t collidable,std::uint32_t check,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
void source_remove_physics_entity_collidable_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entity,std::uint32_t collidable,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Exact source-owned constraint/listener storage, real small-block allocator
// and retained contact manager/agent. These do not run a solver or manufacture
// a current contact. Unknown retirement/virtual classes remain required.
std::uint32_t source_construct_physics_pair_constraint_1_1(SourcePhysicsContactMemory &,
    std::uint32_t constraint,const SourcePhysicsContactRecordObserver & = {});
void source_bind_physics_pair_constraint_body_1_1(SourcePhysicsContactMemory &,
    std::uint32_t constraint,std::uint32_t body,bool second,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_pair_contact_manager_1_1(SourcePhysicsContactMemory &,
    std::uint32_t manager,const std::array<std::uint32_t,3> &arguments,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_allocate_physics_pair_contact_manager_1_1(SourcePhysicsContactMemory &,
    std::uint32_t factory,const std::array<std::uint32_t,3> &arguments,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_pair_agent_base_1_1(SourcePhysicsContactMemory &,
    std::uint32_t agent,std::uint32_t manager,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_allocate_physics_pair_agent_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,4> &arguments,
    const SourcePhysicsContactRecordObserver & = {});
// Actual2F3730 transformed-shape agent. The temporary collision input and
// composed transform are caller ABI storage; only the allocated wrapper and
// its recursively selected child agent are retained.
std::uint32_t source_allocate_physics_transformed_pair_agent_1_1(
    SourcePhysicsContactMemory &,const std::array<std::uint32_t,4> &,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_validate_physics_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t a,std::uint32_t b,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_write_physics_pair_validation_1_1(SourcePhysicsContactMemory &,
    std::uint32_t validator,std::uint32_t output,std::uint32_t a,std::uint32_t b,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_island_pair_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,const SourcePhysicsPairManagerServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_island_pair_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,const SourcePhysicsPairManagerServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
void source_order_physics_pairs_1_1(SourcePhysicsContactMemory &,std::uint32_t pairs,
    std::uint32_t count,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete31D980 storage/delivery owner. frame is the source ESP at31D9A7;
// descriptor=frame+C,378EB0 entry=frame-1C. Required storage must be backed.
void source_update_physics_island_pairs_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,const std::array<std::uint32_t,4> &arguments,
    std::uint32_t frame,const SourcePhysicsPairManagerServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_append_physics_island_pair_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island_list,std::uint32_t pair,const SourcePhysicsPairManagerScratch &,
    const SourcePhysicsPairManagerServices &,const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
