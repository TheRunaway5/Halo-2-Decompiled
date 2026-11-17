#pragma once

namespace halo2::native_codec_model_initialize_detail {
enum class Boundary { returned, without_secondary, with_secondary };

// Initial 2835B0 setup through the real secondary-mode branch. The low
// weight path owns its original RET28; both other paths leave a live frame.
template<class View>Boundary initialize(View& view) {
    if(!view.begin()) {
        view.finish_low_weight();
        return Boundary::returned;
    }
    view.acquire_index();
    view.clear_first_planes();
    view.publish_pair_and_finish_planes();
    if(view.negative_index())view.zero_index();
    else if(view.above_last_index())view.use_last_index();
    view.publish_decoder_inputs();
    return view.has_secondary()?Boundary::with_secondary:Boundary::without_secondary;
}
} // namespace halo2::native_codec_model_initialize_detail
