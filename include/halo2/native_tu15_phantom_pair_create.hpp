#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"

namespace halo2 {
struct NativeTu15PhantomPairCreateServices {
    // Actual phantom virtual+1C RET8. The second argument's low byte is zero;
    // its remaining bytes come from the current phantom identity, exactly as
    // the source's current-DATA push followed by byte initialization.
    std::function<void(std::uint32_t phantom,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload other,std::uint32_t flags,
        NativeTu15PhysicsFrameFp&)> add_overlap;
};
// Actual31FFC0. Incoming listener ECX is unused. Current second pair/type
// and first handle are reread after the first actual callback.
void native_tu15_create_phantom_pair(SourcePhysicsContactMemory&,std::uint32_t pair,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomPairCreateServices&);
} // namespace halo2
