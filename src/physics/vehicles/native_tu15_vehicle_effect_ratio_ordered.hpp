#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#endif

namespace halo2::native_vehicle_effect_ratio_ordered_detail {

// D26FA / D378A owns the real SHL4 flag image on the TU1.5 x86 path.
// Other executable images and backends keep their existing source policy.
template<class State,class Word> Word shift_index(State& state,Word value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image==SourcePhysicsContactImage::TitleUpdate15)
        return native_mopp_aabb_abi_detail::shifted_immediate<4>(state,value);
#endif
    return state.flags.shift(value,4u,4u);
}

} // namespace halo2::native_vehicle_effect_ratio_ordered_detail
