#pragma once

namespace halo2::native_object_type_query_detail {
// The authored type inheritance list is ordered and terminated by a null
// descriptor. Each callback can publish the result and stop the search. The
// view owns live source fields, callback ABI, output writes and return state.
template<class View>auto query(View&view){
    view.begin();
    for(;;){
        if(!view.method_present())return view.finish_empty();
        if(view.callback_present())view.invoke();
        view.advance();
        if(view.accepted())return view.finish_found();
    }
}
} // namespace halo2::native_object_type_query_detail
