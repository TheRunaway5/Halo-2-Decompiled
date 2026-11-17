#pragma once

namespace halo2::native_codec_translation_interpolation_detail {
// The two translation rows are staged through the caller's real global
// scratch. Keep the component acquisitions separate: writes may alias rows,
// scratch, the factory globals or the eventual output.
template<class View>void decode(View&view){
    const auto input=view.input();
    const auto x=view.interpolate(input.second,input.first,input.factor);
    view.publish_x(x);
    const auto first_y=view.first_y();
    const auto second_y=view.second_y();
    const auto y=view.interpolate(second_y,first_y,input.factor);
    view.publish_y(y);
    const auto first_z=view.first_z();
    const auto second_z=view.second_z();
    const auto z=view.interpolate(second_z,first_z,input.factor);
    view.publish_z(z);
    view.finish();
}
} // namespace halo2::native_codec_translation_interpolation_detail
