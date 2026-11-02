#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
namespace halo2 {
struct NativeTu15WrappedContactArguments {
    // Read current ordinary inputs at the actual source phases. These are
    // input providers, never expected constructor or classifier outcomes.
    std::function<std::uint32_t()> first,second,context,user_data;
};
struct NativeTu15WrappedContactScratch {
    // Sixteen caller-owned DATA bytes: child shape, transform, payload word
    // and original first-payload identity. No control-frame data is retained.
    std::uint32_t child_payload{};
};
struct NativeTu15TransformedContactScratch {
    // Sixteen child-payload bytes and96 aligned copied/transformed DATA
    // bytes, retained independently across the selected real child factory.
    std::uint32_t child_payload{},geometry{};
};
struct NativeTu15WrappedContactServices {
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ContactAgentDispatchServices dispatch;
};
// Complete actual2F0DF0. The selected child is constructed by its real owner;
// the wrapper never supplies a child pointer or class. Its FuncInfo46B4B8
// identifies the actual2F7A10 base cleanup on a constructor exception.
NativeTu15ContactAgent native_tu15_initialize_wrapped_contact_agent(
    SourcePhysicsContactMemory&,NativeTu15ContactAgent,
    const NativeTu15WrappedContactArguments&,NativeTu15WrappedContactScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15WrappedContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete actual2F0EB0: allocate20 bytes/class1A, publish the16-bit size,
// then call2F0DF0 with the original first/second order. Unlike2F1CC0 there
// is no derived-vtable store. Shared FuncInfo46B4DC releases through2F0380
// if construction throws.
NativeTu15ContactAgent native_tu15_create_bv_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15WrappedContactArguments&,
    NativeTu15WrappedContactScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15WrappedContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete actual2F1CC0: allocate20 bytes/class1A, publish size, read the
// four current arguments, construct2F0DF0 with first/second reversed, then
// publish derived vtable41CAF0.
// FuncInfo46B4DC retains actual2F0380 ownership on construction failure.
NativeTu15ContactAgent native_tu15_create_wrapped_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15WrappedContactArguments&,
    NativeTu15WrappedContactScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15WrappedContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete actual2F7190. Copy the six aligned vectors in source order, compose
// the last four with genuine2E44D0, then construct the selected real child.
// FuncInfo46BA6C owns allocation at state0 and base cleanup at state1.
NativeTu15ContactAgent native_tu15_create_transformed_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15WrappedContactArguments&,
    NativeTu15TransformedContactScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15WrappedContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Companion2F75F0 proposal: transform the second payload, construct the child
// with the transformed payload first, then publish derived vtable41CC84.
// Native comparison and production admission are tracked separately.
NativeTu15ContactAgent native_tu15_create_reversed_transformed_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15WrappedContactArguments&,
    NativeTu15TransformedContactScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15WrappedContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
