#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <vector>
namespace halo2 {
bool source_campaign_dof_recipe_tu15_owns_entry(std::uint32_t);
void source_run_campaign_dof_recipe_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
enum class SourceCampaignDofCommandKindTu15 { program,texture,texture_state,render_state,draw };
struct SourceCampaignDofCommandTu15 {
    SourceCampaignDofCommandKindTu15 kind{};
    std::uint32_t source_entry{},source_callsite{};
    // program:60 original words; texture:{target,stage};
    // texture_state:{stage,state}; render_state:{state,value};
    // draw:11 literal34A80 arguments, in original stack order.
    std::vector<std::uint32_t> words;
    bool operator==(const SourceCampaignDofCommandTu15 &)const=default;
};
// Whole21A60 plus21180/20F10 and real source CRT pow. A private CPU value
// bank records void graphics commands; it never allocates native GPU targets,
// supplies an SDK/game return, renders pixels or mutates the retained world.
// Positive blur counts above2^24 have a nonadvancing original binary32 loop
// and remain unadmitted. The source seven-REAL setter retains every raw word.
std::vector<SourceCampaignDofCommandTu15> source_campaign_dof_recipe_tu15(
    float far_blur,float near_blur,float focal_distance,std::uint32_t current_target);
}
