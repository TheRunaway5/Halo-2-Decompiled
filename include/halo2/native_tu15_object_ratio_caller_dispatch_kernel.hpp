#pragma once
#include <cstdint>

namespace halo2::native_object_ratio_caller_detail {
enum class Route : std::uint32_t {
    low=0x10487a, middle=0x104adf, high=0x104bfe,
    exact=0x104cc5, unknown=0x1049d0, extended=0x104747,
};

template<class View> Route select_initial_route(View& view) {
    view.compare_low_key();
    view.publish_definition();
    if(view.at_or_below_low())return Route::low;
    if(view.at_or_below_middle())return Route::middle;
    if(view.at_or_below_high())return Route::high;
    if(view.matches_exact())return Route::exact;
    return view.matches_extended()?Route::extended:Route::unknown;
}
} // namespace halo2::native_object_ratio_caller_detail
