#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original42CE0 two-stream geometry callback,42C40 view fade,
// BCD80 object color,17F8A0 fade curves and13A070 bitmap frame selection.
// The caller supplies actual
// contrail/point pools, view globals and the callback's three stack arguments.
//17EAD0/17F3D0 supply the separate original tick-owned point update/retirement.
// Geometry calls alone neither advance that clock nor admit render resources.
bool source_contrail_geometry_tu15_owns_entry(std::uint32_t);
void source_run_contrail_geometry_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
