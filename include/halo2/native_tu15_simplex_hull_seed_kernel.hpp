#pragma once

namespace halo2 {
// The hull solver receives paired simplex, payload and difference rows.
// Their ordering and mutable backing are determined by the real caller.
template<class View> void native_tu15_simplex_hull_seed_kernel(View& view) {
    view.open_frame();
    if(view.seed_first_pair()) {
        view.seed_second_shape_rows();
        if(view.second_shape_has_four())view.seed_fourth_from_second();
        else if(view.first_shape_has_two())view.seed_fourth_from_first_pair();
        else view.keep_three();
    }else if(view.first_shape_has_three()) {
        view.seed_first_shape_rows();
        if(view.first_shape_has_four())view.seed_fourth_from_first();
        else if(view.second_shape_has_two())view.seed_fourth_from_second_pair();
        else view.keep_three();
    }else view.keep_one();
    view.call_hull();
}
} // namespace halo2
