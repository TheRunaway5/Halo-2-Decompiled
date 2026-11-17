#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2::native_campaign_ai_command_source_owner {
// Only these independently owned TU1.5 entry points cross back into the
// same retained world's native owner service. This is not a call dispatcher.
constexpr bool owns(std::uint32_t entry) {
    switch(entry) {
    case 0x1fc720u:case 0x279100u:
    case 0x2801d0u:case 0x261be0u:case 0x261a90u:case 0x261d50u:
    case 0x262250u:case 0x262190u:case 0x263b80u:case 0x262870u:
    case 0x2629d0u:case 0x2117e0u:case 0x211a80u:case 0x211cb0u:
    case 0x211840u:case 0x262590u:case 0x262710u:
    case 0x264860u:case 0x263bd0u:case 0x262a00u:case 0x263140u:
    case 0x263aa0u:return true;
    default:return false;
    }
}
template<std::uint32_t Entry,class State>bool bridge(State &state) {
    static_assert(owns(Entry),"Unowned native AI command entry");
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    // The caller's return word is already on its real source stack. A tail
    // handoff adds no synthetic frame. State::required transports and reloads
    // the child's actual ABI/FP on both return and exception; the native
    // lifecycle service retains its real HS association/retirement callbacks.
    state.required(Entry,Entry,true);
    return true;
}
} // namespace halo2::native_campaign_ai_command_source_owner
