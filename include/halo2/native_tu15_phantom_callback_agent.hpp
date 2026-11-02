#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
namespace halo2 {
// Complete original308540: MOV EAX,22; RET. Receiver/FP are unread.
std::uint32_t native_tu15_phantom_shape_class_22() noexcept;
struct NativeTu15PhantomCallbackAgentArguments {
    // Ordinary current cdecl inputs, not classifications, pointers returned by
    // a constructor or expected callback outcomes.
    std::function<std::uint32_t()> first,second,context,user_data;
};
struct NativeTu15PhantomCallbackAgentServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)>classify_shape;
    // Actual shape virtual+28, thiscall3/RET12. Both root payloads are derived
    // from current parent links at the source phases; context is the source's
    // third cdecl input. The callback must execute its genuine owner.
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload self_root,NativeTu15ContactPayload other_root,
        NativeTu15ContactContext,NativeTu15PhysicsFrameFp&)>enter;
};
// Actual2F7820. Preserve current parent traversal, classify each current shape
// through its actual owner, and leave all omitted bytes unchanged. The source
// FuncInfo46BA90 owns2F7A10 base cleanup during constructor exceptions.
NativeTu15ContactAgent native_tu15_initialize_phantom_callback_agent(
    SourcePhysicsContactMemory&,NativeTu15ContactAgent,
    const NativeTu15PhantomCallbackAgentArguments&,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhantomCallbackAgentServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2F78C0. Allocate36 bytes/class1A and call2F7820. Allocation cleanup
// (FuncInfo46BAB4,2F0380) ends after the first class read, BEFORE any enter
// callback. After a callback, reread the second class and current parent links.
// Component/original comparison and production binding are separate gates.
NativeTu15ContactAgent native_tu15_create_phantom_callback_agent(
    SourcePhysicsContactMemory&,const NativeTu15PhantomCallbackAgentArguments&,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomCallbackAgentServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
