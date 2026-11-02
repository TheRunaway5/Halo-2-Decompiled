#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"

namespace halo2 {
enum class NativeTu15AgentCleanupKind {records_2f3100,pointers_2f7a20};
struct NativeTu15AgentCleanupServices {
    // Actual child virtual+14, ordinary void RET0. A binding can recurse into
    // another qualified cleanup owner; it must not supply a child outcome.
    std::function<void(NativeTu15ContactAgent,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> cleanup_child;
    // Actual scalar deleting destructor virtual0, ordinary RET4(flag1).
    std::function<void(std::uint32_t object,NativeTu15ContactVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&)> dispose;
};
// Actual2F3100/2F7A20. The12B-record loop caches its original end; the pointer
// loop rereads its count/backing after callbacks. Neither owns a CPU frame.
void native_tu15_cleanup_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15AgentCleanupKind,NativeTu15ContactAgent,NativeTu15PhysicsFrameFp&,
    const NativeTu15AgentCleanupServices&);
// Actual323220: a null reference is returned untouched; otherwise dispatch
// the current destructor with flag1. FP changes belong to the real service.
void native_tu15_dispose_contact_reference(SourcePhysicsContactMemory&,
    std::uint32_t object,NativeTu15PhysicsFrameFp&,const NativeTu15AgentCleanupServices&);
} // namespace halo2
