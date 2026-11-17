#pragma once
#include <cstdint>

namespace halo2 {
// Actual 313300's modulo-32 seed recurrence. The view owns the three
// authored publications, lazy reloads and x87 value lifetime.
inline constexpr std::uint32_t native_solver_hull_sample_multiplier=0x3e39b193u;
inline constexpr std::uint32_t native_solver_hull_sample_offset=0x3039u;
inline constexpr std::uint32_t native_solver_hull_sample_mask=0x7fffffffu;

template<class View>void native_tu15_solver_hull_sample_kernel(View&view){
    view.begin_range();
    view.begin_seed();
    for(unsigned axis=0;axis<3;++axis){
        view.scale_sample(axis);
        view.add_lower(axis);
        view.scale_range();
        if(axis<2u)view.advance_seed(axis);
        view.publish_sample(axis);
    }
    view.finish();
}
} // namespace halo2
