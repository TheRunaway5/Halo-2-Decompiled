#pragma once
#include <cstdint>

namespace halo2::native_object_ratio_caller_detail {
enum class ExtendedStage : std::uint32_t {
    finish=0x104cf8, vector_maximum=0x10477c, scalar_maximum=0x1047c6, scan_records=0x1047e3,
};

template<class View> ExtendedStage prepare_extended(View& view) {
    if(!view.allow_extended())return ExtendedStage::finish;
    view.reload_definition();
    return view.locate_initial_scan();
}

template<class View> void scan_extended_records(View& view) {
    if(view.begin_record_scan()){
        do {
            if(view.has_positive_weight())view.sample_record();
            if(view.has_secondary_term())view.add_secondary_term();
        }while(view.advance_record());
    }
    view.include_object_term();
}
} // namespace halo2::native_object_ratio_caller_detail
