#pragma once

namespace halo2::native_codec_model_selector_setup_detail {
enum class Boundary { returned, bypassed, before_selector };

// Set up the actual first decoder invocation. The selected path leaves
// execution before the CALL; the bypass path leaves before its join.
template<class View>Boundary prepare_selector(View& view) {
    if(view.has_override())view.publish_override();
    else view.publish_default_limit();
    view.select_weight();
    if(view.is_blended_model()) {
        view.publish_blended_mode();
        view.publish_model_weight();
    } else if(view.uses_zero_weight_mode()) {
        view.publish_zero_weight_mode();
        view.publish_model_weight();
    }
    if(!view.needs_primary_selector() || !view.has_primary_stream())return Boundary::bypassed;
    view.publish_decoder_table();
    view.publish_stream_offsets();
    view.publish_selector_channel();
    return Boundary::before_selector;
}
} // namespace halo2::native_codec_model_selector_setup_detail
