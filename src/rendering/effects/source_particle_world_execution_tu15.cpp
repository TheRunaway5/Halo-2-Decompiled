#include "halo2/source_particle_world_execution_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_particle_target_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_shadow_frame_tu15.hpp"
#include "halo2/source_transparent_group_tu15.hpp"
#include "halo2/source_transparent_collection_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include "halo2/cache.hpp"
#include <sstream>

namespace halo2 {
namespace {
bool execute(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(source_effect_particle_draw_tu15_owns_entry(entry))
        source_run_effect_particle_draw_tu15(memory,entry,abi,fp,services,observe);
    else if(source_particle_target_tu15_owns_entry(entry))
        source_run_particle_target_tu15(memory,entry,abi,fp,services,observe);
    else if(source_shadow_frame_tu15_owns_entry(entry))
        source_run_shadow_frame_tu15(memory,entry,abi,fp,services,observe);
    else if(source_transparent_group_tu15_owns_entry(entry))
        source_run_transparent_group_tu15(memory,entry,abi,fp,services,observe);
    else if(source_transparent_collection_tu15_owns_entry(entry))
        source_run_transparent_collection_tu15(memory,entry,abi,fp,services,observe);
    else if(source_model_render_tu15_owns_entry(entry))
        source_run_model_render_tu15(memory,entry,abi,fp,services,observe);
    else if(source_effect_particle_frame_tu15_owns_entry(entry))
        source_run_effect_particle_frame_tu15(memory,entry,abi,fp,services,observe);
    else if(source_physics_live_support_tu15_owns_entry(entry))
        source_run_physics_live_support_tu15(memory,entry,abi,fp,services,observe);
    else if(entry==0x144910u && source_scene_device_constructor_tu15_owns_entry(entry))
        source_run_scene_device_constructor_tu15(memory,entry,abi,fp,services,observe);
    else if(source_map_world_support_tu15_owns_entry(entry))
        source_run_map_world_support_tu15(memory,entry,abi,fp,services,observe);
    else if(source_lens_flare_tu15_owns_entry(entry))
        source_run_lens_flare_tu15(memory,entry,abi,fp,services,observe);
    else if(entry==0x37dc0au || entry==0x37cde8u)
        source_run_physics_vehicle_motion_math_runtime(memory,
            entry==0x37dc0au?0x37a15au:0x379338u,abi,
            SourcePhysicsContactImage::TitleUpdate15,fp,services,observe);
    else return false;
    return true;
}
}
bool source_particle_world_execution_tu15_owns_entry(std::uint32_t entry) {
    return source_effect_particle_draw_tu15_owns_entry(entry) ||
        source_particle_target_tu15_owns_entry(entry) ||
        source_shadow_frame_tu15_owns_entry(entry) ||
        source_transparent_group_tu15_owns_entry(entry) ||
        source_transparent_collection_tu15_owns_entry(entry) ||
        source_model_render_tu15_owns_entry(entry) ||
        source_effect_particle_frame_tu15_owns_entry(entry) ||
        source_physics_live_support_tu15_owns_entry(entry) ||
        (entry==0x144910u && source_scene_device_constructor_tu15_owns_entry(entry)) ||
        source_map_world_support_tu15_owns_entry(entry) ||
        source_lens_flare_tu15_owns_entry(entry) ||
        entry==0x37dc0au || entry==0x37cde8u;
}
void source_run_particle_world_execution_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    auto bound=services;
    bound.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observer) {
        if(execute(m,call.target,state,state.floating_point.value_or(fp),bound,observer))return;
        if(services.required_owner){services.required_owner(m,call,state,observer);return;}
        std::ostringstream error;error<<"Original particle/world execution requires owner0x"
            <<std::hex<<call.target<<" at0x"<<call.instruction;
        throw FormatError(error.str());
    };
    if(!execute(memory,entry,abi,abi.floating_point.value_or(fp),bound,observe))
        throw FormatError("Unowned original particle/world execution entry");
}
}
