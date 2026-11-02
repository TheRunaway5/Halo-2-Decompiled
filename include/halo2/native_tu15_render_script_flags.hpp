#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// A live World supplies its checked/journaled caller-input publisher. Plain
// memory adapters use write(). Exactly one publication occurs on each active
// path; the typed API starts after the script VM evaluated its arguments.
using NativeTu15RenderFlagPublisher=
    std::function<void(std::uint32_t,std::span<const std::byte>)>;
//2AE3A9..2AE3B6: the original BOOLEAN low byte is inverted into the retained
// game-state disable latch. No lighting value or host mirror is manufactured.
void native_tu15_warthog_chaingun_light_enable(SourcePhysicsContactMemory&,
    std::uint8_t enable,const NativeTu15RenderFlagPublisher& = {});
// Complete BE180 state effect. NONE does not read memory. Other handles use
// the original low16 row lookup, preserving every flag except bit21.
void native_tu15_object_cinematic_visibility(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint8_t visible,const NativeTu15RenderFlagPublisher& = {});
}
