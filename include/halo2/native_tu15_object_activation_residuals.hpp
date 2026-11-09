#pragma once
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/native_tu15_activation_agents.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_activation_payload_filter.hpp"
#include "halo2/native_tu15_activation_contact_manager.hpp"
#include "halo2/native_tu15_body_policy.hpp"
#include "halo2/native_tu15_body_constraint_update.hpp"
#include "halo2/native_tu15_model_constraint_lookup.hpp"
#include "halo2/native_tu15_model_constraint_capacity.hpp"
#include "halo2/native_tu15_model_constraint_removal.hpp"
#include "halo2/native_tu15_constraint_runtime_handles.hpp"
#include "halo2/native_tu15_constraint_handle_destruction.hpp"
#include "halo2/native_tu15_constraint_tag_values.hpp"
#include "halo2/native_tu15_model_constraint_creation.hpp"
#include "halo2/native_tu15_constraint_runtime_update.hpp"
#include "halo2/native_tu15_constraint_effect_selector.hpp"
#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include <map>
#include <optional>
#include <span>
namespace halo2 {
// These are ordinary caller-owned result bytes, not a retained CPU frame.
struct NativeTu15ObjectActivationScratch {
    std::uint32_t filter_result{},discrete_result{},policy_bounds{},shape_scratch_first{},shape_scratch_bytes{};
};
// Only genuine remaining work crosses this boundary. The binding never owns
// GP/SIMD registers or supplies their old postimages. RET-pop is an explicit
// callback ABI contract for the external owner adapter to verify.
using NativeTu15ObjectActivationResidualCall=std::function<std::uint32_t(
    std::uint32_t owner,std::span<const std::uint32_t> arguments,
    std::optional<std::uint32_t> receiver,unsigned return_pop,
    NativeTu15PhysicsFrameFp&)>;
// A native caller can supply its already-qualified recursive factory graph.
// Inputs are ordinary semantic DATA and actual FP/depth, never GP registers.
struct NativeTu15ObjectActivationNativeServices {
    std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        NativeTu15ActivationContactManager,NativeTu15PhysicsFrameFp&,unsigned)> create_agent;
};
struct NativeTu15ObjectActivationBindingActivity {
    std::uint64_t native_bound_agent_factory_attempts{},native_bound_agent_factory_calls{},native_bound_agent_factory_refusals{};
    std::uint64_t setters{},native_agent_factories{},native_shape_type_getters{},
        native_motion_type_getters{},native_policy_type_getters{},native_manager_factories{},unconverted_manager_factories{},native_array_growths{},
        native_pool_allocations{},native_pool_releases{},unconverted_shape_type_getters{},
        unconverted_motion_type_getters{},unconverted_policy_type_getters{},
        unconverted_agent_factories{},residual_calls{};
    NativeTu15IslandActivationStats activation;
    NativeTu15ActivationAgentStats agents;
    NativeTu15ActivationServiceStats services;
    NativeTu15ActivationPayloadFilterStats payload_filter;
    NativeTu15ActivationContactManagerStats managers;
    NativeTu15BodyPolicyStats body_policy;
    NativeTu15ModelConstraintUpdateStats body_constraints;
    NativeTu15ModelConstraintLookupStats constraint_lookup;
    NativeTu15ModelConstraintCapacityStats constraint_capacity;
    NativeTu15ModelConstraintRemovalStats constraint_removal;
    NativeTu15ConstraintRuntimeHandleStats constraint_runtime_handles;
    NativeTu15ConstraintHandleDestructionStats constraint_handle_destruction;
    NativeTu15ConstraintTagValueStats constraint_tag_values;
    NativeTu15ModelConstraintCreationStats constraint_creation;
    NativeTu15ConstraintRuntimeUpdateStats constraint_runtime_update;
    NativeTu15ConstraintEffectSelectorStats constraint_effect_selector;
    std::uint64_t unavailable_constraint_runtime_storage{},unqualified_constraint_runtime_profiles{};
    std::uint64_t unavailable_constraint_creation_storage{};
    std::map<std::uint32_t,std::uint64_t> native_policy_bounds_owners,unconverted_policy_bounds_owners;
    std::map<std::uint32_t,std::uint64_t> residual_owners;
};
// Native B9800 and the qualified activation/service dependency closure. It
// retains the remaining factory/query/policy/body-update owners explicitly.
void native_tu15_set_object_velocities_with_residuals(SourcePhysicsContactMemory&,
    std::uint32_t object,const Vec3*linear,const Vec3*angular,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    NativeTu15ObjectActivationScratch,const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectActivationBindingActivity* = nullptr);
// Actual316D40 using the SAME native agents/body-listener/constraint owner
// as B9800. This is a typed World operation, never a synthesized velocity call.
void native_tu15_activate_island_with_residuals(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,NativeTu15ObjectActivationScratch,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectActivationBindingActivity* = nullptr);
// Same316D40 and shared services, with an explicit native factory supplier.
// Existing entry points retain their signatures and original refusal policy.
void native_tu15_activate_island_with_native_services(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,NativeTu15ObjectActivationScratch,
    const NativeTu15ObjectActivationNativeServices&,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectActivationBindingActivity* = nullptr);
// Actual315CB0 with the SAME native activation/listener/lifetime builder.
void native_tu15_activate_body_with_native_services(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,unsigned,
    NativeTu15ObjectActivationScratch,const NativeTu15ObjectActivationNativeServices&,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectActivationBindingActivity* = nullptr);
// Native1D92C0/1D9100 for the actual1DA750 caller. The same body wake owner
// handles required growth/listeners; an unavailable child is a named refusal.
void native_tu15_deliver_lift_body_velocity(SourcePhysicsContactMemory &,
    const NativeTu15LiftVelocityDelivery &, NativeTu15PhysicsFrameFp &,
    NativeTu15ObjectActivationScratch, const NativeTu15ObjectActivationNativeServices &,
    const NativeTu15RetainedStoreObserver &, NativeTu15ObjectActivationBindingActivity * = nullptr);
} // namespace halo2
