#pragma once
#include "halo2/native_tu15_camera_horizontal_basis.hpp"
#include "halo2/native_tu15_function_curves.hpp"
#include "halo2/native_tu15_look_vector_angle.hpp"
#include "halo2/native_tu15_vector_checks.hpp"
namespace halo2 {
struct NativeTu15CameraAdjustmentScratch {std::uint32_t data{};}; //40 caller-owned DATA bytes.
struct NativeTu15CameraAdjustmentStats {
    std::uint64_t calls{},completed{},refusals{},orthogonality_calls{},downward_angles{},curves{},adjustments{};
};
// Whole E18D0. Position is mutable3-word DATA; forward/perpendicular are its
// caller's initialized3-word direction packets. Scratch holds only the source
// object identity and arithmetic DATA, never saved GP/return metadata.
void native_tu15_adjust_unit_camera_position(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t position,std::uint32_t forward,
    std::uint32_t perpendicular,NativeTu15CameraAdjustmentScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraAdjustmentStats* = nullptr);
}
