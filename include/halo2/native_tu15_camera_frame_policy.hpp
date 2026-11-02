#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete2C470. The caller's view count selects retained renderer policy
// fields. Authored scalar words are transported without floating arithmetic;
// every read and publication keeps its original lazy order.
void native_tu15_camera_frame_policy(SourcePhysicsContactMemory &,std::uint32_t view_count,
    const SourcePhysicsContactRecordObserver & = {});
// Complete25940 over genuine retained target records. This only queries
// allocation/retirement state; it neither creates a target nor authorizes GPU
// capture. Native Renderer capabilities have a separate owner.
std::uint32_t native_tu15_camera_target_query(const SourcePhysicsContactMemory &);
}
