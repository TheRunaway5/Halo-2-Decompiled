#pragma once
#include "halo2/native_tu15_camera_adjustment.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15CameraPositionScratch {
    std::uint32_t parent_data{}; //164B ordinary camera vectors, marker/result and cached tag DATA
    NativeTu15CameraAdjustmentScratch adjustment;
};
struct NativeTu15CameraSegment {
    std::uint32_t mask{},origin{},displacement{},ignore_first{},ignore_second{},result{};
};
struct NativeTu15CameraPositionServices {
    // Read-only diagnostic phase boundary; every child still executes its
    // semantic native kernel directly. No result/state is supplied by it.
    std::function<void(std::uint32_t owner,bool entering)> native_child;
    // Actual16C620 RET24. A genuine geometric query must compute its boolean,
    // FP and result DATA; no result or initialized packet is supplied here.
    std::function<bool(const NativeTu15CameraSegment&,NativeTu15PhysicsFrameFp&)> segment;
};
struct NativeTu15CameraPositionStats {
    std::uint64_t calls{},completed{},refusals{},world_points{},origins{},markers{},
        on_foot{},adjustments{},segment_calls{},unbound_segments{};
};
// Whole CD4A0 semantic parent. All source arithmetic/marker/query DATA uses
// caller-owned storage; saved GP/return/control bytes are never represented.
void native_tu15_compute_unit_camera_position(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t destination,NativeTu15CameraPositionScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15CameraPositionServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraPositionStats* = nullptr);
}
