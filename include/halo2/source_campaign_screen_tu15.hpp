#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cinematic_crossfade.hpp"
namespace halo2 {
bool source_campaign_screen_tu15_owns_entry(std::uint32_t);
void source_run_campaign_screen_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Native argument/result composition of the original HS command stores.
// These value seams do not execute the HS parser or complete an HS thread.
void source_campaign_screen_start_tu15(SourceRuntimeMemory &,bool reset);
void source_campaign_crossfade_set_tu15(SourceRuntimeMemory &,float seconds,float strength=1.f);
// Native renderer clock publication. Whole renderer/frame/SDK execution is
// separate; accepted elapsed has already passed13AA50's native value adapter.
void source_campaign_screen_clock_tu15(SourceRuntimeMemory &,double clock);
void source_campaign_screen_update_tu15(PhysicsLiveWorldTu15 &);
void source_campaign_screen_stop_tu15(PhysicsLiveWorldTu15 &);
void source_campaign_screen_capture_complete_tu15(PhysicsLiveWorldTu15 &);
CinematicCrossfadeState source_campaign_crossfade_state_tu15(const SourceRuntimeMemory &);
CinematicCrossfadeFrame source_campaign_crossfade_frame_tu15(const SourceRuntimeMemory &);
}
