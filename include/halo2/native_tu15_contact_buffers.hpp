#pragma once
#include "halo2/native_tu15_contact_records.hpp"
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
struct NativeTu15ContactComponentBuffer {std::uint32_t identity{};};
struct NativeTu15ContactBufferAllocator {std::uint32_t identity{};};
struct NativeTu15ContactBufferOwner {std::uint32_t identity{};};
struct NativeTu15ContactBufferServices {
    std::function<std::uint32_t(NativeTu15ContactBufferAllocator,NativeTu15ContactBufferOwner,
        std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> allocate_stack_block;
    std::function<void(NativeTu15ContactBufferAllocator,NativeTu15ContactBufferOwner,
        std::uint32_t block,std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> release_stack_block;
    std::function<void(NativeTu15ContactBufferAllocator,NativeTu15ContactBufferOwner,
        std::uint32_t block,std::uint32_t bytes,std::uint32_t memory_class,
        NativeTu15PhysicsFrameFp&)> release_owned_components;
};
// Actual303C70/303D00. Records here are256-byte opaque component storage;
// these owners allocate/release that backing without initializing its contents.
NativeTu15ContactComponentBuffer native_tu15_initialize_contact_components(
    SourcePhysicsContactMemory&,NativeTu15ContactComponentBuffer,std::uint32_t count,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactBufferServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_contact_components(SourcePhysicsContactMemory&,
    NativeTu15ContactComponentBuffer,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactBufferServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual303B70/303F50 base cleanup owners, including genuine unwind actions.
// Only owned backing is released. Borrowed/inline backing remains attached to
// its outer lifetime; neither owner changes header fields or invents a reset.
void native_tu15_release_owned_contact_components(SourcePhysicsContactMemory&,
    NativeTu15ContactComponentBuffer,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactBufferServices&);
void native_tu15_release_owned_contact_records(SourcePhysicsContactMemory&,
    NativeTu15ContactRecordArray,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&);
// Actual303EE0. The opaque constructor record is an explicit caller-owned
// INPUT. Only its two flag bytes are initialized here. No GP/SSE postimage,
// zero-filled packet, expected record or invented uninitialized byte is used.
// Native2E3250 grows the array through genuine allocator services when needed;
// native303BA0 then copies only its actual verified fields in original order.
NativeTu15ContactRecord native_tu15_append_contact_record(SourcePhysicsContactMemory&,
    NativeTu15ContactRecordArray,NativeTu15ContactRecord constructor_input,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
