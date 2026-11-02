#pragma once
#include "halo2/native_tu15_agent_cleanup_children.hpp"
namespace halo2 {
// Actual2F81B0 calls the authentic2F81E0 jump to2F7E90, then performs the
// same current size/allocator/class1A scalar release as2F8090. Reuse that sole
// native storage/lifetime algorithm; this adds no original execution owner.
inline NativeTu15ContactAgent native_tu15_delete_reversed_list_contact_agent(
    SourcePhysicsContactMemory&memory,NativeTu15ContactAgent agent,std::uint32_t disposing,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15AgentCleanupChildrenServices&services,
    const SourcePhysicsContactRecordObserver&observe={}){
    return native_tu15_destroy_contact_agent(memory,NativeTu15AgentDestructor::owner_2f8090,
        agent,disposing,fp,services,observe);
}
}
