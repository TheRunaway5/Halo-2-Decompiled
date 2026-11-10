#pragma once
#include "halo2/native_tu15_id_search_kernel.hpp"

namespace halo2::native_model_marker_lookup_detail {
enum class Invalid {datum,name,empty_name};
// First named marker in the captured model marker table. The same typed
// ID-search loop serves the renderer queue; the view retains this owner's
// signed count, twelve-byte row layout and source acquisition boundaries.
template<class View>auto lookup(View&view,std::uint32_t datum,std::uint32_t name){
    if(datum==0xffffffffu)return view.invalid(Invalid::datum);
    if(name==0xffffffffu)return view.invalid(Invalid::name);
    if(!name)return view.invalid(Invalid::empty_name);
    view.open(datum,name);
    return native_id_search_detail::find(view);
}
} // namespace halo2::native_model_marker_lookup_detail
