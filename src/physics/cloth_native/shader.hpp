#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
namespace halo2::cloth_native_tu15_detail {
// Original1191A0: the live cloth instance owns the definition identity;
// the admitted tag directory then owns that definition's actual shader.
// Preserve both volatile pointer results and original arithmetic flags.
template<class State>void shader(State& state) {
    using physics_world_runtime_detail::r;
    const auto context=r(state.memory,state.gp[4]+4u,4);
    const auto pool=r(state.memory,0x4ecc08u,4);
    state.gp[2]=r(state.memory,pool+0x44u,4);
    state.gp[1]=r(state.memory,0x4f0414u,4);
    auto index=state.flags.logic(context&65535u,4);
    const auto row=state.flags.product(index,0x1258u);
    const auto definition=r(state.memory,state.gp[2]+row+4u,4);
    index=state.flags.logic(definition&65535u,4);
    const auto entry=state.flags.shift(index,4,4);
    state.gp[2]=r(state.memory,state.gp[1]+entry+8u,4);
    state.gp[0]=r(state.memory,state.gp[2]+0xcu,4);
    state.pop();state.gp[4]+=4u;
}
}
