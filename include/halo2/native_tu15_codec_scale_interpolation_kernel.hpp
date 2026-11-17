#pragma once

namespace halo2::native_codec_scale_interpolation_detail {
// Keep the factor acquisition after the subtraction: that read can fault,
// and the source has already published its difference in XMM1.
template<class View>void decode(View&view){
    const auto input=view.input();
    const auto difference=view.subtract(input.second,input.first);
    const auto product=view.scale_difference(difference);
    const auto result=view.add_first(product,input.first);
    view.publish(result);
    view.finish();
}
} // namespace halo2::native_codec_scale_interpolation_detail
