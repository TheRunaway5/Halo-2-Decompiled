#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>

namespace halo2 {
struct NativeTu15PhysicalCallbackRequest {
    std::uint32_t owner{},address{},context{},current{};
};
struct NativeTu15PhysicalCallbackResult {std::uint32_t current{};};
struct NativeTu15PhysicalCallbackServices {
    std::function<NativeTu15PhysicalCallbackResult(const NativeTu15PhysicalCallbackRequest&,NativeTu15PhysicsFrameFp&)> invoke;
};
enum class NativeTu15PhysicalCallbackPhase {binding,callback_owner,callback_context,callback_address,
    callback_call,reacquired_head,head_check,search_compare,search_next,next,restart,current_check};
struct NativeTu15PhysicalCallbackPhaseData {
    std::uint32_t head{},current{},owner{},address{},context{},scan{};
};
struct NativeTu15PhysicalCallbackPhases {
    std::function<void(NativeTu15PhysicalCallbackPhase,const NativeTu15PhysicalCallbackPhaseData&)> observe;
};
struct NativeTu15PhysicalCallbackStats {
    std::uint64_t calls{},visited{},callbacks{},restarts{},unbound_callbacks{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual12EF10: callbacks may unlink the current allocation or replace the
// list. Reacquire its head after each callback and search for the saved walk
// entry before advancing; restart at the new head if that entry disappeared.
void native_tu15_notify_physical_allocations(SourcePhysicsContactMemory&,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhysicalCallbackServices&,const NativeTu15PhysicalCallbackPhases& = {},
    NativeTu15PhysicalCallbackStats* = nullptr);
} // namespace halo2
