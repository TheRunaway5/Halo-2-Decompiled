#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole TU1.5 156480: actual global fade and four registered local effect
// slots, including flash, vibration and camera-response lifetimes. No clock
// increment, response request, local membership or successful admission is supplied.
void native_player_effect_tick_tu15(SourcePhysicsContactMemory&,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {});
void native_player_effect_tick_tu15_run(SourcePhysicsContactMemory&,SourcePhysicsWorldRuntimeAbi&,
    const SourcePhysicsContactRecordObserver& = {});
}
