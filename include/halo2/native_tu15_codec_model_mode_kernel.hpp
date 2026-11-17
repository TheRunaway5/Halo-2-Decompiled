#pragma once
#include "halo2/native_tu15_codec_model_initialize_kernel.hpp"

namespace halo2::native_codec_model_mode_detail {
using native_codec_model_initialize_detail::Boundary;

template<class View>void compose_mode(View& view,Boundary boundary) {
    if(boundary==Boundary::with_secondary) {
        view.acquire_secondary_index();
        view.publish_secondary_interval();
        view.publish_secondary_fraction();
    } else if(boundary==Boundary::without_secondary && view.late_primary_mode()) {
        if(!view.at_last_primary_index()) {
            view.publish_primary_ceiling();
            view.publish_primary_fraction();
            if(!view.below_primary_epsilon()) {
                if(!view.above_primary_limit())return;
                view.promote_primary_ceiling();
            }
            view.clear_primary_fraction();
        }
        view.clear_primary_mode();
    }
}
} // namespace halo2::native_codec_model_mode_detail
