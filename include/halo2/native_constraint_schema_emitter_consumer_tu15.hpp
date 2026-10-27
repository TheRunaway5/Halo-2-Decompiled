#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <functional>
#include <thread>
namespace halo2 {
struct NativeConstraintSchemaEmitterActivityTu15 {
    std::uint64_t calls{},completed_calls{},live_cursor_calls{},refusals{};
    std::array<std::uint64_t,3>caller_kinds{};
    std::uint64_t observation_refusals{},caller_refusals{},stack_refusals{},executor_refusals{},image_refusals{};
};
struct NativeConstraintSchemaEmitterConsumerPolicyTu15 {
    std::uint32_t caller_stack_begin{},caller_stack_end{};
    // Actual parent callsite provenance, queried before publishing leaf entry.
    // This is the world's real last-instruction cursor, not a supplied result.
    std::function<std::uint32_t()>current_actual_phase;
    std::function<void(std::uint32_t actual_instruction)>publish_phase;
    const bool*first_trace{};const bool*second_trace{};
    bool observation_only_stores{};
    NativeConstraintSchemaEmitterActivityTu15*world_activity{};
};
class NativeConstraintSchemaEmitterConsumerScopeTu15 {
    SourcePhysicsContactMemory*memory_;NativeConstraintSchemaEmitterConsumerPolicyTu15 policy_;
    std::thread::id executor_;NativeConstraintSchemaEmitterConsumerScopeTu15*previous_{};
    friend const NativeConstraintSchemaEmitterConsumerPolicyTu15*native_constraint_schema_emitter_policy_tu15(const SourcePhysicsContactMemory&);
public:
    NativeConstraintSchemaEmitterConsumerScopeTu15(SourcePhysicsContactMemory&,NativeConstraintSchemaEmitterConsumerPolicyTu15);
    ~NativeConstraintSchemaEmitterConsumerScopeTu15();
    NativeConstraintSchemaEmitterConsumerScopeTu15(const NativeConstraintSchemaEmitterConsumerScopeTu15&)=delete;
    NativeConstraintSchemaEmitterConsumerScopeTu15&operator=(const NativeConstraintSchemaEmitterConsumerScopeTu15&)=delete;
};
enum class NativeConstraintSchemaEmitterRefusalTu15 {Observation,Caller,Stack,Executor,Image};
const NativeConstraintSchemaEmitterConsumerPolicyTu15*native_constraint_schema_emitter_policy_tu15(const SourcePhysicsContactMemory&);
bool native_constraint_schema_emitter_bank_scoped_tu15(const SourcePhysicsContactMemory&);
NativeConstraintSchemaEmitterActivityTu15 native_constraint_schema_emitter_activity_tu15(const SourcePhysicsContactMemory&);
void native_constraint_schema_emitter_activity_reset_tu15(const SourcePhysicsContactMemory&);
void native_constraint_schema_emitter_call_tu15(const SourcePhysicsContactMemory&,unsigned caller);
void native_constraint_schema_emitter_completed_tu15(const SourcePhysicsContactMemory&);
void native_constraint_schema_emitter_refused_tu15(const SourcePhysicsContactMemory&,NativeConstraintSchemaEmitterRefusalTu15);
} // namespace halo2
