#pragma once
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include <vector>

namespace halo2 {
class PhysicsLiveWorldTu15;
// Native kernel bank over a live TU1.5 world: its retained memory, its
// journaled write_source_call_input publisher and, when requested, its
// current x87/MXCSR controls. No original code is executed.
NativeCampaignScriptFloatingPoint native_campaign_script_floating_point(PhysicsLiveWorldTu15&);
NativeCampaignScriptBank native_campaign_script_bank(PhysicsLiveWorldTu15&,bool floating_point=false);
// HS object_set_velocity (1100C0) through the native motion owners, with
// every store journaled as a world source-call input.
void native_campaign_object_set_velocity(PhysicsLiveWorldTu15&,std::uint32_t object,float forward,float left,float up);


}
