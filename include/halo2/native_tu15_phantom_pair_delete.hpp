#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"

namespace halo2 {
struct NativeTu15PhantomPairCreateServices {
    // Actual phantom vtable+1C, RET8(other payload, notification flag).
    // The source initializes the flag's low byte to zero after pushing the
    // current phantom pointer; retain all four argument bytes for the owner.
    std::function<void(std::uint32_t phantom,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload other,std::uint32_t flag_word,
        NativeTu15PhysicsFrameFp&)> add_overlap;
};
// Actual31FFC0. Current pair words/types are reread after a real callback.
void native_tu15_create_phantom_pair(SourcePhysicsContactMemory&,std::uint32_t pair,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomPairCreateServices&);
struct NativeTu15PhantomPairDeleteServices {
    // Actual phantom vtable+20; receiver is payload+20, other is the opposite
    // collidable payload. Both ordinary call and tail-call paths are RET4.
    std::function<void(std::uint32_t phantom,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload other,NativeTu15PhysicsFrameFp&)> remove_overlap;
};
// Actual320040. Its incoming listener receiver is unused. Pair words and
// types are reread at the source boundary after the first real callback.
void native_tu15_delete_phantom_pair(SourcePhysicsContactMemory&,std::uint32_t pair,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomPairDeleteServices&);
// Authenticated one-site2E4D60 RET4. It reads no receiver/argument/FP state.
void native_tu15_ignore_removed_phantom_overlap() noexcept;
} // namespace halo2
