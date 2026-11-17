#pragma once
#include "halo2/native_tu15_codec_transform_packed_kernel.hpp"

namespace halo2::native_codec_transform_publication_detail {
template<class View>void publish_result(View& view) {
    if(!view.first_output())return;
    do {
        if(view.output_in_range()) {
            view.copy_result();
            view.publish_planes();
        }
    } while(view.next_output());
}

// Private complete 283E10 composition. All transform and output paths use
// the actual caller storage and child returns; no result packet is supplied.
template<class Ancestry,class Packed,class Output>
void compose_rows(Ancestry& ancestry,Packed& packed,Output& output) {
    if(!ancestry.open()) {
        ancestry.finish_empty();
        return;
    }
    ancestry.initialize_cursor();
    do {
        if(!ancestry.admit_row())continue;
        ancestry.stage_transform();
        while(ancestry.has_parent())ancestry.compose_parent();
        ancestry.inverse_base();
        native_codec_transform_packed_detail::compose_packed(packed);
        publish_result(output);
    } while(ancestry.advance_rejected());
    ancestry.finish_rejected();
}
} // namespace halo2::native_codec_transform_publication_detail
