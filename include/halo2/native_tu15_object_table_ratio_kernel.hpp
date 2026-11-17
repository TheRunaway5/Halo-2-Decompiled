#pragma once

namespace halo2::native_object_table_ratio_detail {
// TU1.5 24B430 selects an object row by its signed word. The fallback
// path divides by the live signed state word (zero is represented by one).
template<class View> auto query(View& view) {
    view.open_object_row();
    if(view.has_index()){
        if(view.has_constant_flag()){
            view.load_constant();
            return view.finish_constant();
        }
        view.scale_positive_count();
    }
    return view.finish();
}
} // namespace halo2::native_object_table_ratio_detail
