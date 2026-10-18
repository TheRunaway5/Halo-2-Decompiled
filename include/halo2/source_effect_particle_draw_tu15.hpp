#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
void mount_source_particle_graphics_layout_data_tu15(SourceRuntimeMemory &);
void mount_source_particle_declaration_data_tu15(SourceRuntimeMemory &);
// Whole132B0 view-context copy/camera publication,15350 stencil selection,
//3FF670 stencil methods,13430 per-view reset and400270/400140 viewport/depth arithmetic,
//1770C0/1772C0 admission,176B30/177E20 queued rendering,
//22ACF0/22ADB0/247D70 renderer context,4012B0/401410 constant emission
// and1E0F0/1E4C0 transparent storage. Native graphics resources and every
// downstream unimplemented owner remain explicit required inputs.
bool source_effect_particle_draw_tu15_owns_entry(std::uint32_t);
void source_run_effect_particle_draw_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
