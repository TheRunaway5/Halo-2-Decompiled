#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
void source_run_ratio_threshold_crt_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
