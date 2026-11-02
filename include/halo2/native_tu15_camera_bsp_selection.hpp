#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// A source-layout BSP tree and point are retained data owners. Nodes carry a
// signed plane index and two packed signed24-bit children; there is no host
// visibility/pathability result or substitute spatial index here.
std::int32_t native_tu15_camera_bsp_leaf(const SourcePhysicsContactMemory &,
    std::uint32_t tree,std::uint32_t point,std::uint32_t node,
    NativeTu15PhysicsFrameFp &);
class NativeTu15CameraBspSelectionArgument {
public:
    virtual ~NativeTu15CameraBspSelectionArgument()=default;
    virtual std::uint32_t cluster_output()const=0;
};
// The actual2B700 policy. Success publishes leaf then signed cluster; failure
// keeps valid retained history or repairs both outputs toNONE in source order.
bool native_tu15_camera_bsp_select(SourcePhysicsContactMemory &,std::uint32_t point,
    std::uint32_t leaf_output,const NativeTu15CameraBspSelectionArgument &,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
}
