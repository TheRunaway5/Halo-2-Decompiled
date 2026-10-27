#pragma once
#include "halo2/native_tu15_contact_records.hpp"
#include "halo2/native_tu15_geometry.hpp"
namespace halo2 {
struct NativeTu15ContactProducer {std::uint32_t identity{};};
struct NativeTu15ContactPayload {std::uint32_t identity{};};
struct NativeTu15ContactContext {std::uint32_t identity{};};
struct NativeTu15ContactOutput {std::uint32_t identity{};};
struct NativeTu15ContactChild {std::uint32_t identity{};};
struct NativeTu15ContactComponent {std::uint32_t identity{};};
struct NativeTu15ContactVirtualOwner {std::uint32_t identity{};};
// Caller-owned retained packets, including the four inline128-byte records.
// This backing must preserve existing bytes. Zero-filled scratch would invent
// values for source fields that the actual constructor/query never writes.
// identity is the aligned retained packet-layout base; it is not a CPU stack.
struct NativeTu15ContactProducerScratch {std::uint32_t identity{};};
// Genuine remaining owners of original304350. These callbacks own actual
// services and publications, never a fabricated contact or expected result.
// Their numerical FP changes are synchronized at each real dependency phase.
struct NativeTu15ContactProducerServices {
    std::function<std::uint64_t()> timestamp_counter;
    std::function<void(NativeTu15ContactProducer,NativeTu15ContactPayload,
        NativeTu15ContactPayload,NativeTu15ContactContext,std::uint32_t,
        NativeTu15PhysicsFrameFp&)> update_candidates; // actual2F36F0 +2F3140 callback
    std::function<void(std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> initialize_component_buffer; //303C70
    // Transitional source-compatibility field, never invoked: actual30BAD0
    // is now the native constant child identity48CDC4 inside the producer.
    std::function<std::uint32_t(NativeTu15PhysicsFrameFp&)> invalid_child;
    std::function<NativeTu15ContactComponent(NativeTu15ContactChild,
        NativeTu15ContactVirtualOwner,std::uint32_t,std::uint32_t,
        NativeTu15PhysicsFrameFp&)> resolve_component; // actual virtual34
    std::function<std::uint32_t(NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,NativeTu15PhysicsFrameFp&)> component_class; //virtual14
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> test_pair; //46E46C; returns actual byte-result reference
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        std::uint32_t,NativeTu15ContactPayload,NativeTu15ContactPayload,
        NativeTu15ContactContext,std::uint32_t,NativeTu15PhysicsFrameFp&)> query_contact; //virtual20
    std::function<void(NativeTu15ContactRecordArray,NativeTu15PhysicsFrameFp&)> append_record; //303EE0
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        NativeTu15ContactOutput,NativeTu15PhysicsFrameFp&)> process_rejected_pair; //virtual10
    std::function<void(NativeTu15ContactPayload,NativeTu15ContactPayload,
        NativeTu15ContactContext,NativeTu15ContactRecord,std::uint32_t,
        std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> publish_contact; //30CD00
    std::function<void(NativeTu15ContactPayload,NativeTu15ContactPayload,
        NativeTu15ContactContext,std::uint32_t,std::uint32_t,std::uint32_t,
        NativeTu15ContactOutput,std::uint32_t,NativeTu15PhysicsFrameFp&)> publish_distance; //30CF40
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> destroy_component_buffer; //303D00
    std::function<void(std::uint32_t,NativeTu15ContactVirtualOwner,
        std::uint32_t,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> release_records; //actual allocator virtual14
};
// Proper typed owner of the full534-site actual304350 producer. No decoder or
// original GP/XMM execution file is part of this algorithm. Production binding
// and physical caller-service closure must be separately qualified.
void native_tu15_produce_contacts(SourcePhysicsContactMemory&,NativeTu15ContactProducer,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext,NativeTu15ContactOutput,NativeTu15ContactProducerScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ContactProducerServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
