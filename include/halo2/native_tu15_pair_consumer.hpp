#pragma once
#include "halo2/native_tu15_broad_phase.hpp"
namespace halo2 {
struct NativeTu15PairRecordBuffer {std::uint32_t identity{};};
// Mutable formal DATA aliases used by37C960: range ends and two filter outputs.
// Interior/tail output storage initially holds current first.data/filter DATA.
struct NativeTu15PairMergeScratch {
    std::uint32_t first_end{},second_end{},interior_decision{},tail_decision{};
};
struct NativeTu15PairConsumerScratch {
    NativeTu15PairRecordBuffer temporary;
    NativeTu15PairMergeScratch merge;
};
struct NativeTu15PairConsumerServices {
    NativeTu15BroadPhaseServices arena;
    std::function<std::uint32_t(NativeTu15BroadPhaseAllocator,NativeTu15PhysicsVirtualOwner,
        std::uint32_t bytes,std::uint32_t tag,NativeTu15PhysicsFrameFp&)> allocate_owned_records;
    std::function<std::uint32_t(std::uint32_t filter,NativeTu15PhysicsVirtualOwner,
        std::uint32_t decision,std::uint32_t first_body,std::uint32_t second_body,
        NativeTu15PhysicsFrameFp&)> filter_pair;
    std::function<void(std::uint32_t handler,NativeTu15PhysicsVirtualOwner,
        std::uint32_t pair,NativeTu15PhysicsFrameFp&)> create_pair,delete_pair;
};
// Actual37CB20/37CBF0: unsigned object-ID pair normalization and source quicksort.
void native_tu15_sort_pairs(SourcePhysicsContactMemory&,std::uint32_t data,
    std::uint32_t first,std::uint32_t last,const SourcePhysicsContactRecordObserver& = {});
void native_tu15_normalize_pairs(SourcePhysicsContactMemory&,NativeTu15BroadPhasePairs,
    const SourcePhysicsContactRecordObserver& = {});
// Actual317A40/317AD0:20-byte header,16-byte record stride, explicit arena owners.
void native_tu15_construct_pair_records(SourcePhysicsContactMemory&,NativeTu15PairRecordBuffer,
    std::uint32_t reserve,NativeTu15PhysicsFrameFp&,const NativeTu15PairConsumerServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_pair_records(SourcePhysicsContactMemory&,NativeTu15PairRecordBuffer,
    NativeTu15PhysicsFrameFp&,const NativeTu15PairConsumerServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual37C960: merge cancellation plus current typed filter/create/delete owners.
void native_tu15_merge_pairs(SourcePhysicsContactMemory&,std::uint32_t dispatcher,
    NativeTu15BroadPhasePairs first,NativeTu15BroadPhasePairs second,std::uint32_t filter,
    NativeTu15PairMergeScratch,NativeTu15PhysicsFrameFp&,const NativeTu15PairConsumerServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual321510. Temporary records and merge aliases are genuine owned DATA.
void native_tu15_consume_pairs(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
    NativeTu15BroadPhasePairs first,NativeTu15BroadPhasePairs second,
    NativeTu15PairConsumerScratch,NativeTu15PhysicsFrameFp&,const NativeTu15PairConsumerServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
