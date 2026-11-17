#pragma once
#include <cstdint>

namespace halo2::native_object_ratio_caller_detail {
enum class MiddleRoute : std::uint32_t {
    first_key=0x104cc5, upper=0x104bcb, second_key=0x104b79,
    record_ratio=0x104b50, mode_ratio=0x104b13, unknown=0x1049d0,
};

template<class View> MiddleRoute select_middle_route(View& view) {
    if(view.first_key())return MiddleRoute::first_key;
    view.compare_upper_key();
    if(view.above_upper())return MiddleRoute::upper;
    if(view.equal_upper())return MiddleRoute::second_key;
    if(view.matches_record_key())return MiddleRoute::record_ratio;
    if(view.matches_second_key())return MiddleRoute::second_key;
    return view.matches_mode_key()?MiddleRoute::mode_ratio:MiddleRoute::unknown;
}

template<class View> void run_record_branch(View& view) {
    if(view.has_record_index()){
        view.call_record_ratio();
        view.clamp_result();
    }
    view.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
