#pragma once

namespace halo2::native_codec_transform_chain_detail {
enum class Boundary { returned_empty, returned_rejected, before_packed_decode };

// Phase one owns the live record cursor, optional remap and actual ancestry
// composition. A selected row stops at the next original parent acquisition,
// after the genuine inverse child has returned. No packed result is supplied.
template<class View>Boundary compose_ancestry(View& view) {
    if(!view.open()) {
        view.finish_empty();
        return Boundary::returned_empty;
    }
    view.initialize_cursor();
    do {
        if(view.admit_row()) {
            view.stage_transform();
            while(view.has_parent())view.compose_parent();
            view.inverse_base();
            return Boundary::before_packed_decode;
        }
    } while(view.advance_rejected());
    view.finish_rejected();
    return Boundary::returned_rejected;
}
} // namespace halo2::native_codec_transform_chain_detail
