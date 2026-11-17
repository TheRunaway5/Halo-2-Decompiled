#pragma once

namespace halo2::native_node_function_progress_detail {
enum class Route { other, direct, complement };
enum class Return { absent_object, direct_child, restore_object };

// D3950 selects direct or complementary progress for the two original
// function types. Only a valid record can call the node progress query.
template<class View> void run(View& view) {
    if(!view.has_object())return view.finish(Return::absent_object);
    switch(view.select()){
        case Route::direct:
            if(view.template valid_record<false>()){
                view.template progress<false>();
                return view.finish(Return::direct_child);
            }
            break;
        case Route::complement:
            if(view.template valid_record<true>()){
                view.template progress<true>();
                view.reload_one();
            }
            view.complement();
            break;
        case Route::other:
            break;
    }
    view.finish(Return::restore_object);
}
} // namespace halo2::native_node_function_progress_detail
