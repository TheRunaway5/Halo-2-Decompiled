#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
namespace halo2 {
// Retained caller packets, including128 inline keys, child resolution backing,
// relative transform and the temporary12-byte entry-array header. Existing
// bytes are inputs; constructors initialize only their actual source fields.
struct NativeTu15ContactCandidateScratch {std::uint32_t identity{};};
enum class NativeTu15ContactCandidateBranch {unsorted,sorted_merge,sorted_tail};
enum class NativeTu15ContactCandidateRelease {replacement,final_keys,unwind_keys};
struct NativeTu15ContactCandidateServices {
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        std::uint32_t transform,std::uint32_t margin_bits,std::uint32_t output,
        NativeTu15PhysicsFrameFp&)> bounds;
    std::function<void(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        std::uint32_t bounds,std::uint32_t key_array,NativeTu15PhysicsFrameFp&)> query_keys;
    std::function<NativeTu15ContactChild(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        std::uint32_t key,std::uint32_t backing,NativeTu15ContactCandidateBranch,
        NativeTu15PhysicsFrameFp&)> resolve_child;
    // Actual filter returns its byte-result identity, not a supplied decision.
    std::function<std::uint32_t(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t result,NativeTu15ContactContext,NativeTu15ContactPayload,
        NativeTu15ContactPayload,std::uint32_t parent_key,std::uint32_t key,
        NativeTu15ContactCandidateBranch,NativeTu15PhysicsFrameFp&)> filter;
    std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner factory,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        std::uint32_t user_data,NativeTu15ContactCandidateBranch,
        NativeTu15PhysicsFrameFp&)> create_agent;
    std::function<void(NativeTu15ContactAgent,NativeTu15ContactVirtualOwner,
        NativeTu15ContactCandidateBranch,NativeTu15PhysicsFrameFp&)> release_agent;
    std::function<void(std::uint32_t keys,std::uint32_t first,std::uint32_t last,
        std::uint32_t comparator,NativeTu15PhysicsFrameFp&)> sort_keys; //2F33B0
    std::function<void(std::uint32_t header,std::uint32_t count,NativeTu15PhysicsFrameFp&)> initialize_entries; //2F3430
    std::function<void(std::uint32_t header,NativeTu15PhysicsFrameFp&)> destroy_entries; //2F34D0
    std::function<void(std::uint32_t header,std::uint32_t capacity,std::uint32_t stride,
        NativeTu15PhysicsFrameFp&)> reserve_entries; //2E31E0
    std::function<void(std::uint32_t allocator,NativeTu15ContactVirtualOwner,
        std::uint32_t data,std::uint32_t bytes,std::uint32_t memory_class,
        NativeTu15ContactCandidateRelease,NativeTu15PhysicsFrameFp&)> release;
    std::function<std::uint32_t(std::uint32_t allocator,NativeTu15ContactVirtualOwner,
        std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&)> allocate;
};
struct NativeTu15ContactCandidateStats {std::uint64_t calls{},unchanged_bounds{},sorted{},unsorted{},created{},released{},reserve_calls{};};
// Actual2F36F0. Remaining genuine service ownership and caller qualification
// are explicit; this interface supplies no GP file, decoder or expected result.
void native_tu15_update_contact_candidates(SourcePhysicsContactMemory&,
    NativeTu15ContactProducer,NativeTu15ContactPayload,NativeTu15ContactPayload,
    NativeTu15ContactContext,NativeTu15ContactVirtualOwner factory,
    NativeTu15ContactCandidateScratch,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ContactCandidateServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactCandidateStats* = nullptr);
} // namespace halo2
