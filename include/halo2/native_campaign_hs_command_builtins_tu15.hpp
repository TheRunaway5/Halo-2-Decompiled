#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>

namespace halo2 {
// This service invokes actual native parser/evaluation owners in the same
// retained world. It never supplies evaluated arguments or completion values.
struct NativeCampaignHsCommandChildrenTu15 {
    std::function<void(std::uint32_t instruction,std::uint32_t entry,
        SourcePhysicsWorldRuntimeAbi &)> call;
};
// Actual HS wrapper ABI: stack opcode, thread datum, initial-evaluation byte;
// RET0xC. 2B2470 writes command+81, 2B2670 writes command+76, and 210A10
// performs the source sleep-forever scheduling operation. 2B1200 forwards
// actual boolean/point arguments to the native 280C90 facing owner.
// Authored Armory look/object-aim/object-face and enable-looking wrappers use
// the same actual parser contract; object aim/face call native 280D70.
// 2B0E30 publishes both script points and kind3, then yields its actual HS
// thread with wake=-3 after 211DA0. Parser and return
// ownership remain 212010/211DA0; optional named sleep requires 212130.
void native_campaign_hs_command_builtins_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignHsCommandChildrenTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
