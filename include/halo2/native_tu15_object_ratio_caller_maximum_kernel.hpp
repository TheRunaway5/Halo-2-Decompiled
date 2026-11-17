#pragma once
#include "halo2/native_tu15_object_ratio_caller_extended_kernel.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<class View> void scan_initial_maximum(View& view,ExtendedStage stage) {
    if(stage==ExtendedStage::vector_maximum){
        view.begin_groups();
        do{view.sample_group();}while(view.advance_group());
        if(!view.has_remaining())return;
    }
    view.begin_remainder();
    do{view.sample_remainder();}while(view.advance_remainder());
}
} // namespace halo2::native_object_ratio_caller_detail
