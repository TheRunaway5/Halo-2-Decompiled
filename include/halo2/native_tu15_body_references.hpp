#pragma once
#include "halo2/native_tu15_island_transition.hpp"
namespace halo2 {
enum class NativeTu15BodyReferenceCohort {first,second,third};
struct NativeTu15BodyReferenceServices {
    std::function<void(NativeTu15PhysicsArrayHeader,std::uint32_t capacity,
        std::uint32_t stride,NativeTu15PhysicsFrameFp&)>reserve_array;
};
struct NativeTu15BodyReferenceStats {
    std::uint64_t calls{},cohorts{},moved_entries{},reserves{};
};
// Actual3173C0/317430 compact the second/third reference arrays. The first
// cohort is the3174A0 prefix; the root below always performs all three.
void native_tu15_compact_body_reference_cohort(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15BodyReferenceCohort,NativeTu15PhysicsFrameFp&,
    const NativeTu15BodyReferenceServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BodyReferenceStats* = nullptr);
// Actual3174A0. Preserve cached original count and the initial pointer scan,
// then reread the header pointer for each later entry and reserve at the
// original phase. This compacts source data; it does not destroy the body.
void native_tu15_compact_body_references(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,const NativeTu15BodyReferenceServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BodyReferenceStats* = nullptr);
} // namespace halo2
