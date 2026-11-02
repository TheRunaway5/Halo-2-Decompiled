#pragma once
#include "halo2/native_tu15_compound_process.hpp"
#include "halo2/native_tu15_geometry.hpp"
namespace halo2 {
struct NativeTu15WrappedContactProcessScratch {
    // Sixteen descriptor bytes and96 aligned geometry bytes, owned by the
    // caller for the entire child call. Neither is a virtual CPU frame.
    NativeTu15CompoundBodyProxy descriptor;
    std::uint32_t geometry{};
};
struct NativeTu15WrappedContactProcessStats {
    std::uint64_t calls{},geometry_calls{},child_calls{},child_returns{};
};
// Actual2F6CA0. Preserve the two different source-transform reads, copy the
// first32 bytes, compose the remaining64 through native2E44D0, and read the
// current child/shape/vtable at their original phases. The selected child
// owns its actual processing; no collision result is supplied by this parent.
void native_tu15_process_wrapped_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15IslandContactOutput,NativeTu15WrappedContactProcessScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15CompoundProcessServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15WrappedContactProcessStats* = nullptr);
}
