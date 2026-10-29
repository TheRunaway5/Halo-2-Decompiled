#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// TU1.5 path-workspace priority heap. Count is the exclusive tail (root 1),
// slots hold signed WORD node IDs/priorities, and each node owns its slot
// backpointer. The caller supplies its actual leased workspace, not a host
// navigation graph. Reads and writes stay lazy when these fields alias.
// These integer owners do not quantize floating path costs or decide routes.
void native_campaign_ai_path_heap_sift_up_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::int16_t slot,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_path_heap_sift_down_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::int16_t slot,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_path_heap_insert_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::int16_t node,std::int16_t priority,
    const SourcePhysicsContactRecordObserver & = {});
}
