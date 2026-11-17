#pragma once
#include <cstdint>

namespace halo2 {
inline constexpr std::uint32_t native_tu15_simplex_hull_triangle_edges_resume=0x30e5ecu;

template<class View> void native_tu15_simplex_hull_triangle_edges_kernel(View& view,bool first) {
    if(first){
        if(!view.first_edge_projection())view.place_near_first();
        else if(view.place_far_first())view.extend_first();
    }else{
        if(!view.second_edge_projection())view.place_near_second();
        else if(view.place_far_second())view.extend_second();
    }
}
} // namespace halo2
