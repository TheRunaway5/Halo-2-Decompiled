#pragma once
#include "halo2/physics_gjk.hpp"

namespace halo2 {
// Retained source storage, not a captured contact or chosen backend success.
// Every callback must execute its real owning implementation or fail.
struct SourcePhysicsContactBatchOwnerCall {
    std::uint32_t instruction,entry,entry_esp;
    // Absent for cdecl owners whose ECX is merely an unowned scratch register.
    std::optional<std::uint32_t> ecx;
    std::vector<std::uint32_t> arguments;
};
struct SourcePhysicsContactBatchServices {
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsContactBatchOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
};

// Complete original128-byte packet copy: holes28..2F and7E..7F remain untouched.
std::uint32_t source_copy_physics_contact_packet_1_1(SourcePhysicsContactMemory &,
    std::uint32_t destination,std::uint32_t source,
    const SourcePhysicsContactRecordObserver & = {});
// Complete scratch-bank allocation/disposal and packet append. Required source
// adapter virtual allocation/free and growth remain explicit; no address replay.
std::uint32_t source_construct_physics_contact_scratch_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},const SourcePhysicsContactRecordObserver & = {});
void source_dispose_physics_contact_scratch_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_append_physics_contact_packet_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},const SourcePhysicsContactRecordObserver & = {});
// Complete2E0BA0 local-to-world point transform and31FEB0 point velocity.
void source_transform_physics_contact_point_1_1(SourcePhysicsContactMemory &,
    std::uint32_t output,std::uint32_t matrix,std::uint32_t point,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
void source_physics_contact_point_velocity_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion,std::uint32_t point,std::uint32_t output,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete300650 packet pruning, including literal128-byte swap/holes and
// original caller-argument alias.2FFD60 is a required geometry owner; the
// genuine measured range is empty, so its positive branches remain listed-only.
std::uint32_t source_prune_physics_contact_packets_1_1(SourcePhysicsContactMemory &,
    std::uint32_t parent,const std::array<std::uint32_t,5> &,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete309210 retained contact selection/append. Manager+4 and five-point
// reduction are required actual owners. No synthetic contact/index is returned.
void source_retain_physics_contact_packet_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,7> &,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete309450 output retention/retirement. Real grow and manager+8 providers
// are required when reached; contact lifetime is owned by its original banks.
void source_emit_retained_physics_contacts_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,8> &,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Entire31F1B0 source body; native comparisons currently end at313800.
// Its later tail has not completed natively. Constraint/world insertion313800, point/scalar
// slot allocation322410 and world/entity listeners are required actual owners.
// No live callback consumes a captured workspace, bank or successful result.
std::uint16_t source_create_physics_managed_contact_1_1(SourcePhysicsContactMemory &,
    std::uint32_t manager,const std::array<std::uint32_t,4> &,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
