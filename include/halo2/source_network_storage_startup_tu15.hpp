#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Complete TU1.5 6DE60, 70660 and 7EC90 source paths. Native shared allocation,
// pool construction and XNet operations remain explicit required owners.
void source_run_network_storage_startup_tu15(SourcePhysicsContactMemory&,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
void mount_native_tu15_network_storage_data(SourceRuntimeMemory&);
// Complete 6E010 body; link/XNet/Winsock/online APIs and later subsystem
// callbacks remain required owners. A lowered body does not imply completion.
void source_run_network_connection_startup_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
// Complete 70CE0 body. The title-address API remains an explicit native owner.
void source_run_network_title_address_startup_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void source_run_network_availability_startup_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
// The shared31D400 initializer is literally one RET instruction.
void source_run_startup_noop_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&);
}
