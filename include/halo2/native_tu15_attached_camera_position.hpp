#pragma once
#include "halo2/native_tu15_look_context.hpp"
namespace halo2 {
struct NativeTu15AttachedCameraPositionStats {
    std::uint64_t calls{},completed{},refusals{},base_position_calls{},
        marker_requests{},on_foot_requirements{};
};
// SourceCD4A0's attached-object branch, with full nativeBBF20/BBE00 and
// nativeBAD60 marker computation. The on-foot collision/crouch camera branch
// remains an explicit requirement, never a guessed camera or CPU fallback.
// All private marker DATA is a genuine host value; only three initialized
// world translation words are copied to the retained output.
void native_tu15_compute_attached_camera_position(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15AttachedCameraPositionStats* = nullptr);
} // namespace halo2
