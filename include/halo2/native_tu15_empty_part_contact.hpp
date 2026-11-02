#pragma once
#include "halo2/native_tu15_body_properties.hpp"
namespace halo2 {
// Sole shared zero-response phase after the source contact scan. This also
// supplies the zero-result branch of the complete response parent.
void native_tu15_clear_part_contact_response(SourcePhysicsContactMemory&,
    std::uint32_t part_slot,std::uint32_t removed_result,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Exact1E0A60 when no current matching part contact has flag20. Nonempty lists
// are scanned in source order; a matching flagged row requires the remaining
// material/normal/conveyor branch and refuses before response/property writes.
// removed_result is independently owned4B DATA for the dead1D7660 result.
void native_tu15_release_empty_part_contact(SourcePhysicsContactMemory&,
    std::uint32_t part_slot,std::int32_t part,std::uint32_t component,
    std::uint32_t removed_result,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
