#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>

namespace halo2 {
// This service invokes actual native parser/evaluation owners in the same
// retained world. It never supplies evaluated arguments or completion values.
struct NativeCampaignHsCommandChildrenTu15 {
    std::function<void(std::uint32_t instruction,std::uint32_t entry,
        SourcePhysicsWorldRuntimeAbi &)> call;
    std::function<void(std::uint32_t instruction)> instruction;
};
// Actual HS wrapper ABI: stack opcode, thread datum, and a 32-bit argument
// whose low byte is the initial-evaluation flag. RET0xC. 2B2470 writes
// command+81, 2B2670 writes command+76, and 210A10 performs the source
// sleep-forever scheduling operation. 2B1200 forwards
// actual boolean/point arguments to the native 280C90 facing owner.
// 2B2340 writes the parsed combat status short into command+7C.
// 2B01A0 parses ai_play_line_at_player and stops at the actual dialogue
// service 27FEE0 once the two script arguments are available.
// Authored Armory look/object-aim/object-face and enable-looking wrappers use
// the same actual parser contract; object aim/face call native 280D70.
// 2B0E30 publishes both script points and kind3, then yields its actual HS
// thread with wake=-3 after 211DA0. Parser and return
// ownership remain 212010/211DA0; optional named sleep requires 212130.
void native_campaign_hs_command_builtins_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignHsCommandChildrenTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
// 1100C0 consumes the real 2ABA90 argument transport. Its existing object
// velocity and wake children retain authority over the live physics records.
void native_campaign_hs_object_velocity_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const NativeCampaignHsCommandChildrenTu15 &,
    const SourcePhysicsContactRecordObserver &);
}
