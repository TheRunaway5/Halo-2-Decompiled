#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole authenticated TU1.5 423B0 kind3 callback. It consumes the actual
// source group payload and global bank, preserving collection/copy/layer
// ordering and bank restoration. Graphics, collection and layer-selector
// callees remain required services; ownership alone does not render a group.
bool source_transparent_group_tu15_owns_entry(std::uint32_t);
void source_run_transparent_group_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
