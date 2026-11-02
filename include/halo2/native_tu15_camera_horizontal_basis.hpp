#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Whole E05D0. Two independent two-word output vectors, source3 direction
// and fallback DATA. Preserve current output rereads, including aliases.
void native_tu15_camera_horizontal_basis(SourcePhysicsContactMemory&,
    std::uint32_t direction,std::uint32_t fallback,std::uint32_t first_output,
    std::uint32_t perpendicular_output,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {});
}
