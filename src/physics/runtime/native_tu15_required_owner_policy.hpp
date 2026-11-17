#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <thread>

namespace halo2::native_required_owner_policy_detail {
// This checks an explicit owned-body contract, not arbitrary callback intent.
// A provider-free route keeps its genuine unavailable-owner stop. Immutable
// copies of the same certified callable/capture remain valid; stale metadata,
// another owner, wrappers and another executor do not gain this capability.
inline bool preserves_instruction_policy(const SourcePhysicsContactRuntimeServices&services)noexcept{
    if(!services.required_owner)return true;
    const auto&policy=services.required_owner_policy;
    return policy.callback_type&&policy.owner&&policy.matches&&
        policy.executor==std::this_thread::get_id()&&
        services.required_owner.target_type()==*policy.callback_type&&
        policy.matches(services.required_owner,policy.owner);
}
} // namespace halo2::native_required_owner_policy_detail
