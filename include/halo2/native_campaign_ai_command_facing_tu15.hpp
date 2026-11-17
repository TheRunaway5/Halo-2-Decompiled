#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
//280C90 consumes DL=enabled and EDI=authored point reference;280D70 consumes
// the same registers with EDI=object datum. Both update the actual current
// command's persistent facing/look fields, not command
// kind or actor position. RET0; GP/result effects and retained stores follow
// the source. A missing current command is the original no-op.
void native_campaign_ai_command_facing_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
