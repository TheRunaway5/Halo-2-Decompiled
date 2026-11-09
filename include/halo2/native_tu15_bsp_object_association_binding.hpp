#pragma once
#include "halo2/native_tu15_bsp_object_association.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
namespace halo2 {
struct NativeTu15BspObjectAssociationResources {std::uint32_t first{},bytes{};};
// Two Source-produced iterator DATA records live in one owned native sidecar.
// The caller supplies a disjoint logical identity; no original stack bytes,
// registers, after-image or original execution service enters this binding.
inline void native_tu15_initialize_bound_bsp_object_associations(
    SourcePhysicsContactMemory&memory,NativeTu15BspObjectAssociationResources resources,
    std::span<const NativeTu15ContactPrivateRange>retained,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BspObjectAssociationStats*stats=nullptr){
    if(resources.bytes!=32u||(resources.first&15u)||
        std::uint64_t(resources.first)+resources.bytes>0x100000000ull)
        throw std::runtime_error("Native BSP association requires disjoint aligned32-byte iterator DATA");
    const std::array owned{NativeTu15ContactPrivateRange{resources.first,resources.bytes}};
    NativeTu15ContactCandidatePrivateData data(memory,owned,retained);
    native_tu15_initialize_bsp_object_associations(data,
        {{resources.first},{resources.first+16u}},observe,stats);
}
} // namespace halo2
