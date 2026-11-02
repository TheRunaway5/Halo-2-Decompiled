#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
namespace halo2 {
struct NativeTu15ListContactArguments {
    // Current ordinary cdecl inputs, read at their actual source phases.
    std::function<std::uint32_t()> first,second,context,user_data;
};
struct NativeTu15ListContactScratch {
    // Sixteen retained DATA bytes, independent of every nested factory's
    // control stack. output is the caller-owned context argument cell: its
    // initial low byte must be the current context input's low byte.
    std::uint32_t child_payload{},output{};
};
struct NativeTu15ListContactServices {
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ContactAgentDispatchServices dispatch;
    std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> child_count;
    // Actual virtual RET24: (out,context,second,first,collection,index).
    // The real owner writes/returns its actual output pointer. Permission is
    // read from that pointer; callers never supply an admission outcome.
    std::function<std::uint32_t(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t output,NativeTu15ContactContext,NativeTu15ContactPayload second,
        NativeTu15ContactPayload first,NativeTu15ContactChild,std::uint32_t index,
        NativeTu15PhysicsFrameFp&)> filter_child;
};
// Actual2E15C0: release an owned pointer array, without resetting its header.
void native_tu15_destroy_contact_pointer_array(SourcePhysicsContactMemory&,
    NativeTu15ArrayHeader,NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&);
// Complete actual2F7D20. Keep the real count/filter/class/factory callbacks,
// late argument/header rereads, signed bounds, inline storage and exact unwind
// ownership. Constructor failure destroys the array then the base; it does
// not invent destruction of partially initialized child-agent slots.
NativeTu15ContactAgent native_tu15_initialize_list_contact_agent(
    SourcePhysicsContactMemory&,NativeTu15ContactAgent,const NativeTu15ListContactArguments&,
    NativeTu15ListContactScratch,NativeTu15PhysicsFrameFp&,const NativeTu15ListContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete actual2F8030: allocate40/class1A, publish its16-bit size, read
// arguments4/3/2/1, then retain real2F0380 storage ownership around2F7D20.
NativeTu15ContactAgent native_tu15_create_list_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15ListContactArguments&,
    NativeTu15ListContactScratch,NativeTu15PhysicsFrameFp&,const NativeTu15ListContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2F81F0: the same list constructor receives second/first, then the
// factory publishes derived vtable41CCE8. Its original FuncInfo46BB70 owns
// the same2F0380 storage cleanup, with argument read order4/3/1/2.
NativeTu15ContactAgent native_tu15_create_reversed_list_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15ListContactArguments&,
    NativeTu15ListContactScratch,NativeTu15PhysicsFrameFp&,const NativeTu15ListContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
