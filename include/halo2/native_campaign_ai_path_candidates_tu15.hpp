#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// Actual27C410 retained path-node table lookup. Return the signed source WORD;
// NONE is -1. The original full-table cycle has no invented iteration limit.
std::int16_t native_campaign_ai_path_find_node_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t workspace, std::uint32_t key);
// Actual27C450 segment candidate. This preserves the source's (start-query)
// numerator and endpoint-copy branches, including their NaN/FP behavior.
void native_campaign_ai_path_segment_candidate_tu15(SourcePhysicsContactMemory &,
    std::uint32_t start, std::uint32_t end, std::uint32_t query,
    std::uint32_t destination, NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
