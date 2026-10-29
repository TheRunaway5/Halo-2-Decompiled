#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Complete27B100 query start: validate the actual navigation root/sector,
// quantize the original framed-point distance, publish the source node and
// insert it into the retained native priority heap. No route is invented.
bool native_campaign_ai_path_query_start_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
