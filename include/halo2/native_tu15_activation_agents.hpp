#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "halo2/native_tu15_island_transition.hpp"
namespace halo2 {
struct NativeTu15ActivationContactManager {std::uint32_t identity{};};
struct NativeTu15ActivationAgentScratch {std::uint32_t filter_result{},discrete_result{};};
struct NativeTu15ActivationAgentServices {
    std::function<std::uint32_t(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t result,NativeTu15ContactPayload,NativeTu15ContactPayload,
        NativeTu15PhysicsFrameFp&)>filter_pair;
    std::function<NativeTu15ActivationContactManager(std::uint32_t factory,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        NativeTu15PhysicsFrameFp&)>create_manager;
    std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>classify_shape;
    std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        NativeTu15ActivationContactManager,NativeTu15PhysicsFrameFp&)>create_agent;
    std::function<std::uint32_t(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>motion_kind;
    std::function<std::uint32_t(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)>discrete_dispatcher; //316200
    std::function<std::uint32_t(std::uint32_t dispatcher,NativeTu15ContactVirtualOwner,
        std::uint32_t result,NativeTu15ContactPayload,NativeTu15ContactPayload,
        NativeTu15PhysicsFrameFp&)>test_discrete_pair;
    std::function<void(NativeTu15ActivationContactManager,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>release_manager;
    std::function<std::uint32_t(std::uint32_t policy,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>body_policy_kind;
    std::function<void(NativeTu15PhysicsBody,std::uint32_t flags,
        NativeTu15PhysicsFrameFp&)>mark_body; //31B730
};
struct NativeTu15ActivationAgentStats {
    std::uint64_t calls{},pairs{},existing_agents{},filtered_pairs{},managers_created{},
        factory_calls{},retained_agents{},released_managers{},class_calls{},motion_queries{},
        discrete_queries{},zero_ranks{},disabled_ranks{},body_policy_queries{},bodies_marked{};
};
// Actual282870. The source's listener receiver is unused; its island owns the
// real contact entries/world. Scratch is caller-owned byte-result backing,
// never an x86 control frame. Every callback owns genuine current work.
// This function has no SEH cleanup: do not invent manager release on failure.
void native_tu15_recreate_island_agents(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
    NativeTu15ActivationAgentScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ActivationAgentServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ActivationAgentStats* = nullptr);
} // namespace halo2
