#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct NativeTu15ObjectQueueResult {
    bool appended{};
    std::uint32_t eax{},ecx{},edx{};
};
NativeTu15ObjectQueueResult native_tu15_object_queue_append_166b60(SourcePhysicsContactMemory&,
    std::uint32_t queue,std::uint32_t object,std::uint32_t flags,std::uint32_t value,
    std::uint32_t association,std::uint32_t entry_eax);
struct NativeTu15LightParent {
    std::uint32_t object=0xffffffffu,body_residue=0xffffffffu,steps{};
};
// The first type0 ancestor, or the root when the chain contains no biped.
// The original ignores salts. Cycles are invalid inputs (bounded at65536).
NativeTu15LightParent native_tu15_light_parent_biped_walk(const SourcePhysicsContactMemory&,std::uint32_t owner);
struct NativeTu15LightParentServices {
    std::function<NativeTu15ObjectQueueResult(std::uint32_t queue,std::uint32_t object,
        std::uint32_t flags,std::uint32_t value,std::uint32_t association,std::uint32_t entry_eax)> append;
};
struct NativeTu15LightParentResult {
    bool taken{},owner_none{};
    std::uint32_t object=0xffffffffu,flags{},steps{};
    NativeTu15ObjectQueueResult queue;
};
//31EC2..31ED5 and whole31C10/166B60. Both late definition/shadow reads
// remain at the caller's phase, after31650 and before2CB10. No FP operations.
// Only persistent queue state is consumed by that caller; volatile residues
// on the untaken/owner-NONE paths are not outputs of this typed interface.
NativeTu15LightParentResult native_tu15_light_parent_biped_31ec2(SourcePhysicsContactMemory&,
    std::uint32_t definition_data,std::uint32_t shadow_address,std::uint32_t owner,
    std::uint32_t context,const NativeTu15LightParentServices& = {});
}
